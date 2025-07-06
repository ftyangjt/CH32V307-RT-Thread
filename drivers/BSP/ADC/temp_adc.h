/**
 ****************************************************************************************************
 * @file        temp_adc.h
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

#ifndef __TEMP_ADC_H
#define __TEMP_ADC_H

#include "./SYSTEM/sys/sys.h"


/* �ڲ��¶ȴ�����ADC��ض��� */
#define ADC_ADCX                            ADC1 
#define ADC_ADCX_TEMPSENSOR_CHY             ADC_Channel_16                                                     /* ͨ��16 */
#define ADC_ADCX_CHY_CLK_ENABLE()           do{RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);}while(0)   /* ADC1ʱ��ʹ�� */

/******************************************************************************************/

void adc_temperature_init(void);                                /* ADC�¶Ȳɼ���ʼ������ */
uint32_t adc_get_result(uint32_t ch);                           /* ���ĳ��ͨ��ֵ  */
uint32_t adc_get_result_average(uint32_t ch, uint8_t times);    /* �õ�ĳ��ͨ����������������ƽ��ֵ */
short adc_get_temperature(void);                                /* ��ȡ�ڲ��¶ȴ��������¶� */

#endif 

