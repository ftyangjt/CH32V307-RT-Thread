/**
 ****************************************************************************************************
 * @file        sdio_sdcard.h
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

#ifndef __SDIO_SDCARD_H
#define __SDIO_SDCARD_H

#include "./SYSTEM/sys/sys.h"


/* SDIO相关标志位 */
#define SDIO_FLAG_CCRCFAIL                  ((uint32_t)0x00000001)
#define SDIO_FLAG_DCRCFAIL                  ((uint32_t)0x00000002)
#define SDIO_FLAG_CTIMEOUT                  ((uint32_t)0x00000004)
#define SDIO_FLAG_DTIMEOUT                  ((uint32_t)0x00000008)
#define SDIO_FLAG_TXUNDERR                  ((uint32_t)0x00000010)
#define SDIO_FLAG_RXOVERR                   ((uint32_t)0x00000020)
#define SDIO_FLAG_CMDREND                   ((uint32_t)0x00000040)
#define SDIO_FLAG_CMDSENT                   ((uint32_t)0x00000080)
#define SDIO_FLAG_DATAEND                   ((uint32_t)0x00000100)
#define SDIO_FLAG_STBITERR                  ((uint32_t)0x00000200)
#define SDIO_FLAG_DBCKEND                   ((uint32_t)0x00000400)
#define SDIO_FLAG_CMDACT                    ((uint32_t)0x00000800)
#define SDIO_FLAG_TXACT                     ((uint32_t)0x00001000)
#define SDIO_FLAG_RXACT                     ((uint32_t)0x00002000)
#define SDIO_FLAG_TXFIFOHE                  ((uint32_t)0x00004000)
#define SDIO_FLAG_RXFIFOHF                  ((uint32_t)0x00008000)
#define SDIO_FLAG_TXFIFOF                   ((uint32_t)0x00010000)
#define SDIO_FLAG_RXFIFOF                   ((uint32_t)0x00020000)
#define SDIO_FLAG_TXFIFOE                   ((uint32_t)0x00040000)
#define SDIO_FLAG_RXFIFOE                   ((uint32_t)0x00080000)
#define SDIO_FLAG_TXDAVL                    ((uint32_t)0x00100000)
#define SDIO_FLAG_RXDAVL                    ((uint32_t)0x00200000)
#define SDIO_FLAG_SDIOIT                    ((uint32_t)0x00400000)
#define SDIO_FLAG_CEATAEND                  ((uint32_t)0x00800000)

/* SDIO时钟分频 */
#define SDIO_INIT_CLK_DIV        0xB2       /* SDIO初始化速率，72MHz / (0xB2 + 2) = 400KHz */
#define SDIO_TRANSFER_CLK_DIV    0x01       /* SDIO传输速率，72MHz / (0x01 + 2) = 24MHz */

/* SDIO工作模式定义 */
#define SD_POLLING_MODE          0          /* 查询模式,该模式下,如果读写有问题,建议增大SDIO_TRANSFER_CLK_DIV的设置 */
#define SD_DMA_MODE              1          /* DMA模式,该模式下,如果读写有问题,建议增大SDIO_TRANSFER_CLK_DIV的设置 */

/* SDIO错误定义 */
typedef enum
{
    /* 特殊错误 */
    SD_CMD_CRC_FAIL                    = (1),
    SD_DATA_CRC_FAIL                   = (2),
    SD_CMD_RSP_TIMEOUT                 = (3),
    SD_DATA_TIMEOUT                    = (4),
    SD_TX_UNDERRUN                     = (5),
    SD_RX_OVERRUN                      = (6),
    SD_START_BIT_ERR                   = (7),
    SD_CMD_OUT_OF_RANGE                = (8),
    SD_ADDR_MISALIGNED                 = (9),
    SD_BLOCK_LEN_ERR                   = (10),
    SD_ERASE_SEQ_ERR                   = (11),
    SD_BAD_ERASE_PARAM                 = (12),
    SD_WRITE_PROT_VIOLATION            = (13),
    SD_LOCK_UNLOCK_FAILED              = (14),
    SD_COM_CRC_FAILED                  = (15),
    SD_ILLEGAL_CMD                     = (16),
    SD_CARD_ECC_FAILED                 = (17),
    SD_CC_ERROR                        = (18),
    SD_GENERAL_UNKNOWN_ERROR           = (19),
    SD_STREAM_READ_UNDERRUN            = (20),
    SD_STREAM_WRITE_OVERRUN            = (21),
    SD_CID_CSD_OVERWRITE               = (22),
    SD_WP_ERASE_SKIP                   = (23),
    SD_CARD_ECC_DISABLED               = (24),
    SD_ERASE_RESET                     = (25),
    SD_AKE_SEQ_ERROR                   = (26),
    SD_INVALID_VOLTRANGE               = (27),
    SD_ADDR_OUT_OF_RANGE               = (28),
    SD_SWITCH_ERROR                    = (29),
    SD_SDIO_DISABLED                   = (30),
    SD_SDIO_FUNCTION_BUSY              = (31),
    SD_SDIO_FUNCTION_FAILED            = (32),
    SD_SDIO_UNKNOWN_FUNCTION           = (33),
    /* 标准错误 */
    SD_INTERNAL_ERROR,
    SD_NOT_CONFIGURED,
    SD_REQUEST_PENDING,
    SD_REQUEST_NOT_APPLICABLE,
    SD_INVALID_PARAMETER,
    SD_UNSUPPORTED_FEATURE,
    SD_UNSUPPORTED_HW,
    SD_ERROR,
    SD_OK = 0
} SD_Error;

