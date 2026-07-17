# 01 — 工程搭建与窗口

喵～目标：装好工具 → 摆好依赖文件 → 写 `main.cpp` → 天空蓝窗口。

## 本章动手地图

| 步骤 | 为了…… | 请您现在做 |
|------|--------|------------|
| A | 能编译 C++ | 装 VS（勾选「使用 C++ 的桌面开发」）+ CMake |
| B | 能开窗读输入 | 把 GLFW 放入 `include/GLFW/`、`lib/` |
| C | 能调 OpenGL 函数 | 把 GLAD 放入 `include/glad/`、`include/KHR/`、`src/glad.c` |
| D | 告诉编译器编谁 | 写根目录 `CMakeLists.txt` |
| E | 真正跑起来 | 写 `src/main.cpp` |

请记住：

> **GLFW** 管窗口/输入，**GLAD** 接通画图函数，驱动真正画；**GLM** 本章可先不用。

---

## 1. 心智模型（不改代码）

```text
C++ 程序 → GLFW(窗) → GLAD(函数地址) → OpenGL/驱动 → 屏幕
```

---

## 2. 为了能编译：安装工具

**为了**把 `.cpp` 变成 exe，**请在系统里：**

1. 安装 [Visual Studio](https://visualstudio.microsoft.com/)，勾选「使用 C++ 的桌面开发」  
2. 安装 [CMake](https://cmake.org/download/)（`.msi` 勾选 PATH，或 ZIP 解压后把 `bin` 加 PATH）

验证（新开终端）：`cmake --version`

---

## 3. 为了开窗口：放入 GLFW

**为了**调用 `glfwCreateWindow`，**请主人现在：**

1. 官网下 Windows 预编译 ZIP（如 `glfw-*.bin.WIN64`）很正常  
2. 从包里取出必要文件到工程：

```text
OPENGLMC/
├── include/GLFW/glfw3.h      ← 从 zip 的 include/GLFW/ 拷来
└── lib/glfw3.lib             ← 从 zip 的 lib-vc2022/ 拷来（VS 新版用 vc2022）
```

完整 zip 可扔进 `外部/` 备份；工程编译只认上面两处。

> 人话：GLFW = 开窗 + 读键盘鼠标，**不画三角形**。

---

## 4. 为了接通 OpenGL：放入 GLAD

**为了**能调用 `glClear` 等，**请主人现在：**

1. 打开 [https://glad.dav1d.de/](https://glad.dav1d.de/)  
2. Language C/C++，gl **3.3**，Profile **Core**，勾选 Generate a loader → 下 zip  
3. 解压后拷到工程：

```text
include/glad/glad.h
include/KHR/khrplatform.h
src/glad.c                 ← 必须参与编译！
```

---

## 5. 为了描述怎么编译：写 CMakeLists.txt

**为了** VS / 命令行知道编哪些文件、链接谁，**请在项目根目录新建/改：** `CMakeLists.txt`

最小骨架（与预编译 GLFW 匹配）：

```cmake
cmake_minimum_required(VERSION 3.20)
project(OPENGLMC LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(openglmc
    src/main.cpp
    src/glad.c
)

target_include_directories(openglmc PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)
target_link_directories(openglmc PRIVATE ${CMAKE_SOURCE_DIR}/lib)
find_package(OpenGL REQUIRED)
target_link_libraries(openglmc PRIVATE glfw3 OpenGL::GL)
```

若新建了更多 `.cpp`，以后都要加进 `add_executable`。

---

## 6. 为了亮起窗口：写 src/main.cpp

**为了**看到天空蓝，**请新建：** `src/main.cpp`

文件顶部：

```cpp
#include <glad/glad.h>   // 必须先于 GLFW
#include <GLFW/glfw3.h>
#include <iostream>
```

`main()` 里推荐顺序（整段可参考教程示例或您已有工程）：

```text
① glfwInit + WindowHint(3.3 Core)
② glfwCreateWindow；失败则 glfwTerminate
③ glfwMakeContextCurrent
④ gladLoadGLLoader —— 必须在 MakeContextCurrent 之后
⑤ while 主循环：
     ESC → 关窗
     glClearColor 天空蓝 + glClear
     SwapBuffers + PollEvents
⑥ DestroyWindow + Terminate
```

窗口缩放回调里：`glViewport(0,0,w,h)`。

---

## 7. 构建与运行

**为了**跑起来：

- 命令行：`cmake -B build -S .` → `cmake --build build`  
- 或 VS：**打开文件夹** → 等 CMake 配置 → 选启动项 `openglmc.exe` → F5  

成功：标题 `OPENGLMC`，背景天蓝，ESC 退出。

---

## 8. 常见问题

| 现象 | 可能原因 |
|------|----------|
| `gladLoadGLLoader` 失败 | 没先 `MakeContextCurrent` |
| 找不到 `glad/glad.h` | include 路径没指到 `include/` |
| 链接找不到 glfw | 没链 `lib/glfw3.lib` |
| VS 只有「选择启动项」 | CMake 没配置出目标 |
| 乱码注释 | 用 UTF-8 保存源文件 |

## 本章检查点

- [ ] 说得出 GLFW / GLAD / OpenGL 分工  
- [ ] 必须编译 `glad.c`  
- [ ] 天蓝窗口 + ESC  

下一章：[02-triangle-and-cube.md](02-triangle-and-cube.md) — 新建 `shaders/`、画立方体。
