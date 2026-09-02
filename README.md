# WiiU GamePad Tester

**作者 / Authors:** Walking Bunny x Codex

[中文](#中文) | [English](#english)

## 中文

WiiU GamePad Tester 是运行在 Wii U 主机上的 GamePad（DRC/VPAD）硬件测试程序，提供液晶屏纯色测试以及图形化按键、摇杆、触摸和传感器测试。

### 功能

#### 屏幕测试（Screen Test）

全屏显示纯色，用于检查坏点、亮点、色彩异常和背光不均。

- 颜色顺序：黑、白、红、绿、蓝、青、品红、黄、灰
- 第一张黑色画面会短暂显示操作提示，随后恢复为纯黑
- 按 `A` 显示下一种颜色
- 按 `B` 返回主菜单

除第一张黑色画面的短暂提示外，测试画面不会显示文字。

#### 按键与传感器测试（Key / Sensor Test）

GamePad 屏幕使用实物外观图片，并在真实按键位置实时显示输入状态：

- A/B/X/Y、方向键、L/R、ZL/ZR、+/-、HOME、TV 和左右摇杆按下
- 按键按下时，整个对应按键覆盖半透明红色，并保留照片纹理和按键字母
- 蓝点实时显示左右摇杆位置
- 触摸点实时显示在中央屏幕区域
- 显示陀螺仪和加速度计的 X/Y/Z 数据
- 显示电池状态和音量滑块数值

同时按住 `+` 和 `-` 返回主菜单。

### 编译

需要安装包含 devkitPPC 和 wut 的 [devkitPro](https://devkitpro.org/wiki/Getting_Started)。

```sh
make
```

使用 Docker 在 PowerShell 中编译：

```powershell
docker run --rm -v "${PWD}:/workspace" -w /workspace devkitpro/devkitppc:20250102 make
```

使用 Docker 在 Linux/macOS shell 中编译：

```sh
docker run --rm -v "$PWD:/workspace" -w /workspace \
  devkitpro/devkitppc:20250102 make
```

### 安装

将生成的 `WiiUDrcTest.wuhb` 复制到 SD 卡：

```text
sd:/wiiu/apps/WiiUDrcTest/WiiUDrcTest.wuhb
```

然后从 Aroma Environment 的 Wii U Menu 或其他兼容的 homebrew 启动方式运行。

---

## English

WiiU GamePad Tester is a hardware diagnostic homebrew application for the Wii U GamePad (DRC/VPAD). It provides full-screen solid-colour display tests and a graphical interface for testing buttons, sticks, touch input, and motion sensors.

### Features

#### Screen Test

Displays full-screen solid colours to help identify dead pixels, stuck pixels, colour problems, and uneven backlighting.

- Colour sequence: black, white, red, green, blue, cyan, magenta, yellow, and gray
- The first black screen briefly displays the controls before returning to pure black
- Press `A` to display the next colour
- Press `B` to return to the main menu

Apart from the brief instruction on the first black screen, no text is drawn over the test colours.

#### Key / Sensor Test

The GamePad display uses a hardware image with live input overlays positioned over the corresponding controls:

- Tests A/B/X/Y, D-pad, L/R, ZL/ZR, +/-, HOME, TV, and both stick buttons
- A pressed button receives a translucent red overlay while retaining its original texture and label
- Blue indicators show the live positions of both analog sticks
- Touch input is displayed inside the central screen area
- Shows X/Y/Z gyroscope and accelerometer readings
- Shows battery status and volume-slider values

Hold `+` and `-` together to return to the main menu.

### Building

[devkitPro](https://devkitpro.org/wiki/Getting_Started) with devkitPPC and wut is required.

```sh
make
```

Build with Docker from PowerShell:

```powershell
docker run --rm -v "${PWD}:/workspace" -w /workspace devkitpro/devkitppc:20250102 make
```

Build with Docker from a Linux/macOS shell:

```sh
docker run --rm -v "$PWD:/workspace" -w /workspace \
  devkitpro/devkitppc:20250102 make
```

### Installation

Copy the generated `WiiUDrcTest.wuhb` to the SD card:

```text
sd:/wiiu/apps/WiiUDrcTest/WiiUDrcTest.wuhb
```

Launch it from the Wii U Menu under Aroma Environment or another compatible homebrew launcher.

## Disclaimer / 免责声明

This is an unofficial homebrew project and is not affiliated with or endorsed by Nintendo. Wii U and Wii U GamePad are trademarks of Nintendo. All referenced trademarks belong to their respective owners.

这是一个非官方 homebrew 项目，与任天堂没有从属关系，也未获得任天堂认可。Wii U、Wii U GamePad 及相关商标归其各自权利人所有。
