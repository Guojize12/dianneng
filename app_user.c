


#include "app_user.h"
#include <string.h>
#include "bsp_uart.h"
#include "bsp_rs485.h"
#include "bsp_crc16.h"


// 485能耗参数传输相关全局参数
static float g_k = 1.0f, g_b = 0.0f;
static float g_voltage = 220.0f; // 默认电压220V，可下发

// 06功能码：写参数
void APP_USER_Handle06(uint8_t addr, const uint8_t *buffer, uint8_t size)
{
    uint16_t reg = (buffer[0]<<8) | buffer[1];
    uint16_t data = (buffer[2]<<8) | buffer[3];
    switch(reg) {
        case 0x0001:
            energy_set_params(data, 0); // 只改采样点数
            break;
        case 0x0002:
            energy_set_params(0, data); // 只改通道数
            break;
        case 0x0003:
            memcpy(&g_k, &data, sizeof(uint16_t)); // 仅支持低16位写入，实际项目可扩展为完整float写入
            break;
        case 0x0004:
            memcpy(&g_b, &data, sizeof(uint16_t));
            break;
        case 0x0005:
            memcpy(&g_voltage, &data, sizeof(uint16_t)); // 支持低16位写入
            break;
        default:
            break;
    }
}

// 03功能码：上传能耗参数
void APP_USER_Handle03(uint8_t addr, const uint8_t *buffer, uint8_t size)
{
    energy_analyze(); // 计算所有通道功率
    uint8_t tx_buf[3 * (sizeof(float)*2 + 1)] = {0};
    uint8_t tx_num = 0;
    for(uint8_t ch=0; ch<3; ch++) {
        const energy_result_t* res = energy_get_result(ch);
        if(res) {
            memcpy(&tx_buf[tx_num], &res->rms, sizeof(float)); tx_num += sizeof(float);
            memcpy(&tx_buf[tx_num], &res->power, sizeof(float)); tx_num += sizeof(float);
            tx_buf[tx_num++] = res->clip_flag;
        }
    }
    // 发送回主机（寄存器0x1000，数据长度tx_num）
    BSP_MODBUS_SendRtu_03(addr, 0x1000, tx_buf, tx_num);
}


void APP_RS485_Rec_Handle(uint8_t *buffer, uint16_t len)
{
    switch(buffer[1])
    {
    case 0x03:
        APP_USER_Handle03(buffer[0], buffer+2, buffer[2]);
        break;
    case 0x06:
        APP_USER_Handle06(buffer[0], buffer+2, 4);
        break;
    default:
        break;
    }
}

// 轮询接收处理（需外部实现BSP_UART_Rec_Read、BSP_RS485_UART、BSP_RS485_UART_BUF、BSP_CRC16、BSP_TIMER_Modify_Start等）

// 完整485接收处理，支持自动切换收发方向
void APP_USER_RS485HANDLE(void)
{
    // 切换到接收模式
    RS485_SetReceive();
    // 轮询接收（假定BSP_UART3为485口）
    if (BSP_UART_Rec_Read(BSP_UART3) == USR_EOK)
    {
        uint8_t *rxBuf = g_uart_buf[BSP_UART3].rxBuf;
        uint16_t rxLen = g_uart_buf[BSP_UART3].rxLen;
        if (rxLen > 4)
        {
            uint16_t crc = BSP_CRC16(rxBuf, rxLen - 2);
            if (rxBuf[rxLen - 2] == (uint8_t)(crc >> 8) && rxBuf[rxLen - 1] == (uint8_t)crc)
            {
                // 收到有效帧，分发处理
                APP_RS485_Rec_Handle(rxBuf, rxLen);
            }
        }
    }
}

// 临时空实现，防止链接错误，后续请补充真实功能
void BSP_MODBUS_SendRtu_03(uint8_t addr, uint16_t reg, uint8_t *data, uint8_t len)
{
    // TODO: 实现03功能码数据发送
}