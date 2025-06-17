/**
 ****************************************************************************************************
 * @file        sdio_sdcard.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2023-07-20
 * @brief       SD�� ��������
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

#include "string.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/SDIO/sdio_sdcard.h"
#include "string.h"


void SDIO_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

SDIO_InitTypeDef g_sdio_init_struct;
SDIO_CmdInitTypeDef g_sdio_cmd_struct;
SDIO_DataInitTypeDef g_sdio_data_struct;

static uint8_t g_sd_card_type = SDIO_STD_CAPACITY_SD_CARD_V1_1;
static uint32_t g_csd_tab[4], g_cid_tab[4], g_rca = 0;
static uint8_t g_device_mode = SD_DMA_MODE;
static uint8_t g_stop_condition = 0;
volatile SD_Error g_transfer_error = SD_OK;
volatile uint8_t g_transfer_end = 0;
SD_CardInfo g_sd_card_info;

__attribute__ ((aligned(4))) uint8_t SDIO_DATA_BUFFER[512];

/**
 * @brief       ���CMD0��ִ��״̬
 * @param       ��
 * @retval      �������
 */
static SD_Error cmd_error(void)
{
    SD_Error errorstatus = SD_OK;
    uint32_t timeout = SDIO_CMD0TIMEOUT;

    while(timeout--)
    {
        if(SDIO_GetFlagStatus(SDIO_FLAG_CMDSENT) != RESET)    /* �����ѷ���(������Ӧ) */
        {
          break;
        }
    }

    if(timeout == 0)                            /* ��ʱ */
    {
        return SD_CMD_RSP_TIMEOUT;
    }

    SDIO_ClearFlag(SDIO_STATIC_FLAGS);          /* �����־ */

    return errorstatus;
}

/**
 * @brief       ���R7��Ӧ�Ĵ���״̬
 * @param       ��
 * @retval      �������
 */
static SD_Error cmd_resp7_error(void)
{
    SD_Error errorstatus = SD_OK;
    uint32_t status = 0;
    uint32_t timeout = SDIO_CMD0TIMEOUT;

    while(timeout--)
    {
        status = SDIO->STA;
        if(status&((1<<0)|(1<<2)|(1<<6)))       /* CRC����/������Ӧ��ʱ/�Ѿ��յ���Ӧ(CRCУ��ɹ�) */
        {
            break;
        }
    }

    /* ��Ӧ��ʱ */
    if((timeout == 0) || (status&(1<<2)))
    {
        errorstatus = SD_CMD_RSP_TIMEOUT;       /* ��ǰ������2.0���ݿ�,���߲�֧���趨�ĵ�ѹ��Χ */
        SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);     /* ���������Ӧ��ʱ��־ */
        return errorstatus;
    }

    /* �ɹ����յ���Ӧ */
    if((status) & ( 1<<6 ))
    {
        errorstatus = SD_OK;                    /* ��¼״̬ */
        SDIO_ClearFlag(SDIO_FLAG_CMDREND);      /* �����־ */
    }
    return errorstatus;
}

/**
 * @brief       ���R1��Ӧ�Ĵ���״̬
 * @param       cmd: ����
 * @retval      �������
 */
static SD_Error cmd_resp1_error(uint8_t cmd)
{
    uint32_t status;

    while(1)
    {
        status = SDIO->STA;
        if(status&((1<<0)|(1<<2)|(1<<6)))               /* CRC����/������Ӧ��ʱ/�Ѿ��յ���Ӧ(CRCУ��ɹ�) */
        {
            break;
        }
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_CTIMEOUT) != RESET) /* ��Ӧ��ʱ */
    {
        SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);             /* ���������Ӧ��ʱ��־ */
        return SD_CMD_RSP_TIMEOUT;                      /* ���ش������� */
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_CCRCFAIL) != RESET) /* CRC���� */
    {
        SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);             /* �����־ */
        return SD_CMD_CRC_FAIL;                         /* ���ش������� */
    }

    if(SDIO->RESPCMD != cmd)                            /* �Ƿ����� */
    {
        return SD_ILLEGAL_CMD;
    }

    SDIO->ICR = 0X5FF;                                  /* ������б�־ */
    return (SD_Error)(SDIO->RESP1&SD_OCR_ERRORBITS);    /* ���ؿ���Ӧ */
}

/**
 * @brief       ���R3��Ӧ�Ĵ���״̬
 * @param       ��
 * @retval      �������
 */
static SD_Error cmd_resp3_error(void)
{
    uint32_t status;

    while(1)
    {
        status = SDIO->STA;
        if(status&((1<<0)|(1<<2)|(1<<6)))               /* CRC����/������Ӧ��ʱ/�Ѿ��յ���Ӧ(CRCУ��ɹ�) */
        {
            break;
        }
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_CTIMEOUT) != RESET) /* ��Ӧ��ʱ */
    {
        SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);             /* ���������Ӧ��ʱ��־ */
        return SD_CMD_RSP_TIMEOUT;                      /* ���ش������� */
    }

   SDIO_ClearFlag(SDIO_STATIC_FLAGS);                   /* �����־ */
   return SD_OK;                                        /* ����SD��״̬ */
}

/**
 * @brief       ���R2��Ӧ�Ĵ���״̬
 * @param       ��
 * @retval      �������
 */
static SD_Error cmd_resp2_error(void)
{
    SD_Error errorstatus = SD_OK;
    uint32_t status = 0;
    uint32_t timeout = SDIO_CMD0TIMEOUT;

    while(timeout--)
    {
        status = SDIO->STA;
        if(status&((1<<0)|(1<<2)|(1<<6)))               /* CRC����/������Ӧ��ʱ/�Ѿ��յ���Ӧ(CRCУ��ɹ�) */
        {
            break;
        }
    }

    if((timeout == 0)||(status&(1<<2)))                 /* ��Ӧ��ʱ */
    {
        errorstatus = SD_CMD_RSP_TIMEOUT;
        SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);             /* ���������Ӧ��ʱ��־ */
        return errorstatus;                             /* ���ش������� */
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_CCRCFAIL) != RESET) /* CRC���� */
    {
        errorstatus = SD_CMD_CRC_FAIL;
        SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);             /* �����Ӧ��־ */
    }

    SDIO_ClearFlag(SDIO_STATIC_FLAGS);                  /* �����־ */
    return errorstatus;
}

/**
 * @brief       ���R6��Ӧ�Ĵ���״̬
 * @param       ��
 * @retval      �������
 */
static SD_Error cmd_resp6_error(uint8_t cmd,uint16_t *prca)
{
    SD_Error errorstatus = SD_OK;
    uint32_t status;
    uint32_t rspr1;

    while(1)
    {
        status = SDIO->STA;
        if(status & ((1<<0)|(1<<2)|(1<<6)))               /* CRC����/������Ӧ��ʱ/�Ѿ��յ���Ӧ(CRCУ��ɹ�) */
        {
            break;
        }
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_CTIMEOUT) != RESET)   /* ��Ӧ��ʱ */
    {
        SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);               /* ���������Ӧ��ʱ��־ */
        return SD_CMD_RSP_TIMEOUT;
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_CCRCFAIL) != RESET)   /* CRC���� */
    {
        SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);               /* �����Ӧ��־ */
        return SD_CMD_CRC_FAIL;
    }

    if(SDIO->RESPCMD != cmd)
    {
        return SD_ILLEGAL_CMD;
    }

    SDIO_ClearFlag(SDIO_STATIC_FLAGS);                    /* �����־ */

    rspr1 = SDIO->RESP1;                                  /* �õ���Ӧ */

    if(SD_ALLZERO == (rspr1 & (SD_R6_GENERAL_UNKNOWN_ERROR|SD_R6_ILLEGAL_CMD|SD_R6_COM_CRC_FAILED)))
    {
        *prca = (uint16_t)(rspr1 >> 16);                  /* ����16λ�õ�,rca */
        return errorstatus;
    }

    if(rspr1 & SD_R6_GENERAL_UNKNOWN_ERROR)
    {
        return SD_GENERAL_UNKNOWN_ERROR;
    }

    if(rspr1 & SD_R6_ILLEGAL_CMD)
    {
        return SD_ILLEGAL_CMD;
    }

    if(rspr1 & SD_R6_COM_CRC_FAILED)
    {
        return SD_COM_CRC_FAILED;
    }
    return errorstatus;
}

/**
 * @brief       ����SD����SCR�Ĵ���ֵ
 * @param       rca:����Ե�ַ
 * @param       pscr:���ݻ�����(�洢SCR����)
 * @retval      �������
 */
