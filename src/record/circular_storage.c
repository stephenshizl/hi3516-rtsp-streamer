#include "circular_storage.h"
#include "hi_log.h"
#include "hi_utils.h"
#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

#define MAX_FILE_PATH_LEN  512

typedef struct {
    char              szPath[MAX_FILE_PATH_LEN];
    time_t            tModTime;
    HI_U64            u64SizeBytes;
    struct list_head  node;
} FILE_NODE_S;

typedef struct {
    CIRCULAR_STORAGE_CONF_S stConf;
    struct list_head         fileList;
    HI_U32                   u32FileCnt;
    pthread_mutex_t          mutex;
    HI_BOOL                  bInitialized;
} CIRCULAR_STORAGE_CTX_S;

static CIRCULAR_STORAGE_CTX_S g_stCsCtx;

static int compare_file_time(const struct dirent **a, const struct dirent **b)
{
    struct stat sa, sb;
    stat((*a)->d_name, &sa);
    stat((*b)->d_name, &sb);
    if (sa.st_mtime < sb.st_mtime) return -1;
    if (sa.st_mtime > sb.st_mtime) return  1;
    return 0;
}

static void free_file_list(void)
{
    struct list_head *pos, *n;
    list_for_each_safe(pos, n, &g_stCsCtx.fileList) {
        FILE_NODE_S *node = list_entry(pos, FILE_NODE_S, node);
        list_del(&node->node);
        free(node);
    }
    g_stCsCtx.u32FileCnt = 0;
}

static HI_S32 scan_directory(void)
{
    DIR            *dir;
    struct dirent  *ent;
    struct stat     st;
    char            szPath[MAX_FILE_PATH_LEN];

    free_file_list();

    dir = opendir(g_stCsCtx.stConf.szPath);
    if (HI_NULL == dir) {
        return HI_FAILURE;
    }

    while ((ent = readdir(dir)) != HI_NULL) {
        if (ent->d_name[0] == '.') {
            continue;
        }

        snprintf(szPath, sizeof(szPath), "%s/%s",
                 g_stCsCtx.stConf.szPath, ent->d_name);
        if (stat(szPath, &st) != 0) {
            continue;
        }

        if (!S_ISREG(st.st_mode)) {
            continue;
        }

        const char *ext = strrchr(ent->d_name, '.');
        if (!ext || (strcmp(ext, ".mp4") != 0 && strcmp(ext, ".mkv") != 0
                     && strcmp(ext, ".h264") != 0 && strcmp(ext, ".h265") != 0)) {
            continue;
        }

        FILE_NODE_S *node = (FILE_NODE_S *)malloc(sizeof(FILE_NODE_S));
        if (HI_NULL == node) {
            break;
        }

        strncpy(node->szPath, szPath, sizeof(node->szPath) - 1);
        node->tModTime    = st.st_mtime;
        node->u64SizeBytes = (HI_U64)st.st_size;
        list_add_tail(&node->node, &g_stCsCtx.fileList);
        g_stCsCtx.u32FileCnt++;
    }

    closedir(dir);
    return HI_SUCCESS;
}

static HI_S32 delete_oldest_files_until_free(HI_U64 u64NeedMb)
{
    HI_S64 s64Free;
    HI_S32 s32Deleted = 0;
    struct list_head *pos, *n;

    s64Free = hi_dir_free_space_mb(g_stCsCtx.stConf.szPath);
    if (s64Free < 0) {
        return HI_FAILURE;
    }

    list_for_each_safe(pos, n, &g_stCsCtx.fileList) {
        if ((HI_U64)s64Free >= u64NeedMb) {
            break;
        }

        FILE_NODE_S *node = list_entry(pos, FILE_NODE_S, node);

        if (remove(node->szPath) == 0) {
            s64Free += (HI_S64)(node->u64SizeBytes / (1024 * 1024));
            HI_LOG_I("Deleted old record: %s", node->szPath);
            s32Deleted++;
        } else {
            HI_LOG_W("Failed to delete: %s err=%s", node->szPath, strerror(errno));
        }

        list_del(&node->node);
        free(node);
        g_stCsCtx.u32FileCnt--;
    }

    return s32Deleted > 0 ? HI_SUCCESS : HI_FAILURE;
}

