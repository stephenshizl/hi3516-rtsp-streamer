#ifndef __HI_UTILS_H__
#define __HI_UTILS_H__

#include "hi_type.h"
#include <stddef.h>
#include <pthread.h>

HI_U64  hi_get_timestamp_ms(void);
HI_U64  hi_get_timestamp_us(void);
HI_VOID hi_sleep_ms(HI_U32 u32Ms);

HI_S32  hi_mkdir_p(const char *pszPath, HI_U32 u32Mode);
HI_BOOL hi_file_exists(const char *pszPath);
HI_S64  hi_file_size(const char *pszPath);
HI_S32  hi_dir_free_space_mb(const char *pszPath);

HI_VOID hi_hex_dump(const HI_U8 *pBuf, HI_U32 u32Len, const char *pszTag);

typedef struct {
    HI_U8  *pBuf;
    HI_U32  u32Size;
    HI_U32  u32Head;
    HI_U32  u32Tail;
    HI_U32  u32Used;
    pthread_mutex_t mutex;
    pthread_cond_t  cond_not_empty;
    pthread_cond_t  cond_not_full;
} RING_BUF_S;

HI_S32  ring_buf_init(RING_BUF_S *pstRing, HI_U32 u32Size);
HI_VOID ring_buf_deinit(RING_BUF_S *pstRing);
HI_S32  ring_buf_write(RING_BUF_S *pstRing, const HI_U8 *pData, HI_U32 u32Len);
HI_S32  ring_buf_read(RING_BUF_S *pstRing, HI_U8 *pData, HI_U32 u32Len);
HI_U32  ring_buf_used(RING_BUF_S *pstRing);
HI_VOID ring_buf_flush(RING_BUF_S *pstRing);

#endif /* __HI_UTILS_H__ */
