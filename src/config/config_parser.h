#ifndef __CONFIG_PARSER_H__
#define __CONFIG_PARSER_H__

#include "hi_type.h"
#include "hi_conf.h"

HI_S32  config_parse(const char *pszFile, IPCAMERA_CONF_S *pstConf);
HI_S32  config_save(const char *pszFile, const IPCAMERA_CONF_S *pstConf);
HI_VOID ipcamera_conf_set_default(IPCAMERA_CONF_S *pstConf);

#endif /* __CONFIG_PARSER_H__ */
