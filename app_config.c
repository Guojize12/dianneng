
#include "app_config.h"
#include "bsp_flash.h"

// 参数结构体全局变量
app_cfg_def  g_app_cfg  =
{
    .did = 123456,              // 默认设备ID
    .model = "00G-0-0G0",       // 产品型号
    .channel = 
    {
        {220.0f, 1.0f, 0.0f, 0.0f}, // CH0
        {220.0f, 1.0f, 0.0f, 0.0f}, // CH1
        {220.0f, 1.0f, 0.0f, 0.0f}  // CH2
    }
};

// 指定Flash参数区（可以根据实际FLASH分区修改，示例地址）
#define APP_CONFIG_FLASH_ADDR  (BSP_FLASH_PARA_START) // 推荐用分区起始地址

void APP_CONFIG_Save(void)
{
    // 写整个结构体到Flash
    // 擦除Flash页
    BSP_FLASH_Erase_Pages(APP_CONFIG_FLASH_ADDR, APP_CONFIG_FLASH_ADDR + sizeof(app_cfg_def));
    // 写数据（按4字节对齐）
    BSP_FLASH_Write(APP_CONFIG_FLASH_ADDR, (uint32_t*)&g_app_cfg, sizeof(app_cfg_def));
}

void APP_CONFIG_Load(void)
{
    // 读整个结构体到内存
    BSP_FLASH_Read(APP_CONFIG_FLASH_ADDR, (uint32_t*)&g_app_cfg, sizeof(app_cfg_def));
}

void APP_CONFIG_Did_Set(uint32_t did)
{
    g_app_cfg.did = did;
    APP_CONFIG_Save(); // 写入Flash
}

uint32_t APP_CONFIG_Did_Get(void)
{
    return g_app_cfg.did;
}

uint32_t APP_CONFIG_Get_DwinPassword(void)
{
    return g_app_cfg.dwin_psd;
}

void APP_CONFIG_Set_DwinPassword(uint32_t password)
{
    g_app_cfg.dwin_psd = password;
    APP_CONFIG_Save();
}

uint8_t APP_CONFIG_Get_Type(void)
{
    return g_app_cfg.type;
}

void APP_CONFIG_Set_Type(uint8_t type)
{
    g_app_cfg.type = type;
    APP_CONFIG_Save();
}

void APP_CONFIG_SlaveSum_Set(uint32_t sum)
{
    // 预留，具体实现根据实际需求
}

uint32_t APP_CONFIG_SlaveSum_Get(void)
{
    return 0;
}

void APP_CONFIG_Init(void)
{
    // 启动时先读Flash参数
    APP_CONFIG_Load();
    // 如果Flash无效（首次启动），可重新初始化默认值
    // 也可在APP_CONFIG_Load中做内容合法性校验
}

// 设置单通道电能参数并存储
void APP_CONFIG_EnergyParam_Set(const energy_channel_param_t *param, uint8_t ch)
{
    if (ch < APP_ENERGY_CHANNELS && param) {
        g_app_cfg.channel[ch] = *param;
        APP_CONFIG_Save();
    }
}

// 读取单通道电能参数
void APP_CONFIG_EnergyParam_Get(energy_channel_param_t *param, uint8_t ch)
{
    if (ch < APP_ENERGY_CHANNELS && param) {
        *param = g_app_cfg.channel[ch];
    }
}

// 一次性设置全部通道电能参数并存储
void APP_CONFIG_EnergyParam_AllSet(const energy_channel_param_t *params)
{
    if (params) {
        for (uint8_t i = 0; i < APP_ENERGY_CHANNELS; i++) {
            g_app_cfg.channel[i] = params[i];
        }
        APP_CONFIG_Save();
    }
}

// 一次性读取全部通道电能参数
void APP_CONFIG_EnergyParam_AllGet(energy_channel_param_t *params)
{
    if (params) {
        for (uint8_t i = 0; i < APP_ENERGY_CHANNELS; i++) {
            params[i] = g_app_cfg.channel[i];
        }
    }
}

/*****END OF FILE****/