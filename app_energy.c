#include "adc.h"
#include <math.h>
#include "app_energy.h"
#include <string.h>
#include "user_log.h"

#define ADC_CHANNELS   3
#define POINTS_PER_CH  200
#define DMA_BUF_SIZE   (ADC_CHANNELS * POINTS_PER_CH)

/* 一次性DMA采样缓冲区（600点） */
static uint16_t energy_adc_buf[DMA_BUF_SIZE] = {0};
/* 分拣后的二维数据：每通道200点 */
static uint16_t rms_data[POINTS_PER_CH][ADC_CHANNELS] = {0};
static volatile uint8_t rms_ready_flag = 0;

/* 参数（默认3通道、每通道200点） */
static uint16_t g_rms_length = POINTS_PER_CH;
static uint8_t  g_rms_channel = ADC_CHANNELS;

/* 结果 */
static energy_result_t g_energy_result[ADC_CHANNELS];

void energy_print_params(void) {
    LOG("[ADC] params: length=%d, channel=%d\r\n", g_rms_length, g_rms_channel);
}

void energy_set_params(uint16_t length, uint8_t channel)
{
    if (length > 0 && length <= POINTS_PER_CH) g_rms_length = length;
    if (channel > 0 && channel <= ADC_CHANNELS) g_rms_channel = channel;
    LOG("[Config] set: length=%d, channel=%d\r\n", g_rms_length, g_rms_channel);
}

/* 削顶检测 */
uint8_t energy_clip_detect(const uint16_t *data, uint16_t length)
{
    for (uint16_t i = 0; i < length; i++) {
        if (data[i] == 0 || data[i] >= 4095) return 1;
    }
    return 0;
}

/* 有效值计算（去直流分量） */
float energy_rms_calc(const uint16_t *data, uint16_t length)
{
    float sum = 0.0f;
    for (uint16_t i = 0; i < length; i++) sum += data[i];
    float mean = sum / length;

    float sq_sum = 0.0f;
    for (uint16_t i = 0; i < length; i++) {
        float ac = (float)data[i] - mean;
        sq_sum += ac * ac;
    }
    return sqrtf(sq_sum / length);
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
        /* 将 interleaved 的 600 点拆分到 [200][3] */
        for (uint16_t i = 0; i < g_rms_length; i++) {
            for (uint8_t ch = 0; ch < g_rms_channel; ch++) {
                rms_data[i][ch] = energy_adc_buf[i * ADC_CHANNELS + ch];
            }
        }
        rms_ready_flag = 1;
        LOG("[ADC] %d*%d samples done, ready to analyze.\r\n", g_rms_channel, g_rms_length);
    }
}

/* 启动DMA采样：一次搬运600点 */
void energy_adc_start(void)
{
    rms_ready_flag = 0;
    extern ADC_HandleTypeDef hadc1;
    HAL_StatusTypeDef ret = HAL_ADC_Start_DMA(&hadc1, (uint32_t*)energy_adc_buf, DMA_BUF_SIZE);
    if (ret != HAL_OK) {
        LOG("[ERROR] HAL_ADC_Start_DMA failed! ret=%d\r\n", ret);
    } else {
        LOG("[ADC] DMA sampling started.\r\n");
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

void energy_analyze(float voltage, float k, float b)
{
    if (!rms_ready_flag) return;

    LOG("========== Energy Analyze ==========""\r\n");
    for (uint8_t ch = 0; ch < g_rms_channel; ch++) {
        uint16_t buf[POINTS_PER_CH];
        for (uint16_t i = 0; i < g_rms_length; i++) buf[i] = rms_data[i][ch];

        uint8_t clip = energy_clip_detect(buf, g_rms_length);
        float rms = energy_rms_calc(buf, g_rms_length);
        float power = energy_power_calc(rms, voltage, k, b);

        g_energy_result[ch].rms = rms;
        g_energy_result[ch].power = power;
        g_energy_result[ch].clip_flag = clip;

        if (clip) LOG("[Warn] CH%d clip detected!\r\n", ch);
        LOG("CH%d: RMS=%.3f Power=%.3f Clip=%d\r\n", ch, rms, power, clip);
    }
    LOG("================================\r\n");

    rms_ready_flag = 0;
    /* 自动开启下一批采样 */
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
    if (rms_ready_flag) {
        energy_analyze(220.0f, 1.0f, 0.0f);
    }
}
