#ifndef __BSP_ADC_H__
#define __BSP_ADC_H__

#include "stdint.h"

#include "stm32f1xx_hal.h" // 只用于采集相关句柄声明

#define ADC_DMA_BUF_SIZE 1024 // 批量采集点数

typedef struct {
    uint16_t value_raw;
    float value_volt;
    uint16_t dma_buf[ADC_DMA_BUF_SIZE]; // DMA批量采集缓冲区
    uint16_t dma_count; // 当前采集计数
} bsp_adc_t;

extern bsp_adc_t g_adc;



void BSP_ADC_Init(void); // 采集相关变量初始化
void BSP_ADC_Start_DMA(void); // 启动批量DMA采集
void BSP_ADC_Stop_DMA(void);  // 停止DMA采集
uint16_t BSP_ADC_GetValue(void);
float BSP_ADC_GetVoltage(void);
void BSP_ADC_OnDMACplt(void); // DMA采集完成回调

#endif