/* SD卡CSD寄存器数据 */
typedef struct
{
    uint8_t  CSDStruct;
    uint8_t  SysSpecVersion;
    uint8_t  Reserved1;
    uint8_t  TAAC;
    uint8_t  NSAC;
    uint8_t  MaxBusClkFrec;
    uint16_t CardComdClasses;
    uint8_t  RdBlockLen;
    uint8_t  PartBlockRead;
    uint8_t  WrBlockMisalign;
    uint8_t  RdBlockMisalign;
    uint8_t  DSRImpl;
    uint8_t  Reserved2;
    uint32_t DeviceSize;
    uint8_t  MaxRdCurrentVDDMin;
    uint8_t  MaxRdCurrentVDDMax;
    uint8_t  MaxWrCurrentVDDMin;
    uint8_t  MaxWrCurrentVDDMax;
    uint8_t  DeviceSizeMul;
    uint8_t  EraseGrSize;
    uint8_t  EraseGrMul;
    uint8_t  WrProtectGrSize;
    uint8_t  WrProtectGrEnable;
    uint8_t  ManDeflECC;
    uint8_t  WrSpeedFact;
    uint8_t  MaxWrBlockLen;
    uint8_t  WriteBlockPaPartial;
    uint8_t  Reserved3;
    uint8_t  ContentProtectAppli;
    uint8_t  FileFormatGrouop;
    uint8_t  CopyFlag;
    uint8_t  PermWrProtect;
    uint8_t  TempWrProtect;
    uint8_t  FileFormat;
    uint8_t  ECC;
    uint8_t  CSD_CRC;
    uint8_t  Reserved4;
} SD_CSD;

/* SD卡CID寄存器数据 */
typedef struct
{
    uint8_t  ManufacturerID;
    uint16_t OEM_AppliID;
    uint32_t ProdName1;
    uint8_t  ProdName2;
    uint8_t  ProdRev;
    uint32_t ProdSN;
    uint8_t  Reserved1;
    uint16_t ManufactDate;
    uint8_t  CID_CRC;
    uint8_t  Reserved2;
} SD_CID;

/* SD卡状态 */
typedef enum
{
    SD_CARD_READY                  = ((uint32_t)0x00000001),
    SD_CARD_IDENTIFICATION         = ((uint32_t)0x00000002),
    SD_CARD_STANDBY                = ((uint32_t)0x00000003),
    SD_CARD_TRANSFER               = ((uint32_t)0x00000004),
    SD_CARD_SENDING                = ((uint32_t)0x00000005),
    SD_CARD_RECEIVING              = ((uint32_t)0x00000006),
    SD_CARD_PROGRAMMING            = ((uint32_t)0x00000007),
    SD_CARD_DISCONNECTED           = ((uint32_t)0x00000008),
    SD_CARD_ERROR                  = ((uint32_t)0x000000FF)
}SDCardState;

/* SD卡信息,包括CSD,CID等数据 */
typedef struct
{
  SD_CSD SD_csd;
  SD_CID SD_cid;
  long long CardCapacity;
  uint32_t CardBlockSize;
  uint16_t RCA;
  uint8_t CardType;
}SD_CardInfo;

