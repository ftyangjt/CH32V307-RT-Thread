#ifndef __MAIN_H__
#define __MAIN_H__

#include "ch32v30x.h"
#include <rtthread.h>

/* 函数原型声明 */

/**
 * @brief 将字符串转换为整数
 * @param str 要转换的字符串
 * @return 转换后的整数值
 */
static int str_to_int(const char* str);

/**
 * @brief 启动彩虹效果的MSH命令处理函数
 * @param argc 参数数量
 * @param argv 参数数组
 * @return RT_EOK表示成功
 */
static int cmd_rainbow(int argc, char **argv);

/**
 * @brief 停止彩虹效果的MSH命令处理函数
 * @param argc 参数数量
 * @param argv 参数数组
 * @return RT_EOK表示成功
 */
static int cmd_rainbow_stop(int argc, char **argv);

/**
 * @brief 设置彩虹效果亮度的MSH命令处理函数
 * @param argc 参数数量
 * @param argv 参数数组
 * @return RT_EOK表示成功
 */
static int cmd_brightness(int argc, char **argv);

/**
 * @brief 字库初始化线程入口函数
 * @param parameter 线程参数
 */
static void font_init_thread_entry(void *parameter);

/**
 * @brief 显示主界面
 */
static void display_main_ui(void);

/**
 * @brief 更新显示彩虹效果参数
 */
static void update_display(void);

/**
 * @brief 主函数
 * @return 程序执行结果
 */
int main(void);

#endif /* __MAIN_H__ */