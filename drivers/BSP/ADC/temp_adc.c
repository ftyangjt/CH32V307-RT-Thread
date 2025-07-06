/**
 ****************************************************************************************************
 * @file        temp_adc.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2023-07-20
 * @brief       �ڲ��¶ȴ�������ADC) ��������
 * @license     Copyright (c) 2020-2032, ������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:����ԭ�� CH32V307������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
 *
 * �޸�˵��
 * V1.0 20230720
 * ��һ�η���
 *
 ****************************************************************************************************
 */

#include "./BSP/ADC/temp_adc.h"
#include "./SYSTEM/delay/delay.h"
#include "rtthread.h"


/**
 * @brief       ADC �ڲ��¶ȴ����� ��ʼ������
 *   @note      ע��: CH32V307�ڲ��¶ȴ�����ֻ������ADC1��ͨ��16��, ����ADC�޷�����ת��
 *
 * @param       ��
 * @retval      ��
 */
void adc_temperature_init(void)
{
    ADC_InitTypeDef adc_handle;

    ADC_ADCX_CHY_CLK_ENABLE();          /* ʹ��ADCxʱ�� */

    RCC_ADCCLKConfig(RCC_PCLK2_Div6);   /* ����ADC��Ƶ��6��Ƶ */

    adc_handle.ADC_Mode= ADC_Mode_Independent;                      /* ����ģʽ  */
    adc_handle.ADC_DataAlign = ADC_DataAlign_Right;                 /* �Ҷ��� */
    adc_handle.ADC_ScanConvMode = DISABLE;                          /* ��ɨ��ģʽ */
    adc_handle.ADC_ContinuousConvMode = DISABLE;                    /* �ر�����ת�� */
    adc_handle.ADC_NbrOfChannel = 1;                                /* ˳����й���ת����ADCͨ������Ŀ */
    adc_handle.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;    /* ������� */
    ADC_Init(ADC_ADCX,&adc_handle);                                 /* ��ʼ��ADC */

    ADC_TempSensorVrefintCmd(ENABLE);                               /* ʹ���ڲ��¶ȴ�����  */

    ADC_Cmd(ADC_ADCX, ENABLE);                                      /* ʹ��ADCx */

    ADC_ResetCalibration(ADC_ADCX);                                 /* ʹ�ܸ�λУ׼  */

    while(ADC_GetResetCalibrationStatus(ADC_ADCX));                 /* �ȴ���λУ׼���� */

    ADC_StartCalibration(ADC_ADCX);                                 /* ����ADCУ׼ */

    while(ADC_GetCalibrationStatus(ADC_ADCX));                      /* �ȴ�У׼���� */
}

/**
 * @brief       ���ADCת����Ľ��
 * @param       ch: ͨ��ֵ
 * @retval      ��
 */
uint32_t adc_get_result(uint32_t ch)
{
    ADC_RegularChannelConfig(ADC_ADCX, ch, 1, ADC_SampleTime_239Cycles5 );  /* ����ͨ�������кͲ���ʱ�� */

    ADC_SoftwareStartConvCmd(ADC_ADCX,ENABLE);            /* ����ADCx */

    while(!ADC_GetFlagStatus(ADC_ADCX, ADC_FLAG_EOC ));   /* ��ѯת�� */

    return ADC_GetConversionValue(ADC_ADCX);              /* �������һ��ADCx�������ת����� */
}

/**
 * @brief       ��ȡͨ��ch��ת��ֵ��ȡtimes��,Ȼ��ƽ��
 * @param       ch      : ͨ����
 * @param       times   : ��ȡ����
 * @retval      ͨ��ch��times��ת�����ƽ��ֵ
 */
uint32_t adc_get_result_average(uint32_t ch, uint8_t times)
{
    uint32_t temp_val = 0;
    uint8_t t;

    for (t = 0; t < times; t++)     /* ��ȡtimes������ */
    {
        temp_val += adc_get_result(ch);
        rt_thread_mdelay(5);
    }

    return temp_val / times;        /* ����ƽ��ֵ */
}

/**
 * @brief       ��ȡ�ڲ��¶ȴ������¶�ֵ
 * @param       ��
 * @retval      �¶�ֵ(������100��,��λ:��)
 */
short adc_get_temperature(void)
{
    uint32_t adcx;
    short result;
    double temperature;

    adcx = adc_get_result_average(ADC_ADCX_TEMPSENSOR_CHY, 10); /* ��ȡ�ڲ��¶ȴ�����ͨ��,10��ȡƽ�� */
    temperature = (float)adcx*(3.3/4096);                       /* ��ȡ��ѹֵ */
    temperature = (temperature-1.4)/0.0043+25;                  /* ����ѹֵת��Ϊ�¶�ֵ */
    result = temperature *= 100;                                /* ����100�� */
    return result;
}

