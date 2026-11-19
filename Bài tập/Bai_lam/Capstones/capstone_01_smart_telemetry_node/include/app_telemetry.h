/**
 * @file    app_telemetry.h
 * @brief   Capstone 01: Shared Contracts, Discriminated Union Messages & Static RTOS Handles
 */

#ifndef APP_TELEMETRY_H
#define APP_TELEMETRY_H

#include "sim_rtos.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Event Group Rendezvous Bits (Task Startup Sync) */
#define SYNC_BIT_SENSOR     ((EventBits_t)(1U << 0U))
#define SYNC_BIT_MOTOR      ((EventBits_t)(1U << 1U))
#define SYNC_BIT_GATEKEEPER ((EventBits_t)(1U << 2U))
#define SYNC_ALL_READY      (SYNC_BIT_SENSOR | SYNC_BIT_MOTOR | SYNC_BIT_GATEKEEPER)

/* Queue Dimensions (100% Static Allocation - Zero malloc) */
#define TELEMETRY_Q_LEN     (8U)
#define LOG_Q_LEN           (16U)
#define LOG_MSG_LEN         (80U)

typedef enum {
    MSG_SENSOR_TEMP = 1,
    MSG_EMERGENCY_TRIP = 2,
    MSG_SYSTEM_RESET = 3
} TelemetryMsgType_t;

typedef struct {
    TelemetryMsgType_t eType;
    uint32_t           ulSeqId;
    uint32_t           ulTimestampMs;
    union {
        struct {
            int32_t iTempDeciC;  /* VD: 365 = 36.5 degC */
            uint16_t usRpm;
        } sensor;
        struct {
            uint32_t ulFaultCode;
        } fault;
    } data;
} TelemetryPacket_t;

typedef struct {
    TickType_t xTimestamp;
    char       pcSource[14];
    bool       bFromIsr;
    char       pcLine[LOG_MSG_LEN];
} GatekeeperLog_t;

typedef struct {
    int32_t  iSimulatedTempDeciC;
    bool     bFaultInjected;
    uint32_t ulPacketsProcessed;
    uint32_t ulFanSpeedPercent;
    uint32_t ulHeartbeatCount;
    bool     bBacklightOn;
    uint32_t ulBacklightOffCount;
} NodeTelemetryState_t;

/* Shared Handles initialized by Composition Root (main.c) */
extern QueueHandle_t        g_xTelemetryQueue;
extern QueueHandle_t        g_xLogQueue;
extern EventGroupHandle_t   g_xStartupSyncGroup;
extern SemaphoreHandle_t    g_xStateMutex;
extern TimerHandle_t        g_xHeartbeatTimer;
extern TimerHandle_t        g_xBacklightTimer;
extern NodeTelemetryState_t g_xNodeState;

/* Task Entry Points & Helpers */
void App_Tasks_InitObjects(void);
void App_SensorAcqTask(void *pvParam);
void App_MotorActuatorTask(void *pvParam);
void App_UartGatekeeperTask(void *pvParam);

void Gatekeeper_EnqueueLog(const char *pcSource, bool bFromIsr, const char *pcFmt, ...);
void App_HeartbeatTimerCb(TimerHandle_t xTimer);
void App_BacklightTimerCb(TimerHandle_t xTimer);

#ifdef __cplusplus
}
#endif

#endif /* APP_TELEMETRY_H */
