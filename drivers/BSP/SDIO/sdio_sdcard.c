/**
 ****************************************************************************************************
 * @file        sdio_sdcard.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-07-20
 * @brief       SD卡 驱动代码
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
 * @brief       检查CMD0的执行状态
 * @param       无
 * @retval      错误代码
 */
static SD_Error cmd_error(void)
{
    SD_Error errorstatus = SD_OK;
    uint32_t timeout = SDIO_CMD0TIMEOUT;

    while(timeout--)
    {
        if(SDIO_GetFlagStatus(SDIO_FLAG_CMDSENT) != RESET)    /* 命令已发送(无需响应) */
        {
          break;
        }
    }

    if(timeout == 0)                            /* 超时 */
    {
        return SD_CMD_RSP_TIMEOUT;
    }

    SDIO_ClearFlag(SDIO_STATIC_FLAGS);          /* 清除标志 */

    return errorstatus;
}

/**
 * @brief       检查R7响应的错误状态
 * @param       无
 * @retval      错误代码
 */
static SD_Error cmd_resp7_error(void)
{
    SD_Error errorstatus = SD_OK;
    uint32_t status = 0;
    uint32_t timeout = SDIO_CMD0TIMEOUT;

    while(timeout--)
    {
        status = SDIO->STA;
        if(status&((1<<0)|(1<<2)|(1<<6)))       /* CRC错误/命令响应超时/已经收到响应(CRC校验成功) */
        {
            break;
        }
    }

    /* 响应超时 */
    if((timeout == 0) || (status&(1<<2)))
    {
        errorstatus = SD_CMD_RSP_TIMEOUT;       /* 当前卡不是2.0兼容卡,或者不支持设定的电压范围 */
        SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);     /* 清除命令响应超时标志 */
        return errorstatus;
    }

    /* 成功接收到响应 */
    if((status) & ( 1<<6 ))
    {
        errorstatus = SD_OK;                    /* 记录状态 */
        SDIO_ClearFlag(SDIO_FLAG_CMDREND);      /* 清除标志 */
    }
    return errorstatus;
}

/**
 * @brief       检查R1响应的错误状态
 * @param       cmd: 命令
 * @retval      错误代码
 */
static SD_Error cmd_resp1_error(uint8_t cmd)
{
    uint32_t status;

    while(1)
    {
        status = SDIO->STA;
        if(status&((1<<0)|(1<<2)|(1<<6)))               /* CRC错误/命令响应超时/已经收到响应(CRC校验成功) */
        {
            break;
        }
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_CTIMEOUT) != RESET) /* 响应超时 */
    {
        SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);             /* 清除命令响应超时标志 */
        return SD_CMD_RSP_TIMEOUT;                      /* 返回错误类型 */
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_CCRCFAIL) != RESET) /* CRC错误 */
    {
        SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);             /* 清除标志 */
        return SD_CMD_CRC_FAIL;                         /* 返回错误类型 */
    }

    if(SDIO->RESPCMD != cmd)                            /* 非法命令 */
    {
        return SD_ILLEGAL_CMD;
    }

    SDIO->ICR = 0X5FF;                                  /* 清除所有标志 */
    return (SD_Error)(SDIO->RESP1&SD_OCR_ERRORBITS);    /* 返回卡响应 */
}

/**
 * @brief       检查R3响应的错误状态
 * @param       无
 * @retval      错误代码
 */
static SD_Error cmd_resp3_error(void)
{
    uint32_t status;

    while(1)
    {
        status = SDIO->STA;
        if(status&((1<<0)|(1<<2)|(1<<6)))               /* CRC错误/命令响应超时/已经收到响应(CRC校验成功) */
        {
            break;
        }
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_CTIMEOUT) != RESET) /* 响应超时 */
    {
        SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);             /* 清除命令响应超时标志 */
        return SD_CMD_RSP_TIMEOUT;                      /* 返回错误类型 */
    }

   SDIO_ClearFlag(SDIO_STATIC_FLAGS);                   /* 清除标志 */
   return SD_OK;                                        /* 返回SD卡状态 */
}

/**
 * @brief       检查R2响应的错误状态
 * @param       无
 * @retval      错误代码
 */
static SD_Error cmd_resp2_error(void)
{
    SD_Error errorstatus = SD_OK;
    uint32_t status = 0;
    uint32_t timeout = SDIO_CMD0TIMEOUT;

    while(timeout--)
    {
        status = SDIO->STA;
        if(status&((1<<0)|(1<<2)|(1<<6)))               /* CRC错误/命令响应超时/已经收到响应(CRC校验成功) */
        {
            break;
        }
    }

    if((timeout == 0)||(status&(1<<2)))                 /* 响应超时 */
    {
        errorstatus = SD_CMD_RSP_TIMEOUT;
        SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);             /* 清除命令响应超时标志 */
        return errorstatus;                             /* 返回错误类型 */
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_CCRCFAIL) != RESET) /* CRC错误 */
    {
        errorstatus = SD_CMD_CRC_FAIL;
        SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);             /* 清除响应标志 */
    }

    SDIO_ClearFlag(SDIO_STATIC_FLAGS);                  /* 清除标志 */
    return errorstatus;
}

/**
 * @brief       检查R6响应的错误状态
 * @param       无
 * @retval      错误代码
 */
static SD_Error cmd_resp6_error(uint8_t cmd,uint16_t *prca)
{
    SD_Error errorstatus = SD_OK;
    uint32_t status;
    uint32_t rspr1;

    while(1)
    {
        status = SDIO->STA;
        if(status & ((1<<0)|(1<<2)|(1<<6)))               /* CRC错误/命令响应超时/已经收到响应(CRC校验成功) */
        {
            break;
        }
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_CTIMEOUT) != RESET)   /* 响应超时 */
    {
        SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);               /* 清除命令响应超时标志 */
        return SD_CMD_RSP_TIMEOUT;
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_CCRCFAIL) != RESET)   /* CRC错误 */
    {
        SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);               /* 清除响应标志 */
        return SD_CMD_CRC_FAIL;
    }

    if(SDIO->RESPCMD != cmd)
    {
        return SD_ILLEGAL_CMD;
    }

    SDIO_ClearFlag(SDIO_STATIC_FLAGS);                    /* 清除标志 */

    rspr1 = SDIO->RESP1;                                  /* 得到响应 */

    if(SD_ALLZERO == (rspr1 & (SD_R6_GENERAL_UNKNOWN_ERROR|SD_R6_ILLEGAL_CMD|SD_R6_COM_CRC_FAILED)))
    {
        *prca = (uint16_t)(rspr1 >> 16);                  /* 右移16位得到,rca */
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
 * @brief       查找SD卡的SCR寄存器值
 * @param       rca:卡相对地址
 * @param       pscr:数据缓存区(存储SCR内容)
 * @retval      错误代码
 */
static SD_Error sd_find_scr(uint16_t rca,uint32_t *pscr)
{
    uint32_t index = 0;
    SD_Error errorstatus = SD_OK;
    uint32_t tempscr[2] = {0, 0};

    /* 发送CMD16命令并设置块大小为8字节 */
    g_sdio_cmd_struct.SDIO_Argument = (uint32_t)8;
    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&g_sdio_cmd_struct);

    /* 等待R1响应 */
    errorstatus = cmd_resp1_error(SD_CMD_SET_BLOCKLEN);

    if(errorstatus != SD_OK)
    {
        return errorstatus;
    }

    /* 发送CMD55 + RCA */
    g_sdio_cmd_struct.SDIO_Argument = (uint32_t)g_rca << 16;
    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_APP_CMD;
    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&g_sdio_cmd_struct);

    /* 等待R1响应 */
    errorstatus = cmd_resp1_error(SD_CMD_APP_CMD);

    if(errorstatus != SD_OK)
    {
        return errorstatus;
    }

    /* 配置数据格式 */
    g_sdio_data_struct.SDIO_DataTimeOut = SD_DATATIMEOUT;
    g_sdio_data_struct.SDIO_DataLength = 8;
    g_sdio_data_struct.SDIO_DataBlockSize = SDIO_DataBlockSize_8b;
    g_sdio_data_struct.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;
    g_sdio_data_struct.SDIO_TransferMode = SDIO_TransferMode_Block;
    g_sdio_data_struct.SDIO_DPSM = SDIO_DPSM_Enable;
    SDIO_DataConfig(&g_sdio_data_struct);

    /* 发送ACMD51命令 */
    g_sdio_cmd_struct.SDIO_Argument = 0x0;
    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SD_APP_SEND_SCR;
    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&g_sdio_cmd_struct);

    /* 等待R1响应 */
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

    /* 清除所有标志 */
    SDIO_ClearFlag(SDIO_STATIC_FLAGS);

    *(pscr+1) = ((tempscr[0] & SD_0TO7BITS) << 24) | ((tempscr[0] & SD_8TO15BITS) <<8 )|
                ((tempscr[0] & SD_16TO23BITS) >> 8) | ((tempscr[0] & SD_24TO31BITS) >> 24);

    *(pscr) = ((tempscr[1] & SD_0TO7BITS) << 24) | ((tempscr[1] & SD_8TO15BITS) << 8)|
              ((tempscr[1] & SD_16TO23BITS) >> 8) | ((tempscr[1] & SD_24TO31BITS) >> 24);

    return errorstatus;
}

