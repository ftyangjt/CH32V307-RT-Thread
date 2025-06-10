from PIL import Image

def rgb888_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def convert_image_to_rgb565_array(image_path, out_c_path, var_name_prefix="gImage"):
    img = Image.open(image_path).convert('RGB')
    width, height = img.size
    pixels = list(img.getdata())
    rgb565_data = [rgb888_to_rgb565(r, g, b) for (r, g, b) in pixels]

    var_name = f"{var_name_prefix}_{width}x{height}"
    with open(out_c_path, 'w') as f:
        f.write(f"const unsigned short {var_name}[{width*height}] = {{\n")
        for i, val in enumerate(rgb565_data):
            f.write(f"0x{val:04X},")
            if (i + 1) % 12 == 0:
                f.write("\n")
        f.write("\n};\n")
    print(f"图片已转换为C数组，尺寸：{width}x{height}，输出文件：{out_c_path}")

# 用法示例
convert_image_to_rgb565_array("Kano_LCD_S.png", "gImage_auto.c")