static SD_Error sd_find_scr(uint16_t rca,uint32_t *pscr)
{
    uint32_t index = 0;
    SD_Error errorstatus = SD_OK;
    uint32_t tempscr[2] = {0, 0};

    /* ����CMD16������ÿ��СΪ8�ֽ� */
    g_sdio_cmd_struct.SDIO_Argument = (uint32_t)8;
    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&g_sdio_cmd_struct);

    /* �ȴ�R1��Ӧ */
    errorstatus = cmd_resp1_error(SD_CMD_SET_BLOCKLEN);

    if(errorstatus != SD_OK)
    {
        return errorstatus;
    }

    /* ����CMD55 + RCA */
    g_sdio_cmd_struct.SDIO_Argument = (uint32_t)g_rca << 16;
    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_APP_CMD;
    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&g_sdio_cmd_struct);

    /* �ȴ�R1��Ӧ */
    errorstatus = cmd_resp1_error(SD_CMD_APP_CMD);

    if(errorstatus != SD_OK)
    {
        return errorstatus;
    }

    /* �������ݸ�ʽ */
    g_sdio_data_struct.SDIO_DataTimeOut = SD_DATATIMEOUT;
    g_sdio_data_struct.SDIO_DataLength = 8;
    g_sdio_data_struct.SDIO_DataBlockSize = SDIO_DataBlockSize_8b;
    g_sdio_data_struct.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;
    g_sdio_data_struct.SDIO_TransferMode = SDIO_TransferMode_Block;
    g_sdio_data_struct.SDIO_DPSM = SDIO_DPSM_Enable;
    SDIO_DataConfig(&g_sdio_data_struct);

    /* ����ACMD51���� */
    g_sdio_cmd_struct.SDIO_Argument = 0x0;
    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SD_APP_SEND_SCR;
    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&g_sdio_cmd_struct);

    /* �ȴ�R1��Ӧ */
    errorstatus = cmd_resp1_error(SD_CMD_SD_APP_SEND_SCR);

    if(errorstatus != SD_OK)
    {
        return errorstatus;
    }

    while(!(SDIO->STA&(SDIO_FLAG_RXOVERR|SDIO_FLAG_DCRCFAIL|SDIO_FLAG_DTIMEOUT|SDIO_FLAG_DBCKEND|SDIO_FLAG_STBITERR)))
    {
        if(SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET)
        {
            *(tempscr + index) = SDIO_ReadData();
            index++;

            if(index >= 2)
            {
                break;
            }
        }
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)
    {
        SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);
        return SD_DATA_TIMEOUT;
    }
    else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)
    {
        SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);
        return SD_DATA_CRC_FAIL;
    }
    else if(SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET)
    {
        SDIO_ClearFlag(SDIO_FLAG_RXOVERR);
        return SD_RX_OVERRUN;
    }
    else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)
    {
        SDIO_ClearFlag(SDIO_FLAG_STBITERR);
        return SD_START_BIT_ERR;
    }

    /* ������б�־ */
    SDIO_ClearFlag(SDIO_STATIC_FLAGS);

    *(pscr+1) = ((tempscr[0] & SD_0TO7BITS) << 24) | ((tempscr[0] & SD_8TO15BITS) <<8 )|
                ((tempscr[0] & SD_16TO23BITS) >> 8) | ((tempscr[0] & SD_24TO31BITS) >> 24);

    *(pscr) = ((tempscr[1] & SD_0TO7BITS) << 24) | ((tempscr[1] & SD_8TO15BITS) << 8)|
              ((tempscr[1] & SD_16TO23BITS) >> 8) | ((tempscr[1] & SD_24TO31BITS) >> 24);

    return errorstatus;
}

/**
 * @brief       ʹ��SDIO������ģʽ
 * @param       enx:0,��ʹ��;1,ʹ��
 * @retval      �������
 */
static SD_Error sd_en_wide_bus(uint8_t enx)
{
    SD_Error errorstatus = SD_OK;
    uint32_t scr[2] = {0, 0};
    uint8_t arg = 0x00;

    if(enx)
    {
        arg = 0x02;   /* 4bit���߿�� */
    }
    else
    {
        arg = 0x00;   /* 1bit���߿�� */
    }

    if(SDIO->RESP1 & SD_CARD_LOCKED)
    {
        return SD_LOCK_UNLOCK_FAILED;
    }

    /* ��ȡSCR�Ĵ���ֵ */
    errorstatus = sd_find_scr(g_rca, scr);

    if(errorstatus != SD_OK)
    {
        return errorstatus;
    }

    /* SD��֧�����߿��ģʽ */
    if((scr[1] & SD_WIDE_BUS_SUPPORT) != SD_ALLZERO)
    {
        /* ����CMD55 + RCA */
        g_sdio_cmd_struct.SDIO_Argument = (uint32_t)g_rca << 16;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_APP_CMD;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* �ȴ�R1��Ӧ */
        errorstatus = cmd_resp1_error(SD_CMD_APP_CMD);

        if(errorstatus != SD_OK)
        {
            return errorstatus;
        }

        /* ����ACMD6���� */
        g_sdio_cmd_struct.SDIO_Argument = arg;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_APP_SD_SET_BUSWIDTH;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* �ȴ�R1��Ӧ */
        errorstatus = cmd_resp1_error(SD_CMD_APP_SD_SET_BUSWIDTH);
        return errorstatus;
    }
    else
    {
        return SD_REQUEST_NOT_APPLICABLE;
    }
}

/**
 * @brief       ���SD���Ƿ�����ִ��д����
 * @param       pstatus: ���ĵ�ǰ״̬
 * @retval      �������
 */
static SD_Error sd_is_card_programming(uint8_t *pstatus)
{
    volatile uint32_t respR1 = 0, status = 0;

    /* ����CMD13 + RCA */
    g_sdio_cmd_struct.SDIO_Argument = (uint32_t)g_rca << 16;
    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SEND_STATUS;
    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&g_sdio_cmd_struct);

    /* ��ȡSDIO��־ */
    status = SDIO->STA;

    while(!(status&((1<<0)|(1<<6)|(1<<2))))
    {
        status = SDIO->STA;
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_CCRCFAIL) != RESET)
    {
        SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
        return SD_CMD_CRC_FAIL;
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_CTIMEOUT) != RESET)
    {
        SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
        return SD_CMD_RSP_TIMEOUT;
    }

    if(SDIO->RESPCMD != SD_CMD_SEND_STATUS)
    {
        return SD_ILLEGAL_CMD;
    }

    SDIO_ClearFlag(SDIO_STATIC_FLAGS);

    respR1 = SDIO->RESP1;

    *pstatus = (uint8_t)((respR1>>9) & 0x0000000F);

    return SD_OK;
}

/**
 * @brief       �õ�bytes��2Ϊ�׵�ָ��
 * @param       bytes:�ֽ���
 * @retval      ��2Ϊ�׵�ָ��ֵ
 */
static uint8_t convert_from_bytes_to_power_of_two(uint16_t bytes)
{
    uint8_t count = 0;

    while(bytes != 1)
    {
        bytes>>=1;
        count++;
    }

    return count;
}

/**
 * @brief       ��ʼ��SD��
 * @param       ��
 * @retval      �������
 */
SD_Error sd_init(void)
{
    NVIC_InitTypeDef nvic_init_struct;
    GPIO_InitTypeDef gpio_init_struct;

    uint8_t clkdiv = 0;
    SD_Error errorstatus = SD_OK;

    /* ʹ��ʱ�� */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_SDIO|RCC_AHBPeriph_DMA2, ENABLE);

    /* ����SDIO������� */
    gpio_init_struct.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12;
    gpio_init_struct.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &gpio_init_struct);

    gpio_init_struct.GPIO_Pin = GPIO_Pin_2;
    gpio_init_struct.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &gpio_init_struct);

    /* ��λSDIO */
    SDIO_DeInit();

    /* SDIO�ж����� */
    nvic_init_struct.NVIC_IRQChannel = SDIO_IRQn;
    nvic_init_struct.NVIC_IRQChannelPreemptionPriority = 0;
    nvic_init_struct.NVIC_IRQChannelSubPriority = 0;
    nvic_init_struct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init_struct);

    /* �ϵ� */
    errorstatus = sd_power_on();

    if(errorstatus == SD_OK)
    {
        errorstatus = sd_initialize_cards();              /* ��ʼ��SD����ʹ��������״̬ */
    }

    if(errorstatus == SD_OK)
    {
        errorstatus = sd_get_info(&g_sd_card_info);       /* ��ȡSD����Ϣ */
    }

    if(errorstatus == SD_OK)
    {
        errorstatus = sd_select_deselect((uint32_t)(g_sd_card_info.RCA << 16));   /* ѡ�п� */
    }

    if(errorstatus == SD_OK)
    {
        errorstatus = sd_enable_wide_bus_operation(1);    /* ����SDIO���߿��Ϊ4bit */
    }

    if((errorstatus == SD_OK) || (SDIO_MULTIMEDIA_CARD == g_sd_card_type))
    {
        /* ��������ΪV1.1��2.0 */
        if(g_sd_card_info.CardType == SDIO_STD_CAPACITY_SD_CARD_V1_1 || g_sd_card_info.CardType == SDIO_STD_CAPACITY_SD_CARD_V2_0)
        {
            clkdiv = SDIO_TRANSFER_CLK_DIV + 6;
        }
        else    /* ��������ΪSDHC������ */
        {
            clkdiv = SDIO_TRANSFER_CLK_DIV;
        }
        sd_clock_set(clkdiv);

        errorstatus = sd_set_device_mode(SD_POLLING_MODE);
    }
    return errorstatus;
}

/**
 * @brief       SDIOʱ������
 * @param       clkdiv����Ƶϵ��
 * @retval      ��
 */
void sd_clock_set(uint8_t clkdiv)
{
    uint32_t tmpreg = SDIO->CLKCR;

    tmpreg &= 0XFFFFFF00;
    tmpreg |= clkdiv;
    SDIO->CLKCR = tmpreg;
}

/**
 * @brief       SD���ϵ�
 * @param       ��
 * @retval      �������(0,�޴���)
 */