/**
 * @brief       使能SDIO宽总线模式
 * @param       enx:0,不使能;1,使能
 * @retval      错误代码
 */
static SD_Error sd_en_wide_bus(uint8_t enx)
{
    SD_Error errorstatus = SD_OK;
    uint32_t scr[2] = {0, 0};
    uint8_t arg = 0x00;

    if(enx)
    {
        arg = 0x02;   /* 4bit总线宽度 */
    }
    else
    {
        arg = 0x00;   /* 1bit总线宽度 */
    }

    if(SDIO->RESP1 & SD_CARD_LOCKED)
    {
        return SD_LOCK_UNLOCK_FAILED;
    }

    /* 获取SCR寄存器值 */
    errorstatus = sd_find_scr(g_rca, scr);

    if(errorstatus != SD_OK)
    {
        return errorstatus;
    }

    /* SD卡支持总线宽度模式 */
    if((scr[1] & SD_WIDE_BUS_SUPPORT) != SD_ALLZERO)
    {
        /* 发送CMD55 + RCA */
        g_sdio_cmd_struct.SDIO_Argument = (uint32_t)g_rca << 16;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_APP_CMD;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* 等待R1响应 */
        errorstatus = cmd_resp1_error(SD_CMD_APP_CMD);

        if(errorstatus != SD_OK)
        {
            return errorstatus;
        }

        /* 发送ACMD6命令 */
        g_sdio_cmd_struct.SDIO_Argument = arg;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_APP_SD_SET_BUSWIDTH;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* 等待R1响应 */
        errorstatus = cmd_resp1_error(SD_CMD_APP_SD_SET_BUSWIDTH);
        return errorstatus;
    }
    else
    {
        return SD_REQUEST_NOT_APPLICABLE;
    }
}

/**
 * @brief       检查SD卡是否正在执行写操作
 * @param       pstatus: 卡的当前状态
 * @retval      错误代码
 */
static SD_Error sd_is_card_programming(uint8_t *pstatus)
{
    volatile uint32_t respR1 = 0, status = 0;

    /* 发送CMD13 + RCA */
    g_sdio_cmd_struct.SDIO_Argument = (uint32_t)g_rca << 16;
    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SEND_STATUS;
    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&g_sdio_cmd_struct);

    /* 获取SDIO标志 */
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
 * @brief       得到bytes以2为底的指数
 * @param       bytes:字节数
 * @retval      以2为底的指数值
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
 * @brief       初始化SD卡
 * @param       无
 * @retval      错误代码
 */
SD_Error sd_init(void)
{
    NVIC_InitTypeDef nvic_init_struct;
    GPIO_InitTypeDef gpio_init_struct;

    uint8_t clkdiv = 0;
    SD_Error errorstatus = SD_OK;

    /* 使能时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_SDIO|RCC_AHBPeriph_DMA2, ENABLE);

    /* 配置SDIO相关引脚 */
    gpio_init_struct.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12;
    gpio_init_struct.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &gpio_init_struct);

    gpio_init_struct.GPIO_Pin = GPIO_Pin_2;
    gpio_init_struct.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &gpio_init_struct);

    /* 复位SDIO */
    SDIO_DeInit();

    /* SDIO中断配置 */
    nvic_init_struct.NVIC_IRQChannel = SDIO_IRQn;
    nvic_init_struct.NVIC_IRQChannelPreemptionPriority = 0;
    nvic_init_struct.NVIC_IRQChannelSubPriority = 0;
    nvic_init_struct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init_struct);

    /* 上电 */
    errorstatus = sd_power_on();

    if(errorstatus == SD_OK)
    {
        errorstatus = sd_initialize_cards();              /* 初始化SD卡，使其进入就绪状态 */
    }

    if(errorstatus == SD_OK)
    {
        errorstatus = sd_get_info(&g_sd_card_info);       /* 获取SD卡信息 */
    }

    if(errorstatus == SD_OK)
    {
        errorstatus = sd_select_deselect((uint32_t)(g_sd_card_info.RCA << 16));   /* 选中卡 */
    }

    if(errorstatus == SD_OK)
    {
        errorstatus = sd_enable_wide_bus_operation(1);    /* 设置SDIO总线宽度为4bit */
    }

    if((errorstatus == SD_OK) || (SDIO_MULTIMEDIA_CARD == g_sd_card_type))
    {
        /* 卡的类型为V1.1或2.0 */
        if(g_sd_card_info.CardType == SDIO_STD_CAPACITY_SD_CARD_V1_1 || g_sd_card_info.CardType == SDIO_STD_CAPACITY_SD_CARD_V2_0)
        {
            clkdiv = SDIO_TRANSFER_CLK_DIV + 6;
        }
        else    /* 卡的类型为SDHC或其他 */
        {
            clkdiv = SDIO_TRANSFER_CLK_DIV;
        }
        sd_clock_set(clkdiv);

        errorstatus = sd_set_device_mode(SD_POLLING_MODE);
    }
    return errorstatus;
}

/**
 * @brief       SDIO时钟设置
 * @param       clkdiv：分频系数
 * @retval      无
 */
void sd_clock_set(uint8_t clkdiv)
{
    uint32_t tmpreg = SDIO->CLKCR;

    tmpreg &= 0XFFFFFF00;
    tmpreg |= clkdiv;
    SDIO->CLKCR = tmpreg;
}

/**
 * @brief       SD卡上电
 * @param       无
 * @retval      错误代码(0,无错误)
 */
