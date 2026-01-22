// ...existing code...
#include "adc.h"
#include <string.h>
#include "app_energy.h"
#include <math.h>
#include "user_log.h"


// DMA采样缓冲区（3路）
uint16_t energy_adc_buf[3] = {0};

uint16_t g_rms_length = 200;
uint8_t g_rms_channel = 3;
uint16_t rms_data[1024][3]; // 最大支持1024点、3通道（可根据实际需求调整上限）
static volatile uint16_t rms_count = 0;
static volatile uint8_t rms_ready_flag = 0;


// 打印采样参数
void energy_print_params(void) {
    LOG("[ADC] Params: length=%d, channel=%d\r\n", g_rms_length, g_rms_channel);
}


static energy_result_t g_energy_result[3]; // 最多支持3通道

// 参数动态设置函数
void energy_set_params(uint16_t length, uint8_t channel)
{
    // 设置采样点数和通道数，范围保护
    if (length > 0 && length <= 1024) g_rms_length = length;
    if (channel > 0 && channel <= 3) g_rms_channel = channel;
    LOG("[Config] RMS_LENGTH=%d, RMS_CHANNEL=%d\r\n", g_rms_length, g_rms_channel);
    // 如需动态调整DMA采样参数，可在此处重启ADC采集
    // energy_adc_restart();
    energy_print_params();
}

// 削顶检测函数，返回1=削顶，0=正常
uint8_t energy_clip_detect(const uint16_t *data, uint16_t length)
{
    const uint16_t ADC_MIN = 0;
    const uint16_t ADC_MAX = 4095;
    for (uint16_t i = 0; i < length; i++) {
        if (data[i] <= ADC_MIN || data[i] >= ADC_MAX) return 1;
    }
    return 0;
}

// 有效值计算函数（去直流分量）
float energy_rms_calc(const uint16_t *data, uint16_t length)
{
    float sum = 0.0f;
    for (uint16_t i = 0; i < length; i++) sum += data[i];
    float mean = sum / length;
    float sq_sum = 0.0f;
    for (uint16_t i = 0; i < length; i++) {
        float ac_val = (float)data[i] - mean;
        sq_sum += ac_val * ac_val;
    }
    return sqrtf(sq_sum / length);
}

// 功率计算函数（支持校准系数和偏置）
float energy_power_calc(float rms_current, float voltage, float k, float b)
{
    return k * voltage * rms_current + b;
}

// DMA采集完成回调（由HAL库自动调用）
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    LOG("[DEBUG] HAL_ADC_ConvCpltCallback() called, rms_count=%d, rms_ready_flag=%d\r\n", rms_count, rms_ready_flag);
    if (hadc->Instance == ADC1)
    {
        if (rms_count >= g_rms_length) {
            LOG("[Error] DMA buffer overflow! Data may be invalid.\r\n");
            rms_count = 0;
            rms_ready_flag = 0;
            energy_adc_restart();
            return;
        }
        for (uint8_t ch = 0; ch < g_rms_channel; ch++)
            rms_data[rms_count][ch] = energy_adc_buf[ch];
        LOG("[DEBUG] After copy: rms_count=%d\r\n", rms_count);
        rms_count++;
        if (rms_count >= g_rms_length)
        {
            rms_count = 0;
            rms_ready_flag = 1; // 标记RMS数据已满
            LOG("[ADC] Sample full, ready for analysis. rms_ready_flag=%d\r\n", rms_ready_flag);
        }
    }
}
// 采集重启函数，异常时自动恢复
void energy_adc_restart(void)
{
    energy_adc_stop();
    energy_adc_start();
    LOG("[Status] ADC sampling restarted after error.\r\n");
}

// 启动DMA采集
void energy_adc_start(void)
{
    rms_count = 0;
    rms_ready_flag = 0;
    extern ADC_HandleTypeDef hadc1;
    HAL_StatusTypeDef ret = HAL_ADC_Start_DMA(&hadc1, (uint32_t*)energy_adc_buf, g_rms_channel);
    if (ret != HAL_OK) {
        LOG("[ERROR] HAL_ADC_Start_DMA failed! ret=%d\r\n", ret);
    } else {
        LOG("[Status] ADC sampling started.\r\n");
    }
    energy_print_params();
    LOG("[DEBUG] energy_adc_start() called\r\n");
}
// 停止DMA采集
void energy_adc_stop(void)
{
    extern ADC_HandleTypeDef hadc1;
    HAL_ADC_Stop_DMA(&hadc1);
    LOG("[Status] ADC sampling stopped.\r\n");
}
void energy_analyze(float voltage, float k, float b)
{
    LOG("[DEBUG] energy_analyze() called\r\n");
    if (!rms_ready_flag) {
        LOG("[Error] Data not ready, analysis skipped! rms_ready_flag=%d\r\n", rms_ready_flag);
        return;
    }
    LOG("\r\n========== Energy Analysis ==========");
    LOG("[Status] Energy analysis started.\r\n");
    LOG("Voltage=%.2f, k=%.3f, b=%.3f, Points=%d, Channels=%d\r\n", voltage, k, b, g_rms_length, g_rms_channel);
    for (uint8_t ch = 0; ch < g_rms_channel; ch++) {
        // 修正数据访问，分析每个通道的所有采样点
        uint16_t channel_buf[1024];
        for (uint16_t i = 0; i < g_rms_length; i++) {
            channel_buf[i] = rms_data[i][ch];
        }
        uint8_t clip = energy_clip_detect(channel_buf, g_rms_length);
        float rms = energy_rms_calc(channel_buf, g_rms_length);
        float power = energy_power_calc(rms, voltage, k, b);
        g_energy_result[ch].rms = rms;
        g_energy_result[ch].power = power;
        g_energy_result[ch].clip_flag = clip;
        if (clip) {
            LOG("[Warning] CH%d signal clipping detected, result may be distorted!\r\n", ch);
        }
        LOG("[Result] CH%d | RMS=%.3f | Power=%.3f | Clip=%d\r\n", ch, rms, power, clip);
        LOG("-------------------------------------\r\n");
    }
    LOG("[Status] Energy analysis finished.\r\n");
    LOG("======================================\r\n\r\n");
    rms_ready_flag = 0;
}

// 结果接口函数，返回指定通道分析结果指针
const energy_result_t* energy_get_result(uint8_t ch)
{
    if (ch < g_rms_channel) return &g_energy_result[ch];
    LOG("[Error] energy_get_result: channel %d out of range!\r\n", ch);
    return NULL;
}

// 电能监测处理函数
void energy_Handle(void)
{
    extern volatile uint8_t rms_ready_flag;
 //   LOG("[DEBUG] energy_Handle() called, rms_ready_flag=%d\r\n", rms_ready_flag);
    if (rms_ready_flag)
    {
        energy_analyze(220.0f, 1.0f, 0.0f);
    }
}


