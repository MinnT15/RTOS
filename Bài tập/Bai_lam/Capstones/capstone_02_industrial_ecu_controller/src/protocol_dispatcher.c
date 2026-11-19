/**
 * @file    protocol_dispatcher.c
 * @brief   Capstone 02: Lockless SPSC Stream Buffer + Protocol FSM Dispatcher + Worker Task
 */

#include "ecu_arch.h"
#include <stdatomic.h>
#include <string.h>

typedef enum {
    FSM_WAIT_SYNC = 0,
    FSM_WAIT_TYPE,
    FSM_WAIT_LEN,
    FSM_ACCUM_PAYLOAD,
    FSM_VERIFY_CRC
} FsmDecodeState_t;

void Stream_Init(LocklessStream_t *pxStream) {
    if (pxStream != NULL) {
        memset((void *)pxStream, 0, sizeof(LocklessStream_t));
    }
}

uint32_t Stream_WriteFromISR(LocklessStream_t *pxStream, const uint8_t *pucData, uint32_t ulLen) {
    if ((pxStream == NULL) || (pucData == NULL)) {
        return 0U;
    }
    uint32_t written = 0U;
    uint32_t head = pxStream->ulHead;
    uint32_t tail = pxStream->ulTail;
    atomic_thread_fence(memory_order_acquire);

    while (written < ulLen) {
        uint32_t nextHead = (head + 1U) % ECU_STREAM_CAPACITY;
        if (nextHead == tail) {
            break; /* Full */
        }
        pxStream->ucRing[head] = pucData[written++];
        head = nextHead;
    }
    atomic_thread_fence(memory_order_release);
    pxStream->ulHead = head;
    return written;
}

uint32_t Stream_ReadTask(LocklessStream_t *pxStream, uint8_t *pucOut, uint32_t ulMaxLen) {
    if ((pxStream == NULL) || (pucOut == NULL)) {
        return 0U;
    }
    uint32_t readCnt = 0U;
    uint32_t head = pxStream->ulHead;
    uint32_t tail = pxStream->ulTail;
    atomic_thread_fence(memory_order_acquire);

    while ((readCnt < ulMaxLen) && (tail != head)) {
        pucOut[readCnt++] = pxStream->ucRing[tail];
        tail = (tail + 1U) % ECU_STREAM_CAPACITY;
    }
    atomic_thread_fence(memory_order_release);
    pxStream->ulTail = tail;
    return readCnt;
}

void Ecu_InjectUartFrameFromISR(EcuMsgType_t eType, const uint8_t *pucPayload, uint8_t ucLen, bool bCorruptCrc) {
    if (ucLen == 0U || ucLen > ECU_FRAME_MAX_PAYLOAD || pucPayload == NULL) {
        return;
    }
    uint8_t rawFrame[ECU_FRAME_MAX_PAYLOAD + 8U];
    uint32_t idx = 0U;
    rawFrame[idx++] = 0x55U; /* Giả lập 1 byte nhiễu đường truyền trước Header */
    rawFrame[idx++] = FRAME_SYNC_BYTE;
    rawFrame[idx++] = (uint8_t)eType;
    rawFrame[idx++] = ucLen;

    uint8_t crc = 0U;
    for (uint8_t i = 0U; i < ucLen; i++) {
        rawFrame[idx++] = pucPayload[i];
        crc = (uint8_t)(crc + pucPayload[i]);
    }
    rawFrame[idx++] = bCorruptCrc ? (uint8_t)(crc ^ 0xFFU) : crc;

    Stream_WriteFromISR(&g_xUartRxStream, rawFrame, idx);
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(g_xDispatcherHandle, &woken);
}