SD_Error sd_power_on(void)
{
    uint8_t i = 0;
    SD_Error errorstatus = SD_OK;
    uint32_t response = 0, count = 0, validvoltage = 0;
    uint32_t SDType = SD_STD_CAPACITY;

    /* 配置SDIO */
    g_sdio_init_struct.SDIO_ClockDiv = SDIO_INIT_CLK_DIV;                           /* SDIO输出时钟分频系数 */
    g_sdio_init_struct.SDIO_ClockEdge = SDIO_ClockEdge_Rising;                      /* SDIO时钟极性 */
    g_sdio_init_struct.SDIO_ClockBypass = SDIO_ClockBypass_Disable;                 /* 禁用旁路时钟 */
    g_sdio_init_struct.SDIO_ClockPowerSave = SDIO_ClockPowerSave_Disable;           /* 禁用SDIO省电模式 */
    g_sdio_init_struct.SDIO_BusWide = SDIO_BusWide_1b;                              /* SDIO总线宽度 */
    g_sdio_init_struct.SDIO_HardwareFlowControl = SDIO_HardwareFlowControl_Disable; /* 禁用SDIO硬件流控制 */
    SDIO_Init(&g_sdio_init_struct);

    SDIO_SetPowerState(SDIO_PowerState_ON);                                         /* 输出时钟给SD卡 */

    SDIO_ClockCmd(ENABLE);                                                          /* 使能SDIO时钟 */

    /* 发送74个时钟，等待SD卡完成内部操作 */
    for(i = 0; i < 74; i++)
    {
        /* 发送CMD0以进入空闲阶段 */
        g_sdio_cmd_struct.SDIO_Argument = 0x0;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_GO_IDLE_STATE;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_No;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* 等待响应 */
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

    /* 发送CMD8以检查SD卡接口特性 */
    g_sdio_cmd_struct.SDIO_Argument = SD_CHECK_PATTERN;
    g_sdio_cmd_struct.SDIO_CmdIndex = SDIO_SEND_IF_COND;
    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&g_sdio_cmd_struct);

    /* 等待R7响应 */
    errorstatus = cmd_resp7_error();

    if(errorstatus == SD_OK)
    {
        g_sd_card_type = SDIO_STD_CAPACITY_SD_CARD_V2_0;
        SDType = SD_HIGH_CAPACITY;
    }

    /* 在CMD命令之前发送CMD55命令 */
    g_sdio_cmd_struct.SDIO_Argument = 0x00;
    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_APP_CMD;
    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&g_sdio_cmd_struct);

    /* 等待R1响应 */
    errorstatus = cmd_resp1_error(SD_CMD_APP_CMD);

    /* SD V2.0或SD V1.1 */
    if(errorstatus == SD_OK)
    {
        /* 发送CMD55 + CMD41 */
        while((!validvoltage) && (count<SD_MAX_VOLT_TRIAL))
        {
            g_sdio_cmd_struct.SDIO_Argument = 0x00;
            g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_APP_CMD;
            g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
            g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
            g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
            SDIO_SendCommand(&g_sdio_cmd_struct);

            /* 等待R1响应 */
            errorstatus = cmd_resp1_error(SD_CMD_APP_CMD);

            if(errorstatus != SD_OK)
            {
                return errorstatus;
            }

            /* 发送CMD41命令 */
            g_sdio_cmd_struct.SDIO_Argument = SD_VOLTAGE_WINDOW_SD | SDType;
            g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SD_APP_OP_COND;
            g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
            g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
            g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
            SDIO_SendCommand(&g_sdio_cmd_struct);

            /* 等待R3响应 */
            errorstatus = cmd_resp3_error();

            if(errorstatus != SD_OK)
            {
                return errorstatus;
            }

            /* 获取响应 */
            response = SDIO->RESP1;

            /* 判断上电是否完成 */
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
    else    /* MMC卡 */
    {
        while((!validvoltage) && (count<SD_MAX_VOLT_TRIAL))
        {
            /* 发送CMD1 */
            g_sdio_cmd_struct.SDIO_Argument = SD_VOLTAGE_WINDOW_MMC;
            g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SEND_OP_COND;
            g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
            g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
            g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
            SDIO_SendCommand(&g_sdio_cmd_struct);

            /* 等待R3响应 */
            errorstatus = cmd_resp3_error();

            if(errorstatus != SD_OK)
            {
                return errorstatus;
            }

            /* 获取响应 */
            response = SDIO->RESP1;

            /* 判断上电是否完成 */
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
 * @brief       关闭SD卡
 * @param       无
 * @retval      错误代码(0,无错误)
 */
SD_Error sd_power_off(void)
{
    SDIO_SetPowerState(SDIO_PowerState_OFF);  /* SDIO电源关闭,时钟停止 */
    return SD_OK;
}

/**
 * @brief       初始化SD卡,使其进入就绪状态
 * @param       无
 * @retval      错误代码(0,无错误)
 */
SD_Error sd_initialize_cards(void)
{
    SD_Error errorstatus = SD_OK;
    uint16_t rca = 0x01;

    /* 检查SD卡的上电状态 */
    if(SDIO_GetPowerState() == 0)
    {
        return SD_REQUEST_NOT_APPLICABLE;
    }

    /* 非SDIO卡 */
    if(SDIO_SECURE_DIGITAL_IO_CARD != g_sd_card_type)
    {
        /* 发送CMD2 */
        g_sdio_cmd_struct.SDIO_Argument = 0x0;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_ALL_SEND_CID;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Long;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* 等待R2响应 */
        errorstatus = cmd_resp2_error();

        if(errorstatus != SD_OK)
        {
            return errorstatus;
        }

        /* 获取CID */
        g_cid_tab[0] = SDIO->RESP1;
        g_cid_tab[1] = SDIO->RESP2;
        g_cid_tab[2] = SDIO->RESP3;
        g_cid_tab[3] = SDIO->RESP4;
    }

    /* 判断卡类型 */
    if((SDIO_STD_CAPACITY_SD_CARD_V1_1 == g_sd_card_type) ||(SDIO_STD_CAPACITY_SD_CARD_V2_0 == g_sd_card_type) ||
       (SDIO_SECURE_DIGITAL_IO_COMBO_CARD == g_sd_card_type) ||(SDIO_HIGH_CAPACITY_SD_CARD == g_sd_card_type))
    {
        /* 发送CMD3 */
        g_sdio_cmd_struct.SDIO_Argument = 0x00;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SET_REL_ADDR;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* 等待R6响应 */
        errorstatus = cmd_resp6_error(SD_CMD_SET_REL_ADDR,&rca);

        if(errorstatus != SD_OK)
        {
            return errorstatus;
        }
    }

    if (SDIO_MULTIMEDIA_CARD == g_sd_card_type)
    {
        /* 发送CMD3 */
        g_sdio_cmd_struct.SDIO_Argument = (uint32_t)(rca << 16);
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SET_REL_ADDR;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* 等待R2响应 */
        errorstatus = cmd_resp2_error();

        if(errorstatus != SD_OK)
        {
            return errorstatus;
        }
    }

    /* 非SDIO卡 */
    if (SDIO_SECURE_DIGITAL_IO_CARD != g_sd_card_type)
    {
        g_rca = rca;

        /* 发送CMD9 + rca */
        g_sdio_cmd_struct.SDIO_Argument = (uint32_t)(rca << 16);
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SEND_CSD;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Long;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* 等待R2响应 */
        errorstatus = cmd_resp2_error();

        if(errorstatus != SD_OK)
        {
            return errorstatus;
        }

        /* 获取CSD */
        g_csd_tab[0] = SDIO->RESP1;
        g_csd_tab[1] = SDIO->RESP2;
        g_csd_tab[2] = SDIO->RESP3;
        g_csd_tab[3] = SDIO->RESP4;
    }
    return SD_OK;
}

/**
 * @brief       获取SD卡信息
 * @param       cardinfo: SD卡信息
 * @retval      错误代码
 */
SD_Error sd_get_info(SD_CardInfo *cardinfo)
{
    SD_Error errorstatus = SD_OK;
    uint8_t tmp = 0;

    cardinfo->CardType = (uint8_t)g_sd_card_type;      /* 卡类型 */
    cardinfo->RCA = (uint16_t)g_rca;                   /* 卡RCA值 */
    tmp = (uint8_t)((g_csd_tab[0]&0xFF000000)>>24);
    cardinfo->SD_csd.CSDStruct = (tmp&0xC0)>>6;        /* CSD结构 */
    cardinfo->SD_csd.SysSpecVersion = (tmp&0x3C)>>2;   /* 2.0协议还没定义这部分 */
    cardinfo->SD_csd.Reserved1 = tmp&0x03;             /* 2个保留位 */
    tmp = (uint8_t)((g_csd_tab[0]&0x00FF0000)>>16);    /* 第1个字节 */
    cardinfo->SD_csd.TAAC = tmp;                       /* 数据读时间1 */
    tmp = (uint8_t)((g_csd_tab[0]&0x0000FF00)>>8);     /* 第2个字节 */
    cardinfo->SD_csd.NSAC = tmp;                       /* 数据读时间2 */
    tmp = (uint8_t)(g_csd_tab[0]&0x000000FF);          /* 第3个字节 */
    cardinfo->SD_csd.MaxBusClkFrec = tmp;              /* 传输速度 */
    tmp = (uint8_t)((g_csd_tab[1]&0xFF000000)>>24);    /* 第4个字节 */
    cardinfo->SD_csd.CardComdClasses = tmp<<4;         /* 卡指令类高四位 */
    tmp = (uint8_t)((g_csd_tab[1]&0x00FF0000)>>16);    /* 第5个字节 */
    cardinfo->SD_csd.CardComdClasses |= (tmp&0xF0)>>4; /* 卡指令类低四位 */
    cardinfo->SD_csd.RdBlockLen = tmp&0x0F;            /* 最大读取数据长度 */
    tmp = (uint8_t)((g_csd_tab[1]&0x0000FF00)>>8);     /* 第6个字节 */
    cardinfo->SD_csd.PartBlockRead = (tmp&0x80)>>7;    /* 允许分块读 */
    cardinfo->SD_csd.WrBlockMisalign = (tmp&0x40)>>6;  /* 写块错位 */
    cardinfo->SD_csd.RdBlockMisalign = (tmp&0x20)>>5;  /* 读块错位 */
    cardinfo->SD_csd.DSRImpl = (tmp&0x10)>>4;
    cardinfo->SD_csd.Reserved2 = 0;                    /* 保留 */

    if((g_sd_card_type == SDIO_STD_CAPACITY_SD_CARD_V1_1) || (g_sd_card_type == SDIO_STD_CAPACITY_SD_CARD_V2_0) ||
       (SDIO_MULTIMEDIA_CARD==g_sd_card_type))         /* 标准1.1/2.0卡/MMC卡 */
    {
        cardinfo->SD_csd.DeviceSize = (tmp&0x03)<<10;                       /* C_SIZE(12位) */
        tmp = (uint8_t)(g_csd_tab[1]&0x000000FF);                           /* 第7个字节 */
        cardinfo->SD_csd.DeviceSize |= (tmp)<<2;
        tmp = (uint8_t)((g_csd_tab[2]&0xFF000000)>>24);                     /* 第8个字节 */
        cardinfo->SD_csd.DeviceSize |= (tmp&0xC0)>>6;
        cardinfo->SD_csd.MaxRdCurrentVDDMin = (tmp&0x38)>>3;
        cardinfo->SD_csd.MaxRdCurrentVDDMax = (tmp&0x07);
        tmp = (uint8_t)((g_csd_tab[2]&0x00FF0000)>>16);                     /* 第9个字节 */
        cardinfo->SD_csd.MaxWrCurrentVDDMin = (tmp&0xE0)>>5;
        cardinfo->SD_csd.MaxWrCurrentVDDMax = (tmp&0x1C)>>2;
        cardinfo->SD_csd.DeviceSizeMul = (tmp&0x03)<<1;
        tmp = (uint8_t)((g_csd_tab[2]&0x0000FF00)>>8);                      /* 第10个字节 */
        cardinfo->SD_csd.DeviceSizeMul |= (tmp&0x80)>>7;
        cardinfo->CardCapacity = (cardinfo->SD_csd.DeviceSize+1);           /* 计算卡容量 */
        cardinfo->CardCapacity *= (1<<(cardinfo->SD_csd.DeviceSizeMul+2));
        cardinfo->CardBlockSize = 1<<(cardinfo->SD_csd.RdBlockLen);         /* 块大小 */
        cardinfo->CardCapacity *= cardinfo->CardBlockSize;
    }
    else if(g_sd_card_type == SDIO_HIGH_CAPACITY_SD_CARD)                   /* 高容量卡 */
    {
        tmp = (uint8_t)(g_csd_tab[1]&0x000000FF);
        cardinfo->SD_csd.DeviceSize = (tmp&0x3F)<<16;
        tmp = (uint8_t)((g_csd_tab[2]&0xFF000000)>>24);
        cardinfo->SD_csd.DeviceSize |= (tmp<<8);
        tmp = (uint8_t)((g_csd_tab[2]&0x00FF0000)>>16);
        cardinfo->SD_csd.DeviceSize |= (tmp);
        tmp = (uint8_t)((g_csd_tab[2]&0x0000FF00)>>8);
        cardinfo->CardCapacity = (long long)(cardinfo->SD_csd.DeviceSize+1)*512*1024;
        cardinfo->CardBlockSize = 512;                                      /* 块大小固定为512字节 */
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
 * @brief       设置SDIO总线宽度
 * @param       wmode:位宽模式(0,1位数据宽度; 1,4位数据宽度; 2,8位数据宽度)
 * @retval      错误代码
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
                SDIO->CLKCR &= ~(3 << 11);              /* 清除之前的位宽设置 */
                SDIO->CLKCR |= (uint16_t)wmode << 11;   /* 1位/4位总线宽度 */
                SDIO->CLKCR |= 0 << 14;                 /* 不开启硬件流控制 */
            }
        }
    }
    return errorstatus;
}

/**
 * @brief       设置SD卡工作模式
 * @param       mode:模式
 * @retval      错误代码
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
 * @brief       选择SD卡
 *  @note       发送CMD7,选择相对地址(rca)为addr的卡,取消其他卡。如果为0,则都不选择。
 * @param       addr:卡的RCA地址
 * @retval      错误代码
 */
SD_Error sd_select_deselect(uint32_t addr)
{
    /* 发送CMD7,选择卡,短响应 */
    g_sdio_cmd_struct.SDIO_Argument = addr;
    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SEL_DESEL_CARD;
    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&g_sdio_cmd_struct);

    return cmd_resp1_error(SD_CMD_SEL_DESEL_CARD);
}

/**
 * @brief       SD卡读取一个块
 * @param       buf:读数据缓存区(必须4字节对齐!!)
 * @param       addr:读取地址
 * @param       blksize:块大小
 * @retval      错误代码
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

    SDIO->DCTRL = 0x0;                                  /* 数据控制寄存器清零(关DMA) */

    if(g_sd_card_type == SDIO_HIGH_CAPACITY_SD_CARD)    /* 大容量卡 */
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

    if(SDIO->RESP1 & SD_CARD_LOCKED)                    /* 卡锁了 */
    {
        return SD_LOCK_UNLOCK_FAILED;
    }

    if((blksize>0) && (blksize <= 2048) && ((blksize&(blksize-1)) == 0))
    {
        power = convert_from_bytes_to_power_of_two(blksize);

        /* 发送CMD16 */
        g_sdio_cmd_struct.SDIO_Argument = blksize;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* 等待R1响应 */
        errorstatus = cmd_resp1_error(SD_CMD_SET_BLOCKLEN);

        /* 响应错误 */
        if(errorstatus != SD_OK)
        {
            return errorstatus;
        }
    }
    else
    {
        return SD_INVALID_PARAMETER;
    }

    /* 配置数据格式 */
    g_sdio_data_struct.SDIO_DataBlockSize = power<<4;
    g_sdio_data_struct.SDIO_DataLength = blksize;
    g_sdio_data_struct.SDIO_DataTimeOut = SD_DATATIMEOUT;
    g_sdio_data_struct.SDIO_DPSM = SDIO_DPSM_Enable;
    g_sdio_data_struct.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;
    g_sdio_data_struct.SDIO_TransferMode = SDIO_TransferMode_Block;
    SDIO_DataConfig(&g_sdio_data_struct);

    /* 发送CMD17 */
    g_sdio_cmd_struct.SDIO_Argument = addr;
    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_READ_SINGLE_BLOCK;
    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&g_sdio_cmd_struct);

    /* 等待R1响应 */
    errorstatus = cmd_resp1_error(SD_CMD_READ_SINGLE_BLOCK);

    /* 响应错误 */
    if(errorstatus != SD_OK)
    {
        return errorstatus;
    }

    if(g_device_mode == SD_POLLING_MODE)
    {
        /* 无上溢/CRC/超时/完成(标志)/起始位错误 */
        while(!(SDIO->STA & ((1<<5)|(1<<1)|(1<<3)|(1<<10)|(1<<9))))
        {
            /* 接收区半满,表示至少存了8个字 */
            if(SDIO_GetFlagStatus(SDIO_FLAG_RXFIFOHF) != RESET)
            {
                /* 循环读取数据 */
                for(count = 0; count < 8; count++)
                {
                    *(tempbuff + count) = SDIO->FIFO;
                }
                tempbuff += 8;
                timeout = 0X7FFFFF;     /* 读数据溢出时间 */
            }
            else                        /* 处理超时 */
            {
                if(timeout == 0)
                {
                    return SD_DATA_TIMEOUT;
                }
                timeout--;
            }
        }
        if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)         /* 数据超时错误 */
        {
            SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);                     /* 清除错误标志 */
            return SD_DATA_TIMEOUT;
        }
        else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)    /* 数据块CRC错误 */
        {
            SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);                     /* 清除错误标志 */
            return SD_DATA_CRC_FAIL;
        }
        else if(SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET)     /* 接收fifo上溢错误 */
        {
            SDIO_ClearFlag(SDIO_FLAG_RXOVERR);                      /* 清除错误标志 */
            return SD_RX_OVERRUN;
        }
        else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)    /* 接收起始位错误 */
        {
            SDIO_ClearFlag(SDIO_FLAG_STBITERR);                     /* 清除错误标志 */
            return SD_START_BIT_ERR;
        }
        while(SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET)        /* FIFO里面,还存在可用数据 */
        {
            *tempbuff = SDIO_ReadData();                            /* 循环读取数据 */
            tempbuff++;
        }
        SDIO_ClearFlag(SDIO_STATIC_FLAGS);                          /* 清除标志 */
    }
    else if(g_device_mode == SD_DMA_MODE)
    {
        g_transfer_error = SD_OK;
        g_stop_condition = 0;
        g_transfer_end = 0;

        sd_dma_config((uint32_t*)buf, blksize, DMA_DIR_PeripheralSRC);

        SDIO->MASK |= (1<<1)|(1<<3)|(1<<8)|(1<<5)|(1<<9);           /* 配置需要的中断 */
        SDIO_DMACmd(ENABLE);                                        /* SDIO DMA使能 */

        while(((DMA2->INTFR&0X2000) == RESET) && (g_transfer_end == 0) && (g_transfer_error == SD_OK)&&timeout)
        {
            timeout--;   /* 等待传输完成 */
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
 * @brief       SD卡读取多个块
 * @param       buf:读数据缓存区
 * @param       addr:读取地址
 * @param       blksize:块大小
 * @param       nblks:要读取的块数
 * @retval      错误代码
 */
SD_Error sd_read_multi_blocks(uint8_t *buf, long long addr, uint16_t blksize, uint32_t nblks)
{
    SD_Error errorstatus = SD_OK;
    uint8_t power;
    uint32_t count = 0;
    uint32_t timeout = SDIO_DATATIMEOUT;
    p_tempbuff = (uint32_t*)buf;

    SDIO->DCTRL = 0x0;                  /* 数据控制寄存器清零(关DMA) */

    if(g_sd_card_type == SDIO_HIGH_CAPACITY_SD_CARD)  /* 大容量卡 */
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
    
    if(SDIO->RESP1 & SD_CARD_LOCKED)    /* 卡锁了 */
    {
        return SD_LOCK_UNLOCK_FAILED;
    }
    if((blksize>0) && (blksize <= 2048) && ((blksize&(blksize-1)) == 0))
    {
        power = convert_from_bytes_to_power_of_two(blksize);

        /* 发送CMD16+设置数据长度为blksize,短响应 */
        g_sdio_cmd_struct.SDIO_Argument = blksize;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        errorstatus = cmd_resp1_error(SD_CMD_SET_BLOCKLEN); /* 等待R1响应 */

        if(errorstatus != SD_OK)       /* 响应错误 */
        {
            return errorstatus;
        }
    }
    else
    {
        return SD_INVALID_PARAMETER;
    }
    if(nblks > 1)                      /* 多块读 */
    {
        if(nblks * blksize > SD_MAX_DATA_LENGTH)            /* 判断是否超过最大接收长度 */
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

        /* 发送CMD18+从addr地址出读取数据,短响应 */
        g_sdio_cmd_struct.SDIO_Argument = addr;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_READ_MULT_BLOCK;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* 等待R1响应 */
        errorstatus = cmd_resp1_error(SD_CMD_READ_MULT_BLOCK);

        /* 响应错误 */
        if(errorstatus != SD_OK)
        {
            return errorstatus;
        }
        if(g_device_mode == SD_POLLING_MODE)
        {
            while(!(SDIO->STA&((1<<5)|(1<<1)|(1<<3)|(1<<8)|(1<<9))))        /* 无上溢/CRC/超时/完成(标志)/起始位错误 */
            {
                if(SDIO_GetFlagStatus(SDIO_FLAG_RXFIFOHF) != RESET)
                {
                    for(count = 0; count < 8; count++)                      /* 循环读取数据 */
                    {
                        *(p_tempbuff + count) = SDIO->FIFO;
                    }
                    p_tempbuff += 8;
                    timeout = 0X7FFFFF;                                     /* 读数据溢出时间 */
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

            if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)             /* 数据超时错误 */
            {
                SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);                         /* 清除错误标志 */
                return SD_DATA_TIMEOUT;
            }
            else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)        /* 数据块CRC错误 */
            {
                SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);                         /* 清除错误标志 */
                return SD_DATA_CRC_FAIL;
            }
            else if(SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET)         /* 接收fifo上溢错误 */
            {
                SDIO_ClearFlag(SDIO_FLAG_RXOVERR);                          /* 清除错误标志 */
                return SD_RX_OVERRUN;
            }
            else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)        /* 接收起始位错误 */
            {
                SDIO_ClearFlag(SDIO_FLAG_STBITERR);                         /* 清除错误标志 */
                return SD_START_BIT_ERR;
            }

            while(SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET)            /* FIFO里面,还存在可用数据 */
            {
                *p_tempbuff = SDIO_ReadData();                              /* 循环读取数据 */
                p_tempbuff++;
            }

            if(SDIO_GetFlagStatus(SDIO_FLAG_DATAEND) != RESET)              /* 接收结束 */
            {
                if((SDIO_STD_CAPACITY_SD_CARD_V1_1 == g_sd_card_type) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == g_sd_card_type) ||
                   (SDIO_HIGH_CAPACITY_SD_CARD == g_sd_card_type))
                {
                    /* 发送CMD12+结束传输 */
                    g_sdio_cmd_struct.SDIO_Argument = 0;
                    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_STOP_TRANSMISSION;
                    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
                    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
                    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
                    SDIO_SendCommand(&g_sdio_cmd_struct);

                    /* 等待R1响应 */
                    errorstatus = cmd_resp1_error(SD_CMD_STOP_TRANSMISSION);

                    if(errorstatus != SD_OK)
                    {
                        return errorstatus;
                    }
                }
            }
            SDIO_ClearFlag(SDIO_STATIC_FLAGS);                  /* 清除标记 */
        }
        else if(g_device_mode == SD_DMA_MODE)
        {
            g_transfer_error = SD_OK;
            g_stop_condition = 1;
            g_transfer_end = 0;

            sd_dma_config((uint32_t*)buf, nblks * blksize, DMA_DIR_PeripheralSRC);

            SDIO->MASK |= (1<<1)|(1<<3)|(1<<8)|(1<<5)|(1<<9); /* 配置需要的中断 */
            SDIO->DCTRL |= 1<<3;                              /* SDIO DMA使能 */

            while(((DMA2->INTFR&0X2000) == RESET)&&timeout)   /* 等待传输完成 */
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
 * @brief       SD卡写1个块
 * @param       buf:数据缓存区
 * @param       addr:写地址
 * @param       blksize:块大小
 * @retval      错误代码
 */
SD_Error sd_write_block(uint8_t *buf, long long addr, uint16_t blksize)
{
    SD_Error errorstatus = SD_OK;
    uint8_t power = 0, cardstate = 0;
    uint32_t timeout = 0, bytestransferred = 0;
    uint32_t cardstatus = 0, count = 0, restwords = 0;
    uint32_t tlen = blksize;
    uint32_t *tempbuff = (uint32_t*)buf;

    if(buf == NULL)                     /* 参数错误 */
    {
        return SD_INVALID_PARAMETER;
    }

    SDIO->DCTRL = 0x0;                  /* 数据控制寄存器清零(关DMA) */

    g_sdio_data_struct.SDIO_DataBlockSize = 0;
    g_sdio_data_struct.SDIO_DataLength = 0;
    g_sdio_data_struct.SDIO_DataTimeOut = SD_DATATIMEOUT;
    g_sdio_data_struct.SDIO_DPSM = SDIO_DPSM_Enable;
    g_sdio_data_struct.SDIO_TransferDir = SDIO_TransferDir_ToCard;
    g_sdio_data_struct.SDIO_TransferMode = SDIO_TransferMode_Block;
    SDIO_DataConfig(&g_sdio_data_struct);

    if(SDIO->RESP1 & SD_CARD_LOCKED)    /* 卡锁了 */
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

        /* 发送CMD16+设置数据长度为blksize,短响应 */
        g_sdio_cmd_struct.SDIO_Argument = blksize;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* 等待R1响应 */
        errorstatus = cmd_resp1_error(SD_CMD_SET_BLOCKLEN);

        if(errorstatus != SD_OK)    /* 响应错误 */
        {
            return errorstatus;
        }
    }
    else
    {
        return SD_INVALID_PARAMETER;
    }

    /* 发送CMD13,查询卡的状态,短响应 */
    g_sdio_cmd_struct.SDIO_Argument = (uint32_t)g_rca<<16;
    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SEND_STATUS;
    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&g_sdio_cmd_struct);

    /* 等待R1响应 */
    errorstatus = cmd_resp1_error(SD_CMD_SEND_STATUS);

    if(errorstatus != SD_OK)    /* 响应错误 */
    {
        return errorstatus;
    }

    cardstatus = SDIO->RESP1;
    timeout = SD_DATATIMEOUT;

    while(((cardstatus&0x00000100) == 0) && (timeout>0))    /* 检查READY_FOR_DATA位是否置位 */
    {
        timeout--;

        /* 发送CMD13,查询卡的状态,短响应 */
        g_sdio_cmd_struct.SDIO_Argument = (uint32_t)g_rca<<16;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SEND_STATUS;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* 等待R1响应 */
        errorstatus = cmd_resp1_error(SD_CMD_SEND_STATUS);

        /* 响应错误 */
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

    /* 发送CMD24,写单块指令,短响应 */
    g_sdio_cmd_struct.SDIO_Argument = addr;
    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_WRITE_SINGLE_BLOCK;
    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&g_sdio_cmd_struct);

    /* 等待R1响应 */
    errorstatus = cmd_resp1_error(SD_CMD_WRITE_SINGLE_BLOCK);

    if(errorstatus != SD_OK)
    {
        return errorstatus;
    }
    g_stop_condition = 0;       /* 单块写,不需要发送停止传输指令 */

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
        while(!(SDIO->STA&((1<<10)|(1<<4)|(1<<1)|(1<<3)|(1<<9))))   /* 数据块发送成功/下溢/CRC/超时/起始位错误 */
        {
            if(SDIO_GetFlagStatus(SDIO_FLAG_TXFIFOHE) != RESET)     /* 发送区半空,表示至少存了8个字 */
            {
                if((tlen-bytestransferred) < SD_HALFFIFOBYTES)      /* 不够32字节了 */
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
                timeout = 0X3FFFFFFF;                               /* 写数据溢出时间 */
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
        if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)         /* 数据超时错误 */
        {
            SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);                     /* 清除错误标志 */
            return SD_DATA_TIMEOUT;
        }
        else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)    /* 数据块CRC错误 */
        {
            SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);                     /* 清除错误标志 */
            return SD_DATA_CRC_FAIL;
        }
        else if(SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) != RESET)    /* 接收fifo下溢错误 */
        {
            SDIO_ClearFlag(SDIO_FLAG_TXUNDERR);                     /* 清除错误标志 */
            return SD_TX_UNDERRUN;
        }
        else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)    /* 接收起始位错误 */
        {
            SDIO_ClearFlag(SDIO_FLAG_STBITERR);                     /* 清除错误标志 */
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

        SDIO->MASK |= (1<<1)|(1<<3)|(1<<8)|(1<<4)|(1<<9);           /* 配置产生数据接收完成中断 */
        SDIO->DCTRL |= 1<<3;                                        /* SDIO DMA使能 */

        while(((DMA2->INTFR&0X2000) == RESET)&&timeout)             /* 等待传输完成 */
        {
            timeout--;
        }
        if(timeout == 0)
        {
            sd_init();                          /* 重新初始化SD卡,可以解决写入死机的问题 */
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
    SDIO_ClearFlag(SDIO_STATIC_FLAGS);          /* 清除标志 */

    errorstatus = sd_is_card_programming(&cardstate);

    while((errorstatus == SD_OK) && ((cardstate == SD_CARD_PROGRAMMING) || (cardstate == SD_CARD_RECEIVING)))
    {
        errorstatus = sd_is_card_programming(&cardstate);
    }
    return errorstatus;
}

/**
 * @brief       SD卡写多个块
 * @param       buf:数据缓存区
 * @param       addr:写地址
 * @param       blksize:块大小
 * @param       nblks:要写入的块数
 * @retval      错误代码
 */
SD_Error sd_write_multi_blocks(uint8_t *buf, long long addr, uint16_t blksize, uint32_t nblks)
{
    SD_Error errorstatus = SD_OK;
    uint8_t  power = 0, cardstate = 0;
    uint32_t timeout = 0, bytestransferred = 0;
    uint32_t count = 0, restwords = 0;
    uint32_t tlen = nblks * blksize;
    uint32_t *tempbuff = (uint32_t*)buf;

    if(buf == NULL)           /* 参数错误 */
    {
        return SD_INVALID_PARAMETER;
    }

    SDIO->DCTRL = 0x0;        /* 数据控制寄存器清零(关DMA) */

    g_sdio_data_struct.SDIO_DataBlockSize = 0;
    g_sdio_data_struct.SDIO_DataLength = 0;
    g_sdio_data_struct.SDIO_DataTimeOut = SD_DATATIMEOUT;
    g_sdio_data_struct.SDIO_DPSM = SDIO_DPSM_Enable;
    g_sdio_data_struct.SDIO_TransferDir = SDIO_TransferDir_ToCard;
    g_sdio_data_struct.SDIO_TransferMode = SDIO_TransferMode_Block;
    SDIO_DataConfig(&g_sdio_data_struct);

    if(SDIO->RESP1 & SD_CARD_LOCKED)      /* 卡锁了 */
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

        /* 发送CMD16+设置数据长度为blksize,短响应 */
        g_sdio_cmd_struct.SDIO_Argument = blksize;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* 等待R1响应 */
        errorstatus = cmd_resp1_error(SD_CMD_SET_BLOCKLEN);

        if(errorstatus != SD_OK)    /* 响应错误 */
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
            /* 发送ACMD55,短响应 */
            g_sdio_cmd_struct.SDIO_Argument = (uint32_t)g_rca<<16;
            g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_APP_CMD;
            g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
            g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
            g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
            SDIO_SendCommand(&g_sdio_cmd_struct);

            /* 等待R1响应 */
            errorstatus = cmd_resp1_error(SD_CMD_APP_CMD);

            if(errorstatus != SD_OK)
            {
                return errorstatus;
            }

            /* 发送CMD23,设置块数量,短响应 */
            g_sdio_cmd_struct.SDIO_Argument = nblks;
            g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SET_BLOCK_COUNT;
            g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
            g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
            g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
            SDIO_SendCommand(&g_sdio_cmd_struct);

            /* 等待R1响应 */
            errorstatus = cmd_resp1_error(SD_CMD_SET_BLOCK_COUNT);

            if(errorstatus != SD_OK)
            {
                return errorstatus;
            }
        }

        /* 发送CMD25,多块写指令,短响应 */
        g_sdio_cmd_struct.SDIO_Argument = addr;
        g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_WRITE_MULT_BLOCK;
        g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
        g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
        g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
        SDIO_SendCommand(&g_sdio_cmd_struct);

        /* 等待R1响应 */
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

            while(!(SDIO->STA & ((1<<4)|(1<<1)|(1<<8)|(1<<3)|(1<<9))))    /* 下溢/CRC/数据结束/超时/起始位错误 */
            {
                if(SDIO->STA & (1<<14))
                {
                    if((tlen-bytestransferred) < SD_HALFFIFOBYTES)        /* 不够32字节了 */
                    {
                        restwords = ((tlen-bytestransferred)%4 == 0)?((tlen-bytestransferred)/4):((tlen-bytestransferred)/4+1);

                        for(count = 0; count < restwords; count++, tempbuff++, bytestransferred += 4)
                        {
                            SDIO_WriteData(*tempbuff);
                        }
                    }
                    else    /* 发送区半空,可以发送至少8字(32字节)数据 */
                    {
                        for(count = 0; count < SD_HALFFIFO; count++)
                        {
                            SDIO_WriteData(*(tempbuff + count));
                        }
                        tempbuff += SD_HALFFIFO;
                        bytestransferred += SD_HALFFIFOBYTES;
                    }
                    timeout = 0X3FFFFFFF;     /* 写数据溢出时间 */
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
            if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)         /* 数据超时错误 */
            {
                SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);                     /* 清除错误标志 */
                return SD_DATA_TIMEOUT;
            }
            else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)    /* 数据块CRC错误 */
            {
                SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);                     /* 清除错误标志 */
                return SD_DATA_CRC_FAIL;
            }
            else if(SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) != RESET)    /* 接收fifo下溢错误 */
            {
                SDIO_ClearFlag(SDIO_FLAG_TXUNDERR);                     /* 清除错误标志 */
                return SD_TX_UNDERRUN;
            }
            else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)    /* 接收起始位错误 */
            {
                SDIO_ClearFlag(SDIO_FLAG_STBITERR);                     /* 清除错误标志 */
                return SD_START_BIT_ERR;
            }
            if(SDIO_GetFlagStatus(SDIO_FLAG_DATAEND) != RESET)          /* 发送结束 */
            {
                if((SDIO_STD_CAPACITY_SD_CARD_V1_1 == g_sd_card_type) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == g_sd_card_type) ||
                   (SDIO_HIGH_CAPACITY_SD_CARD == g_sd_card_type))
                {
                    /* 发送CMD12+结束传输 */
                    g_sdio_cmd_struct.SDIO_Argument = 0;
                    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_STOP_TRANSMISSION;
                    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
                    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
                    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
                    SDIO_SendCommand(&g_sdio_cmd_struct);

                    /* 等待R1响应 */
                    errorstatus = cmd_resp1_error(SD_CMD_STOP_TRANSMISSION);

                    if(errorstatus != SD_OK)
                    {
                        return errorstatus;
                    }
                }
            }
            SDIO_ClearFlag(SDIO_STATIC_FLAGS);      /* 清除标记 */
        }
        else if(g_device_mode == SD_DMA_MODE)
        {
            g_transfer_error = SD_OK;
            g_stop_condition = 1;
            g_transfer_end = 0;

            sd_dma_config((uint32_t*)buf, nblks * blksize, DMA_DIR_PeripheralDST);

            SDIO->MASK |= (1<<1)|(1<<3)|(1<<8)|(1<<4)|(1<<9);     /* 配置产生数据接收完成中断 */
            SDIO->DCTRL |= 1<<3;                                  /* SDIO DMA使能 */

            timeout = SDIO_DATATIMEOUT;

            while(((DMA2->INTFR&0X2000) == RESET) && timeout)     /* 等待传输完成 */
            {
                timeout--;
            }

            if(timeout == 0)
            {
                sd_init();                  /* 重新初始化SD卡,可以解决写入死机的问题 */
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
    SDIO_ClearFlag(SDIO_STATIC_FLAGS);                          /* 清除标记 */

    errorstatus = sd_is_card_programming(&cardstate);

    while((errorstatus == SD_OK) && ((cardstate == SD_CARD_PROGRAMMING) || (cardstate == SD_CARD_RECEIVING)))
    {
        errorstatus = sd_is_card_programming(&cardstate);
    }
    return errorstatus;
}

/**
 * @brief       SDIO中断服务函数
 * @param       无
 * @retval      无
 */
void SDIO_IRQHandler(void)
{
    sd_process_irq_src();
}

/**
 * @brief       SDIO中断处理函数
 *  @note       处理SDIO传输过程中的各种中断事务
 * @param       无
 * @retval      错误代码
 */
SD_Error sd_process_irq_src(void)
{
    if(SDIO->STA & (1<<8))
    {
        if (g_stop_condition == 1)
        {
            /* 发送CMD12+结束传输 */
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

        SDIO->ICR |= 1<<8;                                  /* 清除完成中断标记 */
        SDIO->MASK &= ~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));     /* 关闭相关中断 */

        g_transfer_end = 1;

        return(g_transfer_error);
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)     /* 数据CRC错误 */
    {
        SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);                 /* 清除错误标志 */

        SDIO->MASK &= ~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));     /* 关闭相关中断 */

        g_transfer_error = SD_DATA_CRC_FAIL;

        return(SD_DATA_CRC_FAIL);
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)     /* 数据超时错误 */
    {
        SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);                 /* 清除错误标志 */

        SDIO->MASK &= ~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));     /* 关闭相关中断 */

        g_transfer_error = SD_DATA_TIMEOUT;

        return(SD_DATA_TIMEOUT);
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET)      /* FIFO上溢错误 */
    {
        SDIO_ClearFlag(SDIO_FLAG_RXOVERR);                  /* 清除错误标志 */

        SDIO->MASK &= ~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));     /* 关闭相关中断 */

        g_transfer_error = SD_RX_OVERRUN;

        return(SD_RX_OVERRUN);
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) != RESET)     /* FIFO下溢错误 */
    {
        SDIO_ClearFlag(SDIO_FLAG_TXUNDERR);                 /* 清除错误标志 */

        SDIO->MASK &= ~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));     /* 关闭相关中断 */

        g_transfer_error = SD_TX_UNDERRUN;

        return(SD_TX_UNDERRUN);
    }

    if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)     /* 起始位错误 */
    {
        SDIO_ClearFlag(SDIO_FLAG_STBITERR);                 /* 清除错误标志 */

        SDIO->MASK &= ~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));     /* 关闭相关中断 */

        g_transfer_error = SD_START_BIT_ERR;

        return(SD_START_BIT_ERR);
    }
    return(SD_OK);
}

