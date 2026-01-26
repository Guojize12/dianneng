#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif
 
#include "bsp_app.h"
#include "app_version.h"
#include "app_main.h" 
#include "app_config.h" 

/** 系统参数*/
typedef struct
{
    uint32_t  did; //设备ID
    uint32_t  dwin_psd;  //密码
    char      model[20];/** 产品型号*/
    char      ota_model[20];/** OTA型号*/
    uint8_t   logo;
    uint8_t   type;//设备类型(首页), 0深基坑 1围堰
    uint8_t   voice;
    uint8_t   ip_status;
    uint32_t  pin;
    uint32_t  pin_pw;
    uint32_t  en_otr;//其它使能
    uint16_t  rtd_save_int;//存储间隔 
} app_cfg_def;
extern app_cfg_def  g_app_cfg;
 
void APP_CONFIG_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_CONFIG_H */

/*****END OF FILE****/

