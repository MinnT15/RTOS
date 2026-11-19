/**
 * @file    sensor_motor_tasks.c
 * @brief   Capstone 01: Concurrent Sensor Acquisition (xTaskDelayUntil) & Motor Actuator Tasks
 */

#include "app_telemetry.h"

void App_SensorAcqTask(void *pvParam) {
    (void)pvParam;
    SimRtos_Log("SensorAcq", "Khoi tao ADC DMA -> Cho EventGroup Rendezvous...");
    EventBits_t sync = xEventGroupSync(g_xStartupSyncGroup, SYNC_BIT_SENSOR, SYNC_ALL_READY, pdMS_TO_TICKS(1000U));
    SimRtos_Log("SensorAcq", "Rendezvous OK (Bits=0x%02X)! Chay chu ky co dinh 50ms.", sync);

    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint32_t seq = 0U;

    while (SimRtos_ShouldRun()) {
        seq++;
        xSemaphoreTake(g_xStateMutex, portMAX_DELAY);
        int32_t temp = g_xNodeState.iSimulatedTempDeciC;
        bool trip = g_xNodeState.bFaultInjected;
        xSemaphoreGive(g_xStateMutex);

        TelemetryPacket_t pkt;
        pkt.ulSeqId = seq;
        pkt.ulTimestampMs = xTaskGetTickCount();
        if (trip) {
            pkt.eType = MSG_EMERGENCY_TRIP;
            pkt.data.fault.ulFaultCode = 0xDEAD0001U;
            Gatekeeper_EnqueueLog("SensorAcq", false, "PKT#%u: PHAT HIEN OVER-TEMP (%d.%dC) -> EMERGENCY_TRIP!",
                                  seq, temp / 10, (temp >= 0 ? temp % 10 : (-temp) % 10));
        } else {
            pkt.eType = MSG_SENSOR_TEMP;
            pkt.data.sensor.iTempDeciC = temp;
            pkt.data.sensor.usRpm = (uint16_t)(1200U + (seq * 50U));
            Gatekeeper_EnqueueLog("SensorAcq", false, "PKT#%u: Temp=%d.%dC, RPM=%u -> Push xTelemetryQueue",
                                  seq, temp / 10, (temp >= 0 ? temp % 10 : (-temp) % 10), pkt.data.sensor.usRpm);
        }

        if (xQueueSend(g_xTelemetryQueue, &pkt, pdMS_TO_TICKS(10U)) != pdPASS) {
            Gatekeeper_EnqueueLog("SensorAcq", false, "WARN: Telemetry Queue FULL -> Dropped PKT#%u", seq);
        }

        xTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50U));
    }
}

void App_MotorActuatorTask(void *pvParam) {
    (void)pvParam;
    SimRtos_Log("MotorCtrl", "Khoi tao PWM Driver -> Cho EventGroup Rendezvous...");
    xEventGroupSync(g_xStartupSyncGroup, SYNC_BIT_MOTOR, SYNC_ALL_READY, pdMS_TO_TICKS(1000U));
    SimRtos_Log("MotorCtrl", "Rendezvous OK! Dang lang nghe xTelemetryQueue...");

    TelemetryPacket_t rxPkt;
    while (SimRtos_ShouldRun()) {
        if (xQueueReceive(g_xTelemetryQueue, &rxPkt, pdMS_TO_TICKS(40U)) == pdPASS) {
            xSemaphoreTake(g_xStateMutex, portMAX_DELAY);
            g_xNodeState.ulPacketsProcessed++;

            if (rxPkt.eType == MSG_EMERGENCY_TRIP) {
                g_xNodeState.ulFanSpeedPercent = 100U;
                Gatekeeper_EnqueueLog("MotorCtrl", false, "TRIP CMD (0x%X)! Ep quat lam mat 100%% PWM & Ngat dong co!",
                                      rxPkt.data.fault.ulFaultCode);
            } else if (rxPkt.eType == MSG_SENSOR_TEMP) {
                if (rxPkt.data.sensor.iTempDeciC >= 400) {
                    g_xNodeState.ulFanSpeedPercent = 80U;
                } else {
                    g_xNodeState.ulFanSpeedPercent = 30U;
                }
                Gatekeeper_EnqueueLog("MotorCtrl", false, "Xu ly PKT#%u (Temp=%d.%dC) -> Dieu chinh PWM Quat = %u%%",
                                      rxPkt.ulSeqId,
                                      rxPkt.data.sensor.iTempDeciC / 10,
                                      rxPkt.data.sensor.iTempDeciC % 10,
                                      g_xNodeState.ulFanSpeedPercent);
            } else {
                g_xNodeState.ulFanSpeedPercent = 0U;
                g_xNodeState.bFaultInjected = false;
            }
            xSemaphoreGive(g_xStateMutex);
        }
    }
}
