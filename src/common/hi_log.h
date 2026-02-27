#ifndef __HI_LOG_H__
#define __HI_LOG_H__

#include "hi_type.h"
#include <stdio.h>

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_BUTT,
} LOG_LEVEL_E;

typedef enum {
    LOG_OUTPUT_STDOUT  = 0x01,
    LOG_OUTPUT_FILE    = 0x02,
    LOG_OUTPUT_SYSLOG  = 0x04,
} LOG_OUTPUT_E;

typedef struct {
    LOG_LEVEL_E  enLevel;
    LOG_OUTPUT_E enOutput;
    char         szLogFile[256];
    HI_U32       u32MaxFileSize;
    HI_U32       u32MaxBackups;
} LOG_CONF_S;

HI_S32  hi_log_init(const LOG_CONF_S *pstConf);
HI_VOID hi_log_deinit(void);
HI_VOID hi_log_set_level(LOG_LEVEL_E enLevel);
HI_VOID hi_log_write(LOG_LEVEL_E enLevel, const char *pszFile,
                     HI_S32 s32Line, const char *pszFmt, ...);

#define HI_LOG_D(fmt, ...) \
    hi_log_write(LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define HI_LOG_I(fmt, ...) \
    hi_log_write(LOG_LEVEL_INFO,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define HI_LOG_W(fmt, ...) \
    hi_log_write(LOG_LEVEL_WARN,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define HI_LOG_E(fmt, ...) \
    hi_log_write(LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define HI_LOG_F(fmt, ...) \
    hi_log_write(LOG_LEVEL_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif /* __HI_LOG_H__ */