SD_Error sd_power_on(void)
{
    uint8_t i = 0;
    SD_Error errorstatus = SD_OK;
    uint32_t response = 0, count = 0, validvoltage = 0;
    uint32_t SDType = SD_STD_CAPACITY;

    /* ����SDIO */
    g_sdio_init_struct.SDIO_ClockDiv = SDIO_INIT_CLK_DIV;                           /* SDIO���ʱ�ӷ�Ƶϵ�� */
    g_sdio_init_struct.SDIO_ClockEdge = SDIO_ClockEdge_Rising;                      /* SDIOʱ�Ӽ��� */
    g_sdio_init_struct.SDIO_ClockBypass = SDIO_ClockBypass_Disable;                 /* ������·ʱ�� */
    g_sdio_init_struct.SDIO_ClockPowerSave = SDIO_ClockPowerSave_Disable;           /* ����SDIOʡ��ģʽ */
    g_sdio_init_struct.SDIO_BusWide = SDIO_BusWide_1b;                              /* SDIO���߿�� */
    g_sdio_init_struct.SDIO_HardwareFlowControl = SDIO_HardwareFlowControl_Disable; /* ����SDIOӲ�������� */
    SDIO_Init(&g_sdio_init_struct);

    SDIO_SetPowerState(SDIO_PowerState_ON);                                         /* ���ʱ�Ӹ�SD�� */

    SDIO_ClockCmd(ENABLE);                                                          /* ʹ��SDIOʱ�� */

    /* ����74��ʱ�ӣ��ȴ�SD������ڲ����� */
    for(i = 0; i < 74; i++)
    {
        /* ����CMD0�Խ�����н׶� */
        g_sdio_cmd_struct.SDIO_Argument = 0x0;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_GO_IDLE_STATE;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_No;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* �ȴ���Ӧ */
        errorstatus = cmd_error();

        if(errorstatus == SD_OK)
        {
            break;
        }
    }
    if(errorstatus)
    {
        return errorstatus;
    }

    /* ����CMD8�Լ��SD���ӿ����� */
    g_sdio_cmd_struct.SDIO_Argument = SD_CHECK_PATTERN;
    g_sdio_cmd_struct.SDIO_CmdIndex = SDIO_SEND_IF_COND;
    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&g_sdio_cmd_struct);

    /* �ȴ�R7��Ӧ */
    errorstatus = cmd_resp7_error();

    if(errorstatus == SD_OK)
    {
        g_sd_card_type = SDIO_STD_CAPACITY_SD_CARD_V2_0;
        SDType = SD_HIGH_CAPACITY;
    }

    /* ��CMD����֮ǰ����CMD55���� */
    g_sdio_cmd_struct.SDIO_Argument = 0x00;
    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_APP_CMD;
    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&g_sdio_cmd_struct);

    /* �ȴ�R1��Ӧ */
    errorstatus = cmd_resp1_error(SD_CMD_APP_CMD);

    /* SD V2.0��SD V1.1 */
    if(errorstatus == SD_OK)
    {
        /* ����CMD55 + CMD41 */
        while((!validvoltage) && (count<SD_MAX_VOLT_TRIAL))
        {
            g_sdio_cmd_struct.SDIO_Argument = 0x00;
            g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_APP_CMD;
            g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
            g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
            g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
            SDIO_SendCommand(&g_sdio_cmd_struct);

            /* �ȴ�R1��Ӧ */
            errorstatus = cmd_resp1_error(SD_CMD_APP_CMD);

            if(errorstatus != SD_OK)
            {
                return errorstatus;
            }

            /* ����CMD41���� */
            g_sdio_cmd_struct.SDIO_Argument = SD_VOLTAGE_WINDOW_SD | SDType;
            g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SD_APP_OP_COND;
            g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
            g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
            g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
            SDIO_SendCommand(&g_sdio_cmd_struct);

            /* �ȴ�R3��Ӧ */
            errorstatus = cmd_resp3_error();

            if(errorstatus != SD_OK)
            {
                return errorstatus;
            }

            /* ��ȡ��Ӧ */
            response = SDIO->RESP1;

            /* �ж��ϵ��Ƿ���� */
            validvoltage = (((response >> 31) == 1) ? 1:0);
            count++;
        }

        if(count >= SD_MAX_VOLT_TRIAL)
        {
            errorstatus = SD_INVALID_VOLTRANGE;
            return errorstatus;
        }

        if(response &= SD_HIGH_CAPACITY)
        {
            g_sd_card_type = SDIO_HIGH_CAPACITY_SD_CARD;
        }
    }
    else    /* MMC�� */
    {
        while((!validvoltage) && (count<SD_MAX_VOLT_TRIAL))
        {
            /* ����CMD1 */
            g_sdio_cmd_struct.SDIO_Argument = SD_VOLTAGE_WINDOW_MMC;
            g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SEND_OP_COND;
            g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
            g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
            g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
            SDIO_SendCommand(&g_sdio_cmd_struct);

            /* �ȴ�R3��Ӧ */
            errorstatus = cmd_resp3_error();

            if(errorstatus != SD_OK)
            {
                return errorstatus;
            }

            /* ��ȡ��Ӧ */
            response = SDIO->RESP1;

            /* �ж��ϵ��Ƿ���� */
            validvoltage = (((response >> 31) == 1) ? 1:0);
            count++;
        }
        if(count >= SD_MAX_VOLT_TRIAL)
        {
            errorstatus = SD_INVALID_VOLTRANGE;
            return errorstatus;
        }
        g_sd_card_type = SDIO_MULTIMEDIA_CARD;
    }
    return(errorstatus);
}

/**
 * @brief       �ر�SD��
 * @param       ��
 * @retval      �������(0,�޴���)
 */
SD_Error sd_power_off(void)
{
    SDIO_SetPowerState(SDIO_PowerState_OFF);  /* SDIO��Դ�ر�,ʱ��ֹͣ */
    return SD_OK;
}

/**
 * @brief       ��ʼ��SD��,ʹ��������״̬
 * @param       ��
 * @retval      �������(0,�޴���)
 */
SD_Error sd_initialize_cards(void)
{
    SD_Error errorstatus = SD_OK;
    uint16_t rca = 0x01;

    /* ���SD�����ϵ�״̬ */
    if(SDIO_GetPowerState() == 0)
    {
        return SD_REQUEST_NOT_APPLICABLE;
    }

    /* ��SDIO�� */
    if(SDIO_SECURE_DIGITAL_IO_CARD != g_sd_card_type)
    {
        /* ����CMD2 */
        g_sdio_cmd_struct.SDIO_Argument = 0x0;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_ALL_SEND_CID;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Long;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* �ȴ�R2��Ӧ */
        errorstatus = cmd_resp2_error();

        if(errorstatus != SD_OK)
        {
            return errorstatus;
        }

        /* ��ȡCID */
        g_cid_tab[0] = SDIO->RESP1;
        g_cid_tab[1] = SDIO->RESP2;
        g_cid_tab[2] = SDIO->RESP3;
        g_cid_tab[3] = SDIO->RESP4;
    }

    /* �жϿ����� */
    if((SDIO_STD_CAPACITY_SD_CARD_V1_1 == g_sd_card_type) ||(SDIO_STD_CAPACITY_SD_CARD_V2_0 == g_sd_card_type) ||
       (SDIO_SECURE_DIGITAL_IO_COMBO_CARD == g_sd_card_type) ||(SDIO_HIGH_CAPACITY_SD_CARD == g_sd_card_type))
    {
        /* ����CMD3 */
        g_sdio_cmd_struct.SDIO_Argument = 0x00;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SET_REL_ADDR;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* �ȴ�R6��Ӧ */
        errorstatus = cmd_resp6_error(SD_CMD_SET_REL_ADDR,&rca);

        if(errorstatus != SD_OK)
        {
            return errorstatus;
        }
    }

    if (SDIO_MULTIMEDIA_CARD == g_sd_card_type)
    {
        /* ����CMD3 */
        g_sdio_cmd_struct.SDIO_Argument = (uint32_t)(rca << 16);
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SET_REL_ADDR;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* �ȴ�R2��Ӧ */
        errorstatus = cmd_resp2_error();

        if(errorstatus != SD_OK)
        {
            return errorstatus;
        }
    }

    /* ��SDIO�� */
    if (SDIO_SECURE_DIGITAL_IO_CARD != g_sd_card_type)
    {
        g_rca = rca;

        /* ����CMD9 + rca */
        g_sdio_cmd_struct.SDIO_Argument = (uint32_t)(rca << 16);
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SEND_CSD;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Long;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* �ȴ�R2��Ӧ */
        errorstatus = cmd_resp2_error();

        if(errorstatus != SD_OK)
        {
            return errorstatus;
        }

        /* ��ȡCSD */
        g_csd_tab[0] = SDIO->RESP1;
        g_csd_tab[1] = SDIO->RESP2;
        g_csd_tab[2] = SDIO->RESP3;
        g_csd_tab[3] = SDIO->RESP4;
    }
    return SD_OK;
}

/**
 * @brief       ��ȡSD����Ϣ
 * @param       cardinfo: SD����Ϣ
 * @retval      �������
 */
