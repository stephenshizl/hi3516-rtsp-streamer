#include "file_writer.h"
#include "hi_log.h"
#include "hi_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>

#define MAX_FILE_HANDLES    8
#define MP4_WRITE_BUF_SIZE  (256 * 1024)

typedef struct {
    HI_BOOL     bUsed;
    FILE       *fp;
    FILE_CONF_S stConf;
    HI_U64      u64TotalBytes;
    HI_U32      u32FrameCount;
    HI_U64      u64FirstPts;
    HI_U64      u64LastPts;
    pthread_mutex_t mutex;
} FILE_CTX_S;

static FILE_CTX_S g_astFileCtx[MAX_FILE_HANDLES];
static pthread_mutex_t g_fileMgrMutex = PTHREAD_MUTEX_INITIALIZER;

HI_S32 file_writer_init(void)
{
    memset(g_astFileCtx, 0, sizeof(g_astFileCtx));
    HI_LOG_I("File writer initialized");
    return HI_SUCCESS;
}

HI_VOID file_writer_deinit(void)
{
    HI_S32 i;
    for (i = 0; i < MAX_FILE_HANDLES; i++) {
        if (g_astFileCtx[i].bUsed) {
            file_writer_close(i);
        }
    }
    HI_LOG_I("File writer de-initialized");
}

static HI_S32 alloc_handle(void)
{
    HI_S32 i;
    for (i = 0; i < MAX_FILE_HANDLES; i++) {
        if (!g_astFileCtx[i].bUsed) {
            return i;
        }
    }
    return FILE_WRITER_INVALID_FD;
}

static void write_mp4_header_placeholder(FILE *fp)
{
    uint8_t ftyp[] = {
        0x00, 0x00, 0x00, 0x18,
        'f', 't', 'y', 'p',
        'i', 's', 'o', 'm',
        0x00, 0x00, 0x02, 0x00,
        'i', 's', 'o', 'm', 'm', 'p', '4', '1'
    };
    fwrite(ftyp, 1, sizeof(ftyp), fp);
}

FILE_HANDLE_T file_writer_open(const FILE_CONF_S *pstConf)
{
    FILE_HANDLE_T hFile;
    FILE_CTX_S   *pstCtx;
    char          szDir[256];
    char         *p;

    if (HI_NULL == pstConf) {
        return FILE_WRITER_INVALID_FD;
    }

    pthread_mutex_lock(&g_fileMgrMutex);
    hFile = alloc_handle();
    if (FILE_WRITER_INVALID_FD == hFile) {
        pthread_mutex_unlock(&g_fileMgrMutex);
        HI_LOG_E("No free file handle");
        return FILE_WRITER_INVALID_FD;
    }

    pstCtx = &g_astFileCtx[hFile];
    memset(pstCtx, 0, sizeof(FILE_CTX_S));

    strncpy(szDir, pstConf->szFilePath, sizeof(szDir) - 1);
    p = strrchr(szDir, '/');
    if (p) {
        *p = '\0';
        hi_mkdir_p(szDir, 0755);
    }

    pstCtx->fp = fopen(pstConf->szFilePath, "wb");
    if (HI_NULL == pstCtx->fp) {
        HI_LOG_E("fopen %s failed: %s", pstConf->szFilePath, strerror(errno));
        pthread_mutex_unlock(&g_fileMgrMutex);
        return FILE_WRITER_INVALID_FD;
    }

    memcpy(&pstCtx->stConf, pstConf, sizeof(FILE_CONF_S));

    if (pstConf->enFormat == FILE_FORMAT_MP4) {
        write_mp4_header_placeholder(pstCtx->fp);
    }

    pstCtx->u64FirstPts = 0;
    pstCtx->bUsed       = HI_TRUE;
    pthread_mutex_init(&pstCtx->mutex, HI_NULL);
    pthread_mutex_unlock(&g_fileMgrMutex);

    HI_LOG_I("File opened: %s (handle=%d)", pstConf->szFilePath, hFile);
    return hFile;
}

