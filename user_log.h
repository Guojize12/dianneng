#ifndef __USER_LOG_H
#define __USER_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp_config.h"
#include "bsp_timer.h"

#define USE_LOG // 使用LOG输出

#ifdef USE_LOG
#define USE_LOG_INPUT // 使用LOG输入
#endif

#ifdef USE_LOG

#define LOG printf
#define LOGE                            \
    LOG("%s [%d]", __FILE__, __LINE__); \
    LOG
#define LOG_HEX_0X(buf, len)            \
    for (int abc = 0; abc < len; abc++) \
        LOG("0x%02X ", buf[abc]);       \
    LOG("\n");
#define LOG_HEX(buf, len)               \
    for (int abc = 0; abc < len; abc++) \
        LOG("%02X ", buf[abc]);         \
    LOG("\n");
#define LOG_DEC(buf, len)                    \
    for (uint32_t abc = 0; abc < len; abc++) \
        LOG("%d ", buf[abc]);                \
    LOG("\n");
#define LOGT                                      \
    LOG("%lu ", (unsigned long)USER_LOG_S_Get()); \
    LOG
#define LOGTM                                      \
    LOG("%ld ", (unsigned long)USER_LOG_MS_Get()); \
    LOG
#else
#define LOG(...)
#define LOGE(...)
#define LOGT(...)
#define LOGTM(...)
#define LOG_HEX_0X(buf, len)
#define LOG_HEX(buf, len)
#define LOG_DEC(buf, len)
#endif

uint32_t USER_LOG_S_Get(void);
uint64_t USER_LOG_MS_Get(void);
void USER_LOG_Input_Handle(void);
void USER_LOG_Init(void);

#ifdef __cplusplus
}
#endif

#endif /*__USER_LOG_H */

/*****END OF FILE****/