/**
 * @brief       读取当前卡状态
 * @param       pcardstatus:卡状态
 * @retval      错误代码
 */
SD_Error sd_send_status(uint32_t *pcardstatus)
{
    SD_Error errorstatus = SD_OK;

    if(pcardstatus == NULL)
    {
        errorstatus = SD_INVALID_PARAMETER;
        return errorstatus;
    }

    /* 发送CMD13,短响应 */
    g_sdio_cmd_struct.SDIO_Argument = (uint32_t)g_rca << 16;
    g_sdio_cmd_struct.SDIO_CmdIndex = SD_CMD_SEND_STATUS;
    g_sdio_cmd_struct.SDIO_Response = SDIO_Response_Short;
    g_sdio_cmd_struct.SDIO_Wait = SDIO_Wait_No;
    g_sdio_cmd_struct.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&g_sdio_cmd_struct);

    /* 查询响应状态 */
    errorstatus = cmd_resp1_error(SD_CMD_SEND_STATUS);

    if(errorstatus != SD_OK)
    {
        return errorstatus;
    }

    *pcardstatus = SDIO->RESP1;     /* 读取响应值 */

    return errorstatus;
}

/**
 * @brief       获取SD卡的状态
 * @param       无
 * @retval      错误代码
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
 * @brief       配置SDIO DMA
 * @param       mbuf:存储器地址
 * @param       bufsize:传输数据量
 * @param       dir:方向
 * @retval      无
 */
