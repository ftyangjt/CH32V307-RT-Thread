/**
 ****************************************************************************************************
 * @file        lcd.h
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2023-07-20
 * @brief       2.8��/3.5��/4.3��/7�� TFTLCD(MCU��) ��������
 *              ֧������IC�ͺŰ���:ILI9341/NT35310/NT35510/SSD1963/ST7789/ST7796/ILI9806��
 *
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

#ifndef __LCD_H
#define __LCD_H

#include "stdlib.h"
#include "./SYSTEM/sys/sys.h"


/******************************************************************************************/

/* LCD WR/RD/BL/CS/RS ���� ����
 * LCD_D0~D15,��������̫��,�Ͳ������ﶨ����,ֱ����lcd_init�����޸�.��������ֲ��ʱ��,���˸�
 * ��6��IO��, ���ø�LCD_Init�����D0~D15���ڵ�IO��
 * RESET ��ϵͳ��λ�Ź��� �������ﲻ�ö��� RESET���� */
#define LCD_WR_GPIO_PORT                GPIOD
#define LCD_WR_GPIO_PIN                 GPIO_Pin_5
#define LCD_WR_GPIO_CLK_ENABLE()        do{RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);}while(0) /* WR����ʱ��ʹ�� */

#define LCD_RD_GPIO_PORT                GPIOD
#define LCD_RD_GPIO_PIN                 GPIO_Pin_4
#define LCD_RD_GPIO_CLK_ENABLE()        do{RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);}while(0) /* RD����ʱ��ʹ�� */

#define LCD_BL_GPIO_PORT                GPIOB
#define LCD_BL_GPIO_PIN                 GPIO_Pin_4
#define LCD_BL_GPIO_CLK_ENABLE()        do{RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);}while(0) /* ��������ʱ��ʹ�� */

/* LCD_CS(��Ҫ����LCD_FSMC_NEX������ȷ��IO��)�� LCD_RS(��Ҫ����LCD_FSMC_AX������ȷ��IO��) ���� ���� */
#define LCD_CS_GPIO_PORT                GPIOD
#define LCD_CS_GPIO_PIN                 GPIO_Pin_7
#define LCD_CS_GPIO_CLK_ENABLE()        do{RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);}while(0) /* CS����ʱ��ʹ�� */

#define LCD_RS_GPIO_PORT                GPIOE
#define LCD_RS_GPIO_PIN                 GPIO_Pin_3
#define LCD_RS_GPIO_CLK_ENABLE()        do{RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);}while(0) /* RS����ʱ��ʹ�� */

/* FSMC��ز��� ���� 
 * ע��: ����ͨ��FSMC��1������LCD
 * �޸�LCD_FSMC_AX , ��Ӧ��LCD_RS_GPIO�������Ҳ�ø�
 */
#define LCD_FSMC_AX          19         /* ʹ��FSMC_A19��LCD_RS */

/******************************************************************************************/

/* LCD��Ҫ������ */
typedef struct
{
    uint16_t width;                 /* LCD ��� */
    uint16_t height;                /* LCD �߶� */
    uint16_t id;                    /* LCD ID */
    uint8_t dir;                    /* ���������������ƣ�0��������1�������� */
    uint16_t wramcmd;               /* ��ʼдgramָ�� */
    uint16_t setxcmd;               /* ����x����ָ�� */
    uint16_t setycmd;               /* ����y����ָ�� */
} _lcd_dev;

extern _lcd_dev lcddev;             /* ����LCD��Ҫ���� */

/* LCD�Ļ�����ɫ�ͱ���ɫ */
extern uint32_t  g_point_color;     /* Ĭ�Ϻ�ɫ */
extern uint32_t  g_back_color;      /* ������ɫ.Ĭ��Ϊ��ɫ */

/* LCD������� */
#define LCD_BL(x)   do{ x ? \
                        GPIO_SetBits(LCD_BL_GPIO_PORT, LCD_BL_GPIO_PIN) : \
                        GPIO_ResetBits(LCD_BL_GPIO_PORT, LCD_BL_GPIO_PIN); \
                    }while(0)