SD_Error sd_get_info(SD_CardInfo *cardinfo)
{
    SD_Error errorstatus = SD_OK;
    uint8_t tmp = 0;

    cardinfo->CardType = (uint8_t)g_sd_card_type;      /* ������ */
    cardinfo->RCA = (uint16_t)g_rca;                   /* ��RCAֵ */
    tmp = (uint8_t)((g_csd_tab[0]&0xFF000000)>>24);
    cardinfo->SD_csd.CSDStruct = (tmp&0xC0)>>6;        /* CSD�ṹ */
    cardinfo->SD_csd.SysSpecVersion = (tmp&0x3C)>>2;   /* 2.0Э�黹û�����ⲿ�� */
    cardinfo->SD_csd.Reserved1 = tmp&0x03;             /* 2������λ */
    tmp = (uint8_t)((g_csd_tab[0]&0x00FF0000)>>16);    /* ��1���ֽ� */
    cardinfo->SD_csd.TAAC = tmp;                       /* ���ݶ�ʱ��1 */
    tmp = (uint8_t)((g_csd_tab[0]&0x0000FF00)>>8);     /* ��2���ֽ� */
    cardinfo->SD_csd.NSAC = tmp;                       /* ���ݶ�ʱ��2 */
    tmp = (uint8_t)(g_csd_tab[0]&0x000000FF);          /* ��3���ֽ� */
    cardinfo->SD_csd.MaxBusClkFrec = tmp;              /* �����ٶ� */
    tmp = (uint8_t)((g_csd_tab[1]&0xFF000000)>>24);    /* ��4���ֽ� */
    cardinfo->SD_csd.CardComdClasses = tmp<<4;         /* ��ָ�������λ */
    tmp = (uint8_t)((g_csd_tab[1]&0x00FF0000)>>16);    /* ��5���ֽ� */
    cardinfo->SD_csd.CardComdClasses |= (tmp&0xF0)>>4; /* ��ָ�������λ */
    cardinfo->SD_csd.RdBlockLen = tmp&0x0F;            /* ����ȡ���ݳ��� */
    tmp = (uint8_t)((g_csd_tab[1]&0x0000FF00)>>8);     /* ��6���ֽ� */
    cardinfo->SD_csd.PartBlockRead = (tmp&0x80)>>7;    /* ����ֿ�� */
    cardinfo->SD_csd.WrBlockMisalign = (tmp&0x40)>>6;  /* д���λ */
    cardinfo->SD_csd.RdBlockMisalign = (tmp&0x20)>>5;  /* �����λ */
    cardinfo->SD_csd.DSRImpl = (tmp&0x10)>>4;
    cardinfo->SD_csd.Reserved2 = 0;                    /* ���� */

    if((g_sd_card_type == SDIO_STD_CAPACITY_SD_CARD_V1_1) || (g_sd_card_type == SDIO_STD_CAPACITY_SD_CARD_V2_0) ||
       (SDIO_MULTIMEDIA_CARD==g_sd_card_type))         /* ��׼1.1/2.0��/MMC�� */
    {
        cardinfo->SD_csd.DeviceSize = (tmp&0x03)<<10;                       /* C_SIZE(12λ) */
        tmp = (uint8_t)(g_csd_tab[1]&0x000000FF);                           /* ��7���ֽ� */
        cardinfo->SD_csd.DeviceSize |= (tmp)<<2;
        tmp = (uint8_t)((g_csd_tab[2]&0xFF000000)>>24);                     /* ��8���ֽ� */
        cardinfo->SD_csd.DeviceSize |= (tmp&0xC0)>>6;
        cardinfo->SD_csd.MaxRdCurrentVDDMin = (tmp&0x38)>>3;
        cardinfo->SD_csd.MaxRdCurrentVDDMax = (tmp&0x07);
        tmp = (uint8_t)((g_csd_tab[2]&0x00FF0000)>>16);                     /* ��9���ֽ� */
        cardinfo->SD_csd.MaxWrCurrentVDDMin = (tmp&0xE0)>>5;
        cardinfo->SD_csd.MaxWrCurrentVDDMax = (tmp&0x1C)>>2;
        cardinfo->SD_csd.DeviceSizeMul = (tmp&0x03)<<1;
        tmp = (uint8_t)((g_csd_tab[2]&0x0000FF00)>>8);                      /* ��10���ֽ� */
        cardinfo->SD_csd.DeviceSizeMul |= (tmp&0x80)>>7;
        cardinfo->CardCapacity = (cardinfo->SD_csd.DeviceSize+1);           /* ���㿨���� */
        cardinfo->CardCapacity *= (1<<(cardinfo->SD_csd.DeviceSizeMul+2));
        cardinfo->CardBlockSize = 1<<(cardinfo->SD_csd.RdBlockLen);         /* ���С */
        cardinfo->CardCapacity *= cardinfo->CardBlockSize;
    }
    else if(g_sd_card_type == SDIO_HIGH_CAPACITY_SD_CARD)                   /* �������� */
    {
        tmp = (uint8_t)(g_csd_tab[1]&0x000000FF);
        cardinfo->SD_csd.DeviceSize = (tmp&0x3F)<<16;
        tmp = (uint8_t)((g_csd_tab[2]&0xFF000000)>>24);
        cardinfo->SD_csd.DeviceSize |= (tmp<<8);
        tmp = (uint8_t)((g_csd_tab[2]&0x00FF0000)>>16);
        cardinfo->SD_csd.DeviceSize |= (tmp);
        tmp = (uint8_t)((g_csd_tab[2]&0x0000FF00)>>8);
        cardinfo->CardCapacity = (long long)(cardinfo->SD_csd.DeviceSize+1)*512*1024;
        cardinfo->CardBlockSize = 512;                                      /* ���С�̶�Ϊ512�ֽ� */
    }
    cardinfo->SD_csd.EraseGrSize = (tmp&0x40)>>6;
    cardinfo->SD_csd.EraseGrMul = (tmp&0x3F)<<1;
    tmp = (uint8_t)(g_csd_tab[2]&0x000000FF);
    cardinfo->SD_csd.EraseGrMul |= (tmp&0x80)>>7;
    cardinfo->SD_csd.WrProtectGrSize = (tmp&0x7F);
    tmp = (uint8_t)((g_csd_tab[3]&0xFF000000)>>24);
    cardinfo->SD_csd.WrProtectGrEnable = (tmp&0x80)>>7;
    cardinfo->SD_csd.ManDeflECC = (tmp&0x60)>>5;
    cardinfo->SD_csd.WrSpeedFact = (tmp&0x1C)>>2;
    cardinfo->SD_csd.MaxWrBlockLen = (tmp&0x03)<<2;
    tmp = (uint8_t)((g_csd_tab[3]&0x00FF0000)>>16);
    cardinfo->SD_csd.MaxWrBlockLen |= (tmp&0xC0)>>6;
    cardinfo->SD_csd.WriteBlockPaPartial = (tmp&0x20)>>5;
    cardinfo->SD_csd.Reserved3 = 0;
    cardinfo->SD_csd.ContentProtectAppli = (tmp&0x01);
    tmp = (uint8_t)((g_csd_tab[3]&0x0000FF00)>>8);
    cardinfo->SD_csd.FileFormatGrouop = (tmp&0x80)>>7;
    cardinfo->SD_csd.CopyFlag = (tmp&0x40)>>6;
    cardinfo->SD_csd.PermWrProtect = (tmp&0x20)>>5;
    cardinfo->SD_csd.TempWrProtect = (tmp&0x10)>>4;
    cardinfo->SD_csd.FileFormat = (tmp&0x0C)>>2;
    cardinfo->SD_csd.ECC = (tmp&0x03);
    tmp = (uint8_t)(g_csd_tab[3]&0x000000FF);
    cardinfo->SD_csd.CSD_CRC = (tmp&0xFE)>>1;
    cardinfo->SD_csd.Reserved4 = 1;
    tmp = (uint8_t)((g_cid_tab[0]&0xFF000000)>>24);
    cardinfo->SD_cid.ManufacturerID = tmp;
    tmp = (uint8_t)((g_cid_tab[0]&0x00FF0000)>>16);
    cardinfo->SD_cid.OEM_AppliID = tmp<<8;
    tmp = (uint8_t)((g_cid_tab[0]&0x000000FF00)>>8);
    cardinfo->SD_cid.OEM_AppliID |= tmp;
    tmp = (uint8_t)(g_cid_tab[0]&0x000000FF);
    cardinfo->SD_cid.ProdName1 = tmp<<24;
    tmp = (uint8_t)((g_cid_tab[1]&0xFF000000)>>24);
    cardinfo->SD_cid.ProdName1 |= tmp<<16;
    tmp = (uint8_t)((g_cid_tab[1]&0x00FF0000)>>16);
    cardinfo->SD_cid.ProdName1 |= tmp<<8;
    tmp = (uint8_t)((g_cid_tab[1]&0x0000FF00)>>8);
    cardinfo->SD_cid.ProdName1 |= tmp;
    tmp = (uint8_t)(g_cid_tab[1]&0x000000FF);
    cardinfo->SD_cid.ProdName2 = tmp;
    tmp = (uint8_t)((g_cid_tab[2]&0xFF000000)>>24);
    cardinfo->SD_cid.ProdRev = tmp;
    tmp = (uint8_t)((g_cid_tab[2]&0x00FF0000)>>16);
    cardinfo->SD_cid.ProdSN = tmp<<24;
    tmp = (uint8_t)((g_cid_tab[2]&0x0000FF00)>>8);
    cardinfo->SD_cid.ProdSN |= tmp<<16;
    tmp = (uint8_t)(g_cid_tab[2]&0x000000FF);
    cardinfo->SD_cid.ProdSN |= tmp<<8;
    tmp = (uint8_t)((g_cid_tab[3]&0xFF000000)>>24);
    cardinfo->SD_cid.ProdSN |= tmp;
    tmp = (uint8_t)((g_cid_tab[3]&0x00FF0000)>>16);
    cardinfo->SD_cid.Reserved1 |= (tmp&0xF0)>>4;
    cardinfo->SD_cid.ManufactDate = (tmp&0x0F)<<8;
    tmp = (uint8_t)((g_cid_tab[3]&0x0000FF00)>>8);
    cardinfo->SD_cid.ManufactDate |= tmp;
    tmp = (uint8_t)(g_cid_tab[3]&0x000000FF);
    cardinfo->SD_cid.CID_CRC = (tmp&0xFE)>>1;
    cardinfo->SD_cid.Reserved2 = 1;
    return errorstatus;
}

/**
 * @brief       ����SDIO���߿��
 * @param       wmode:λ��ģʽ(0,1λ���ݿ��; 1,4λ���ݿ��; 2,8λ���ݿ��)
 * @retval      �������
 */
SD_Error sd_enable_wide_bus_operation(uint32_t wmode)
{
    SD_Error errorstatus = SD_OK;

    if(SDIO_MULTIMEDIA_CARD == g_sd_card_type)
    {
        return SD_UNSUPPORTED_FEATURE;
    }
    else if((SDIO_STD_CAPACITY_SD_CARD_V1_1 == g_sd_card_type) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == g_sd_card_type) ||
            (SDIO_HIGH_CAPACITY_SD_CARD == g_sd_card_type))
    {
        if(wmode >= 2)
        {
            return SD_UNSUPPORTED_FEATURE;
        }
        else
        {
            errorstatus = sd_en_wide_bus(wmode);

            if(SD_OK == errorstatus)
            {
                SDIO->CLKCR &= ~(3 << 11);              /* ���֮ǰ��λ������ */
                SDIO->CLKCR |= (uint16_t)wmode << 11;   /* 1λ/4λ���߿�� */
                SDIO->CLKCR |= 0 << 14;                 /* ������Ӳ�������� */
            }
        }
    }
    return errorstatus;
}

/**
 * @brief       ����SD������ģʽ
 * @param       mode:ģʽ
 * @retval      �������
 */
SD_Error sd_set_device_mode(uint32_t mode)
{
    SD_Error errorstatus = SD_OK;

    if((mode == SD_DMA_MODE) || (mode == SD_POLLING_MODE))
    {
        g_device_mode = mode;
    }
    else
    {
        errorstatus = SD_INVALID_PARAMETER;
    }
    return errorstatus;
}

