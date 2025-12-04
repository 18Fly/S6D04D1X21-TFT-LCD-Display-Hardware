/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// 控制引脚(PB5 - PB8)
#define LCD_CS_PORT GPIOB
#define LCD_CS_PIN GPIO_PIN_5
#define LCD_RS_PORT GPIOB
#define LCD_RS_PIN GPIO_PIN_6
#define LCD_WR_PORT GPIOB
#define LCD_WR_PIN GPIO_PIN_7
#define LCD_RST_PORT GPIOB
#define LCD_RST_PIN GPIO_PIN_8
// 数据引脚(PA0 - PA7)
#define LCD_DATA_PORT GPIOA

// 控制引脚宏
#define LCD_CS_LOW() (LCD_CS_PORT->BSRR = (uint32_t)LCD_CS_PIN << 16U)
#define LCD_CS_HIGH() (LCD_CS_PORT->BSRR = LCD_CS_PIN)

#define LCD_RS_LOW() (LCD_RS_PORT->BSRR = (uint32_t)LCD_RS_PIN << 16U)
#define LCD_RS_HIGH() (LCD_RS_PORT->BSRR = LCD_RS_PIN)

#define LCD_WR_LOW() (LCD_WR_PORT->BSRR = (uint32_t)LCD_WR_PIN << 16U)
#define LCD_WR_HIGH() (LCD_WR_PORT->BSRR = LCD_WR_PIN)

#define LCD_RST_LOW() (LCD_RST_PORT->BSRR = (uint32_t)LCD_RST_PIN << 16U)
#define LCD_RST_HIGH() (LCD_RST_PORT->BSRR = LCD_RST_PIN)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

// 用于WS2812B的G、R、B颜色[0-255]二维数组
// uint8_t GRB_Color[3][256];

const uint8_t Font_16x16_Qing[32] = {
    0x00, 0x40, 0x20, 0x40, 0x17, 0xfc, 0x10, 0x40, 0x83, 0xf8, 0x40, 0x40, 0x47, 0xfe, 0x10, 0x00,
    0x13, 0xf8, 0x22, 0x08, 0xe3, 0xf8, 0x22, 0x08, 0x23, 0xf8, 0x22, 0x08, 0x22, 0x28, 0x02, 0x10}; /* "清" */

const uint8_t Font_16x16_Ping[32] = {
    0x00, 0x00, 0x3f, 0xf8, 0x20, 0x08, 0x20, 0x08, 0x3f, 0xf8, 0x24, 0x10, 0x22, 0x20, 0x2f, 0xf8,
    0x22, 0x20, 0x22, 0x20, 0x3f, 0xfc, 0x22, 0x20, 0x42, 0x20, 0x44, 0x20, 0x84, 0x20, 0x08, 0x20}; /* "屏" */

const uint8_t Font_16x16_Zhong[32] = {
    0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x3F, 0xF8, 0x21, 0x08, 0x21, 0x08, 0x21, 0x08,
    0x21, 0x08, 0x21, 0x08, 0x3F, 0xF8, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00}; /* "中" */