/* LCD��ַ�ṹ�� */
typedef struct
{
    volatile uint16_t LCD_REG;
    volatile uint16_t LCD_RAM;
} LCD_TypeDef;


/* LCD_BASE����ϸ���㷽��:
 * ����ʹ��FSMC�Ŀ�1(BLOCK1)������TFTLCDҺ����(MCU��)
 * �洢��1(FSMC_NE1)��ַ��Χ: 0X6000 0000 ~ 0X60FF FFFF
 *
 * ������ʹ��FSMC_NE1����LCD_CS, FSMC_A19����LCD_RS, 16λ������,���㷽������:
 * FSMC_NE1�Ļ���ַΪ: 0X6000 0000;
 * FSMC_A19��Ӧ��ֵַ: 2^19 * 2 = 0X100000; FSMC_Ay��Ӧ�ĵ�ַΪ(y = 16 ~ 23): 2^y * 2
 *
 * LCD->LCD_REG,��ӦLCD_RS = 0(LCD�Ĵ���); LCD->LCD_RAM,��ӦLCD_RS = 1(LCD����)
 * �� LCD->LCD_RAM�ĵ�ַΪ:  0X6000 0000 + 2^19 * 2 = 0X60100000
 *    LCD->LCD_REG�ĵ�ַ����Ϊ LCD->LCD_RAM֮��������ַ.
 * ��������ʹ�ýṹ�����LCD_REG �� LCD_RAM(REG��ǰ,RAM�ں�,��Ϊ16λ���ݿ��)
 * ��� �ṹ��Ļ���ַ(LCD_BASE) = LCD_RAM - 2 = 0X60100000 -2
 *
 * ����ͨ�õļ��㹫ʽΪ(Ƭѡ��FSMC_NE1,RS�ӵ�ַ��FSMC_Ay,y=16~23):
 *          LCD_BASE = (0X6000 0000 | (2^y * 2 -2), ��Ч��(ʹ����λ����):
 *          LCD_BASE = (0X6000 0000 | ((1 << y) * 2 -2)
 */
#define LCD_BASE        (uint32_t)(0X60000000 | (((1 << LCD_FSMC_AX) * 2) -2))
#define LCD             ((LCD_TypeDef *) LCD_BASE)

/******************************************************************************************/
/* LCDɨ�跽�����ɫ ���� */

/* ɨ�跽���� */
#define L2R_U2D         0           /* ������,���ϵ��� */
#define L2R_D2U         1           /* ������,���µ��� */
#define R2L_U2D         2           /* ���ҵ���,���ϵ��� */
#define R2L_D2U         3           /* ���ҵ���,���µ��� */

#define U2D_L2R         4           /* ���ϵ���,������ */
#define U2D_R2L         5           /* ���ϵ���,���ҵ��� */
#define D2U_L2R         6           /* ���µ���,������ */
#define D2U_R2L         7           /* ���µ���,���ҵ��� */

#define DFT_SCAN_DIR    L2R_U2D     /* Ĭ�ϵ�ɨ�跽�� */

/* ���û�����ɫ */
#define WHITE           0xFFFF      /* ��ɫ */
#define BLACK           0x0000      /* ��ɫ */
#define RED             0xF800      /* ��ɫ */
#define GREEN           0x07E0      /* ��ɫ */
#define BLUE            0x001F      /* ��ɫ */ 
#define MAGENTA         0XF81F      /* Ʒ��ɫ/�Ϻ�ɫ = BLUE + RED */
#define YELLOW          0XFFE0      /* ��ɫ = GREEN + RED */
#define CYAN            0X07FF      /* ��ɫ = GREEN + BLUE */  