void Ecu_DispatcherTask(void *pvParam) {
    (void)pvParam;
    FsmDecodeState_t state = FSM_WAIT_SYNC;
    EcuMsgType_t curType = FRAME_TYPE_TELEMETRY;
    uint8_t curLen = 0U, curIdx = 0U, calcCrc = 0U;
    uint8_t accumBuf[ECU_FRAME_MAX_PAYLOAD];
    uint8_t chunk[32];

    SimRtos_Log("Dispatcher", "San sang giai ma Protocol FSM qua Lockless Stream Buffer + TaskNotify.");

    while (SimRtos_ShouldRun()) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(30U));
        uint32_t nBytes = 0U;
        while ((nBytes = Stream_ReadTask(&g_xUartRxStream, chunk, sizeof(chunk))) > 0U) {
            for (uint32_t i = 0U; i < nBytes; i++) {
                uint8_t b = chunk[i];
                switch (state) {
                    case FSM_WAIT_SYNC:
                        if (b == FRAME_SYNC_BYTE) { state = FSM_WAIT_TYPE; }
                        break;
                    case FSM_WAIT_TYPE:
                        curType = (EcuMsgType_t)b;
                        state = FSM_WAIT_LEN;
                        break;
                    case FSM_WAIT_LEN:
                        curLen = b;
                        curIdx = 0U;
                        calcCrc = 0U;
                        state = (curLen > 0U && curLen <= ECU_FRAME_MAX_PAYLOAD) ? FSM_ACCUM_PAYLOAD : FSM_WAIT_SYNC;
                        break;
                    case FSM_ACCUM_PAYLOAD:
                        accumBuf[curIdx++] = b;
                        calcCrc = (uint8_t)(calcCrc + b);
                        if (curIdx >= curLen) { state = FSM_VERIFY_CRC; }
                        break;
                    case FSM_VERIFY_CRC:
                        if (b == calcCrc) {
                            ZeroCopyBuf_t *pBuf = ZeroCopyPool_Alloc("Dispatcher");
                            if (pBuf != NULL) {
                                pBuf->eMsgType = curType;
                                pBuf->ucLen = curLen;
                                memcpy(pBuf->ucPayload, accumBuf, curLen);
                                pBuf->pcOwner = "InQueue->Wrkr";
                                SimRtos_Log("Dispatcher", "Frame OK (Type=0x%02X, Len=%uB) -> ZeroCopy Slot#%u (%p)",
                                            curType, curLen, pBuf->ulSlotId, (void *)pBuf);
                                /* Truyền ĐỊA CHỈ CON TRỎ (Zero-Copy Ownership Handover!) */
                                if (xQueueSend(g_xWorkerPtrQueue, &pBuf, pdMS_TO_TICKS(10U)) != pdPASS) {
                                    ZeroCopyPool_Release(pBuf);
                                }
                            } else {
                                xSemaphoreTake(g_xMetricsMutex, portMAX_DELAY);
                                g_xEcuMetrics.ulPoolExhaustedCount++;
                                xSemaphoreGive(g_xMetricsMutex);
                            }
                        } else {
                            xSemaphoreTake(g_xMetricsMutex, portMAX_DELAY);
                            g_xEcuMetrics.ulBadCrcRejected++;
                            xSemaphoreGive(g_xMetricsMutex);
                            SimRtos_Log("Dispatcher", "REJECTED corrupted frame! Expected CRC=0x%02X != Got=0x%02X",
                                        calcCrc, b);
                        }
                        state = FSM_WAIT_SYNC;
                        break;
                }
            }
        }
    }
}

void Ecu_WorkerTask(void *pvParam) {
    (void)pvParam;
    SimRtos_Log("EcuWorker", "Khoi tao HIL Actuator Driver (%s)...", g_xRelayDriver.pcDriverName);
    g_xRelayDriver.pxVptr->pfnInit(g_xRelayDriver.pvHwCtx);

    ZeroCopyBuf_t *pRxBuf = NULL;
    while (SimRtos_ShouldRun() || (uxQueueMessagesWaiting(g_xWorkerPtrQueue) > 0U)) {
        if (xQueueReceive(g_xWorkerPtrQueue, &pRxBuf, pdMS_TO_TICKS(30U)) == pdPASS) {
            if (pRxBuf != NULL) {
                pRxBuf->pcOwner = "EcuWorker";
                xSemaphoreTake(g_xMetricsMutex, portMAX_DELAY);
                if (pRxBuf->eMsgType == FRAME_TYPE_TELEMETRY) {
                    g_xEcuMetrics.ulTelemetryHandled++;
                    SimRtos_Log("EcuWorker", "Telemetry Slot#%u: \"%.*s\"",
                                pRxBuf->ulSlotId, pRxBuf->ucLen, (char *)pRxBuf->ucPayload);
                } else if (pRxBuf->eMsgType == FRAME_TYPE_CMD_PWM) {
                    uint32_t pct = pRxBuf->ucPayload[0];
                    g_xRelayDriver.pxVptr->pfnSetOutput(g_xRelayDriver.pvHwCtx, pct);
                    g_xEcuMetrics.ulCommandsHandled++;
                    g_xEcuMetrics.ulLastPwmApplied = pct;
                    SimRtos_Log("EcuWorker", "CMD_PWM %u%% -> HAL VTable CCR=%u, PinLevel=%s (Active-Low!)",
                                pct, g_xStm32RelayCtx.ulPwmCcr, g_xStm32RelayCtx.bPinLevel ? "HIGH(OFF)" : "LOW(ON)");
                } else if (pRxBuf->eMsgType == FRAME_TYPE_OTA_CHUNK) {
                    g_xEcuMetrics.ulOtaChunksHandled++;
                    SimRtos_Log("EcuWorker", "OTA Chunk Slot#%u (%uB) staged cleanly", pRxBuf->ulSlotId, pRxBuf->ucLen);
                }
                xSemaphoreGive(g_xMetricsMutex);

                /* Hoàn trả Buffer về Pool sau khi xong việc */
                ZeroCopyPool_Release(pRxBuf);
                pRxBuf = NULL;
            }
        }
    }
}
