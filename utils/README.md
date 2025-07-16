# 字体工具集 （作者：Claude Sonnet 4）

这个工具集包含两个Python程序，用于处理和显示字体数据。

## 文件说明

- `ttfToC.py` - TTF字体转C数组工具
- `showChar.py` - 字体字符显示工具
- `font_ARLRDBD_32.h` - 示例字体头文件（32x32像素ARLRDBD字体）

## 1. ttfToC.py - TTF字体转C数组工具

### 功能
将TTF字体文件中的ASCII字符转换为C语言数组格式，适用于嵌入式系统显示。

### 特性
- 支持自定义字体大小
- 生成ASCII 32-126字符集（95个字符）
- 输出标准C头文件格式
- 自动计算字符位图数据
- 居中对齐字符渲染

### 使用方法

#### 基本用法
```bash
python ttfToC.py <TTF字体文件路径> <字体大小>
```

#### 指定输出文件
```bash
python ttfToC.py <TTF字体文件路径> <字体大小> [输出文件路径]
```

#### 示例
```bash
# 将Arial字体转换为16x16像素
python ttfToC.py arial.ttf 16

# 将字体转换为32x32像素并指定输出文件
python ttfToC.py arial.ttf 32 my_font_32.h

# 使用自定义字体
python ttfToC.py MyFont.ttf 24 custom_font_24.h
```

### 输出格式
生成的C头文件包含：
- 文件头注释（字体信息、大小、字符范围等）
- 预处理器保护宏
- 字符数组声明 `asc2_XXYY[95][字节数]`
- 每个字符的位图数据和注释

### 依赖库
```bash
pip install Pillow numpy
```

## 2. showChar.py - 字体字符显示工具

### 功能
解析C头文件中的字体数据，将所有字符可视化显示在一张图片上。

### 特性
- 自动解析C头文件格式
- 网格布局显示所有字符
- 生成高清PNG图片
- 添加字符标签和网格线
- 支持32x32像素字体显示

### 使用方法

#### 运行程序
```bash
python showChar.py
```

**注意**: 程序会自动查找当前目录下的 `font_ARLRDBD_32.h` 文件。

#### 修改字体文件
如需显示其他字体文件，请修改 `showChar.py` 中的文件路径：
```python
font_file = 'your_font_file.h'  # 修改为你的字体文件名
```

### 输出结果
- 弹出显示窗口展示所有字符
- 生成 `font_display.png` 图片文件
- 控制台输出字符统计信息

### 显示特性
- 16列网格布局
- 红色网格线分隔
- 第一行显示字符标签
- 字符范围：ASCII 32-126

### 依赖库
```bash
pip install numpy matplotlib Pillow
```

## 工作流程

### 典型使用流程

1. **转换TTF字体**
   ```bash
   python ttfToC.py arial.ttf 32 font_arial_32.h
   ```

2. **显示字体字符**
   ```bash
   # 修改showChar.py中的字体文件路径
   python showChar.py
   ```

3. **在嵌入式项目中使用**
   - 将生成的.h文件包含到项目中
   - 使用字符数组显示文本

### 嵌入式使用示例

```c
#include "font_arial_32.h"

// 显示字符 'A' (ASCII 65, 数组索引 65-32=33)
void display_char_A() {
    const unsigned char* char_data = asc2_3232[33];  // 'A'
    // 将char_data发送到显示设备
}

// 显示字符串
void display_string(const char* str) {
    while (*str) {
        if (*str >= 32 && *str <= 126) {
            int index = *str - 32;
            const unsigned char* char_data = asc2_3232[index];
            // 显示字符数据
        }
        str++;
    }
}
```

## 字体数据格式说明

### C数组格式
```c
const unsigned char asc2_3232[95][128] = {
    {0x00,0x00,...},  /*" ",0*/   // 空格字符
    {0x00,0x03,...},  /*"!",1*/   // 感叹号
    // ... 其他字符
};
```

### 位图编码
- 每个像素用1位表示（0=背景，1=前景）
- 按行存储，每行从左到右
- 每字节最高位在前（MSB first）
- 32x32字体需要128字节（32×32÷8）

## 故障排除

### 常见问题

1. **找不到字体文件**
   - 检查TTF文件路径是否正确
   - 确保字体文件存在且可读

2. **显示乱码或空白**
   - 检查字体是否包含ASCII字符
   - 尝试调整字体大小

3. **程序运行错误**
   - 确保安装了所有依赖库
   - 检查Python版本（建议3.6+）

4. **生成的图片看不清**
   - 调整DPI设置
   - 修改图片尺寸参数

### 调试技巧

1. **检查字体数据**
   ```python
   # 在showChar.py中添加调试输出
   print(f"字符数据长度: {len(char_data)}")
   print(f"前8个字节: {char_data[:8]}")
   ```

2. **验证位图转换**
   ```python
   # 输出位图数组
   print("位图数据:")
   print(char_bitmap)
   ```

## 技术参数

### 支持的字体格式
- 输入：TTF、OTF字体文件
- 输出：C语言头文件（.h）

### 字符支持
- ASCII范围：32-126（95个字符）
- 包含：数字、字母、标点符号、特殊字符

### 性能参数
- 处理速度：约10-20字符/秒
- 内存占用：字体大小相关
- 输出大小：32x32字体约12KB

## 版本信息

- 版本：1.0
- 兼容性：Python 3.6+
- 平台：Windows/Linux/macOS

## 许可证

本工具集用于学习和开发目的，请遵守相关字体的许可证条款。
