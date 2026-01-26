#include "app_config.h"

app_cfg_def  g_app_cfg  =
{
    .did=123456,  			   //默认设备ID
    .model="00G-0-0G0",        //设备型号
};
 

void APP_CONFIG_Did_Set(uint32_t did)
{
    g_app_cfg.did = did;
    BSP_MEMORY_Write32(BSP_MEMORY_DID,g_app_cfg.did);
}

uint32_t APP_CONFIG_Did_Get(void)
{
    g_app_cfg.did= BSP_MEMORY_Read32(BSP_MEMORY_DID);
    return g_app_cfg.did;
}
 
uint32_t APP_CONFIG_Get_DwinPassword(void)
{ 
    return g_app_cfg.dwin_psd;
}

void APP_CONFIG_Set_DwinPassword(uint32_t password)
{
    g_app_cfg.dwin_psd=password; 
}
 
uint8_t APP_CONFIG_Get_Type(void)
{ 
    return g_app_cfg.type;
}

void APP_CONFIG_Set_Type(uint8_t type)
{
    g_app_cfg.type=type; 
}
 
void APP_CONFIG_SlaveSum_Set(uint32_t sum)
{
 
}

uint32_t APP_CONFIG_SlaveSum_Get(void)
{ 

    return 0;
}
 
void APP_CONFIG_Init(void)
{
    //初始化参数 
    
    APP_CONFIG_Did_Get();
}
/*****END OF FILE****/