const uint8_t dot[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x80, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; /* "." */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/**
 * @brief  将 8 位数据写入到数据总线 (PA0-PA7)
 * @param  data: 要写入的 8 位数据
 */
static void Lcd_Write_Data_Bus(uint8_t data)
{
  // ☆★☆★☆★☆★☆★!!! 使用ODR的方式不是原子操作，在读取和写入之间的时间间隔内，会被中断影响 !!!☆★☆★☆★☆★☆★
  // 使用 BSRR 直接操作，无需读取 ODR
  // BSRR 高16位是 Reset (置0)，低16位是 Set (置1)
  // 我们操作的是 PA0-PA7，所以数据要左移 8 位

  uint32_t set_bits = ((uint32_t)data);
  uint32_t reset_bits = ((uint32_t)(~data & 0xFF));

  LCD_DATA_PORT->BSRR = (reset_bits << 16) | set_bits;
}

/**
 * @brief  向LCD写入一个命令 (RS=0)
 * @param  cmd: 8位命令
 */
void Lcd_Write_Cmd(uint8_t cmd)
{
  LCD_CS_LOW(); // 1. 片选拉低，选中LCD
  LCD_RS_LOW(); // 2. RS拉低，表示这是命令

  Lcd_Write_Data_Bus(cmd); // 3. 将命令写入数据总线

  LCD_WR_LOW(); // 4. WR拉低

  LCD_WR_HIGH(); // 5. WR拉高 (产生一个上升沿，数据被锁存)

  LCD_CS_HIGH(); // 6. 片选拉高，释放LCD
}

/**
 * @brief  向LCD写入一个数据 (RS=1)
 * @param  data: 8位数据
 */
void Lcd_Write_Data(uint8_t data)
{
  LCD_CS_LOW();  // 1. 片选拉低
  LCD_RS_HIGH(); // 2. RS拉高，表示这是数据

  Lcd_Write_Data_Bus(data); // 3. 将数据写入数据总线

  LCD_WR_LOW(); // 4. WR拉低

  LCD_WR_HIGH(); // 5. WR拉高 (锁存数据)

  LCD_CS_HIGH(); // 6. 片选拉高
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief LCD硬件复位
 *
 */
void Lcd_Reset(void)
{
  // // 1. 先拉高，确保复位引脚处于已知状态
  LCD_RST_HIGH();
  HAL_Delay(50);

  // 2. 拉低复位
  LCD_RST_LOW();
  HAL_Delay(120); // 保持足够长的时间

  // 3. 拉高结束复位
  LCD_RST_HIGH();
  HAL_Delay(120); // 等待 LCD 内部逻辑复位完成
}

void Lcd_Init(void)
{
  HAL_Delay(200);

  // 1. 执行硬件复位
  Lcd_Reset();

  // 2. 开始执行S6D04D1X21初始化序列
  Lcd_Write_Cmd(0xE0);  // MDDI Control 1 (E0h)
  Lcd_Write_Data(0x01); // VWAKE_EN=1
  HAL_Delay(150);

  Lcd_Write_Cmd(0x11); // Sleep Out (11h)
  HAL_Delay(150);      // 必须等待 150ms

  Lcd_Write_Cmd(0xF3);  // Power Control Register (F3h)
  Lcd_Write_Data(0x01); // VCI1_EN
  Lcd_Write_Data(0x00);
  Lcd_Write_Data(0x00);
  Lcd_Write_Data(0x0C); // VCI1 level
  Lcd_Write_Data(0x03); // VGH, VGL
  Lcd_Write_Data(0x75);
  Lcd_Write_Data(0x75);
  Lcd_Write_Data(0x30);

  Lcd_Write_Cmd(0xF4); // VCOM Control Register (F4h)
  Lcd_Write_Data(0x4C);
  Lcd_Write_Data(0x4C);
  Lcd_Write_Data(0x44);
  Lcd_Write_Data(0x44);
  Lcd_Write_Data(0x22);

  Lcd_Write_Cmd(0xF5); // Source Output Control Register (F5h)
  Lcd_Write_Data(0x10);
  Lcd_Write_Data(0x22);
  Lcd_Write_Data(0x05);
  Lcd_Write_Data(0xF0);
  Lcd_Write_Data(0x70);
  Lcd_Write_Data(0x1F);
  HAL_Delay(30);

  // VCI1 逐步上电
  Lcd_Write_Cmd(0xF3); // Power Control Register (F3h)
  Lcd_Write_Data(0x03);
  HAL_Delay(30);

  Lcd_Write_Cmd(0xF3);
  Lcd_Write_Data(0x07);
  HAL_Delay(30);

  Lcd_Write_Cmd(0xF3);
  Lcd_Write_Data(0x0F);
  HAL_Delay(30);

  Lcd_Write_Cmd(0xF3);
  Lcd_Write_Data(0x1F);
  HAL_Delay(30);

  Lcd_Write_Cmd(0xF3);
  Lcd_Write_Data(0x7F);
  HAL_Delay(30);

  // --- Gamma 设置 (非常重要) ---
  Lcd_Write_Cmd(0xF7); // Positive Gamma Control Register for Red (F7h)
  Lcd_Write_Data(0x80);
  Lcd_Write_Data(0x00);
  Lcd_Write_Data(0x00);
  Lcd_Write_Data(0x05);
  Lcd_Write_Data(0x0D);
  Lcd_Write_Data(0x1F);
  Lcd_Write_Data(0x26);
  Lcd_Write_Data(0x2D);
  Lcd_Write_Data(0x14);
  Lcd_Write_Data(0x15);
  Lcd_Write_Data(0x26);
  Lcd_Write_Data(0x20);
  Lcd_Write_Data(0x01);
  Lcd_Write_Data(0x22);
  Lcd_Write_Data(0x22);

  Lcd_Write_Cmd(0xF8); // Negative Gamma Control Register for Red
  Lcd_Write_Data(0x80);
  Lcd_Write_Data(0x00);
  Lcd_Write_Data(0x00);
  Lcd_Write_Data(0x00);
  Lcd_Write_Data(0x07);
  Lcd_Write_Data(0x1E);
  Lcd_Write_Data(0x2A);
  Lcd_Write_Data(0x32);
  Lcd_Write_Data(0x10);
  Lcd_Write_Data(0x16);
  Lcd_Write_Data(0x36);
  Lcd_Write_Data(0x3C);
  Lcd_Write_Data(0x3B);
  Lcd_Write_Data(0x22);
  Lcd_Write_Data(0x22);

  Lcd_Write_Cmd(0xF9); // Positive Gamma Control Register for Green
  Lcd_Write_Data(0x80);
  Lcd_Write_Data(0x00);
  Lcd_Write_Data(0x00);
  Lcd_Write_Data(0x05);
  Lcd_Write_Data(0x0D);
  Lcd_Write_Data(0x1F);
  Lcd_Write_Data(0x26);
  Lcd_Write_Data(0x2D);
  Lcd_Write_Data(0x14);
  Lcd_Write_Data(0x15);
  Lcd_Write_Data(0x26);
  Lcd_Write_Data(0x20);
  Lcd_Write_Data(0x01);
  Lcd_Write_Data(0x22);
  Lcd_Write_Data(0x22);

  Lcd_Write_Cmd(0xFA); // Negative Gamma Control Register for Green
  Lcd_Write_Data(0x80);
  Lcd_Write_Data(0x00);
  Lcd_Write_Data(0x00);
  Lcd_Write_Data(0x00);
  Lcd_Write_Data(0x07);
  Lcd_Write_Data(0x1E);
  Lcd_Write_Data(0x2A);
  Lcd_Write_Data(0x32);
  Lcd_Write_Data(0x10);
  Lcd_Write_Data(0x16);
  Lcd_Write_Data(0x36);
  Lcd_Write_Data(0x3C);
  Lcd_Write_Data(0x3B);
  Lcd_Write_Data(0x22);
  Lcd_Write_Data(0x22);

  Lcd_Write_Cmd(0xFB); // Positive Gamma Control Register for Blue
  Lcd_Write_Data(0x80);
  Lcd_Write_Data(0x00);
  Lcd_Write_Data(0x00);
  Lcd_Write_Data(0x05);
  Lcd_Write_Data(0x0D);
  Lcd_Write_Data(0x1F);
  Lcd_Write_Data(0x26);
  Lcd_Write_Data(0x2D);
  Lcd_Write_Data(0x14);
  Lcd_Write_Data(0x15);
  Lcd_Write_Data(0x26);
  Lcd_Write_Data(0x20);
  Lcd_Write_Data(0x01);
  Lcd_Write_Data(0x22);
  Lcd_Write_Data(0x22);

  Lcd_Write_Cmd(0xFC); // Negative Gamma Control Register for Blue
  Lcd_Write_Data(0x80);
  Lcd_Write_Data(0x00);
  Lcd_Write_Data(0x00);
  Lcd_Write_Data(0x00);
  Lcd_Write_Data(0x07);
  Lcd_Write_Data(0x1E);
  Lcd_Write_Data(0x2A);
  Lcd_Write_Data(0x32);
  Lcd_Write_Data(0x10);
  Lcd_Write_Data(0x16);
  Lcd_Write_Data(0x36);
  Lcd_Write_Data(0x3C);
  Lcd_Write_Data(0x3B);
  Lcd_Write_Data(0x22);
  Lcd_Write_Data(0x22);

  // --- 结束 Gamma 设置 ---

  Lcd_Write_Cmd(0x34); // Tearing Effect Line OFF (34h)

  Lcd_Write_Cmd(0x36);         // Memory Data Access Control (36h)
  Lcd_Write_Data(0x80 | 0x40); // 设置扫描方向等, 如果显示反了, 调整这个值

  Lcd_Write_Cmd(0x3A);  // Interface Pixel Format (3Ah)
  Lcd_Write_Data(0x55); // 0x55 = 16bit/pixel (RGB565)

  Lcd_Write_Cmd(0xF2); // Display Control Register (F2h)
  Lcd_Write_Data(0x17);
  Lcd_Write_Data(0x17);
  Lcd_Write_Data(0x0F);
  Lcd_Write_Data(0x08);
  Lcd_Write_Data(0x08);
  Lcd_Write_Data(0x00);
  Lcd_Write_Data(0x00);
  Lcd_Write_Data(0x00);
  Lcd_Write_Data(0x06);
  Lcd_Write_Data(0x13);
  Lcd_Write_Data(0x00);

  Lcd_Write_Cmd(0xF6); // Interface Control Register (F6h)
  Lcd_Write_Data(0x00);
  Lcd_Write_Data(0x80); // 8-bit 8080 interface
  Lcd_Write_Data(0x00);
  Lcd_Write_Data(0x00);

  Lcd_Write_Cmd(0xFD); // Gate Control Register (FDh)
  Lcd_Write_Data(0x02);
  Lcd_Write_Data(0x00); // 240*432

  HAL_Delay(20);

  // 3. 最终开启显示
  Lcd_Write_Cmd(0x29); // Display On (29h)
  HAL_Delay(20);

  // --- 初始化完成 ---
}

/**
 * @brief 设置LCD的绘图窗口
 *
 * @param x_start X起始位置
 * @param y_start Y起始位置
 * @param x_end X结束位置
 * @param y_end Y结束位置
 */
void Lcd_Set_Window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end)
{
  // 设置 X 坐标
  Lcd_Write_Cmd(0x2A);
  Lcd_Write_Data(x_start >> 8);
  Lcd_Write_Data(x_start & 0xFF);
  Lcd_Write_Data(x_end >> 8);
  Lcd_Write_Data(x_end & 0xFF);

  // 设置 Y 坐标
  Lcd_Write_Cmd(0x2B);
  Lcd_Write_Data(y_start >> 8);
  Lcd_Write_Data(y_start & 0xFF);
  Lcd_Write_Data(y_end >> 8);
  Lcd_Write_Data(y_end & 0xFF);
}

/**
 * @brief 绘制一个16*16的字符
 *
 * @param x x坐标
 * @param y y坐标
 * @param font 要绘制的字
 * @param color 字体颜色
 */
void Lcd_Draw_Char_16x16(uint16_t x, uint16_t y, const uint8_t *font, uint16_t color, uint16_t bg_color)
{
  uint16_t i, j;
  // 设置绘制窗口为 16x16 区域
  Lcd_Set_Window(x, y, x + 15, y + 15);
  Lcd_Write_Cmd(0x2C);

  LCD_CS_LOW();
  LCD_RS_HIGH();

  for (i = 0; i < 32; i++) // 32字节数据
  {
    uint8_t temp = font[i];
    for (j = 0; j < 8; j++) // 每字节8位
    {
      // 【核心修复】根据位值选择颜色：1用字体色，0用背景色
      uint16_t pixel_color = (temp & 0x80) ? color : bg_color;

      // --- 写入高8位 ---
      Lcd_Write_Data_Bus(pixel_color >> 8);
      LCD_WR_LOW();
      LCD_WR_HIGH();

      // --- 写入低8位 ---
      Lcd_Write_Data_Bus(pixel_color & 0xFF);
      LCD_WR_LOW();
      LCD_WR_HIGH();

      temp <<= 1;
    }
  }
  LCD_CS_HIGH();
}

/**
 * @brief  使用特定颜色填充整个屏幕 (高速优化版)
 * @param  color: 16-bit (RGB565) 颜色值
 */
void Lcd_Clear(uint16_t color)
{
  unsigned long i;
  // 您的代码中 96000 = 240 * 432
  unsigned long total_pixels = (unsigned long)240 * 432;

  // 1. 设置窗口为全屏 (坐标 0,0 到 239,431)
  Lcd_Set_Window(0, 0, 240 - 1, 432 - 1);

  // 2. 发送 "准备写内存" 命令
  Lcd_Write_Cmd(0x2C);

  // 3. 准备好要发送的高8位和低8位
  uint8_t color_high = (uint8_t)(color >> 8);
  uint8_t color_low = (uint8_t)(color & 0xFF);

  // 4. --- 开始高速写入 ---
  // 保持 CS=Low, RS=High，这使我们能连续发送数据
  LCD_CS_LOW();
  LCD_RS_HIGH();

  for (i = 0; i < total_pixels; i++)
  {
    // 发送高8位
    Lcd_Write_Data_Bus(color_high);
    LCD_WR_LOW();
    // for (volatile int d = 0; d < 20; d++)
    //   __NOP();
    LCD_WR_HIGH();

    // 发送低8位
    Lcd_Write_Data_Bus(color_low);
    LCD_WR_LOW();
    // for (volatile int d = 0; d < 20; d++)
    //   __NOP();
    LCD_WR_HIGH();
  }

  // 5. --- 结束高速写入 ---
  LCD_CS_HIGH();

  uint16_t text_color = ~color;

  // 计算居中位置 (屏幕宽240, 高432)
  // X轴居中: (240 - 16) / 2 = 112
  // Y轴起始: 150 (大概中间偏上)
  uint16_t x_pos = 112;
  uint16_t y_pos = 150;

  Lcd_Draw_Char_16x16(x_pos, y_pos, Font_16x16_Qing, text_color, color);
  Lcd_Draw_Char_16x16(x_pos, y_pos + 20, Font_16x16_Ping, text_color, color); // 垂直间距 20
  Lcd_Draw_Char_16x16(x_pos, y_pos + 40, Font_16x16_Zhong, text_color, color);
  Lcd_Draw_Char_16x16(x_pos, y_pos + 60, dot, text_color, color);
  Lcd_Draw_Char_16x16(x_pos, y_pos + 70, dot, text_color, color);
  Lcd_Draw_Char_16x16(x_pos, y_pos + 80, dot, text_color, color);
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  // HAL_Delay(100);
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
  Lcd_Init();
  Lcd_Clear(0xFFFF);

  uint16_t test_colors[] = {
      0xF800, // 红色 (Red)
      0x07E0, // 绿色 (Green)
      0x001F, // 蓝色 (Blue)
      0xFFFF, // 白色 (White)
      0x0000, // 黑色 (Black)
      0xFFE0, // 黄色 (Yellow)
      0xF81F, // 紫色 (Magenta)
      0x07FF  // 青色 (Cyan)
  };

  uint8_t color_index = 0;
  uint8_t total_colors = sizeof(test_colors) / sizeof(test_colors[0]);

  // int sign = 0;
  // for (int i = 0; i < 256; i++)
  // {
  //   // 这里的逻辑是将 0 - 255 分成三段，确保 R, G, B 不会同时达到最大值 从而产生鲜艳的彩色，而不是白色
  //   if (i < 85)
  //   {
  //     // 第一阶段 (0~84): 红色渐弱，绿色渐强
  //     // R: 255 -> 0
  //     // G: 0 -> 255
  //     // B: 0
  //     GRB_Color[1][i] = 255 - i * 3; // R
  //     GRB_Color[0][i] = i * 3;       // G
  //     GRB_Color[2][i] = 0;           // B
  //   }
  //   else if (i < 170)
  //   {
  //     // 第二阶段 (85~169): 绿色渐弱，蓝色渐强
  //     // R: 0
  //     // G: 255 -> 0
  //     // B: 0 -> 255
  //     GRB_Color[1][i] = 0;                  // R
  //     GRB_Color[0][i] = 255 - (i - 85) * 3; // G
  //     GRB_Color[2][i] = (i - 85) * 3;       // B
  //   }
  //   else
  //   {
  //     // 第三阶段 (170~255): 蓝色渐弱，红色渐强
  //     // R: 0 -> 255
  //     // G: 0
  //     // B: 255 -> 0
  //     GRB_Color[1][i] = (i - 170) * 3;       // R
  //     GRB_Color[0][i] = 0;                   // G
  //     GRB_Color[2][i] = 255 - (i - 170) * 3; // B
  //   }
  // }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    // WS2812_Update(GRB_Color[0][sign], GRB_Color[1][sign], GRB_Color[2][sign]);
    // sign = ++sign > 255 ? 0 : sign;

    // 1. 刷屏当前颜色
    Lcd_Clear(test_colors[color_index]);

    // 2. 切换到下一个颜色
    color_index++;
    if (color_index >= total_colors)
    {
      color_index = 0;
    }

    // 3. 延时 1000ms (1秒)
    HAL_Delay(1000);
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enables the Clock Security System
   */
  HAL_RCC_EnableCSS();
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
