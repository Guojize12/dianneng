#include "bsp_adc.h"
#include "adc.h"         // hadc1
#include "app_energy.h"  // energy_adc_start
#include <string.h>

bsp_adc_t g_adc = {0};

/* 仅做ADC初始化和缓冲区清零；不在这里启动能耗采样，避免重复启动 */
void BSP_ADC_Init(void)
{
    MX_ADC1_Init();
    memset(g_adc.dma_buf, 0, sizeof(g_adc.dma_buf));
    g_adc.dma_count = 0;
}

/* 如需独立BSP层批量采样，可使用以下两个接口（与能耗模块无关） */
void BSP_ADC_Start_DMA(void)
{
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)g_adc.dma_buf, ADC_DMA_BUF_SIZE);
    g_adc.dma_count = 0;
}

void BSP_ADC_Stop_DMA(void)
{
    HAL_ADC_Stop_DMA(&hadc1);
}

void BSP_ADC_OnDMACplt(void)
{
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