/**
 * @brief       ѡ��SD��
 *  @note       ����CMD7,ѡ����Ե�ַ(rca)Ϊaddr�Ŀ�,ȡ�������������Ϊ0,�򶼲�ѡ��
 * @param       addr:����RCA��ַ
 * @retval      �������
 */
SD_Error sd_select_deselect(uint32_t addr)
{
    /* ����CMD7,ѡ��,����Ӧ */
    g_sdio_cmd_struct.SDIO_Argument = addr;
    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SEL_DESEL_CARD;
    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&g_sdio_cmd_struct);

    return cmd_resp1_error(SD_CMD_SEL_DESEL_CARD);
}

/**
 * @brief       SD����ȡһ����
 * @param       buf:�����ݻ�����(����4�ֽڶ���!!)
 * @param       addr:��ȡ��ַ
 * @param       blksize:���С
 * @retval      �������
 */
SD_Error sd_read_block(uint8_t *buf, long long addr, uint16_t blksize)
{
    SD_Error errorstatus = SD_OK;
    uint8_t power;
    uint32_t count=0, *tempbuff = (uint32_t*)buf;
    uint32_t timeout = SDIO_DATATIMEOUT;

    if(NULL == buf)
    {
        return SD_INVALID_PARAMETER;
    }

    SDIO->DCTRL = 0x0;                                  /* ���ݿ��ƼĴ�������(��DMA) */

    if(g_sd_card_type == SDIO_HIGH_CAPACITY_SD_CARD)    /* �������� */
    {
        blksize = 512;
        addr>>=9;
    }

    g_sdio_data_struct.SDIO_DataBlockSize = SDIO_DataBlockSize_1b;
    g_sdio_data_struct.SDIO_DataLength = 0;
    g_sdio_data_struct.SDIO_DataTimeOut = SD_DATATIMEOUT;
    g_sdio_data_struct.SDIO_DPSM = SDIO_DPSM_Enable;
    g_sdio_data_struct.SDIO_TransferDir = SDIO_TransferDir_ToCard;
    g_sdio_data_struct.SDIO_TransferMode = SDIO_TransferMode_Block;
    SDIO_DataConfig(&g_sdio_data_struct);

    if(SDIO->RESP1 & SD_CARD_LOCKED)                    /* ������ */
    {
        return SD_LOCK_UNLOCK_FAILED;
    }

    if((blksize>0) && (blksize <= 2048) && ((blksize&(blksize-1)) == 0))
    {
        power = convert_from_bytes_to_power_of_two(blksize);

        /* ����CMD16 */
        g_sdio_cmd_struct.SDIO_Argument = blksize;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* �ȴ�R1��Ӧ */
        errorstatus = cmd_resp1_error(SD_CMD_SET_BLOCKLEN);

        /* ��Ӧ���� */
        if(errorstatus != SD_OK)
        {
            return errorstatus;
        }
    }
    else
    {
        return SD_INVALID_PARAMETER;
    }

    /* �������ݸ�ʽ */
    g_sdio_data_struct.SDIO_DataBlockSize = power<<4;
    g_sdio_data_struct.SDIO_DataLength = blksize;
    g_sdio_data_struct.SDIO_DataTimeOut = SD_DATATIMEOUT;
    g_sdio_data_struct.SDIO_DPSM = SDIO_DPSM_Enable;
    g_sdio_data_struct.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;
    g_sdio_data_struct.SDIO_TransferMode = SDIO_TransferMode_Block;
    SDIO_DataConfig(&g_sdio_data_struct);

    /* ����CMD17 */
    g_sdio_cmd_struct.SDIO_Argument = addr;
    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_READ_SINGLE_BLOCK;
    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&g_sdio_cmd_struct);

    /* �ȴ�R1��Ӧ */
    errorstatus = cmd_resp1_error(SD_CMD_READ_SINGLE_BLOCK);

    /* ��Ӧ���� */
    if(errorstatus != SD_OK)
    {
        return errorstatus;
    }

    if(g_device_mode == SD_POLLING_MODE)
    {
        /* ������/CRC/��ʱ/���(��־)/��ʼλ���� */
        while(!(SDIO->STA & ((1<<5)|(1<<1)|(1<<3)|(1<<10)|(1<<9))))
        {
            /* ����������,��ʾ���ٴ���8���� */
            if(SDIO_GetFlagStatus(SDIO_FLAG_RXFIFOHF) != RESET)
            {
                /* ѭ����ȡ���� */
                for(count = 0; count < 8; count++)
                {
                    *(tempbuff + count) = SDIO->FIFO;
                }
                tempbuff += 8;
                timeout = 0X7FFFFF;     /* ���������ʱ�� */
            }
            else                        /* ����ʱ */
            {
                if(timeout == 0)
                {
                    return SD_DATA_TIMEOUT;
                }
                timeout--;
            }
        }
        if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)         /* ���ݳ�ʱ���� */
        {
            SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);                     /* ��������־ */
            return SD_DATA_TIMEOUT;
        }
        else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)    /* ���ݿ�CRC���� */
        {
            SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);                     /* ��������־ */
            return SD_DATA_CRC_FAIL;
        }
        else if(SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET)     /* ����fifo������� */
        {
            SDIO_ClearFlag(SDIO_FLAG_RXOVERR);                      /* ��������־ */
            return SD_RX_OVERRUN;
        }
        else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)    /* ������ʼλ���� */
        {
            SDIO_ClearFlag(SDIO_FLAG_STBITERR);                     /* ��������־ */
            return SD_START_BIT_ERR;
        }
        while(SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET)        /* FIFO����,�����ڿ������� */
        {
            *tempbuff = SDIO_ReadData();                            /* ѭ����ȡ���� */
            tempbuff++;
        }
        SDIO_ClearFlag(SDIO_STATIC_FLAGS);                          /* �����־ */
    }
    else if(g_device_mode == SD_DMA_MODE)
    {
        g_transfer_error = SD_OK;
        g_stop_condition = 0;
        g_transfer_end = 0;

        sd_dma_config((uint32_t*)buf, blksize, DMA_DIR_PeripheralSRC);

        SDIO->MASK |= (1<<1)|(1<<3)|(1<<8)|(1<<5)|(1<<9);           /* ������Ҫ���ж� */
        SDIO_DMACmd(ENABLE);                                        /* SDIO DMAʹ�� */

        while(((DMA2->INTFR&0X2000) == RESET) && (g_transfer_end == 0) && (g_transfer_error == SD_OK)&&timeout)
        {
            timeout--;   /* �ȴ�������� */
        }

        if(timeout == 0)
        {
            return SD_DATA_TIMEOUT;
        }

        if(g_transfer_error != SD_OK)
        {
            errorstatus = g_transfer_error;
        }
    }
    return errorstatus;
}

__attribute__ ((aligned(4))) uint32_t *p_tempbuff;

/**
 * @brief       SD����ȡ�����
 * @param       buf:�����ݻ�����
 * @param       addr:��ȡ��ַ
 * @param       blksize:���С
 * @param       nblks:Ҫ��ȡ�Ŀ���
 * @retval      �������
 */
