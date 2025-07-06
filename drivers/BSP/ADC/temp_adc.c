/**
 ****************************************************************************************************
 * @file        temp_adc.c
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

#include "./BSP/ADC/temp_adc.h"
#include "./SYSTEM/delay/delay.h"
#include "rtthread.h"


/**
 * @brief       ADC 内部温度传感器 初始化函数
 *   @note      注意: CH32V307内部温度传感器只连接在ADC1的通道16上, 其他ADC无法进行转换
 *
 * @param       无
 * @retval      无
 */
void adc_temperature_init(void)
{
    ADC_InitTypeDef adc_handle;

    ADC_ADCX_CHY_CLK_ENABLE();          /* 使能ADCx时钟 */

    RCC_ADCCLKConfig(RCC_PCLK2_Div6);   /* 设置ADC分频，6分频 */

    adc_handle.ADC_Mode= ADC_Mode_Independent;                      /* 独立模式  */
    adc_handle.ADC_DataAlign = ADC_DataAlign_Right;                 /* 右对齐 */
    adc_handle.ADC_ScanConvMode = DISABLE;                          /* 非扫描模式 */
    adc_handle.ADC_ContinuousConvMode = DISABLE;                    /* 关闭连续转换 */
    adc_handle.ADC_NbrOfChannel = 1;                                /* 顺序进行规则转换的ADC通道的数目 */
    adc_handle.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;    /* 软件触发 */
    ADC_Init(ADC_ADCX,&adc_handle);                                 /* 初始化ADC */

    ADC_TempSensorVrefintCmd(ENABLE);                               /* 使能内部温度传感器  */

    ADC_Cmd(ADC_ADCX, ENABLE);                                      /* 使能ADCx */

    ADC_ResetCalibration(ADC_ADCX);                                 /* 使能复位校准  */

    while(ADC_GetResetCalibrationStatus(ADC_ADCX));                 /* 等待复位校准结束 */

    ADC_StartCalibration(ADC_ADCX);                                 /* 开启ADC校准 */

    while(ADC_GetCalibrationStatus(ADC_ADCX));                      /* 等待校准结束 */
}

/**
 * @brief       获得ADC转换后的结果
 * @param       ch: 通道值
 * @retval      无
 */
uint32_t adc_get_result(uint32_t ch)
{
    ADC_RegularChannelConfig(ADC_ADCX, ch, 1, ADC_SampleTime_239Cycles5 );  /* 设置通道，序列和采样时间 */

    ADC_SoftwareStartConvCmd(ADC_ADCX,ENABLE);            /* 开启ADCx */

    while(!ADC_GetFlagStatus(ADC_ADCX, ADC_FLAG_EOC ));   /* 轮询转换 */

    return ADC_GetConversionValue(ADC_ADCX);              /* 返回最近一次ADCx规则组的转换结果 */
}

/**
 * @brief       获取通道ch的转换值，取times次,然后平均
 * @param       ch      : 通道号
 * @param       times   : 获取次数
 * @retval      通道ch的times次转换结果平均值
 */
uint32_t adc_get_result_average(uint32_t ch, uint8_t times)
{
    uint32_t temp_val = 0;
    uint8_t t;

    for (t = 0; t < times; t++)     /* 获取times次数据 */
    {
        temp_val += adc_get_result(ch);
        rt_thread_mdelay(5);
    }

    return temp_val / times;        /* 返回平均值 */
}

/**
 * @brief       获取内部温度传感器温度值
 * @param       无
 * @retval      温度值(扩大了100倍,单位:℃)
 */
short adc_get_temperature(void)
{
    uint32_t adcx;
    short result;
    double temperature;

    adcx = adc_get_result_average(ADC_ADCX_TEMPSENSOR_CHY, 10); /* 读取内部温度传感器通道,10次取平均 */
    temperature = (float)adcx*(3.3/4096);                       /* 获取电压值 */
    temperature = (temperature-1.4)/0.0043+25;                  /* 将电压值转换为温度值 */
    result = temperature *= 100;                                /* 扩大100倍 */
    return result;
}