static HI_S32 delete_expired_files(void)
{
    time_t           tNow = time(HI_NULL);
    time_t           tMaxAge;
    HI_S32           s32Deleted = 0;
    struct list_head *pos, *n;

    if (g_stCsCtx.stConf.u32MaxFileAgeDays == 0) {
        return HI_SUCCESS;
    }

    tMaxAge = (time_t)g_stCsCtx.stConf.u32MaxFileAgeDays * 24 * 3600;

    list_for_each_safe(pos, n, &g_stCsCtx.fileList) {
        FILE_NODE_S *node = list_entry(pos, FILE_NODE_S, node);

        if ((tNow - node->tModTime) > tMaxAge) {
            if (remove(node->szPath) == 0) {
                HI_LOG_I("Deleted expired: %s", node->szPath);
                s32Deleted++;
            }
            list_del(&node->node);
            free(node);
            g_stCsCtx.u32FileCnt--;
        }
    }

    return HI_SUCCESS;
}

HI_S32 circular_storage_init(const CIRCULAR_STORAGE_CONF_S *pstConf)
{
    if (HI_NULL == pstConf) {
        return HI_FAILURE;
    }

    memset(&g_stCsCtx, 0, sizeof(g_stCsCtx));
    memcpy(&g_stCsCtx.stConf, pstConf, sizeof(CIRCULAR_STORAGE_CONF_S));
    INIT_LIST_HEAD(&g_stCsCtx.fileList);
    pthread_mutex_init(&g_stCsCtx.mutex, HI_NULL);

    hi_mkdir_p(pstConf->szPath, 0755);
    scan_directory();

    g_stCsCtx.bInitialized = HI_TRUE;
    HI_LOG_I("Circular storage initialized: path=%s minFree=%lluMB maxDays=%u",
             pstConf->szPath,
             (unsigned long long)pstConf->u64MinFreeSpaceMb,
             pstConf->u32MaxFileAgeDays);
    return HI_SUCCESS;
}

HI_VOID circular_storage_deinit(void)
{
    if (!g_stCsCtx.bInitialized) {
        return;
    }

    pthread_mutex_lock(&g_stCsCtx.mutex);
    free_file_list();
    pthread_mutex_unlock(&g_stCsCtx.mutex);

    pthread_mutex_destroy(&g_stCsCtx.mutex);
    g_stCsCtx.bInitialized = HI_FALSE;
}

HI_S32 circular_storage_check(void)
{
    HI_S64 s64Free;

    if (!g_stCsCtx.bInitialized) {
        return HI_FAILURE;
    }

    pthread_mutex_lock(&g_stCsCtx.mutex);

    scan_directory();
    delete_expired_files();

    s64Free = hi_dir_free_space_mb(g_stCsCtx.stConf.szPath);
    if (s64Free >= 0 &&
        (HI_U64)s64Free < g_stCsCtx.stConf.u64MinFreeSpaceMb) {
        HI_LOG_W("Low disk space: %lldMB, cleaning old files", (long long)s64Free);
        delete_oldest_files_until_free(g_stCsCtx.stConf.u64MinFreeSpaceMb);
    }

    pthread_mutex_unlock(&g_stCsCtx.mutex);
    return HI_SUCCESS;
}

HI_S32 circular_storage_add_file(const char *pszFilePath)
{
    struct stat  st;
    FILE_NODE_S *node;

    if (HI_NULL == pszFilePath || !g_stCsCtx.bInitialized) {
        return HI_FAILURE;
    }

    if (stat(pszFilePath, &st) != 0) {
        return HI_FAILURE;
    }

    node = (FILE_NODE_S *)malloc(sizeof(FILE_NODE_S));
    if (HI_NULL == node) {
        return HI_FAILURE;
    }

    strncpy(node->szPath, pszFilePath, sizeof(node->szPath) - 1);
    node->tModTime     = st.st_mtime;
    node->u64SizeBytes = (HI_U64)st.st_size;

    pthread_mutex_lock(&g_stCsCtx.mutex);
    list_add_tail(&node->node, &g_stCsCtx.fileList);
    g_stCsCtx.u32FileCnt++;
    pthread_mutex_unlock(&g_stCsCtx.mutex);

    return HI_SUCCESS;
}

HI_S32 circular_storage_remove_file(const char *pszFilePath)
{
    struct list_head *pos, *n;

    if (HI_NULL == pszFilePath) {
        return HI_FAILURE;
    }

    pthread_mutex_lock(&g_stCsCtx.mutex);

    list_for_each_safe(pos, n, &g_stCsCtx.fileList) {
        FILE_NODE_S *node = list_entry(pos, FILE_NODE_S, node);
        if (strcmp(node->szPath, pszFilePath) == 0) {
            list_del(&node->node);
            free(node);
            g_stCsCtx.u32FileCnt--;
            break;
        }
    }

    pthread_mutex_unlock(&g_stCsCtx.mutex);
    return HI_SUCCESS;
}

HI_S64 circular_storage_get_free_mb(void)
{
    return hi_dir_free_space_mb(g_stCsCtx.stConf.szPath);
}
