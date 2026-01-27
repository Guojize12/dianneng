
#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp_app.h"
#include "app_version.h"
#include "app_main.h"

/** 系统参数*/

#define APP_ENERGY_CHANNELS 3 // 能耗分析通道数宏定义，可根据需要调整

typedef struct {
    float voltage;
    float k;
    float b;
    float rms_base;
} energy_channel_param_t;

typedef struct
{
    uint32_t  did; //设备ID
    uint32_t  dwin_psd;  //密码
    char      model[20];           /** 产品型号*/
    char      ota_model[20];       /** OTA型号*/
    uint8_t   logo;
    uint8_t   type;                //设备类型(首页), 0深基坑 1围堰
    uint8_t   voice;
    uint8_t   ip_status;
    uint32_t  pin;
    uint32_t  pin_pw;
    uint32_t  en_otr;              //其它使能
    uint16_t  rtd_save_int;        //存储间隔

    // 每通道独立参数
    energy_channel_param_t channel[APP_ENERGY_CHANNELS];
} app_cfg_def;

extern app_cfg_def  g_app_cfg;

void APP_CONFIG_Init(void);

// 建议增加保存/加载能耗相关参数的函数，根据实际硬件Flash接口实现
void APP_CONFIG_Save(void);
void APP_CONFIG_Load(void);

#ifdef __cplusplus
}
#endif

// 电能参数单通道读写
void APP_CONFIG_EnergyParam_Set(const energy_channel_param_t *param, uint8_t ch);
void APP_CONFIG_EnergyParam_Get(energy_channel_param_t *param, uint8_t ch);
// 电能参数全通道读写
void APP_CONFIG_EnergyParam_AllSet(const energy_channel_param_t *params);
void APP_CONFIG_EnergyParam_AllGet(energy_channel_param_t *params);



#endif /* __APP_CONFIG_H */

/*****END OF FILE****/