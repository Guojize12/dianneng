    #ifndef __APP_USER_H
    #define __APP_USER_H

    #ifdef __cplusplus
    extern "C" {
    #endif

    #include <stdint.h>

    #include "app_energy.h"


    void BSP_MODBUS_SendRtu_03(uint8_t addr, uint16_t reg, uint8_t *data, uint8_t len);

    // 485能耗参数传输相关接口
    void APP_USER_Handle03(uint8_t addr, const uint8_t *buffer, uint8_t size);
    void APP_USER_Handle06(uint8_t addr, const uint8_t *buffer, uint8_t size);
    void APP_RS485_Rec_Handle(uint8_t *buffer, uint16_t len);
    void APP_USER_RS485HANDLE(void);

    #ifdef __cplusplus
    }
    #endif

    #endif  /* __APP_USER_H */