HI_S32 file_writer_write_video(FILE_HANDLE_T hFile, const HI_U8 *pData,
                                HI_U32 u32Len, HI_U64 u64Pts,
                                HI_BOOL bKeyFrame)
{
    FILE_CTX_S *pstCtx;
    size_t      nWritten;

    if (hFile < 0 || hFile >= MAX_FILE_HANDLES || !pData || u32Len == 0) {
        return HI_FAILURE;
    }

    pstCtx = &g_astFileCtx[hFile];
    if (!pstCtx->bUsed || HI_NULL == pstCtx->fp) {
        return HI_FAILURE;
    }

    pthread_mutex_lock(&pstCtx->mutex);

    if (pstCtx->u64FirstPts == 0 && u64Pts != 0) {
        pstCtx->u64FirstPts = u64Pts;
    }
    pstCtx->u64LastPts = u64Pts;

    if (pstCtx->stConf.enFormat == FILE_FORMAT_MP4) {
        uint8_t mdat_hdr[8];
        uint32_t box_size = u32Len + 8;
        mdat_hdr[0] = (box_size >> 24) & 0xFF;
        mdat_hdr[1] = (box_size >> 16) & 0xFF;
        mdat_hdr[2] = (box_size >>  8) & 0xFF;
        mdat_hdr[3] = (box_size      ) & 0xFF;
        mdat_hdr[4] = 'm'; mdat_hdr[5] = 'd';
        mdat_hdr[6] = 'a'; mdat_hdr[7] = 't';
        fwrite(mdat_hdr, 1, sizeof(mdat_hdr), pstCtx->fp);
    }

    nWritten = fwrite(pData, 1, u32Len, pstCtx->fp);
    if (nWritten != u32Len) {
        HI_LOG_E("fwrite incomplete: %zu/%u", nWritten, u32Len);
        pthread_mutex_unlock(&pstCtx->mutex);
        return HI_FAILURE;
    }

    pstCtx->u64TotalBytes += nWritten;
    pstCtx->u32FrameCount++;

    pthread_mutex_unlock(&pstCtx->mutex);
    return HI_SUCCESS;
}

HI_S32 file_writer_close(FILE_HANDLE_T hFile)
{
    FILE_CTX_S *pstCtx;

    if (hFile < 0 || hFile >= MAX_FILE_HANDLES) {
        return HI_FAILURE;
    }

    pstCtx = &g_astFileCtx[hFile];
    if (!pstCtx->bUsed) {
        return HI_SUCCESS;
    }

    pthread_mutex_lock(&pstCtx->mutex);

    if (pstCtx->fp != HI_NULL) {
        fflush(pstCtx->fp);
        fclose(pstCtx->fp);
        pstCtx->fp = HI_NULL;
    }

    HI_U64 u64DurMs = (pstCtx->u64LastPts > pstCtx->u64FirstPts)
                      ? (pstCtx->u64LastPts - pstCtx->u64FirstPts) / 1000
                      : 0;

    HI_LOG_I("File closed: %s frames=%u dur=%llus size=%lluKB",
             pstCtx->stConf.szFilePath,
             pstCtx->u32FrameCount,
             (unsigned long long)(u64DurMs / 1000),
             (unsigned long long)(pstCtx->u64TotalBytes / 1024));

    pthread_mutex_unlock(&pstCtx->mutex);
    pthread_mutex_destroy(&pstCtx->mutex);

    pthread_mutex_lock(&g_fileMgrMutex);
    pstCtx->bUsed = HI_FALSE;
    pthread_mutex_unlock(&g_fileMgrMutex);

    return HI_SUCCESS;
}

HI_S64 file_writer_get_size(FILE_HANDLE_T hFile)
{
    FILE_CTX_S *pstCtx;
    HI_S64 s64Size;

    if (hFile < 0 || hFile >= MAX_FILE_HANDLES) {
        return -1;
    }

    pstCtx = &g_astFileCtx[hFile];
    if (!pstCtx->bUsed) {
        return -1;
    }

    pthread_mutex_lock(&pstCtx->mutex);
    s64Size = (HI_S64)pstCtx->u64TotalBytes;
    pthread_mutex_unlock(&pstCtx->mutex);

    return s64Size;
}