SD_Error sd_read_multi_blocks(uint8_t *buf, long long addr, uint16_t blksize, uint32_t nblks)
{
    SD_Error errorstatus = SD_OK;
    uint8_t power;
    uint32_t count = 0;
    uint32_t timeout = SDIO_DATATIMEOUT;
    p_tempbuff = (uint32_t*)buf;

    SDIO->DCTRL = 0x0;                  /* ���ݿ��ƼĴ�������(��DMA) */

    if(g_sd_card_type == SDIO_HIGH_CAPACITY_SD_CARD)  /* �������� */
    {
        blksize = 512;
        addr>>=9;
    }

    g_sdio_data_struct.SDIO_DataBlockSize = 0;
    g_sdio_data_struct.SDIO_DataLength = 0;
    g_sdio_data_struct.SDIO_DataTimeOut = SD_DATATIMEOUT;
    g_sdio_data_struct.SDIO_DPSM = SDIO_DPSM_Enable;
    g_sdio_data_struct.SDIO_TransferDir = SDIO_TransferDir_ToCard;
    g_sdio_data_struct.SDIO_TransferMode = SDIO_TransferMode_Block;
    SDIO_DataConfig(&g_sdio_data_struct);
    
    if(SDIO->RESP1 & SD_CARD_LOCKED)    /* ������ */
    {
        return SD_LOCK_UNLOCK_FAILED;
    }
    if((blksize>0) && (blksize <= 2048) && ((blksize&(blksize-1)) == 0))
    {
        power = convert_from_bytes_to_power_of_two(blksize);

        /* ����CMD16+�������ݳ���Ϊblksize,����Ӧ */
        g_sdio_cmd_struct.SDIO_Argument = blksize;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        errorstatus = cmd_resp1_error(SD_CMD_SET_BLOCKLEN); /* �ȴ�R1��Ӧ */

        if(errorstatus != SD_OK)       /* ��Ӧ���� */
        {
            return errorstatus;
        }
    }
    else
    {
        return SD_INVALID_PARAMETER;
    }
    if(nblks > 1)                      /* ���� */
    {
        if(nblks * blksize > SD_MAX_DATA_LENGTH)            /* �ж��Ƿ񳬹������ճ��� */
        {
            return SD_INVALID_PARAMETER;
        }

        g_sdio_data_struct.SDIO_DataBlockSize = power<<4;
        g_sdio_data_struct.SDIO_DataLength = nblks * blksize;
        g_sdio_data_struct.SDIO_DataTimeOut = SD_DATATIMEOUT;
        g_sdio_data_struct.SDIO_DPSM = SDIO_DPSM_Enable;
        g_sdio_data_struct.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;
        g_sdio_data_struct.SDIO_TransferMode = SDIO_TransferMode_Block;
        SDIO_DataConfig(&g_sdio_data_struct);

        /* ����CMD18+��addr��ַ����ȡ����,����Ӧ */
        g_sdio_cmd_struct.SDIO_Argument = addr;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_READ_MULT_BLOCK;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* �ȴ�R1��Ӧ */
        errorstatus = cmd_resp1_error(SD_CMD_READ_MULT_BLOCK);

        /* ��Ӧ���� */
        if(errorstatus != SD_OK)
        {
            return errorstatus;
        }
        if(g_device_mode == SD_POLLING_MODE)
        {
            while(!(SDIO->STA&((1<<5)|(1<<1)|(1<<3)|(1<<8)|(1<<9))))        /* ������/CRC/��ʱ/���(��־)/��ʼλ���� */
            {
                if(SDIO_GetFlagStatus(SDIO_FLAG_RXFIFOHF) != RESET)
                {
                    for(count = 0; count < 8; count++)                      /* ѭ����ȡ���� */
                    {
                        *(p_tempbuff + count) = SDIO->FIFO;
                    }
                    p_tempbuff += 8;
                    timeout = 0X7FFFFF;                                     /* ���������ʱ�� */
                }
                else
                {
                    if(timeout == 0)
                    {
                        return SD_DATA_TIMEOUT;
                    }
                    timeout--;
                }
            }

            if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)             /* ���ݳ�ʱ���� */
            {
                SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);                         /* ��������־ */
                return SD_DATA_TIMEOUT;
            }
            else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)        /* ���ݿ�CRC���� */
            {
                SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);                         /* ��������־ */
                return SD_DATA_CRC_FAIL;
            }
            else if(SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET)         /* ����fifo������� */
            {
                SDIO_ClearFlag(SDIO_FLAG_RXOVERR);                          /* ��������־ */
                return SD_RX_OVERRUN;
            }
            else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)        /* ������ʼλ���� */
            {
                SDIO_ClearFlag(SDIO_FLAG_STBITERR);                         /* ��������־ */
                return SD_START_BIT_ERR;
            }

            while(SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET)            /* FIFO����,�����ڿ������� */
            {
                *p_tempbuff = SDIO_ReadData();                              /* ѭ����ȡ���� */
                p_tempbuff++;
            }

            if(SDIO_GetFlagStatus(SDIO_FLAG_DATAEND) != RESET)              /* ���ս��� */
            {
                if((SDIO_STD_CAPACITY_SD_CARD_V1_1 == g_sd_card_type) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == g_sd_card_type) ||
                   (SDIO_HIGH_CAPACITY_SD_CARD == g_sd_card_type))
                {
                    /* ����CMD12+�������� */
                    g_sdio_cmd_struct.SDIO_Argument = 0;
                    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_STOP_TRANSMISSION;
                    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
                    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
                    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
                    SDIO_SendCommand(&g_sdio_cmd_struct);

                    /* �ȴ�R1��Ӧ */
                    errorstatus = cmd_resp1_error(SD_CMD_STOP_TRANSMISSION);

                    if(errorstatus != SD_OK)
                    {
                        return errorstatus;
                    }
                }
            }
            SDIO_ClearFlag(SDIO_STATIC_FLAGS);                  /* ������ */
        }
        else if(g_device_mode == SD_DMA_MODE)
        {
            g_transfer_error = SD_OK;
            g_stop_condition = 1;
            g_transfer_end = 0;

            sd_dma_config((uint32_t*)buf, nblks * blksize, DMA_DIR_PeripheralSRC);

            SDIO->MASK |= (1<<1)|(1<<3)|(1<<8)|(1<<5)|(1<<9); /* ������Ҫ���ж� */
            SDIO->DCTRL |= 1<<3;                              /* SDIO DMAʹ�� */

            while(((DMA2->INTFR&0X2000) == RESET)&&timeout)   /* �ȴ�������� */
            {
                timeout--;
            }

            if(timeout == 0)
            {
                return SD_DATA_TIMEOUT;
            }

            while((g_transfer_end == 0) && (g_transfer_error == SD_OK));

            if(g_transfer_error != SD_OK)
            {
                errorstatus = g_transfer_error;
            }
        }
    }
    return errorstatus;
}

/**
 * @brief       SD��д1����
 * @param       buf:���ݻ�����
 * @param       addr:д��ַ
 * @param       blksize:���С
 * @retval      �������
 */
SD_Error sd_write_block(uint8_t *buf, long long addr, uint16_t blksize)
{
    SD_Error errorstatus = SD_OK;
    uint8_t power = 0, cardstate = 0;
    uint32_t timeout = 0, bytestransferred = 0;
    uint32_t cardstatus = 0, count = 0, restwords = 0;
    uint32_t tlen = blksize;
    uint32_t *tempbuff = (uint32_t*)buf;

    if(buf == NULL)                     /* �������� */
    {
        return SD_INVALID_PARAMETER;
    }

    SDIO->DCTRL = 0x0;                  /* ���ݿ��ƼĴ�������(��DMA) */

    g_sdio_data_struct.SDIO_DataBlockSize = 0;
    g_sdio_data_struct.SDIO_DataLength = 0;
    g_sdio_data_struct.SDIO_DataTimeOut = SD_DATATIMEOUT;
    g_sdio_data_struct.SDIO_DPSM = SDIO_DPSM_Enable;
    g_sdio_data_struct.SDIO_TransferDir = SDIO_TransferDir_ToCard;
    g_sdio_data_struct.SDIO_TransferMode = SDIO_TransferMode_Block;
    SDIO_DataConfig(&g_sdio_data_struct);

    if(SDIO->RESP1 & SD_CARD_LOCKED)    /* ������ */
    {
        return SD_LOCK_UNLOCK_FAILED;
    }

    if(g_sd_card_type == SDIO_HIGH_CAPACITY_SD_CARD)
    {
        blksize = 512;
        addr>>=9;
    }

    if((blksize>0) && (blksize <= 2048) && ((blksize&(blksize-1)) == 0))
    {
        power = convert_from_bytes_to_power_of_two(blksize);

        /* ����CMD16+�������ݳ���Ϊblksize,����Ӧ */
        g_sdio_cmd_struct.SDIO_Argument = blksize;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* �ȴ�R1��Ӧ */
        errorstatus = cmd_resp1_error(SD_CMD_SET_BLOCKLEN);

        if(errorstatus != SD_OK)    /* ��Ӧ���� */
        {
            return errorstatus;
        }
    }
    else
    {
        return SD_INVALID_PARAMETER;
    }

    /* ����CMD13,��ѯ����״̬,����Ӧ */
    g_sdio_cmd_struct.SDIO_Argument = (uint32_t)g_rca<<16;
    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SEND_STATUS;
    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&g_sdio_cmd_struct);

    /* �ȴ�R1��Ӧ */
    errorstatus = cmd_resp1_error(SD_CMD_SEND_STATUS);

    if(errorstatus != SD_OK)    /* ��Ӧ���� */
    {
        return errorstatus;
    }

    cardstatus = SDIO->RESP1;
    timeout = SD_DATATIMEOUT;

    while(((cardstatus&0x00000100) == 0) && (timeout>0))    /* ���READY_FOR_DATAλ�Ƿ���λ */
    {
        timeout--;

        /* ����CMD13,��ѯ����״̬,����Ӧ */
        g_sdio_cmd_struct.SDIO_Argument = (uint32_t)g_rca<<16;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SEND_STATUS;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* �ȴ�R1��Ӧ */
        errorstatus = cmd_resp1_error(SD_CMD_SEND_STATUS);

        /* ��Ӧ���� */
        if(errorstatus != SD_OK)
        {
            return errorstatus;
        }
        cardstatus = SDIO->RESP1;
    }

    if(timeout == 0)
    {
        return SD_ERROR;
    }

    /* ����CMD24,д����ָ��,����Ӧ */
    g_sdio_cmd_struct.SDIO_Argument = addr;
    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_WRITE_SINGLE_BLOCK;
    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&g_sdio_cmd_struct);

    /* �ȴ�R1��Ӧ */
    errorstatus = cmd_resp1_error(SD_CMD_WRITE_SINGLE_BLOCK);

    if(errorstatus != SD_OK)
    {
        return errorstatus;
    }
    g_stop_condition = 0;       /* ����д,����Ҫ����ֹͣ����ָ�� */

    g_sdio_data_struct.SDIO_DataBlockSize = power<<4;
    g_sdio_data_struct.SDIO_DataLength = blksize;
    g_sdio_data_struct.SDIO_DataTimeOut = SD_DATATIMEOUT;
    g_sdio_data_struct.SDIO_DPSM = SDIO_DPSM_Enable;
    g_sdio_data_struct.SDIO_TransferDir = SDIO_TransferDir_ToCard;
    g_sdio_data_struct.SDIO_TransferMode = SDIO_TransferMode_Block;
    SDIO_DataConfig(&g_sdio_data_struct);

    timeout = SDIO_DATATIMEOUT;

    if (g_device_mode == SD_POLLING_MODE)
    {
        while(!(SDIO->STA&((1<<10)|(1<<4)|(1<<1)|(1<<3)|(1<<9))))   /* ���ݿ鷢�ͳɹ�/����/CRC/��ʱ/��ʼλ���� */
        {
            if(SDIO_GetFlagStatus(SDIO_FLAG_TXFIFOHE) != RESET)     /* ���������,��ʾ���ٴ���8���� */
            {
                if((tlen-bytestransferred) < SD_HALFFIFOBYTES)      /* ����32�ֽ��� */
                {
                    restwords = ((tlen-bytestransferred)%4 == 0)?((tlen-bytestransferred)/4):((tlen-bytestransferred)/4+1);

                    for(count = 0; count < restwords; count++, tempbuff++, bytestransferred += 4)
                    {
                        SDIO_WriteData(*tempbuff);
                    }
                }
                else
                {
                    for(count = 0; count < 8; count++)
                    {
                        SDIO_WriteData(*(tempbuff + count));
                    }
                    tempbuff += 8;
                    bytestransferred += 32;
                }
                timeout = 0X3FFFFFFF;                               /* д�������ʱ�� */
            }
            else
            {
                if(timeout == 0)
                {
                    return SD_DATA_TIMEOUT;
                }
                timeout--;
            }
        }
        if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)         /* ���ݳ�ʱ���� */
        {
            SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);                     /* ��������־ */
            return SD_DATA_TIMEOUT;
        }
        else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)    /* ���ݿ�CRC���� */
        {
            SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);                     /* ��������־ */
            return SD_DATA_CRC_FAIL;
        }
        else if(SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) != RESET)    /* ����fifo������� */
        {
            SDIO_ClearFlag(SDIO_FLAG_TXUNDERR);                     /* ��������־ */
            return SD_TX_UNDERRUN;
        }
        else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)    /* ������ʼλ���� */
        {
            SDIO_ClearFlag(SDIO_FLAG_STBITERR);                     /* ��������־ */
            return SD_START_BIT_ERR;
        }

        SDIO->ICR = 0X5FF;
    }
    else if(g_device_mode == SD_DMA_MODE)
    {
        g_transfer_error = SD_OK;
        g_stop_condition = 0;
        g_transfer_end = 0;

        sd_dma_config((uint32_t*)buf, blksize, DMA_DIR_PeripheralDST);

        SDIO->MASK |= (1<<1)|(1<<3)|(1<<8)|(1<<4)|(1<<9);           /* ���ò������ݽ�������ж� */
        SDIO->DCTRL |= 1<<3;                                        /* SDIO DMAʹ�� */

        while(((DMA2->INTFR&0X2000) == RESET)&&timeout)             /* �ȴ�������� */
        {
            timeout--;
        }
        if(timeout == 0)
        {
            sd_init();                          /* ���³�ʼ��SD��,���Խ��д������������ */
            return SD_DATA_TIMEOUT;
        }

        timeout = SDIO_DATATIMEOUT;

        while((g_transfer_end == 0) && (g_transfer_error == SD_OK) && timeout)
        {
            timeout--;
        }

        if(timeout == 0)
        {
            return SD_DATA_TIMEOUT;
        }

        if(g_transfer_error != SD_OK)
        {
            return g_transfer_error;
        }
    }
    SDIO_ClearFlag(SDIO_STATIC_FLAGS);          /* �����־ */

    errorstatus = sd_is_card_programming(&cardstate);

    while((errorstatus == SD_OK) && ((cardstate == SD_CARD_PROGRAMMING) || (cardstate == SD_CARD_RECEIVING)))
    {
        errorstatus = sd_is_card_programming(&cardstate);
    }
    return errorstatus;
}

