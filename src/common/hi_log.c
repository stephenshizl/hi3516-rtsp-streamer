#include "hi_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <syslog.h>
#include <sys/stat.h>
#include <libgen.h>

#define LOG_BUF_SIZE    1024
#define LOG_TIME_LEN    32

static LOG_CONF_S    g_stLogConf;
static FILE         *g_pLogFile = NULL;
static pthread_mutex_t g_logMutex = PTHREAD_MUTEX_INITIALIZER;
static HI_BOOL       g_bInitialized = HI_FALSE;

static const char *s_szLevelStr[] = {
    "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static const int s_nSyslogPri[] = {
    LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERR, LOG_CRIT
};

static void log_get_time(char *pszBuf, HI_U32 u32Len)
{
    struct timespec ts;
    struct tm       tm_info;

    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r(&ts.tv_sec, &tm_info);
    snprintf(pszBuf, u32Len, "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
             tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday,
             tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec,
             ts.tv_nsec / 1000000);
}

static void log_rotate(void)
{
    struct stat st;
    char        szBakFile[280];

    if (g_pLogFile == HI_NULL) {
        return;
    }

    fstat(fileno(g_pLogFile), &st);
    if ((HI_U32)st.st_size < g_stLogConf.u32MaxFileSize) {
        return;
    }

    fclose(g_pLogFile);
    g_pLogFile = HI_NULL;

    snprintf(szBakFile, sizeof(szBakFile), "%s.1", g_stLogConf.szLogFile);
    rename(g_stLogConf.szLogFile, szBakFile);

    g_pLogFile = fopen(g_stLogConf.szLogFile, "a");
}

HI_S32 hi_log_init(const LOG_CONF_S *pstConf)
{
    if (HI_NULL == pstConf) {
        return HI_FAILURE;
    }

    pthread_mutex_lock(&g_logMutex);

    memcpy(&g_stLogConf, pstConf, sizeof(LOG_CONF_S));

    if (g_stLogConf.u32MaxFileSize == 0) {
        g_stLogConf.u32MaxFileSize = 10 * 1024 * 1024;
    }

    if ((pstConf->enOutput & LOG_OUTPUT_SYSLOG) != 0) {
        openlog("ipcamera", LOG_PID | LOG_CONS, LOG_USER);
    }

    if ((pstConf->enOutput & LOG_OUTPUT_FILE) != 0 &&
        pstConf->szLogFile[0] != '\0') {
        g_pLogFile = fopen(pstConf->szLogFile, "a");
        if (g_pLogFile == HI_NULL) {
            fprintf(stderr, "Failed to open log file: %s\n", pstConf->szLogFile);
        }
    }

    g_bInitialized = HI_TRUE;
    pthread_mutex_unlock(&g_logMutex);
    return HI_SUCCESS;
}

HI_VOID hi_log_deinit(void)
{
    pthread_mutex_lock(&g_logMutex);

    if (g_pLogFile != HI_NULL) {
        fclose(g_pLogFile);
        g_pLogFile = HI_NULL;
    }

    if ((g_stLogConf.enOutput & LOG_OUTPUT_SYSLOG) != 0) {
        closelog();
    }

    g_bInitialized = HI_FALSE;
    pthread_mutex_unlock(&g_logMutex);
}

HI_VOID hi_log_set_level(LOG_LEVEL_E enLevel)
{
    pthread_mutex_lock(&g_logMutex);
    g_stLogConf.enLevel = enLevel;
    pthread_mutex_unlock(&g_logMutex);
}

HI_VOID hi_log_write(LOG_LEVEL_E enLevel, const char *pszFile,
                     HI_S32 s32Line, const char *pszFmt, ...)
{
    char    szTimeBuf[LOG_TIME_LEN];
    char    szMsgBuf[LOG_BUF_SIZE];
    char    szLogBuf[LOG_BUF_SIZE + 128];
    va_list args;
    const char *pszBase;

    if (!g_bInitialized || enLevel < g_stLogConf.enLevel) {
        return;
    }

    va_start(args, pszFmt);
    vsnprintf(szMsgBuf, sizeof(szMsgBuf), pszFmt, args);
    va_end(args);

    pszBase = pszFile;
    const char *p = pszFile;
    while (*p) {
        if (*p == '/' || *p == '\\') {
            pszBase = p + 1;
        }
        p++;
    }

    log_get_time(szTimeBuf, sizeof(szTimeBuf));
    snprintf(szLogBuf, sizeof(szLogBuf), "[%s][%s][%s:%d] %s\n",
             szTimeBuf,
             (enLevel < LOG_LEVEL_BUTT) ? s_szLevelStr[enLevel] : "UNKN",
             pszBase, s32Line, szMsgBuf);

    pthread_mutex_lock(&g_logMutex);

    if ((g_stLogConf.enOutput & LOG_OUTPUT_STDOUT) != 0) {
        fputs(szLogBuf, (enLevel >= LOG_LEVEL_WARN) ? stderr : stdout);
    }

    if ((g_stLogConf.enOutput & LOG_OUTPUT_FILE) != 0 && g_pLogFile != HI_NULL) {
        log_rotate();
        if (g_pLogFile != HI_NULL) {
            fputs(szLogBuf, g_pLogFile);
            fflush(g_pLogFile);
        }
    }

    if ((g_stLogConf.enOutput & LOG_OUTPUT_SYSLOG) != 0 &&
        enLevel < LOG_LEVEL_BUTT) {
        syslog(s_nSyslogPri[enLevel], "%s", szMsgBuf);
    }

    pthread_mutex_unlock(&g_logMutex);
}