/* �ǳ�����ɫ */
#define BROWN           0XBC40      /* ��ɫ */
#define BRRED           0XFC07      /* �غ�ɫ */
#define GRAY            0X8430      /* ��ɫ */ 
#define DARKBLUE        0X01CF      /* ����ɫ */
#define LIGHTBLUE       0X7D7C      /* ǳ��ɫ */ 
#define GRAYBLUE        0X5458      /* ����ɫ */ 
#define LIGHTGREEN      0X841F      /* ǳ��ɫ */  
#define LGRAY           0XC618      /* ǳ��ɫ(PANNEL),���屳��ɫ */ 
#define LGRAYBLUE       0XA651      /* ǳ����ɫ(�м����ɫ) */ 
#define LBBLUE          0X2B12      /* ǳ����ɫ(ѡ����Ŀ�ķ�ɫ) */ 

/******************************************************************************************/
/* SSD1963������ò���(һ�㲻�ø�) */

/* LCD�ֱ������� */ 
#define SSD_HOR_RESOLUTION      800     /* LCDˮƽ�ֱ��� */ 
#define SSD_VER_RESOLUTION      480     /* LCD��ֱ�ֱ��� */ 

/* LCD������������ */ 
#define SSD_HOR_PULSE_WIDTH     1       /* ˮƽ���� */ 
#define SSD_HOR_BACK_PORCH      46      /* ˮƽǰ�� */ 
#define SSD_HOR_FRONT_PORCH     210     /* ˮƽ���� */ 

#define SSD_VER_PULSE_WIDTH     1       /* ��ֱ���� */ 
#define SSD_VER_BACK_PORCH      23      /* ��ֱǰ�� */ 
#define SSD_VER_FRONT_PORCH     22      /* ��ֱǰ�� */ 

/* ���¼����������Զ����� */ 
#define SSD_HT          (SSD_HOR_RESOLUTION + SSD_HOR_BACK_PORCH + SSD_HOR_FRONT_PORCH)
#define SSD_HPS         (SSD_HOR_BACK_PORCH)
#define SSD_VT          (SSD_VER_RESOLUTION + SSD_VER_BACK_PORCH + SSD_VER_FRONT_PORCH)
#define SSD_VPS         (SSD_VER_BACK_PORCH)
   
/******************************************************************************************/

void lcd_wr_data(volatile uint16_t data);            /* LCDд���� */
void lcd_wr_regno(volatile uint16_t regno);          /* LCDд�Ĵ������/��ַ */
void lcd_write_reg(uint16_t regno, uint16_t data);   /* LCDд�Ĵ�����ֵ */

void lcd_init(void);                                 /* ��ʼ��LCD */
void lcd_display_on(void);                           /* ����ʾ */
void lcd_display_off(void);                          /* ����ʾ */
void lcd_scan_dir(uint8_t dir);                      /* ������ɨ�跽�� */
void lcd_display_dir(uint8_t dir);                   /* ������Ļ��ʾ���� */
void lcd_ssd_backlight_set(uint8_t pwm);             /* SSD1963 ������� */

void lcd_write_ram_prepare(void);                           /* ׼��дGRAM */
void lcd_set_cursor(uint16_t x, uint16_t y);                /* ���ù�� */
uint32_t lcd_read_point(uint16_t x, uint16_t y);            /* ����(32λ��ɫ,����LTDC) */
void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color);/* ����(32λ��ɫ,����LTDC) */

void lcd_clear(uint16_t color);                                                             /* LCD���� */
void lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color);                   /* ���ʵ��Բ */
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);                  /* ��Բ */
void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color);                  /* ��ˮƽ�� */
void lcd_set_window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height);             /* ���ô��� */
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);          /* ��ɫ������(32λ��ɫ,����LTDC)*/
void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color);   /* ��ɫ������ */
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);     /* ��ֱ�� */
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);/* ������ */

void lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color);                     /* ��ʾһ���ַ� */
void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color);                   /* ��ʾ���� */
void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color);    /* ��չ��ʾ���� */
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color); /* ��ʾ�ַ��� */

#endif