extern SD_CardInfo g_sd_card_info;                 /* SD卡信息定义 */

/* SDIO指令集 */
#define SD_CMD_GO_IDLE_STATE                       ((uint8_t)0)
#define SD_CMD_SEND_OP_COND                        ((uint8_t)1)
#define SD_CMD_ALL_SEND_CID                        ((uint8_t)2)
#define SD_CMD_SET_REL_ADDR                        ((uint8_t)3)
#define SD_CMD_SET_DSR                             ((uint8_t)4)
#define SD_CMD_SDIO_SEN_OP_COND                    ((uint8_t)5)
#define SD_CMD_HS_SWITCH                           ((uint8_t)6)
#define SD_CMD_SEL_DESEL_CARD                      ((uint8_t)7)
#define SD_CMD_HS_SEND_EXT_CSD                     ((uint8_t)8)
#define SD_CMD_SEND_CSD                            ((uint8_t)9)
#define SD_CMD_SEND_CID                            ((uint8_t)10)
#define SD_CMD_READ_DAT_UNTIL_STOP                 ((uint8_t)11)
#define SD_CMD_STOP_TRANSMISSION                   ((uint8_t)12)
#define SD_CMD_SEND_STATUS                         ((uint8_t)13)
#define SD_CMD_HS_BUSTEST_READ                     ((uint8_t)14)
#define SD_CMD_GO_INACTIVE_STATE                   ((uint8_t)15)
#define SD_CMD_SET_BLOCKLEN                        ((uint8_t)16)
#define SD_CMD_READ_SINGLE_BLOCK                   ((uint8_t)17)
#define SD_CMD_READ_MULT_BLOCK                     ((uint8_t)18)
#define SD_CMD_HS_BUSTEST_WRITE                    ((uint8_t)19)
#define SD_CMD_WRITE_DAT_UNTIL_STOP                ((uint8_t)20)
#define SD_CMD_SET_BLOCK_COUNT                     ((uint8_t)23)
#define SD_CMD_WRITE_SINGLE_BLOCK                  ((uint8_t)24)
#define SD_CMD_WRITE_MULT_BLOCK                    ((uint8_t)25)
#define SD_CMD_PROG_CID                            ((uint8_t)26)
#define SD_CMD_PROG_CSD                            ((uint8_t)27)
#define SD_CMD_SET_WRITE_PROT                      ((uint8_t)28)
#define SD_CMD_CLR_WRITE_PROT                      ((uint8_t)29)
#define SD_CMD_SEND_WRITE_PROT                     ((uint8_t)30)
#define SD_CMD_SD_ERASE_GRP_START                  ((uint8_t)32)
#define SD_CMD_SD_ERASE_GRP_END                    ((uint8_t)33)
#define SD_CMD_ERASE_GRP_START                     ((uint8_t)35)
#define SD_CMD_ERASE_GRP_END                       ((uint8_t)36)
#define SD_CMD_ERASE                               ((uint8_t)38)
#define SD_CMD_FAST_IO                             ((uint8_t)39)
#define SD_CMD_GO_IRQ_STATE                        ((uint8_t)40)
#define SD_CMD_LOCK_UNLOCK                         ((uint8_t)42)
#define SD_CMD_APP_CMD                             ((uint8_t)55)
#define SD_CMD_GEN_CMD                             ((uint8_t)56)
#define SD_CMD_NO_CMD                              ((uint8_t)64)

/* 以下命令为SD卡专用命令
 * 在发送这些命令之前，需要发送CMD55
 */
#define SD_CMD_APP_SD_SET_BUSWIDTH                 ((uint8_t)6)
#define SD_CMD_SD_APP_STAUS                        ((uint8_t)13)
#define SD_CMD_SD_APP_SEND_NUM_WRITE_BLOCKS        ((uint8_t)22)
#define SD_CMD_SD_APP_OP_COND                      ((uint8_t)41)
#define SD_CMD_SD_APP_SET_CLR_CARD_DETECT          ((uint8_t)42)
#define SD_CMD_SD_APP_SEND_SCR                     ((uint8_t)51)
#define SD_CMD_SDIO_RW_DIRECT                      ((uint8_t)52)
#define SD_CMD_SDIO_RW_EXTENDED                    ((uint8_t)53)

/* 以下命令为SD卡相关的安全命令
 * 在发送这些命令之前，需要发送CMD55
 */
