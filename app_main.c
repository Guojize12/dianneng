#include "app_user.h"
#include "app_energy.h"
#include "app_main.h"
#include "app_config.h"
#include "app_sensor.h"
#include "tim.h"
#include "dma.h"
#include "bsp_adc.h"
#include "bsp_timer.h"      // For Timer type, BSP_TIMER_Init, BSP_TIMER_Start, BSP_TIMER_Handle, TIMEOUT_2S
#include "bsp_iwdg.h"       // For BSP_IWDG_Refresh
#include "user_log.h"       // For LOG, USER_LOG_S_Get, USER_LOG_Input_Handle
#include "app_version.h"    // For APP_VERSION_Print


static Timer g_timer_iwdg = {0};
static Timer g_timer_pwm_print = {0};

// 软定时器回调：打印PWM频率
static void APP_MAIN_Callback_PrintPwmFreq(void)
{
    APP_SENSOR_PWM_PrintFreq();
}
/**
 * @brief  喂看门狗
 * @param  None.
 * @retval None.
 */
static void APP_MAIN_Callback_Iwdg(void)
{
    BSP_IWDG_Refresh();
    static uint8_t g_run_time = 0;
    if (++g_run_time > 14)
    {
        g_run_time = 0;
        uint64_t time = USER_LOG_S_Get();
        LOG("system run time: %02lu:%02lu:%02lu\n", (unsigned long)(time / 3600), (unsigned long)(time % 3600 / 60), (unsigned long)(time % 60));

#if 0
        LOG("[%d] %d-%02d-%02d %02d:%02d:%02d\n",g_rtc_time.utc,g_rtc_time.calendar.year,g_rtc_time.calendar.month,g_rtc_time.calendar.date,
            g_rtc_time.calendar.hour,g_rtc_time.calendar.min,g_rtc_time.calendar.sec);
    // 电能分析参数（可根据实际情况调整）
    static float g_energy_voltage = 220.0f; // 电压
    static float g_energy_k = 1.0f;         // 校准系数
    static float g_energy_b = 0.0f;         // 偏置
#endif
    } 
}

/**
 * @brief  功能初始化
 * @param  None.
 * @retval None.
 */
void APP_MAIN_Init(void)
{
    USER_LOG_Init();
    BSP_TIMER_Init(&g_timer_iwdg, APP_MAIN_Callback_Iwdg, TIMEOUT_2S, TIMEOUT_2S); // 1s喂狗
    BSP_TIMER_Start(&g_timer_iwdg);

    //  初始化DMA
    MX_DMA_Init();
    // 初始化ADC
    BSP_ADC_Init();  
   // 初始化定时器3（用于ADC采样触发）
    MX_TIM3_Init();
    // 启动定时器3，确保产生TRGO触发ADC
    extern TIM_HandleTypeDef htim3;
    HAL_TIM_Base_Start(&htim3);
   // 初始化并启动PWM打印软定时器（500ms）
//   BSP_TIMER_Init(&g_timer_pwm_print, APP_MAIN_Callback_PrintPwmFreq, 500, 500);
//    BSP_TIMER_Start(&g_timer_pwm_print);
    APP_VERSION_Print(); 

        /* add app init*/
    APP_CONFIG_Init();
    
    energy_adc_start();
}
/**
 * @brief  主循环
        // 启动电能监测ADC采样
        
 * @param  None.
 * @retval None.
 */
void APP_MAIN_Handle(void)
{
    APP_MAIN_Init();
    while (1)
    {
        /* add bsp handle*/
        /** UART*/
        // BSP_UART_Handle();
        /** TIMER*/
        BSP_TIMER_Handle();
        /* add bsp handle end */

        /* add app handle*/
        APP_USER_RS485HANDLE(); // 轮询485接收和应答
        /* add app handle end */

        /* add log handle*/
        /** RTT*/
        USER_LOG_Input_Handle();
        /* add log end */
        // 电能监测分析（数据满时自动分析）
        energy_Handle();

    }
}

/*****END OF FILE****/