/**
 * @brief       SD��д�����
 * @param       buf:���ݻ�����
 * @param       addr:д��ַ
 * @param       blksize:���С
 * @param       nblks:Ҫд��Ŀ���
 * @retval      �������
 */
SD_Error sd_write_multi_blocks(uint8_t *buf, long long addr, uint16_t blksize, uint32_t nblks)
{
    SD_Error errorstatus = SD_OK;
    uint8_t  power = 0, cardstate = 0;
    uint32_t timeout = 0, bytestransferred = 0;
    uint32_t count = 0, restwords = 0;
    uint32_t tlen = nblks * blksize;
    uint32_t *tempbuff = (uint32_t*)buf;

    if(buf == NULL)           /* �������� */
    {
        return SD_INVALID_PARAMETER;
    }

    SDIO->DCTRL = 0x0;        /* ���ݿ��ƼĴ�������(��DMA) */

    g_sdio_data_struct.SDIO_DataBlockSize = 0;
    g_sdio_data_struct.SDIO_DataLength = 0;
    g_sdio_data_struct.SDIO_DataTimeOut = SD_DATATIMEOUT;
    g_sdio_data_struct.SDIO_DPSM = SDIO_DPSM_Enable;
    g_sdio_data_struct.SDIO_TransferDir = SDIO_TransferDir_ToCard;
    g_sdio_data_struct.SDIO_TransferMode = SDIO_TransferMode_Block;
    SDIO_DataConfig(&g_sdio_data_struct);

    if(SDIO->RESP1 & SD_CARD_LOCKED)      /* ������ */
    {
        return SD_LOCK_UNLOCK_FAILED;
    }

    if(g_sd_card_type == SDIO_HIGH_CAPACITY_SD_CARD)
    {
        blksize = 512;
        addr>>=9;
    }

    if((blksize>0) && (blksize<=2048) && ((blksize&(blksize-1)) == 0))
    {
        power = convert_from_bytes_to_power_of_two(blksize);

        /* ����CMD16+�������ݳ���Ϊblksize,����Ӧ */
        g_sdio_cmd_struct.SDIO_Argument = blksize;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* �ȴ�R1��Ӧ */
        errorstatus = cmd_resp1_error(SD_CMD_SET_BLOCKLEN);

        if(errorstatus != SD_OK)    /* ��Ӧ���� */
        {
            return errorstatus;
        }
    }
    else
    {
        return SD_INVALID_PARAMETER;
    }

    if(nblks>1)
    {
        if(nblks * blksize > SD_MAX_DATA_LENGTH)
        {
            return SD_INVALID_PARAMETER;
        }

        if((SDIO_STD_CAPACITY_SD_CARD_V1_1 == g_sd_card_type) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == g_sd_card_type) ||
           (SDIO_HIGH_CAPACITY_SD_CARD == g_sd_card_type))
        {
            /* ����ACMD55,����Ӧ */
            g_sdio_cmd_struct.SDIO_Argument = (uint32_t)g_rca<<16;
            g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_APP_CMD;
            g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
            g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
            g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
            SDIO_SendCommand(&g_sdio_cmd_struct);

            /* �ȴ�R1��Ӧ */
            errorstatus = cmd_resp1_error(SD_CMD_APP_CMD);

            if(errorstatus != SD_OK)
            {
                return errorstatus;
            }

            /* ����CMD23,���ÿ�����,����Ӧ */
            g_sdio_cmd_struct.SDIO_Argument = nblks;
            g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SET_BLOCK_COUNT;
            g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
            g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
            g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
            SDIO_SendCommand(&g_sdio_cmd_struct);

            /* �ȴ�R1��Ӧ */
            errorstatus = cmd_resp1_error(SD_CMD_SET_BLOCK_COUNT);

            if(errorstatus != SD_OK)
            {
                return errorstatus;
            }
        }

        /* ����CMD25,���дָ��,����Ӧ */
        g_sdio_cmd_struct.SDIO_Argument = addr;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_WRITE_MULT_BLOCK;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* �ȴ�R1��Ӧ */
        errorstatus = cmd_resp1_error(SD_CMD_WRITE_MULT_BLOCK);

        if(errorstatus != SD_OK)
        {
            return errorstatus;
        }

        g_sdio_data_struct.SDIO_DataBlockSize = power<<4;
        g_sdio_data_struct.SDIO_DataLength = nblks * blksize;
        g_sdio_data_struct.SDIO_DataTimeOut = SD_DATATIMEOUT;
        g_sdio_data_struct.SDIO_DPSM = SDIO_DPSM_Enable;
        g_sdio_data_struct.SDIO_TransferDir = SDIO_TransferDir_ToCard;
        g_sdio_data_struct.SDIO_TransferMode = SDIO_TransferMode_Block;
        SDIO_DataConfig(&g_sdio_data_struct);

        if(g_device_mode == SD_POLLING_MODE)
        {
            timeout = SDIO_DATATIMEOUT;

            while(!(SDIO->STA & ((1<<4)|(1<<1)|(1<<8)|(1<<3)|(1<<9))))    /* ����/CRC/���ݽ���/��ʱ/��ʼλ���� */
            {
                if(SDIO->STA & (1<<14))
                {
                    if((tlen-bytestransferred) < SD_HALFFIFOBYTES)        /* ����32�ֽ��� */
                    {
                        restwords = ((tlen-bytestransferred)%4 == 0)?((tlen-bytestransferred)/4):((tlen-bytestransferred)/4+1);

                        for(count = 0; count < restwords; count++, tempbuff++, bytestransferred += 4)
                        {
                            SDIO_WriteData(*tempbuff);
                        }
                    }
                    else    /* ���������,���Է�������8��(32�ֽ�)���� */
                    {
                        for(count = 0; count < SD_HALFFIFO; count++)
                        {
                            SDIO_WriteData(*(tempbuff + count));
                        }
                        tempbuff += SD_HALFFIFO;
                        bytestransferred += SD_HALFFIFOBYTES;
                    }
                    timeout = 0X3FFFFFFF;     /* д�������ʱ�� */
                }
                else
                {
                    if(timeout == 0)
                    {
                        return SD_DATA_TIMEOUT;
                    }
                    timeout--;
                }
            }
            if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)         /* ���ݳ�ʱ���� */
            {
                SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);                     /* ��������־ */
                return SD_DATA_TIMEOUT;
            }
            else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)    /* ���ݿ�CRC���� */
            {
                SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);                     /* ��������־ */
                return SD_DATA_CRC_FAIL;
            }
            else if(SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) != RESET)    /* ����fifo������� */
            {
                SDIO_ClearFlag(SDIO_FLAG_TXUNDERR);                     /* ��������־ */
                return SD_TX_UNDERRUN;
            }
            else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)    /* ������ʼλ���� */
            {
                SDIO_ClearFlag(SDIO_FLAG_STBITERR);                     /* ��������־ */
                return SD_START_BIT_ERR;
            }
            if(SDIO_GetFlagStatus(SDIO_FLAG_DATAEND) != RESET)          /* ���ͽ��� */
            {
                if((SDIO_STD_CAPACITY_SD_CARD_V1_1 == g_sd_card_type) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == g_sd_card_type) ||
                   (SDIO_HIGH_CAPACITY_SD_CARD == g_sd_card_type))
                {
                    /* ����CMD12+�������� */
                    g_sdio_cmd_struct.SDIO_Argument = 0;
                    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_STOP_TRANSMISSION;
                    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
                    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
                    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
                    SDIO_SendCommand(&g_sdio_cmd_struct);

                    /* �ȴ�R1��Ӧ */
                    errorstatus = cmd_resp1_error(SD_CMD_STOP_TRANSMISSION);

                    if(errorstatus != SD_OK)
                    {
                        return errorstatus;
                    }
                }
            }
            SDIO_ClearFlag(SDIO_STATIC_FLAGS);      /* ������ */
        }
        else if(g_device_mode == SD_DMA_MODE)
        {
            g_transfer_error = SD_OK;
            g_stop_condition = 1;
            g_transfer_end = 0;

            sd_dma_config((uint32_t*)buf, nblks * blksize, DMA_DIR_PeripheralDST);

            SDIO->MASK |= (1<<1)|(1<<3)|(1<<8)|(1<<4)|(1<<9);     /* ���ò������ݽ�������ж� */
            SDIO->DCTRL |= 1<<3;                                  /* SDIO DMAʹ�� */

            timeout = SDIO_DATATIMEOUT;

            while(((DMA2->INTFR&0X2000) == RESET) && timeout)     /* �ȴ�������� */
            {
                timeout--;
            }

            if(timeout == 0)
            {
                sd_init();                  /* ���³�ʼ��SD��,���Խ��д������������ */
                return SD_DATA_TIMEOUT;
            }
            timeout = SDIO_DATATIMEOUT;

            while((g_transfer_end == 0) && (g_transfer_error == SD_OK) && timeout)
            {
                timeout--;
            }

            if(timeout == 0)
            {
                return SD_DATA_TIMEOUT;
            }

            if(g_transfer_error != SD_OK)
            {
                return g_transfer_error;
            }
        }
    }
    SDIO_ClearFlag(SDIO_STATIC_FLAGS);                          /* ������ */

    errorstatus = sd_is_card_programming(&cardstate);

    while((errorstatus == SD_OK) && ((cardstate == SD_CARD_PROGRAMMING) || (cardstate == SD_CARD_RECEIVING)))
    {
        errorstatus = sd_is_card_programming(&cardstate);
    }
    return errorstatus;
}

