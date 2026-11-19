/**
 * @file    zero_copy_pool.c
 * @brief   Capstone 02: O(1) Thread-Safe Static Zero-Copy Buffer Pool with Ownership Audit
 */

#include "ecu_arch.h"
#include <stdio.h>
#include <string.h>

static ZeroCopyBuf_t   s_xPool[ECU_POOL_COUNT];
static pthread_mutex_t s_xPoolLock = PTHREAD_MUTEX_INITIALIZER;

void ZeroCopyPool_Init(void) {
    pthread_mutex_lock(&s_xPoolLock);
    memset(s_xPool, 0, sizeof(s_xPool));
    for (uint32_t i = 0U; i < ECU_POOL_COUNT; i++) {
        s_xPool[i].ulSlotId = i;
        s_xPool[i].pcOwner = "POOL_FREE";
    }
    pthread_mutex_unlock(&s_xPoolLock);
}

ZeroCopyBuf_t *ZeroCopyPool_Alloc(const char *pcOwner) {
    ZeroCopyBuf_t *pRet = NULL;
    pthread_mutex_lock(&s_xPoolLock);
    for (uint32_t i = 0U; i < ECU_POOL_COUNT; i++) {
        if (!s_xPool[i].bInUse) {
            s_xPool[i].bInUse = true;
            s_xPool[i].ucLen = 0U;
            s_xPool[i].pcOwner = (pcOwner != NULL) ? pcOwner : "UNKNOWN";
            pRet = &s_xPool[i];
            break;
        }
    }
    pthread_mutex_unlock(&s_xPoolLock);
    return pRet;
}

bool ZeroCopyPool_Release(ZeroCopyBuf_t *pxBuf) {
    if (pxBuf == NULL) {
        return false;
    }
    bool ok = false;
    pthread_mutex_lock(&s_xPoolLock);
    if ((pxBuf >= &s_xPool[0]) && (pxBuf <= &s_xPool[ECU_POOL_COUNT - 1U]) && pxBuf->bInUse) {
        pxBuf->bInUse = false;
        pxBuf->ucLen = 0U;
        pxBuf->pcOwner = "POOL_FREE";
        ok = true;
    }
    pthread_mutex_unlock(&s_xPoolLock);
    return ok;
}

uint32_t ZeroCopyPool_GetFreeCount(void) {
    uint32_t freeCnt = 0U;
    pthread_mutex_lock(&s_xPoolLock);
    for (uint32_t i = 0U; i < ECU_POOL_COUNT; i++) {
        if (!s_xPool[i].bInUse) {
            freeCnt++;
        }
    }
    pthread_mutex_unlock(&s_xPoolLock);
    return freeCnt;
}

void ZeroCopyPool_DumpTable(void) {
    pthread_mutex_lock(&s_xPoolLock);
    printf("  [ZERO-COPY BUFFER POOL STATUS (%u x %zuB)]:\n", ECU_POOL_COUNT, sizeof(ZeroCopyBuf_t));
    for (uint32_t i = 0U; i < ECU_POOL_COUNT; i++) {
        printf("    Slot[%u] @ %p | InUse=%-5s | Owner=%-14s | Len=%uB\n",
               s_xPool[i].ulSlotId, (void *)&s_xPool[i],
               s_xPool[i].bInUse ? "TRUE" : "FALSE",
               s_xPool[i].pcOwner, s_xPool[i].ucLen);
    }
    pthread_mutex_unlock(&s_xPoolLock);
}
