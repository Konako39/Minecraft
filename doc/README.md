# OpenGL Minecraft 克隆 —— 教程总览

**米雅**（猫娘女仆）

欢迎回来，主人喵。本女仆米雅会陪您用现代 **OpenGL**（3.3 Core）从零手写一款 Minecraft ——

不是「类似」，是奔着 1:1 去的：地形、树、水、光影、昼夜、挖放、背包、主菜单、存档，全部亲手实现。

**主人**（您）

我只会一点 C++ 基础，图形学一窍不通。

**米雅**

正好，教程就是为这样的主人写的。每一步都是「**为了什么 → 所以在哪个文件写什么**」，

跟着走，每一章结束都能编译运行看到新东西。

哦对了——路上会有**妖怪**出没。它们是 bug 的化身：编码怪、撕裂妖、花屏妖、连发妖……

每一只都对应一个真实世界里无数程序员踩过的坑。主人会亲手打败它们喵。

## 这份教程怎么用

每一章的固定结构：

1. **开场对话**：这章要干什么、为什么
2. **动手地图**：一张表写清「为了 XX → 新建/修改哪个文件」
3. **分步小节**：带着写，逐段解释；旧知识点出现时**现场复习**，不叫您翻回去
4. **妖怪战**：本章的经典坑，打过就长记性
5. **概念小抄 + 检查点**：跑通没有，一勾便知

素材（贴图图集、字体图）和第三方库（GLAD/GLFW/GLM/stb）由米雅备好或教您放置，**所有工程代码由主人亲手写**。

## 学习路线

| 章 | 文件 | 内容 | 出场妖怪 |
|----|------|------|----------|
| 00 | [00-overview.md](00-overview.md) | 体素游戏的世界观（不写代码） | — |
| 01 | [01-setup.md](01-setup.md) | 装库、摆文件、天蓝窗口 | — |
| 02 | [02-triangle-and-cube.md](02-triangle-and-cube.md) | Shader、VAO/VBO、旋转立方体 | — |
| 03 | [03-camera.md](03-camera.md) | 第一人称相机，WASD 飞行 | — |
| 04 | [04-voxel-chunk.md](04-voxel-chunk.md) | Chunk 与面剔除网格（引擎心脏） | — |
| 05 | [05-world-and-terrain.md](05-world-and-terrain.md) | Perlin 地形、种树、多区块世界 | 撕裂妖·负负 |
| 06 | [06-textures.md](06-textures.md) | 图集贴图、UV | 黑斑妖·阿尔法 |
| 07 | [07-lighting.md](07-lighting.md) | 按面烘焙光照，立体感 | 花屏妖·斯特莱德 |
| 08 | [08-interaction.md](08-interaction.md) | 射线选取、挖与放 | 连发妖·嗒嗒嗒 |
| 09 | [09-crosshair-and-ui.md](09-crosshair-and-ui.md) | 准星、选中框、UI 渲染器 | — |
| 10 | [10-player-physics.md](10-player-physics.md) | 重力、跳跃、AABB 碰撞 | 穿墙妖·隧穿 |
| 11 | [11-water.md](11-water.md) | 海、沙滩、半透明渲染、游泳 | 幽湖妖·薄透 |
| 12 | [12-text-rendering.md](12-text-rendering.md) | 位图字体、调试 HUD | （乱码妖，秒杀） |
| 13 | [13-hotbar-and-inventory.md](13-hotbar-and-inventory.md) | 快捷栏、创造背包、6 种新方块 | 错位妖·帕拉克斯 |
| 14 | [14-main-menu.md](14-main-menu.md) | 状态机、主菜单、创建世界(种子) | — |
| 15 | [15-save-and-load.md](15-save-and-load.md) | 二进制存档、RLE 压缩、读档 | 字节妖·奥夫赛特 |
| 16 | [16-day-night-and-fog.md](16-day-night-and-fog.md) | 昼夜循环、雾、收官 | 虚无之边·终末 |
| 17 | [17-graduation.md](17-graduation.md) | 毕业式与进阶方向 | — |

## 依赖一览（人话）

| 名字 | 因为…… | 比喻 |
|------|--------|------|
| **CMake** | 告诉编译器怎么编工程 | 菜谱 |
| **VS / 编译器** | 把源码变成 exe | 灶台 |
| **GLFW** | 开窗、键盘鼠标 | 开窗 + 门铃 |
| **OpenGL** | GPU 画图接口（驱动自带） | 画笔 |
| **GLAD** | 把画笔函数接到您代码里 | 转接头 |
| **GLM** | 向量矩阵 | 计算器 |
| **stb_image** | 读 PNG（06 章起） | 读图小工具 |

## 最终目录结构（全部完成后）

```
OPENGLMC/
├── doc/                  # 本教程
├── src/
│   ├── main.cpp          # 入口 + 游戏状态机
│   ├── shader.hpp / camera.hpp / glad.c
│   ├── render/           # texture / ui / font / outline
│   ├── world/            # block / chunk / noise / terrain / world / save
│   └── player/           # player / raycast / inventory
├── shaders/              # basic / chunk / ui
├── assets/textures/      # atlas.png / font.png（已备好）
├── include/              # glad、GLFW、glm、stb_image.h
├── lib/                  # glfw3.lib
└── CMakeLists.txt
```

> **编码提醒**：所有源文件请用 **UTF-8 带 BOM** 保存（VS 默认如此）。用别的编辑器新建文件时尤其注意——中文注释 + 无 BOM 会召唤出没有名字的上古编码怪，症状是莫名其妙的语法错误喵。

准备好了就从 [00-overview.md](00-overview.md) 出发。已经写到一半的主人，直接跳到您的进度继续即可。
