#include "adc.h"
#include "app_energy.h"
#include "user_log.h"
#include "app_config.h"  // 新增头文件，使用g_app_cfg

#define ADC_CHANNELS      3
#define POINTS_PER_CH     1000
#define DMA_BUF_SIZE      (ADC_CHANNELS * POINTS_PER_CH)

// 12位ADC最大值
#define ADC_MAX_VALUE     4095U
#define CLIP_THRESHOLD    2  

// RMS底值校准标志（每通道独立，运行时参数）
volatile uint8_t calib_rms_base_flag[ADC_CHANNELS] = {0};

// ADC偏置（每通道）
static float g_adc_offset[ADC_CHANNELS] = {0};

// 一次性DMA采样缓冲区
static uint16_t energy_adc_buf[DMA_BUF_SIZE] = {0};
// 分拣后的二维数据
static uint16_t rms_data[POINTS_PER_CH][ADC_CHANNELS] = {0};
static volatile uint8_t rms_ready_flag = 0;

// 参数（默认3通道、每通道1000点）
static uint16_t g_rms_length = POINTS_PER_CH;
static uint8_t  g_rms_channel = ADC_CHANNELS;

// 结果
static energy_result_t g_energy_result[ADC_CHANNELS];

// ------------------------参数访问方式修改---------------------------
// 1. default_voltage、default_k、default_b、rms_base都改为访问g_app_cfg
// 2. 临时参数如g_adc_offset仍保留本文件静态
// 3. 代码内对底值和计算公式参数全部从g_app_cfg取值

void energy_print_params(void) 
{
    LOG("[ADC] params: length=%d, channel=%d\r\n", g_rms_length, g_rms_channel);
}

void energy_set_params(uint16_t length, uint8_t channel)
{
    if (length > 0 && length <= POINTS_PER_CH) g_rms_length = length;
    if (channel > 0 && channel <= ADC_CHANNELS) g_rms_channel = channel;
    LOG("[Config] set: length=%d, channel=%d\r\n", g_rms_length, g_rms_channel);
}

// 可选：如需更严格可用比例法
// #define CLIP_THRESHOLD  ((length) / 100)  // 1%

uint8_t energy_clip_detect(const uint16_t *data, uint16_t length)
{
    uint16_t clip_count = 0;
    for (uint16_t i = 0; i < length; i++) {
        if (data[i] == 0 || data[i] >= ADC_MAX_VALUE) {
            clip_count++;
        }
    }
    return (clip_count >= CLIP_THRESHOLD) ? 1 : 0;
}

// 有效值计算（去直流分量）-- 支持偏置
float energy_rms_calc_with_offset(const uint16_t *data, uint16_t length, float offset)
{
    float sum = 0.0f;
    for (uint16_t i = 0; i < length; i++) sum += ((float)data[i] - offset);
    float mean = sum / length;

    float sq_sum = 0.0f;
    for (uint16_t i = 0; i < length; i++) {
        float ac = ((float)data[i] - offset) - mean;
        sq_sum += ac * ac;
    }
    return sqrtf(sq_sum / length);
}

// 兼容原有接口，默认无偏置
float energy_rms_calc(const uint16_t *data, uint16_t length)
{
    return energy_rms_calc_with_offset(data, length, 0.0f);
}

// 手动校准ADC偏置（无信号时调用）
void energy_adc_calibrate_offset(void)
{
    // 采集一批数据，默认用rms_data中的最新一批
    for (uint8_t ch = 0; ch < g_rms_channel; ch++) 
    {
        float sum = 0.0f;
        for (uint16_t i = 0; i < g_rms_length; i++) 
        {
            sum += rms_data[i][ch];
        }
        g_adc_offset[ch] = sum / g_rms_length;
        LOG("[Calib] CH%d offset=%.2f\r\n", ch, g_adc_offset[ch]);
    }
    LOG("[Calib] ADC offset calibration done.\r\n");
}

float energy_power_calc(float rms_current, float voltage, float k, float b)
{
    return k * voltage * rms_current + b;
}

