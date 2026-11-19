/**
 * @file    ecu_arch.h
 * @brief   Capstone 02: Layered ECU Architecture (ISR -> StreamBuffer -> FSM Dispatcher -> Zero-Copy Pool -> HAL VTable)
 */

#ifndef ECU_ARCH_H
#define ECU_ARCH_H

#include "sim_rtos.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ECU_POOL_COUNT          (4U)
#define ECU_FRAME_MAX_PAYLOAD   (32U)
#define ECU_STREAM_CAPACITY     (128U)
#define FRAME_SYNC_BYTE         (0xAAU)

typedef enum {
    FRAME_TYPE_TELEMETRY = 0x01,
    FRAME_TYPE_CMD_PWM   = 0x02,
    FRAME_TYPE_OTA_CHUNK = 0x03
} EcuMsgType_t;

/* 1. Zero-Copy Static Packet Buffer */
typedef struct {
    bool         bInUse;
    uint32_t     ulSlotId;
    EcuMsgType_t eMsgType;
    uint8_t      ucLen;
    uint8_t      ucPayload[ECU_FRAME_MAX_PAYLOAD];
    const char  *pcOwner;
} ZeroCopyBuf_t;

void           ZeroCopyPool_Init(void);
ZeroCopyBuf_t *ZeroCopyPool_Alloc(const char *pcOwner);
bool           ZeroCopyPool_Release(ZeroCopyBuf_t *pxBuf);
uint32_t       ZeroCopyPool_GetFreeCount(void);
void           ZeroCopyPool_DumpTable(void);

/* 2. Lockless SPSC Stream Buffer (ISR -> Task) */
typedef struct {
    uint8_t           ucRing[ECU_STREAM_CAPACITY];
    volatile uint32_t ulHead;
    volatile uint32_t ulTail;
} LocklessStream_t;

void     Stream_Init(LocklessStream_t *pxStream);
uint32_t Stream_WriteFromISR(LocklessStream_t *pxStream, const uint8_t *pucData, uint32_t ulLen);
uint32_t Stream_ReadTask(LocklessStream_t *pxStream, uint8_t *pucOut, uint32_t ulMaxLen);

/* 3. Layered HAL/HIL Driver VTable (OOP in C - Swap Hardware without touching App) */
typedef struct IActuatorVTable {
    bool (*pfnInit)(void *pvCtx);
    bool (*pfnSetOutput)(void *pvCtx, uint32_t ulPercent);
    void (*pfnTripSafe)(void *pvCtx);
} IActuatorVTable_t;

typedef struct {
    const IActuatorVTable_t *pxVptr;
    void                    *pvHwCtx;
    const char              *pcDriverName;
} ActuatorInterface_t;

typedef struct {
    uint32_t ulPortBase;
    uint8_t  ucPin;
    bool     bActiveLow;
    bool     bPinLevel;
    uint32_t ulPwmCcr;
} Stm32PinHwCtx_t;

extern const IActuatorVTable_t g_xStm32RelayVTable;

/* 4. Shared ECU Runtime State & Tasks */
typedef struct {
    uint32_t ulTelemetryHandled;
    uint32_t ulCommandsHandled;
    uint32_t ulOtaChunksHandled;
    uint32_t ulBadCrcRejected;
    uint32_t ulPoolExhaustedCount;
    uint32_t ulLastPwmApplied;
} EcuMetrics_t;

extern LocklessStream_t     g_xUartRxStream;
extern QueueHandle_t        g_xWorkerPtrQueue; /* Truyền CON TRỎ ZeroCopyBuf_t* (Zero-Copy!) */
extern TaskHandle_t         g_xDispatcherHandle;
extern ActuatorInterface_t  g_xRelayDriver;
extern Stm32PinHwCtx_t      g_xStm32RelayCtx;
extern EcuMetrics_t         g_xEcuMetrics;
extern SemaphoreHandle_t    g_xMetricsMutex;

void Ecu_DispatcherTask(void *pvParam);
void Ecu_WorkerTask(void *pvParam);
void Ecu_InjectUartFrameFromISR(EcuMsgType_t eType, const uint8_t *pucPayload, uint8_t ucLen, bool bCorruptCrc);

#ifdef __cplusplus
}
#endif

#endif /* ECU_ARCH_H */
