#ifndef __APP_ENERGY_H__
#define __APP_ENERGY_H__

#include <stdint.h>

// 启动DMA采集
void energy_adc_start(void);
// 停止DMA采集
void energy_adc_stop(void);
// 采集重启（异常恢复）
void energy_adc_restart(void);

// 参数动态设置
void energy_set_params(uint16_t length, uint8_t channel);

// 多通道能量分析
void energy_analyze(float voltage, float k, float b);

// 有效值计算函数接口
float energy_rms_calc(const uint16_t *data, uint16_t length);

// 削顶检测
uint8_t energy_clip_detect(const uint16_t *data, uint16_t length);

// 功率计算
float energy_power_calc(float rms_current, float voltage, float k, float b);

// 结果结构体
typedef struct 
{
	float rms;
	float power;
	uint8_t clip_flag;
} energy_result_t;

// 获取分析结果
const energy_result_t* energy_get_result(uint8_t ch);


void energy_Handle(void);

// 可扩展：滤波、物理量计算、报警等接口声明
// void energy_filter(void);
// void energy_calc(void);
// void energy_alarm(void);

// 计算一组数据的RMS作为底值
float energy_calc_rms_base(const uint16_t *data, uint16_t length);
#endif // __APP_ENERGY_H__