#define SD_CMD_SD_APP_GET_MKB                      ((uint8_t)43)
#define SD_CMD_SD_APP_GET_MID                      ((uint8_t)44)
#define SD_CMD_SD_APP_SET_CER_RN1                  ((uint8_t)45)
#define SD_CMD_SD_APP_GET_CER_RN2                  ((uint8_t)46)
#define SD_CMD_SD_APP_SET_CER_RES2                 ((uint8_t)47)
#define SD_CMD_SD_APP_GET_CER_RES1                 ((uint8_t)48)
#define SD_CMD_SD_APP_SECURE_READ_MULTIPLE_BLOCK   ((uint8_t)18)
#define SD_CMD_SD_APP_SECURE_WRITE_MULTIPLE_BLOCK  ((uint8_t)25)
#define SD_CMD_SD_APP_SECURE_ERASE                 ((uint8_t)38)
#define SD_CMD_SD_APP_CHANGE_SECURE_AREA           ((uint8_t)49)
#define SD_CMD_SD_APP_SECURE_WRITE_MKB             ((uint8_t)48)

/* 支持的SD卡定义 */
#define SDIO_STD_CAPACITY_SD_CARD_V1_1             ((uint32_t)0x00000000)
#define SDIO_STD_CAPACITY_SD_CARD_V2_0             ((uint32_t)0x00000001)
#define SDIO_HIGH_CAPACITY_SD_CARD                 ((uint32_t)0x00000002)
#define SDIO_MULTIMEDIA_CARD                       ((uint32_t)0x00000003)
#define SDIO_SECURE_DIGITAL_IO_CARD                ((uint32_t)0x00000004)
#define SDIO_HIGH_SPEED_MULTIMEDIA_CARD            ((uint32_t)0x00000005)
#define SDIO_SECURE_DIGITAL_IO_COMBO_CARD          ((uint32_t)0x00000006)
#define SDIO_HIGH_CAPACITY_MMC_CARD                ((uint32_t)0x00000007)

/* SDIO相关参数定义 */
#define SDIO_STATIC_FLAGS               ((uint32_t)0x000005FF)
#define SDIO_CMD0TIMEOUT                ((uint32_t)0x00010000)
#define SDIO_DATATIMEOUT                ((uint32_t)0xFFFFFFFF)
#define SDIO_FIFO_Address               ((uint32_t)0x40018080)

/* 卡错误状态寄存器R1（OCR寄存器）掩码定义 */
#define SD_OCR_ADDR_OUT_OF_RANGE        ((uint32_t)0x80000000)
#define SD_OCR_ADDR_MISALIGNED          ((uint32_t)0x40000000)
#define SD_OCR_BLOCK_LEN_ERR            ((uint32_t)0x20000000)
#define SD_OCR_ERASE_SEQ_ERR            ((uint32_t)0x10000000)
#define SD_OCR_BAD_ERASE_PARAM          ((uint32_t)0x08000000)
#define SD_OCR_WRITE_PROT_VIOLATION     ((uint32_t)0x04000000)
#define SD_OCR_LOCK_UNLOCK_FAILED       ((uint32_t)0x01000000)
#define SD_OCR_COM_CRC_FAILED           ((uint32_t)0x00800000)
#define SD_OCR_ILLEGAL_CMD              ((uint32_t)0x00400000)
#define SD_OCR_CARD_ECC_FAILED          ((uint32_t)0x00200000)
#define SD_OCR_CC_ERROR                 ((uint32_t)0x00100000)
#define SD_OCR_GENERAL_UNKNOWN_ERROR    ((uint32_t)0x00080000)
#define SD_OCR_STREAM_READ_UNDERRUN     ((uint32_t)0x00040000)
#define SD_OCR_STREAM_WRITE_OVERRUN     ((uint32_t)0x00020000)
#define SD_OCR_CID_CSD_OVERWRIETE       ((uint32_t)0x00010000)
#define SD_OCR_WP_ERASE_SKIP            ((uint32_t)0x00008000)
#define SD_OCR_CARD_ECC_DISABLED        ((uint32_t)0x00004000)
#define SD_OCR_ERASE_RESET              ((uint32_t)0x00002000)
#define SD_OCR_AKE_SEQ_ERROR            ((uint32_t)0x00000008)
#define SD_OCR_ERRORBITS                ((uint32_t)0xFDFFE008)

