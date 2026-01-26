#include "user_log.h"

#ifdef USE_LOG

#include "SEGGER_RTT.h"

#ifndef __MICROLIB // MicroLib
struct __FILE {
    int handle;
};
FILE __stdout;

#endif

#if defined(__GNUC__) && !defined(__clang__)
int __io_putchar(int ch)
{
#if !defined(__GNUC__) || defined(__clang__)
    UNUSED(f);
#endif
    SEGGER_RTT_PutChar(0, ch);
    return ch;
}
#else
int fputc(int ch, FILE *f)
{
    SEGGER_RTT_PutChar(0, ch);
    return ch;
}
#endif

#if (defined(__GNUC__) && !defined(__clang__)) || (defined(__ICCARM__))
#if defined(__GNUC__) && !defined(__clang__)
int _write(int fd, char *pbuffer, int size)
#elif defined(__ICCARM__)
#pragma module_name = "?__write"
int __write(int fd, char *pbuffer, int size)
#endif
{
    UNUSED(fd);
    // for (int i = 0; i < size; i++) {
    //     SEGGER_RTT_PutChar(0, (*pbuffer++));
    // }
    SEGGER_RTT_Write(0,pbuffer,size);
    return size;
}
#endif

extern uint64_t BSP_TIMER_Ticks_Get(void);
uint32_t USER_LOG_S_Get(void)
{
    return BSP_TIMER_Ticks_Get() / 1000;
}
uint64_t USER_LOG_MS_Get(void)
{
    return BSP_TIMER_Ticks_Get();
}

#endif // use log

#ifdef USE_LOG_INPUT

#include "umd.h"
#include "string.h"
//"SEGGER_RTT_Conf.h" ->  BUFFER_SIZE_DOWN
#define LOG_BUF_LEN_RX BUFFER_SIZE_DOWN // < BUFFER_SIZE_DOWN

void USER_LOG_Input_Handle(void)
{
    uint8_t rtt_buf[LOG_BUF_LEN_RX] = {0};
    uint16_t len                    = SEGGER_RTT_Read(0, rtt_buf, LOG_BUF_LEN_RX);
    if (len > 0) {
        g_umd_dev.scan(rtt_buf, len);
    }
}

void USER_LOG_Init(void)
{
    SEGGER_RTT_Init();
}

#else
void USER_LOG_Input_Handle(void)
{
}
#endif

/*****END OF FILE****/
