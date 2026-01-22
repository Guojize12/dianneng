
#include "bsp_adc.h"
#include "adc.h" // for hadc1 extern
#include "app_energy.h" // for energy_adc_start

// DMA批量缓冲区和结构体
#include <string.h>
bsp_adc_t g_adc = {0};


// BSP_ADC_Init 集成DMA、ADC、缓冲区初始化和启动采集
void BSP_ADC_Init(void)
{
    MX_ADC1_Init();     // ADC初始化
    memset(g_adc.dma_buf, 0, sizeof(g_adc.dma_buf)); // 缓冲区初始化
    g_adc.dma_count = 0;
    // 统一由能耗模块启动ADC+DMA，确保缓冲区一致
    energy_adc_start();
}

// 启动批量DMA采集
void BSP_ADC_Start_DMA(void)
{
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)g_adc.dma_buf, ADC_DMA_BUF_SIZE);
    g_adc.dma_count = 0;
}

// 停止DMA采集
void BSP_ADC_Stop_DMA(void)
{
    HAL_ADC_Stop_DMA(&hadc1);
}

// DMA采集完成回调（需在stm32f1xx_it.c或HAL库回调中调用）
void BSP_ADC_OnDMACplt(void)
{
    // 可在此处理批量采集数据，如滤波、RMS、FFT等
    // g_adc.dma_buf 已采集满 ADC_DMA_BUF_SIZE 个点
    g_adc.dma_count = ADC_DMA_BUF_SIZE;
    // 示例：计算均值
    uint32_t sum = 0;
    for (uint16_t i = 0; i < ADC_DMA_BUF_SIZE; i++) sum += g_adc.dma_buf[i];
    g_adc.value_raw = sum / ADC_DMA_BUF_SIZE;
    g_adc.value_volt = g_adc.value_raw * 3.3f / 4095.0f;
}


uint16_t BSP_ADC_GetValue(void)
{
    return g_adc.value_raw;
}

float BSP_ADC_GetVoltage(void)
{
    return g_adc.value_volt;
}