/* DMA采样完成回调：整批600点采满进入一次 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC1)
    {
        for (uint16_t i = 0; i < g_rms_length; i++) 
        {
            for (uint8_t ch = 0; ch < g_rms_channel; ch++) 
            {
                rms_data[i][ch] = energy_adc_buf[i * ADC_CHANNELS + ch];
            }
        }
        rms_ready_flag = 1;
        LOG("[ADC] %d*%d samples done, ready to analyze.\r\n", g_rms_channel, g_rms_length);
    }
}

void energy_adc_start(void)
{
    rms_ready_flag = 0;
    extern ADC_HandleTypeDef hadc1;
    HAL_StatusTypeDef ret = HAL_ADC_Start_DMA(&hadc1, (uint32_t*)energy_adc_buf, DMA_BUF_SIZE);
    if (ret != HAL_OK) {
        LOG("[ERROR] HAL_ADC_Start_DMA failed! ret=%d\r\n", ret);
    } else {
        LOG("[ADC] DMA sampling restarted.\r\n");
    }
}

void energy_adc_stop(void)
{
    extern ADC_HandleTypeDef hadc1;
    HAL_ADC_Stop_DMA(&hadc1);
    LOG("[ADC] DMA sampling stopped.\r\n");
}

void energy_adc_restart(void)
{
    energy_adc_stop();
    energy_adc_start();
    LOG("[ADC] DMA sampling restarted.\r\n");
}

// 主分析接口：每个通道用自己的参数
void energy_analyze(void)
{
    if (!rms_ready_flag) return;

    LOG("========== Energy Analyze ==========\n");
    for (uint8_t ch = 0; ch < g_rms_channel; ch++) 
    {
        uint16_t buf[POINTS_PER_CH];
        for (uint16_t i = 0; i < g_rms_length; i++) buf[i] = rms_data[i][ch];

        uint8_t clip = energy_clip_detect(buf, g_rms_length);

        float rms = energy_rms_calc_with_offset(buf, g_rms_length, g_adc_offset[ch]);

        energy_channel_param_t param;
        APP_CONFIG_EnergyParam_Get(&param, ch);
        float power = energy_power_calc(rms, param.voltage, param.k, param.b);

        g_energy_result[ch].rms = rms;
        g_energy_result[ch].power = power;
        g_energy_result[ch].clip_flag = clip;

        if (clip)
            LOG("[Warn] CH%d clip detected!\r\n", ch);

        LOG("CH%d: RMS=%d Power=%d Clip=%d Offset=%.2f Base=%.2f\r\n",
            ch, (int)(rms*1000), (int)(power*1000), (int)clip, g_adc_offset[ch], param.rms_base);
    }
    LOG("================================\r\n");

    rms_ready_flag = 0;
    energy_adc_start();
}

/* 结果接口 */
const energy_result_t* energy_get_result(uint8_t ch)
{
    if (ch < g_rms_channel) return &g_energy_result[ch];
    LOG("[Error] energy_get_result: channel %d out of range!\r\n", ch);
    return NULL;
}

/* 主循环钩子：采满即分析 */
void energy_Handle(void)
{
    uint8_t any_calib = 0;
    for (uint8_t ch = 0; ch < g_rms_channel; ch++) {
        if (calib_rms_base_flag[ch]) {
            uint16_t buf[POINTS_PER_CH];
            for (uint16_t i = 0; i < g_rms_length; i++) {
                buf[i] = rms_data[i][ch];
            }
            // 校准底值，全局参数存到g_app_cfg.channel[ch].rms_base
            g_app_cfg.channel[ch].rms_base = energy_calc_rms_base(buf, g_rms_length);
            LOG("[Calib] CH%d RMS base=%.2f\r\n", ch, g_app_cfg.channel[ch].rms_base);
            calib_rms_base_flag[ch] = 0;
            any_calib = 1;
        }
    }
    if (any_calib) return;
    if (rms_ready_flag) {
        energy_analyze();
    }
}

// 计算一组数据的RMS作为底值
float energy_calc_rms_base(const uint16_t *data, uint16_t length)
{
    float sum = 0.0f;
    for (uint16_t i = 0; i < length; i++) {
        sum += (float)data[i];
    }
    float mean = sum / length;

    float sq_sum = 0.0f;
    for (uint16_t i = 0; i < length; i++) {
        float ac = (float)data[i] - mean;
        sq_sum += ac * ac;
    }
    return sqrtf(sq_sum / length);
}