void sd_dma_config(uint32_t *mbuf, uint32_t bufsize, uint32_t dir)
{
    DMA_InitTypeDef dma_init_struct;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);

    DMA_DeInit(DMA2_Channel4);
    DMA_Cmd(DMA2_Channel4, DISABLE );

    dma_init_struct.DMA_PeripheralBaseAddr = (uint32_t)&SDIO->FIFO;           /* DMA外设地址 */
    dma_init_struct.DMA_MemoryBaseAddr = (uint32_t)mbuf;                      /* 存储器地址 */
    dma_init_struct.DMA_DIR = dir;                                            /* 传输方向 */
    dma_init_struct.DMA_BufferSize = bufsize/4;                               /* 数据传输量 */
    dma_init_struct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;            /* 外设非增量模式 */
    dma_init_struct.DMA_MemoryInc = DMA_MemoryInc_Enable;                     /* 存储器增量模式 */
    dma_init_struct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;     /* 外设数据长度:32位 */
    dma_init_struct.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;             /* 存储器数据长度:32位 */
    dma_init_struct.DMA_Mode = DMA_Mode_Normal;                               /* 使用普通模式 */
    dma_init_struct.DMA_Priority = DMA_Priority_High;                         /* 高优先级 */
    dma_init_struct.DMA_M2M = DMA_M2M_Disable;                                /* 关闭内存到内存模式 */
    DMA_Init(DMA2_Channel4, &dma_init_struct);                                /* 初始化DMA */

    DMA_Cmd(DMA2_Channel4, DISABLE );                                         /* 开启DMA传输 */
}

