/**
 ****************************************************************************************************
 * @file        temp_adc.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-07-20
 * @brief       内部温度传感器（ADC) 驱动代码
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

#ifndef __TEMP_ADC_H
#define __TEMP_ADC_H

#include "./SYSTEM/sys/sys.h"


/* 内部温度传感器ADC相关定义 */
#define ADC_ADCX                            ADC1 
#define ADC_ADCX_TEMPSENSOR_CHY             ADC_Channel_16                                                     /* 通道16 */
#define ADC_ADCX_CHY_CLK_ENABLE()           do{RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);}while(0)   /* ADC1时钟使能 */

/******************************************************************************************/

void adc_temperature_init(void);                                /* ADC温度采集初始化函数 */
uint32_t adc_get_result(uint32_t ch);                           /* 获得某个通道值  */
uint32_t adc_get_result_average(uint32_t ch, uint8_t times);    /* 得到某个通道给定次数采样的平均值 */
short adc_get_temperature(void);                                /* 获取内部温度传感器的温度 */

#endif 