/* R6响应掩码定义 */
#define SD_R6_GENERAL_UNKNOWN_ERROR     ((uint32_t)0x00002000)
#define SD_R6_ILLEGAL_CMD               ((uint32_t)0x00004000)
#define SD_R6_COM_CRC_FAILED            ((uint32_t)0x00008000)

#define SD_VOLTAGE_WINDOW_SD            ((uint32_t)0x80100000)
#define SD_HIGH_CAPACITY                ((uint32_t)0x40000000)
#define SD_STD_CAPACITY                 ((uint32_t)0x00000000)
#define SD_CHECK_PATTERN                ((uint32_t)0x000001AA)
#define SD_VOLTAGE_WINDOW_MMC           ((uint32_t)0x80FF8000)

#define SD_MAX_VOLT_TRIAL               ((uint32_t)0x0000FFFF)
#define SD_ALLZERO                      ((uint32_t)0x00000000)

#define SD_WIDE_BUS_SUPPORT             ((uint32_t)0x00040000)
#define SD_SINGLE_BUS_SUPPORT           ((uint32_t)0x00010000)
#define SD_CARD_LOCKED                  ((uint32_t)0x02000000)
#define SD_CARD_PROGRAMMING             ((uint32_t)0x00000007)
#define SD_CARD_RECEIVING               ((uint32_t)0x00000006)
#define SD_DATATIMEOUT                  ((uint32_t)0xFFFFFFFF)
#define SD_0TO7BITS                     ((uint32_t)0x000000FF)
#define SD_8TO15BITS                    ((uint32_t)0x0000FF00)
#define SD_16TO23BITS                   ((uint32_t)0x00FF0000)
#define SD_24TO31BITS                   ((uint32_t)0xFF000000)
#define SD_MAX_DATA_LENGTH              ((uint32_t)0x01FFFFFF)

#define SD_HALFFIFO                     ((uint32_t)0x00000008)
#define SD_HALFFIFOBYTES                ((uint32_t)0x00000020)

/* 支持的命令类定义 */
#define SD_CCCC_LOCK_UNLOCK             ((uint32_t)0x00000080)
#define SD_CCCC_WRITE_PROT              ((uint32_t)0x00000040)
#define SD_CCCC_ERASE                   ((uint32_t)0x00000020)

/* 命令8定义 */
#define SDIO_SEND_IF_COND               ((uint32_t)0x00000008)

/******************************************************************************************/

SD_Error sd_init(void);                                             /* 初始化SD卡 */
void sd_clock_set(uint8_t clkdiv);                                  /* SDIO时钟设置 */
SD_Error sd_power_on(void);                                         /* SD卡上电 */
SD_Error sd_power_off(void);                                        /* 关闭SD卡 */
SD_Error sd_initialize_cards(void);                                 /* 初始化SD卡,使其进入就绪状态 */
SD_Error sd_get_info(SD_CardInfo *cardinfo);                        /* 获取SD卡信息 */
SD_Error sd_enable_wide_bus_operation(uint32_t wmode);              /* 设置SDIO总线宽度 */
SD_Error sd_set_device_mode(uint32_t mode);                         /* 设置SD卡工作模式 */
SD_Error sd_select_deselect(uint32_t addr);                         /* 选择SD卡 */
SD_Error sd_send_status(uint32_t *pcardstatus);                     /* 读取当前卡状态 */
SDCardState sd_get_state(void);                                     /* 获取SD卡的状态 */
SD_Error sd_read_block(uint8_t *buf, long long addr, uint16_t blksize);                         /* SD卡读取一个块 */
SD_Error sd_read_multi_blocks(uint8_t *buf, long long addr, uint16_t blksize, uint32_t nblks);  /* SD卡读取多个块 */
SD_Error sd_write_block(uint8_t *buf, long long addr, uint16_t blksize);                        /* SD卡写1个块 */
SD_Error sd_write_multi_blocks(uint8_t *buf, long long addr, uint16_t blksize, uint32_t nblks); /* SD卡写多个块 */
SD_Error sd_process_irq_src(void);                                  /* SDIO中断处理函数 */
void sd_dma_config(uint32_t *mbuf, uint32_t bufsize, uint32_t dir); /* 配置SDIO DMA */
uint8_t sd_read_disk(uint8_t *buf, uint32_t sector, uint8_t cnt);   /* 读SD卡 */
uint8_t sd_write_disk(uint8_t *buf, uint32_t sector, uint8_t cnt);  /* 写SD卡 */

#endif
