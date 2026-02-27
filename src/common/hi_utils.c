#include "hi_utils.h"
#include "hi_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

HI_U64 hi_get_timestamp_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (HI_U64)ts.tv_sec * 1000 + (HI_U64)ts.tv_nsec / 1000000;
}

HI_U64 hi_get_timestamp_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (HI_U64)ts.tv_sec * 1000000 + (HI_U64)ts.tv_nsec / 1000;
}

HI_VOID hi_sleep_ms(HI_U32 u32Ms)
{
    struct timespec ts;
    ts.tv_sec  = u32Ms / 1000;
    ts.tv_nsec = (u32Ms % 1000) * 1000000;
    nanosleep(&ts, HI_NULL);
}

HI_S32 hi_mkdir_p(const char *pszPath, HI_U32 u32Mode)
{
    char   szTmp[512];
    char  *p = HI_NULL;
    size_t len;

    if (HI_NULL == pszPath) {
        return HI_FAILURE;
    }

    snprintf(szTmp, sizeof(szTmp), "%s", pszPath);
    len = strlen(szTmp);
    if (len == 0) {
        return HI_FAILURE;
    }

    if (szTmp[len - 1] == '/') {
        szTmp[len - 1] = '\0';
    }

    for (p = szTmp + 1; *p != '\0'; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(szTmp, (mode_t)u32Mode) != 0 && errno != EEXIST) {
                HI_LOG_E("mkdir %s failed: %s", szTmp, strerror(errno));
                return HI_FAILURE;
            }
            *p = '/';
        }
    }

    if (mkdir(szTmp, (mode_t)u32Mode) != 0 && errno != EEXIST) {
        HI_LOG_E("mkdir %s failed: %s", szTmp, strerror(errno));
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_BOOL hi_file_exists(const char *pszPath)
{
    struct stat st;
    return (stat(pszPath, &st) == 0) ? HI_TRUE : HI_FALSE;
}

HI_S64 hi_file_size(const char *pszPath)
{
    struct stat st;
    if (stat(pszPath, &st) != 0) {
        return -1;
    }
    return (HI_S64)st.st_size;
}

HI_S32 hi_dir_free_space_mb(const char *pszPath)
{
    struct statvfs svfs;
    if (statvfs(pszPath, &svfs) != 0) {
        return -1;
    }
    return (HI_S32)((HI_U64)svfs.f_bsize * svfs.f_bavail / (1024 * 1024));
}

HI_VOID hi_hex_dump(const HI_U8 *pBuf, HI_U32 u32Len, const char *pszTag)
{
    HI_U32 i;
    char   szLine[80];
    HI_U32 u32Off = 0;

    HI_LOG_D("[%s] len=%u", pszTag ? pszTag : "HEX", u32Len);
    for (i = 0; i < u32Len && i < 64; i++) {
        if (i % 16 == 0) {
            u32Off = 0;
            u32Off += snprintf(szLine + u32Off, sizeof(szLine) - u32Off,
                               "%04x: ", i);
        }
        u32Off += snprintf(szLine + u32Off, sizeof(szLine) - u32Off,
                           "%02x ", pBuf[i]);
        if ((i + 1) % 16 == 0) {
            HI_LOG_D("%s", szLine);
        }
    }
    if (i % 16 != 0) {
        HI_LOG_D("%s", szLine);
    }
}

HI_S32 ring_buf_init(RING_BUF_S *pstRing, HI_U32 u32Size)
{
    if (HI_NULL == pstRing || u32Size == 0) {
        return HI_FAILURE;
    }

    pstRing->pBuf = (HI_U8 *)malloc(u32Size);
    if (HI_NULL == pstRing->pBuf) {
        HI_LOG_E("malloc ring buf failed, size=%u", u32Size);
        return HI_FAILURE;
    }

    pstRing->u32Size = u32Size;
    pstRing->u32Head = 0;
    pstRing->u32Tail = 0;
    pstRing->u32Used = 0;
    pthread_mutex_init(&pstRing->mutex, HI_NULL);
    pthread_cond_init(&pstRing->cond_not_empty, HI_NULL);
    pthread_cond_init(&pstRing->cond_not_full, HI_NULL);
    return HI_SUCCESS;
}

HI_VOID ring_buf_deinit(RING_BUF_S *pstRing)
{
    if (HI_NULL == pstRing) {
        return;
    }

    pthread_mutex_lock(&pstRing->mutex);
    if (pstRing->pBuf != HI_NULL) {
        free(pstRing->pBuf);
        pstRing->pBuf = HI_NULL;
    }
    pthread_mutex_unlock(&pstRing->mutex);

    pthread_cond_broadcast(&pstRing->cond_not_empty);
    pthread_cond_broadcast(&pstRing->cond_not_full);
    pthread_mutex_destroy(&pstRing->mutex);
    pthread_cond_destroy(&pstRing->cond_not_empty);
    pthread_cond_destroy(&pstRing->cond_not_full);
}

HI_S32 ring_buf_write(RING_BUF_S *pstRing, const HI_U8 *pData, HI_U32 u32Len)
{
    HI_U32 u32Free;
    HI_U32 u32Part1;

    if (HI_NULL == pstRing || HI_NULL == pData || u32Len == 0) {
        return HI_FAILURE;
    }

    pthread_mutex_lock(&pstRing->mutex);

    u32Free = pstRing->u32Size - pstRing->u32Used;
    if (u32Len > u32Free) {
        pthread_mutex_unlock(&pstRing->mutex);
        return HI_FAILURE;
    }

    u32Part1 = pstRing->u32Size - pstRing->u32Tail;
    if (u32Len <= u32Part1) {
        memcpy(pstRing->pBuf + pstRing->u32Tail, pData, u32Len);
    } else {
        memcpy(pstRing->pBuf + pstRing->u32Tail, pData, u32Part1);
        memcpy(pstRing->pBuf, pData + u32Part1, u32Len - u32Part1);
    }

    pstRing->u32Tail = (pstRing->u32Tail + u32Len) % pstRing->u32Size;
    pstRing->u32Used += u32Len;
    pthread_cond_signal(&pstRing->cond_not_empty);
    pthread_mutex_unlock(&pstRing->mutex);
    return HI_SUCCESS;
}

HI_S32 ring_buf_read(RING_BUF_S *pstRing, HI_U8 *pData, HI_U32 u32Len)
{
    HI_U32 u32Part1;

    if (HI_NULL == pstRing || HI_NULL == pData || u32Len == 0) {
        return HI_FAILURE;
    }

    pthread_mutex_lock(&pstRing->mutex);

    if (pstRing->u32Used < u32Len) {
        pthread_mutex_unlock(&pstRing->mutex);
        return HI_FAILURE;
    }

    u32Part1 = pstRing->u32Size - pstRing->u32Head;
    if (u32Len <= u32Part1) {
        memcpy(pData, pstRing->pBuf + pstRing->u32Head, u32Len);
    } else {
        memcpy(pData, pstRing->pBuf + pstRing->u32Head, u32Part1);
        memcpy(pData + u32Part1, pstRing->pBuf, u32Len - u32Part1);
    }

    pstRing->u32Head = (pstRing->u32Head + u32Len) % pstRing->u32Size;
    pstRing->u32Used -= u32Len;
    pthread_cond_signal(&pstRing->cond_not_full);
    pthread_mutex_unlock(&pstRing->mutex);
    return HI_SUCCESS;
}

HI_U32 ring_buf_used(RING_BUF_S *pstRing)
{
    HI_U32 u32Used;
    pthread_mutex_lock(&pstRing->mutex);
    u32Used = pstRing->u32Used;
    pthread_mutex_unlock(&pstRing->mutex);
    return u32Used;
}

HI_VOID ring_buf_flush(RING_BUF_S *pstRing)
{
    pthread_mutex_lock(&pstRing->mutex);
    pstRing->u32Head = 0;
    pstRing->u32Tail = 0;
    pstRing->u32Used = 0;
    pthread_cond_broadcast(&pstRing->cond_not_full);
    pthread_mutex_unlock(&pstRing->mutex);
}