/**
 * @brief       读SD卡
 * @param       pbuf  : 数据缓存区
 * @param       saddr : 扇区地址
 * @param       cnt   : 扇区个数
 * @retval      0,正常; 其他,错误代码(详见SD_Error定义);
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
            sta = sd_read_block(SDIO_DATA_BUFFER, lsector + 512 * n, 512);  /* 单个sector的读操作 */
            memcpy(buf, SDIO_DATA_BUFFER, 512);
            buf += 512;
        }
    }
    else
    {
        if(cnt == 1)                                                        /* 单个sector的读操作 */
        {
            sta = sd_read_block(buf, lsector, 512);
        }
        else                                                                /* 多个sector的读操作 */
        {
            sta = sd_read_multi_blocks(buf, lsector, 512, cnt);
        }
    }
    return sta;
}

/**
 * @brief       写SD卡
 * @param       pbuf  : 数据缓存区
 * @param       saddr : 扇区地址
 * @param       cnt   : 扇区个数
 * @retval      0,正常;其他,错误代码(详见SD_Error定义);
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
            sta = sd_write_block(SDIO_DATA_BUFFER, lsector + 512 * n, 512);     /* 单个sector的写操作 */
            buf += 512;
        }
    }
    else
    {
        if(cnt == 1)
        {
            sta = sd_write_block(buf, lsector, 512);                            /* 单个sector的写操作 */
        }
        else
        {
            sta = sd_write_multi_blocks(buf, lsector, 512,cnt);                 /* 多个sector的写操作 */
        }
    }
    return sta;
}