/**
 * @brief       SDIO�жϷ�����
 * @param       ��
 * @retval      ��
 */
void SDIO_IRQHandler(void)
{
    sd_process_irq_src();
}

/**
 * @brief       SDIO�жϴ�����
 *  @note       ����SDIO��������еĸ����ж�����
 * @param       ��
 * @retval      �������
 */
SD_Error sd_process_irq_src(void)
{
    if(SDIO->STA & (1<<8))
    {
        if (g_stop_condition == 1)
        {
            /* ����CMD12+�������� */
            g_sdio_cmd_struct.SDIO_Argument = 0;
            g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_STOP_TRANSMISSION;
            g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
            g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
            g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
            SDIO_SendCommand(&g_sdio_cmd_struct);

            g_transfer_error = cmd_resp1_error(SD_CMD_STOP_TRANSMISSION);
        }
        else
        {
            g_transfer_error = SD_OK;
        }

        SDIO->ICR |= 1<<8;                                  /* �������жϱ�� */
        SDIO->MASK &= ~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));     /* �ر�����ж� */

        g_transfer_end = 1;

        return(g_transfer_error);
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)     /* ����CRC���� */
    {
        SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);                 /* ��������־ */

        SDIO->MASK &= ~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));     /* �ر�����ж� */

        g_transfer_error = SD_DATA_CRC_FAIL;

        return(SD_DATA_CRC_FAIL);
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)     /* ���ݳ�ʱ���� */
    {
        SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);                 /* ��������־ */

        SDIO->MASK &= ~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));     /* �ر�����ж� */

        g_transfer_error = SD_DATA_TIMEOUT;

        return(SD_DATA_TIMEOUT);
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET)      /* FIFO������� */
    {
        SDIO_ClearFlag(SDIO_FLAG_RXOVERR);                  /* ��������־ */

        SDIO->MASK &= ~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));     /* �ر�����ж� */

        g_transfer_error = SD_RX_OVERRUN;

        return(SD_RX_OVERRUN);
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) != RESET)     /* FIFO������� */
    {
        SDIO_ClearFlag(SDIO_FLAG_TXUNDERR);                 /* ��������־ */

        SDIO->MASK &= ~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));     /* �ر�����ж� */

        g_transfer_error = SD_TX_UNDERRUN;

        return(SD_TX_UNDERRUN);
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)     /* ��ʼλ���� */
    {
        SDIO_ClearFlag(SDIO_FLAG_STBITERR);                 /* ��������־ */

        SDIO->MASK &= ~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));     /* �ر�����ж� */

        g_transfer_error = SD_START_BIT_ERR;

        return(SD_START_BIT_ERR);
    }
    return(SD_OK);
}

/**
 * @brief       ��ȡ��ǰ��״̬
 * @param       pcardstatus:��״̬
 * @retval      �������
 */
SD_Error sd_send_status(uint32_t *pcardstatus)
{
    SD_Error errorstatus = SD_OK;

    if(pcardstatus == NULL)
    {
        errorstatus = SD_INVALID_PARAMETER;
        return errorstatus;
    }

    /* ����CMD13,����Ӧ */
    g_sdio_cmd_struct.SDIO_Argument = (uint32_t)g_rca << 16;
    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SEND_STATUS;
    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&g_sdio_cmd_struct);

    /* ��ѯ��Ӧ״̬ */
    errorstatus = cmd_resp1_error(SD_CMD_SEND_STATUS);

    if(errorstatus != SD_OK)
    {
        return errorstatus;
    }

    *pcardstatus = SDIO->RESP1;     /* ��ȡ��Ӧֵ */

    return errorstatus;
}

/**
 * @brief       ��ȡSD����״̬
 * @param       ��
 * @retval      �������
 */
SDCardState sd_get_state(void)
{
    uint32_t resp1 = 0;

    if(sd_send_status(&resp1) != SD_OK)
    {
        return SD_CARD_ERROR;
    }
    else
    {
        return (SDCardState)((resp1>>9) & 0x0F);
    }
}

/**
 * @brief       ����SDIO DMA
 * @param       mbuf:�洢����ַ
 * @param       bufsize:����������
 * @param       dir:����
 * @retval      ��
 */
void sd_dma_config(uint32_t *mbuf, uint32_t bufsize, uint32_t dir)
{
    DMA_InitTypeDef dma_init_struct;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);

    DMA_DeInit(DMA2_Channel4);
    DMA_Cmd(DMA2_Channel4, DISABLE );

    dma_init_struct.DMA_PeripheralBaseAddr = (uint32_t)&SDIO->FIFO;           /* DMA�����ַ */
    dma_init_struct.DMA_MemoryBaseAddr = (uint32_t)mbuf;                      /* �洢����ַ */
    dma_init_struct.DMA_DIR = dir;                                            /* ���䷽�� */
    dma_init_struct.DMA_BufferSize = bufsize/4;                               /* ���ݴ����� */
    dma_init_struct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;            /* ���������ģʽ */
    dma_init_struct.DMA_MemoryInc = DMA_MemoryInc_Enable;                     /* �洢������ģʽ */
    dma_init_struct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;     /* �������ݳ���:32λ */
    dma_init_struct.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;             /* �洢�����ݳ���:32λ */
    dma_init_struct.DMA_Mode = DMA_Mode_Normal;                               /* ʹ����ͨģʽ */
    dma_init_struct.DMA_Priority = DMA_Priority_High;                         /* �����ȼ� */
    dma_init_struct.DMA_M2M = DMA_M2M_Disable;                                /* �ر��ڴ浽�ڴ�ģʽ */
    DMA_Init(DMA2_Channel4, &dma_init_struct);                                /* ��ʼ��DMA */

    DMA_Cmd(DMA2_Channel4, DISABLE );                                         /* ����DMA���� */
}

/**
 * @brief       ��SD��
 * @param       pbuf  : ���ݻ�����
 * @param       saddr : ������ַ
 * @param       cnt   : ��������
 * @retval      0,����; ����,�������(���SD_Error����);
 */
uint8_t sd_read_disk(uint8_t*buf, uint32_t sector, uint8_t cnt)
{
    uint8_t sta = SD_OK;
    long long lsector = sector;
    uint8_t n;

    lsector<<=9;

    if((uint32_t)buf % 4 != 0)
    {
        for(n = 0; n < cnt; n++)
        {
            sta = sd_read_block(SDIO_DATA_BUFFER, lsector + 512 * n, 512);  /* ����sector�Ķ����� */
            memcpy(buf, SDIO_DATA_BUFFER, 512);
            buf += 512;
        }
    }
    else
    {
        if(cnt == 1)                                                        /* ����sector�Ķ����� */
        {
            sta = sd_read_block(buf, lsector, 512);
        }
        else                                                                /* ���sector�Ķ����� */
        {
            sta = sd_read_multi_blocks(buf, lsector, 512, cnt);
        }
    }
    return sta;
}

/**
 * @brief       дSD��
 * @param       pbuf  : ���ݻ�����
 * @param       saddr : ������ַ
 * @param       cnt   : ��������
 * @retval      0,����;����,�������(���SD_Error����);
 */
uint8_t sd_write_disk(uint8_t *buf, uint32_t sector, uint8_t cnt)
{
    uint8_t sta = SD_OK;
    uint8_t n;
    long long lsector = sector;

    lsector<<=9;

    if((uint32_t)buf % 4 != 0)
    {
        for(n = 0; n < cnt; n++)
        {
            memcpy(SDIO_DATA_BUFFER, buf, 512);
            sta = sd_write_block(SDIO_DATA_BUFFER, lsector + 512 * n, 512);     /* ����sector��д���� */
            buf += 512;
        }
    }
    else
    {
        if(cnt == 1)
        {
            sta = sd_write_block(buf, lsector, 512);                            /* ����sector��д���� */
        }
        else
        {
            sta = sd_write_multi_blocks(buf, lsector, 512,cnt);                 /* ���sector��д���� */
        }
    }
    return sta;
}

