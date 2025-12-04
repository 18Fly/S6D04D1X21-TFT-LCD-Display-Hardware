#include "ws2812.h"

static __forceinline void WS2812_Send_Byte_Branchless(uint8_t byte)
{
    // 临时变量，用于存放中间时刻要写入 BRR 的值
    uint32_t action;

    // ==============================================================
    // 手动展开 8 次循环。不要用 for 循环！for 循环本质就是 if 跳转！
    // ==============================================================

    // --------------- Bit 7 ---------------
    // 1. 计算中间动作：如果是1，action=0；如果是0，action=PIN_13
    //    注意：(byte & 0x80) >> 7 结果是 1 或 0
    action = RESET_MASK[(byte >> 7) & 0x01];

    WS_PORT->BSRR = WS_PIN; // [T0] 无论 0/1，先拉高

    // [T1] 延时 (对应 T0H 时间)
    // PC13 上升慢，这里不要加太多 NOP，甚至不加
    // 计算 action 本身消耗了时间，刚好充当延时
    __NOP();
    __NOP();

    WS_PORT->BRR = action; // [T2] 关键时刻！
                           // 发0: 这里写入Pin13，电平变低
                           // 发1: 这里写入0，电平保持高

    // [T3] 延时 (对应 T1H - T0H 的时间差)
    __NOP();
    __NOP();
    __NOP();
    __NOP();

    WS_PORT->BRR = WS_PIN; // [T4] 无论 0/1，最后强制拉低

    // [T5] 低电平复位时间 (Low Gap)
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();

    // --------------- Bit 6 ---------------
    action = RESET_MASK[(byte >> 6) & 0x01];
    WS_PORT->BSRR = WS_PIN;
    __NOP();
    __NOP();
    WS_PORT->BRR = action;
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    WS_PORT->BRR = WS_PIN;
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();

    // --------------- Bit 5 ---------------
    action = RESET_MASK[(byte >> 5) & 0x01];
    WS_PORT->BSRR = WS_PIN;
    __NOP();
    __NOP();
    WS_PORT->BRR = action;
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    WS_PORT->BRR = WS_PIN;
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();

    // --------------- Bit 4 ---------------
    action = RESET_MASK[(byte >> 4) & 0x01];
    WS_PORT->BSRR = WS_PIN;
    __NOP();
    __NOP();
    WS_PORT->BRR = action;
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    WS_PORT->BRR = WS_PIN;
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();

    // --------------- Bit 3 ---------------
    action = RESET_MASK[(byte >> 3) & 0x01];
    WS_PORT->BSRR = WS_PIN;
    __NOP();
    __NOP();
    WS_PORT->BRR = action;
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    WS_PORT->BRR = WS_PIN;
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();

    // --------------- Bit 2 ---------------
    action = RESET_MASK[(byte >> 2) & 0x01];
    WS_PORT->BSRR = WS_PIN;
    __NOP();
    __NOP();
    WS_PORT->BRR = action;
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    WS_PORT->BRR = WS_PIN;
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();

    // --------------- Bit 1 ---------------
    action = RESET_MASK[(byte >> 1) & 0x01];
    WS_PORT->BSRR = WS_PIN;
    __NOP();
    __NOP();
    WS_PORT->BRR = action;
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    WS_PORT->BRR = WS_PIN;
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();

    // --------------- Bit 0 ---------------
    action = RESET_MASK[byte & 0x01];
    WS_PORT->BSRR = WS_PIN;
    __NOP();
    __NOP();
    WS_PORT->BRR = action;
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    WS_PORT->BRR = WS_PIN;
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
}

void WS2812_Update(uint8_t sign1, uint8_t sign2, uint8_t sign3)
{
    __disable_irq();

    WS2812_Send_Byte_Branchless(sign1);
    WS2812_Send_Byte_Branchless(sign2);
    WS2812_Send_Byte_Branchless(sign3);

    __enable_irq();
    // Reset 信号
    HAL_Delay(10);
}
