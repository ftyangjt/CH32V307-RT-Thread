/**
 ****************************************************************************************************
 * @file        spi.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-07-20
 * @brief       SPI 驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 CH32V307开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.0 20230720
 * 第一次发布
 *
 ****************************************************************************************************
 */

#include "./BSP/SPI/spi.h"


/**
 * @brief       SPI初始化代码
 *   @note      主机模式,8位数据,禁止硬件片选
 * @param       无
 * @retval      无
 */
void spi2_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;
    SPI_InitTypeDef  spi2_handle;

    SPI2_SPI_CLK_ENABLE();       /* SPI2时钟使能 */
    SPI2_SCK_GPIO_CLK_ENABLE();  /* SPI2_SCK脚时钟使能 */
    SPI2_MISO_GPIO_CLK_ENABLE(); /* SPI2_MISO脚时钟使能 */
    SPI2_MOSI_GPIO_CLK_ENABLE(); /* SPI2_MOSI脚时钟使能 */

    /* SCK引脚模式设置(复用输出) */
    gpio_init_struct.GPIO_Pin = SPI2_SCK_GPIO_PIN;
    gpio_init_struct.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SPI2_SCK_GPIO_PORT, &gpio_init_struct);

    /* MISO引脚模式设置(复用输出) */
    gpio_init_struct.GPIO_Pin = SPI2_MISO_GPIO_PIN;
    GPIO_Init(SPI2_MISO_GPIO_PORT, &gpio_init_struct);

    /* MOSI引脚模式设置(复用输出) */
    gpio_init_struct.GPIO_Pin = SPI2_MOSI_GPIO_PIN;
    GPIO_Init(SPI2_MOSI_GPIO_PORT, &gpio_init_struct);

    spi2_handle.SPI_Mode = SPI_Mode_Master;                                 /* 设置SPI工作模式，设置为主模式 */
    spi2_handle.SPI_Direction = SPI_Direction_2Lines_FullDuplex;            /* 设置SPI单向或者双向的数据模式:SPI设置为双线模式 */
    spi2_handle.SPI_DataSize = SPI_DataSize_8b;                             /* 设置SPI的数据大小:SPI发送接收8位帧结构 */
    spi2_handle.SPI_CPOL = SPI_CPOL_High;                                   /* 串行同步时钟的空闲状态为高电平 */
    spi2_handle.SPI_CPHA = SPI_CPHA_2Edge;                                  /* 串行同步时钟的第二个跳变沿（上升或下降）数据被采样 */
    spi2_handle.SPI_NSS = SPI_NSS_Soft;                                     /* NSS信号由硬件（NSS管脚）还是软件（使用SSI位）管理:内部NSS信号有SSI位控制 */
    spi2_handle.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;          /* 定义波特率预分频的值:波特率预分频值为256 */
    spi2_handle.SPI_FirstBit = SPI_FirstBit_MSB;                            /* 指定数据传输从MSB位还是LSB位开始:数据传输从MSB位开始 */
    spi2_handle.SPI_CRCPolynomial = 7;                                      /* CRC值计算的多项式 */
    SPI_Init(SPI2_SPI,&spi2_handle);                                        /* 初始化 */

    SPI_Cmd(SPI2_SPI, ENABLE);  /* 使能SPI2 */

    spi2_read_write_byte(0Xff); /* 启动传输, 实际上就是产生8个时钟脉冲, 达到清空DR的作用, 非必需 */
}

/**
 * @brief       SPI2速度设置函数
 *   @note      SPI2时钟选择来自APB1, 即PCLK1, 为72Mhz
 *              SPI速度 = PCLK1 / 2^(speed + 1)
 * @param       speed   : SPI2时钟分频系数
                                取值为SPI_SPEED_2~SPI_SPEED_256（0~7）
 * @retval      无
 */
void spi2_set_speed(uint8_t speed)
{
    speed &= 0X07;                      /* 限制范围 */
    SPI_Cmd(SPI2_SPI,DISABLE);          /* 关闭SPI */
    SPI2_SPI->CTLR1 &= 0XFFC7;          /* 位3-5清零，用来设置波特率 */
    SPI2_SPI->CTLR1 |= speed << 3;      /* 设置SPI速度 */
    SPI_Cmd(SPI2_SPI,ENABLE);           /* 使能SPI */
}

/**
 * @brief       SPI2读写一个字节数据
 * @param       txdata  : 要发送的数据(1字节)
 * @retval      接收到的数据(1字节)
 */
uint8_t spi2_read_write_byte(uint8_t txdata)
{
    while (SPI_I2S_GetFlagStatus(SPI2_SPI, SPI_I2S_FLAG_TXE) == RESET);  /* 等待发送区空 */

    SPI_I2S_SendData(SPI2_SPI, txdata);                                  /* 发送一个byte */

    while (SPI_I2S_GetFlagStatus(SPI2_SPI, SPI_I2S_FLAG_RXNE) == RESET); /* 等待接收完一个byte */

    return SPI_I2S_ReceiveData(SPI2_SPI);                                /* 返回收到的数据 */
}


