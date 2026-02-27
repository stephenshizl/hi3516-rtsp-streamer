#ifndef __FILE_WRITER_H__
#define __FILE_WRITER_H__

#include "hi_type.h"
#include "hi_common.h"

typedef enum {
    FILE_FORMAT_RAW  = 0,
    FILE_FORMAT_MP4  = 1,
    FILE_FORMAT_MKV  = 2,
    FILE_FORMAT_BUTT,
} FILE_FORMAT_E;

#define FILE_WRITER_INVALID_FD  (-1)

typedef struct {
    char          szFilePath[256];
    FILE_FORMAT_E enFormat;
    VENC_TYPE_E   enCodec;
    HI_U32        u32Width;
    HI_U32        u32Height;
    HI_U32        u32FrameRate;
    HI_U32        u32BitRate;
    HI_U32        u32Duration;
} FILE_CONF_S;

typedef HI_S32 FILE_HANDLE_T;

HI_S32       file_writer_init(void);
HI_VOID      file_writer_deinit(void);
FILE_HANDLE_T file_writer_open(const FILE_CONF_S *pstConf);
HI_S32       file_writer_write_video(FILE_HANDLE_T hFile, const HI_U8 *pData,
                                      HI_U32 u32Len, HI_U64 u64Pts,
                                      HI_BOOL bKeyFrame);
HI_S32       file_writer_close(FILE_HANDLE_T hFile);
HI_S64       file_writer_get_size(FILE_HANDLE_T hFile);

#endif /* __FILE_WRITER_H__ */
