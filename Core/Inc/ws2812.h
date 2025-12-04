#ifndef __WS2812_H__
#define __WS2812_H__

#include "stm32f1xx_hal.h"

#define WS_PIN GPIO_PIN_13
#define WS_PORT GPIOC

// 定义一个简单的查找表，放在 RAM 或 Flash 均可
// index 0: 发送 '0' 码时，中间需要拉低 -> 写入 Pin_13
// index 1: 发送 '1' 码时，中间保持高电平 -> 写入 0 (无动作)
static const uint32_t RESET_MASK[2] = {WS_PIN, 0};

/**
 * @brief 更新 WS2812 灯带颜色
 *
 * @param sign1 Green
 * @param sign2 Red
 * @param sign3 Blue
 */
void WS2812_Update(uint8_t sign1, uint8_t sign2, uint8_t sign3);

#endif // !__WS2812_H__
