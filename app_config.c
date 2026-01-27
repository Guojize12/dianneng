#include "app_config.h"
#include "bsp_flash.h"
#include <string.h>
#include <math.h>

// 默认参数常量（用于首次上电/参数区无效时恢复）
static const app_cfg_def g_app_cfg_defaults =
{
    .did = 123456,              // 默认设备ID
    .dwin_psd = 0,
    .model = "00G-0-0G0",       // 产品型号
    .ota_model = "",
    .logo = 0,
    .type = 0,
    .voice = 0,
    .ip_status = 0,
    .pin = 0,
    .pin_pw = 0,
    .en_otr = 0,
    .rtd_save_int = 0,
    .channel =
    {
        {220.0f, 1.0f, 0.0f, 0.0f}, // CH0
        {220.0f, 1.0f, 0.0f, 0.0f}, // CH1
        {220.0f, 1.0f, 0.0f, 0.0f}  // CH2
    }
};

// 参数结构体全局变量（运行时快照）
app_cfg_def  g_app_cfg =
{
    .did = 123456,              // 默认设备ID
    .dwin_psd = 0,
    .model = "00G-0-0G0",       // 产品型号
    .ota_model = "",
    .logo = 0,
    .type = 0,
    .voice = 0,
    .ip_status = 0,
    .pin = 0,
    .pin_pw = 0,
    .en_otr = 0,
    .rtd_save_int = 0,
    .channel =
    {
        {220.0f, 1.0f, 0.0f, 0.0f}, // CH0
        {220.0f, 1.0f, 0.0f, 0.0f}, // CH1
        {220.0f, 1.0f, 0.0f, 0.0f}  // CH2
    }
};

// 指定Flash参数区（可以根据实际FLASH分区修改，示例地址）
#define APP_CONFIG_FLASH_ADDR  (BSP_FLASH_PARA_START) // 推荐用分区起始地址

// 简单的参数合法性校验：避免全FF/全0或异常值
static int APP_CONFIG_ParamsValid(const app_cfg_def* cfg)
{
    if (!cfg) return 0;

    // did 为 0xFFFFFFFF 或 0 被视为无效（Flash擦除态或未初始化）
    if (cfg->did == 0xFFFFFFFFu || cfg->did == 0u) {
        return 0;
    }

    // model 字符串需为可打印且非全0xFF（简化：首字符在可见范围）
    if (cfg->model[0] == (char)0xFF || cfg->model[0] == '\0') {
        // 允许空串，但如果空又 did 异常已被前面拦截；这里做额外兜底
        // 不直接据此判无效，继续检查通道
    }

    // 通道参数范围判断（避免 NaN/Inf/极端异常值）
    for (uint8_t i = 0; i < APP_ENERGY_CHANNELS; i++) {
        float v = cfg->channel[i].voltage;
        float k = cfg->channel[i].k;
        float b = cfg->channel[i].b;
        // 合理范围：电压(0, 500)，k(-1000, 1000)，b绝对值<1e6
        if (!(v > 0.0f && v < 500.0f)) return 0;
        if (!(k > -1000.0f && k < 1000.0f)) return 0;
        if (!(fabsf(b) < 1e6f)) return 0;
    }
    return 1;
}

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

    // 检查合法性：无效则写入默认值到Flash
    if (!APP_CONFIG_ParamsValid(&g_app_cfg)) {
        // 恢复默认
        memcpy(&g_app_cfg, &g_app_cfg_defaults, sizeof(g_app_cfg));
        // 永久保存
        APP_CONFIG_Save();
    }
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

// 一次性设置全部通道电��参数并存储
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