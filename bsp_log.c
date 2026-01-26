#include "bsp_app.h"
#include "user_log.h"
#include "app_energy.h"
#include "string.h"
#include "umd.h"

#define UMD_USE_CLK // 主频
#define UMD_USE_DID //设备ID
// #define UMD_USE_RST //恢复出厂

/************引用需要调用的函数********/ 

#ifdef UMD_USE_DID

extern void APP_CONFIG_Did_Set(uint32_t id);
extern uint32_t APP_CONFIG_Did_Get(void);
void did_set(u32 id)
{
    APP_CONFIG_Did_Set(id);
}
void did_get(void)
{
    LOG("device id:%lu\n", (unsigned long)APP_CONFIG_Did_Get());
}
#endif

#ifdef UMD_USE_CLK
static void sys_clk(void);
#endif

static void reboot(void);
static void sys_time(void);
static void user_list_max(void);

#ifdef UMD_USE_RST
extern void APP_CONFIG_Default(void);
void factory_reset(void)
{
#ifndef USE_BOOT
    APP_CONFIG_Default();
#else
    BSP_EEPROM_Reset();
#endif
    reboot();
}
#endif
#ifdef USE_LOG_INPUT
static void energy_calib_cmd(void)
{
    energy_adc_calibrate_offset();
    extern uint8_t g_energy_use_offset;
    g_energy_use_offset = 1;
    LOG("[CMD] energy_adc_calibrate_offset() called, RMS去底已开启.\n");
}

static void offset_reset_cmd(void)
{
    energy_adc_offset_reset();
    LOG("[CMD] energy_adc_offset_reset() called.\n");
}

struct umd_name_def umd_nametab[] =
{
    {(void *)reboot, (uint8_t *)"reboot()"},
#ifdef UMD_USE_CLK
    {(void *)sys_clk, (uint8_t *)"sys_clk()"},
#endif
    {(void *)sys_time, (uint8_t *)"sys_time()"},
/************添加可见函数**************/
    {(void *)energy_calib_cmd, (uint8_t *)"energy_calib()"},
    {(void *)offset_reset_cmd, (uint8_t *)"offset_reset()"},
#ifdef UMD_USE_DID
    {(void *)did_set, (uint8_t *)"did_set(u32 id)"},
    {(void *)did_get, (uint8_t *)"did_get()"},
#endif

    /*****************END******************/
    {(void *)user_list_max, (uint8_t *)"user_list_max"}, // 分割线
#ifdef UMD_USE_RST
        {(void *)factory_reset, (uint8_t *)"factory_reset()"},
//    {(void*)ota_start,(uint8_t* )"ota_start()"},
#ifdef UMD_USE_BOOT_TST
        {(void *)flh_wt, (uint8_t *)"flh_wt()"},
#endif

#endif
        /************添加不可见函数**************/

        /*****************END******************/
};
#endif

void umd_get_fnum(void)
{
    g_umd_dev.fnum = sizeof(umd_nametab) / sizeof(struct umd_name_def);

    for (int i = 0; i < g_umd_dev.fnum; i++)
    {
        if (strstr((char *)g_umd_dev.funs[i].name, "user_list_max") != NULL)
        {
            g_umd_dev.fnum_user = i;
        }
    }
}


static void reboot(void)
{
    for (int i = 0; i < 10000000; i++)
        ;
    NVIC_SystemReset();
}

static void sys_time(void)
{
    u64 time = USER_LOG_S_Get();
    LOG("system time: %02lu:%02lu:%02lu\n", (unsigned long)(time / 3600), (unsigned long)(time % 3600 / 60), (unsigned long)(time % 60));
}

static void user_list_max(void)
{
}


#ifdef UMD_USE_CLK
static void sys_clk(void)
{ 
    LOG("system clock: %luM\n",(long unsigned int)SystemCoreClock/1000000);
}
#endif


/*****END OF FILE****/
