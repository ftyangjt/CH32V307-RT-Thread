#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TTF字体转C数组工具
将TTF字体文件中的ASCII字符转换为C语言数组格式
支持自定义字体大小，生成格式与lcdfont.h中的字体格式兼容
"""

import os
import sys
from PIL import Image, ImageDraw, ImageFont
import numpy as np

def ttf_to_c_array(ttf_path, font_size, output_path=None):
    """修正后的TTF字体转换函数"""
    
    if not os.path.exists(ttf_path):
        print(f"错误: 字体文件 {ttf_path} 不存在")
        return False
    
    try:
        font = ImageFont.truetype(ttf_path, font_size)
        ascii_chars = list(range(32, 127))
        
        # 根据字体大小确定字符矩阵尺寸（匹配现有格式）
        if font_size <= 12:
            char_height, char_width = 12, 6
            array_name = "asc2_1206"
        elif font_size <= 16:
            char_height, char_width = 16, 8
            array_name = "asc2_1608"
        elif font_size <= 24:
            char_height, char_width = 24, 12
            array_name = "asc2_2412"
        elif font_size <= 32:
            char_height, char_width = 32, 16
            array_name = "asc2_3216"
        elif font_size <= 48:
            char_height, char_width = 48, 24
            array_name = "asc2_4824"
        else:
            char_height, char_width = 48, 24  # 默认最大尺寸
            array_name = "asc2_4824"
        
        # 计算每个字符占用的字节数（与LCD函数相同的算法）
        bytes_per_col = (char_height + 7) // 8  # 每列需要的字节数
        csize = bytes_per_col * char_width       # 总字节数
        
        if output_path is None:
            font_name = os.path.splitext(os.path.basename(ttf_path))[0]
            output_path = f"font_{font_name}_{char_height}{char_width:02d}.h"
        
        all_char_data = []
        
        print(f"字符尺寸: {char_height}x{char_width}")
        print(f"每列字节数: {bytes_per_col}, 总字节数: {csize}")
        
        for i, char_code in enumerate(ascii_chars):
            char = chr(char_code)
            
            # 创建图像
            img = Image.new('1', (char_width, char_height), 0)
            draw = ImageDraw.Draw(img)
            
            try:
                bbox = font.getbbox(char)
                text_width = bbox[2] - bbox[0]
                text_height = bbox[3] - bbox[1]
                
                # 居中对齐
                x = max(0, (char_width - text_width) // 2 - bbox[0])
                y = max(0, (char_height - text_height) // 2 - bbox[1])
                
                draw.text((x, y), char, font=font, fill=1)
                
            except Exception as e:
                print(f"警告: 字符 '{char}' 处理失败: {e}")
            
            # 转换为numpy数组
            img_array = np.array(img)
            
            # 生成字体数据（按列优先，垂直扫描）
            char_data = []
            
            # 遍历每一列
            for col in range(char_width):
                # 遍历该列中的每个字节（每8行为一个字节）
                for byte_row in range(0, char_height, 8):
                    byte_value = 0
                    # 生成字节数据（从最高位开始）
                    for bit in range(8):
                        row = byte_row + bit
                        if row < char_height:
                            # 检查该像素是否为前景色
                            if row < len(img_array) and col < len(img_array[row]):
                                if img_array[row][col]:
                                    byte_value |= (0x80 >> bit)  # 最高位对应第一行
                    char_data.append(byte_value)
            
            # 确保数据长度正确
            while len(char_data) < csize:
                char_data.append(0x00)
            
            all_char_data.append(char_data[:csize])
            
            if (i + 1) % 10 == 0:
                print(f"已处理: {i + 1}/{len(ascii_chars)} 个字符")
        
        # 生成C头文件
        with open(output_path, 'w', encoding='utf-8') as f:
            font_name = os.path.splitext(os.path.basename(ttf_path))[0]
            
            f.write(f"/**\n")
            f.write(f" * @file    {os.path.basename(output_path)}\n")
            f.write(f" * @brief   {char_height}*{char_width} ASCII字符字库\n")
            f.write(f" * @note    兼容lcd_show_char函数，列优先垂直扫描格式\n")
            f.write(f" */\n\n")
            
            f.write(f"#ifndef __FONT_{char_height}{char_width:02d}_H__\n")
            f.write(f"#define __FONT_{char_height}{char_width:02d}_H__\n\n")
            
            f.write(f"const unsigned char {array_name}[{len(ascii_chars)}][{csize}]={{\n")
            
            for i, (char_code, char_data) in enumerate(zip(ascii_chars, all_char_data)):
                char = chr(char_code)
                char_display = char.replace('\\', '\\\\').replace('"', '\\"').replace("'", "\\'")
                
                f.write("{")
                for j, byte_val in enumerate(char_data):
                    f.write(f"0x{byte_val:02X}")
                    if j < len(char_data) - 1:
                        f.write(",")
                f.write(f"}},/*\"{char_display}\",{i}*/\n")
            
            f.write("};\n\n")
            f.write(f"#endif\n")
        
        print(f"转换完成！输出文件: {output_path}")
        print(f"数组名称: {array_name}, 兼容lcd_show_char函数")
        
        return True
        
    except Exception as e:
        print(f"错误: {e}")
        return False

def main():
    """主函数"""
    if len(sys.argv) < 3:
        print("用法: python ttfToC.py <TTF字体文件路径> <字体大小> [输出文件路径]")
        print("示例: python ttfToC.py arial.ttf 16")
        print("示例: python ttfToC.py arial.ttf 16 my_font.h")
        print("\n支持的字体大小和对应的字符宽度:")
        print("  ≤12px: 12x6 (如asc2_1206)")
        print("  ≤16px: 16x8 (如asc2_1608)")
        print("  ≤24px: 24x12 (如asc2_2412)")
        print("  >24px: 32x16 (如asc2_3216)")
        return
    
    ttf_path = sys.argv[1]
    font_size = int(sys.argv[2])
    output_path = sys.argv[3] if len(sys.argv) > 3 else None
    
    # 执行转换
    success = ttf_to_c_array(ttf_path, font_size, output_path)
    
    if success:
        print("转换成功！")
    else:
        print("转换失败！")

if __name__ == "__main__":
    main()