# 02 — Shader、VAO 与立方体

喵呜～主人，上一章只有 `glClear`（整屏涂色）。这一章要第一次真正往屏幕上**画几何体**：一个彩色立方体，并绕轴旋转。

本女仆保证：名词保留专业写法，并补「说人话」；更重要的是——**每一步都会写清：文件建在哪、代码插在 `main.cpp` 的哪个位置**。

---

## 本章动手地图（先看这张！）

请对照您现在的工程。本章大致会碰到这些东西：


| 目的               | 您要做什么                                                   | 位置                                                  |
| ---------------- | ------------------------------------------------------- | --------------------------------------------------- |
| 用矩阵算旋转/透视        | 把 GLM 头文件放进工程                                           | 从 `外部/glm-master/glm` 拷到 `include/glm/`（整份 glm 文件夹） |
| 让编译器找到 GLM       | 一般不用改 CMake（已有 `include/`）；确认能 `#include <glm/glm.hpp>` | `CMakeLists.txt` 已指向 `include/` 即可                  |
| 写 GPU 小程序        | **新建文件夹和两个文本文件**                                        | `shaders/basic.vert`、`shaders/basic.frag`           |
| 在 C++ 里编译 Shader | **新建**加载工具（推荐），或先全写在 `main.cpp`                         | 推荐：`src/shader.hpp`（本章可先单头文件）                       |
| 把立方体数据交给 GPU     | 在 `main.cpp` 里，**GLAD 初始化成功之后、主循环之前**                   | `src/main.cpp`                                      |
| 每帧旋转并画出来         | 在 `main.cpp` 主循环里，`**glClear` 之后、`SwapBuffers` 之前**     | `src/main.cpp`                                      |
| 深度遮挡正确           | 初始化阶段开深度测试；清屏时清深度                                       | `src/main.cpp`                                      |


推荐阅读顺序：**§0 术语 → §1 准备 GLM → §2 创建 Shader 文件 → §3 在 main 里加载 → §4 顶点/VAO → §5 主循环绘制**。

> 新手友好约定：本章可以先**不拆很多 `.cpp`**，核心流程都在 `src/main.cpp`；Shader 文本单独放 `shaders/`。下一章再慢慢拆类。

---

## 0. 本章会出现的新词（先混个脸熟）


| 词                          | 人话（女仆翻译）                                         |
| -------------------------- | ------------------------------------------------ |
| **顶点（Vertex）**             | 三角形的角点；至少有 XYZ 位置，还可以带颜色、UV、法线                   |
| **Shader（着色器）**            | 跑在 **GPU** 上的小程序。不是 C++ 普通函数——是显卡每帧帮您跑的「化妆剧本」    |
| **顶点着色器（Vertex Shader）**   | 每个顶点执行一次：决定「这个角点最终出现在屏幕哪」                        |
| **片元着色器（Fragment Shader）** | 大致每个像素执行一次：决定「这个点涂什么颜色」                          |
| **GLSL**                   | 写 Shader 用的语言，长得像 C，但跑在 GPU 上，不是直接当 `.cpp` 编译的   |
| **VBO**                    | 显存里的「顶点数组」                                       |
| **EBO / IBO**              | 索引缓冲：用编号复用顶点                                     |
| **VAO**                    | 记住「属性怎么从 VBO 里解读」的配置卡（说明书）                       |
| **顶点属性（Attribute）**        | 顶点一栏数据：位置、颜色…要和 Shader 的 `layout(location=?)` 对上 |
| **MVP**                    | Model × View × Projection，把 3D 变到屏幕能认的坐标         |
| **深度测试**                   | 近的挡住远的                                           |


---

## 1. 为了算旋转和透视：先把 GLM 放进工程

**为了**能在 C++ 里写 `glm::mat4`、`glm::rotate`、`glm::perspective`，  
**请主人现在：**

1. 打开备份目录：`外部/glm-master/`
2. 把里面的整个 `**glm` 文件夹**（注意是叫 `glm` 的那一层，里面有 `glm.hpp`）
  **复制到**：`include/glm/`
3. 复制后工程应有：`include/glm/glm.hpp`

然后在 `src/main.cpp` **文件最上面的 `#include` 区**（`#include <iostream>` 附近）加入：

```cpp
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
```

您的 `CMakeLists.txt` 已有：

```cmake
target_include_directories(openglmc PRIVATE ${CMAKE_SOURCE_DIR}/include)
```

所以一般**不用再改 CMake**。改完后重新配置/生成一次即可。

---

## 2. 为了让 GPU 会画颜色：在项目里创建 Shader 文件

**为了**让 GPU 知道「顶点怎么变到屏幕上、像素涂什么色」，  
**请主人现在新建文件夹和文件**（不是改 `main.cpp`）：

```text
OPENGLMC/
└── shaders/              ← 新建这个文件夹
    ├── basic.vert        ← 新建：顶点着色器（GLSL）
    └── basic.frag        ← 新建：片元着色器（GLSL）
```

### 2.1 写入 `shaders/basic.vert`（整文件就是这些内容）

```glsl
#version 330 core
layout (location = 0) in vec3 aPos;    // 属性 0：位置
layout (location = 1) in vec3 aColor;  // 属性 1：颜色

uniform mat4 uMVP;
out vec3 vColor;

void main() {
    vColor = aColor;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
```

### 2.2 写入 `shaders/basic.frag`（整文件就是这些内容）

```glsl
#version 330 core
in vec3 vColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vColor, 1.0);
}
```

人话：这两份**不是** C++，是 GLSL；C++ 负责读进来交给 OpenGL 编译。

---

## 3. 为了在 C++ 里用上 Shader：加一个小加载工具

**为了**不必每次在 `main` 里堆一长串 `glCreateShader`，  
**推荐新建**：`src/shader.hpp`（本章先用单头文件，省事）。

内容可以是（入门版）：读两个文件 → 编译 → 链接 → 返回 `program` ID；并提供 `setMat4`。

> 若主人暂时不想拆文件：也可以把「读文件 + 编译链接」函数直接写在 `main.cpp` 里、`main()` 上方。效果一样，只是 `main.cpp` 会变长。

`**main.cpp` 里怎么接到：**

1. 顶部 `#include "shader.hpp"`（如果拆了文件）
2. 在 `**gladLoadGLLoader` 成功之后**、`**while` 主循环之前**，加类似：

```cpp
// —— 位置：GLAD 成功之后，创建 VAO 之前（或紧挨着）——
unsigned int program = loadShaderProgram("shaders/basic.vert", "shaders/basic.frag");
```

工作目录注意：用 VS 运行时，当前目录常常是 `out/build/...`，相对路径 `shaders/` 可能找不到。可选处理：

- 把 `shaders` 复制到可执行文件旁边；或
- CMake 加一句拷贝；或
- 暂时写成绝对路径做调试（确认能跑后再改回相对路径）。

**务必打印 compile/link 日志**——写错一个分号就会黑屏。

---

## 4. 为了把立方体交给 GPU：在 `main.cpp` 初始化段创建 VAO/VBO

**为了**让 GPU 拿到顶点数据，并知道「怎么解读属性」，  
**请在 `src/main.cpp` 的这里插入：**

```text
gladLoadGLLoader(...) 成功
        ↓
   【在这里】准备 vertices / indices，创建 VAO、VBO、EBO，配置属性
        ↓
while (!glfwWindowShouldClose(window)) { ... }
```

也就是：**主循环外面、只做一次**。

### 4.1 顶点数据：可写在 `main()` 里上述位置（局部数组也可）

先搞懂一行数字是什么：

```text
-0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 0.0f
 │      │       │        │     │     │
 x      y       z        R     G     B
 位置（世界/模型坐标）      颜色（0~1）
```

- 立方体边长 1、中心在原点 → 每个轴从 `-0.5` 到 `+0.5`
- 所以 `-0.5f,-0.5f,0.5f` = **左下角、靠前那一面**的那个顶点
- `1,0,0` = 纯红

入门用「**36 个顶点、无索引**」：6 面 × 2 三角 × 3 点，用 `glDrawArrays`，不必建 EBO。

完整数组已写在工程的 `src/main.cpp` 里；六面颜色分别为：前红、后绿、左蓝、右黄、下紫、上青。

### 4.2 紧接着创建缓冲（仍在主循环之前）——逐行人话版

主人工程里**现在用的是无索引版**（只有 VAO + VBO，没有 EBO）。先建立一个画面：

```text
CPU 内存里的 vertices[]     →（拷贝）→     GPU 显存里的 VBO（真正的数据）
                                              ↑
VAO = 一张「说明书」：告诉 GPU
  · 数据在哪个 VBO 里
  · 每 6 个 float 是一个点
  · 前 3 个是位置(属性0)，后 3 个是颜色(属性1)
```

画的时候只要：`绑回这张说明书(VAO)` → `glDrawArrays`，GPU 就按说明书去读。

对应您 `main.cpp` 里的代码（**没有 ebo**，和以前教程里带索引的片段不一样，以您工程为准）：

```cpp
GLuint vao = 0, vbo = 0;
```


| 名字       | 是什么                      | 人话           |
| -------- | ------------------------ | ------------ |
| `GLuint` | OpenGL 用的无符号整数类型         | 用来装「对象编号/句柄」 |
| `vao`    | Vertex Array Object 的编号  | **说明书**的编号   |
| `vbo`    | Vertex Buffer Object 的编号 | **数据箱**的编号   |


```cpp
glGenVertexArrays(1, &vao);   // 向 OpenGL 申请 1 个 VAO，编号写进 vao
glBindVertexArray(vao);       // 「接下来我要布置这张说明书」——变成当前 VAO
```

**绑定（bind）**= 选中某个对象，后面的设置默认记到它头上。先绑 VAO，后面属性配置才会记进这张说明书。

```cpp
glGenBuffers(1, &vbo);                        // 申请 1 个缓冲对象编号
glBindBuffer(GL_ARRAY_BUFFER, vbo);           // 把它当成「顶点数组缓冲」绑上
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
//            ↑绑在哪一类槽位   ↑多大      ↑CPU数据从哪来  ↑提示：很少改，一次上传
```

这一步才是：**把 `vertices` 从 CPU 拷到 GPU**。拷完后，重要数据在显存里；每帧不用再传整表。

`GL_STATIC_DRAW`：提示驱动「我不怎么改这份数据，你按这个优化」。

然后贴说明书（属性解读）——**最重要的四行**：

```cpp
// 属性 0 = 位置：从每个顶点开头取 3 个 float
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);  // 打开属性槽 0，否则 Shader 读不到

// 属性 1 = 颜色：跳过前 3 个 float，再取 3 个
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
glEnableVertexAttribArray(1);
```

对着您数组里的一行想：

```text
-0.5f, -0.5f, 0.5f,  1.0f, 0.0f, 0.0f
└── 属性0 位置 ──┘  └── 属性1 颜色 ──┘
←—————— 一个顶点共 6 个 float（步长）——————→
```


| 参数                           | 在说什么                                           |
| ---------------------------- | ---------------------------------------------- |
| 第一个 `0` / `1`                | 属性槽编号，必须和 Shader 里 `layout(location = 0/1)` 对上 |
| `3`                          | 这个属性有几个数（xyz 或 rgb）                            |
| `GL_FLOAT`                   | 每个数是 float                                     |
| `GL_FALSE`                   | 不要额外把整数压成 0~1（浮点一般 false）                      |
| `6 * sizeof(float)`          | **步长**：走完一个顶点要跨多少字节，才能到下一个顶点                   |
| `(void*)0`                   | 位置从顶点开头偏移 0 开始读                                |
| `(void*)(3 * sizeof(float))` | 颜色从跳过 3 个 float 之后开始读                          |


`glEnableVertexAttribArray(n)`：光配置不够，还要**打开开关**，不然 location n 是关着的。

```cpp
glBindVertexArray(0);  // 解绑：结束布置（可选）。画的时候再 Bind 回来
glEnable(GL_DEPTH_TEST); // 开启深度测试：近的挡住远的
```

**关于教程里曾出现的 EBO：**  
若用索引表 `indices[]`，才会多：

```cpp
glGenBuffers(1, &ebo);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
glBufferData(..., indices, ...);
// 绘制改用 glDrawElements
```

主人现在是 `glDrawArrays(GL_TRIANGLES, 0, 36)`，**可以当 EBO 不存在**，先别被它绕晕。

记忆口诀：**先绑 VAO → 再造 VBO 并上传数据 → 再写属性指针并 Enable → 画之前再绑回 VAO。**

---

## 5. 为了每帧真的画出来：改主循环

您现在的主循环大概是：

```cpp
while (!glfwWindowShouldClose(window)) {
    // ESC ...
    glClearColor(...);
    glClear(GL_COLOR_BUFFER_BIT);   // ← 只清颜色
    glfwSwapBuffers(window);
    glfwPollEvents();
}
```

**为了**立方体能转、有远近遮挡，**请改成下面结构**（仍在同一个 `while` 里）：

```cpp
while (!glfwWindowShouldClose(window)) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // ← 加上深度

    // —— 【在这里】算 MVP、使用 Shader、绑 VAO、绘制 ——
    glm::mat4 model = glm::rotate(glm::mat4(1.f), (float)glfwGetTime(),
                                  glm::vec3(0.5f, 1.f, 0.f));
    glm::mat4 view  = glm::lookAt(glm::vec3(2,2,2), glm::vec3(0,0,0), glm::vec3(0,1,0));
    glm::mat4 proj  = glm::perspective(glm::radians(70.f), 1280.f/720.f, 0.1f, 100.f);
    glm::mat4 mvp   = proj * view * model;

    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "uMVP"),
                       1, GL_FALSE, glm::value_ptr(mvp));
    // 若用了 shader.hpp 的封装：shader.setMat4("uMVP", mvp);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);

    glfwSwapBuffers(window);
    glfwPollEvents();
}
```


| 矩阵        | 人话                  |
| --------- | ------------------- |
| **model** | 物体自己怎么转             |
| **view**  | 相机在哪（本章写死；下一章 WASD） |
| **proj**  | 透视：远小近大             |


`indexCount` = 索引个数（例如每个面 6 个索引 × 6 面 = 36）。

程序结束前（`glfwDestroyWindow` 附近）可以：

```cpp
glDeleteVertexArrays(1, &vao);
glDeleteBuffers(1, &vbo);
glDeleteBuffers(1, &ebo);
glDeleteProgram(program);
```

---

## 6. `main.cpp` 变更顺序总览（对照用）

把整份 `main` 想成五段；本章主要动 ①②④：

```text
① #include 区
   + glm 三个头
   +（可选）"shader.hpp"

② main() 开头到 glad 成功
   （保持上一章）

③【新增·只做一次】
   loadShaderProgram(...)
   准备 vertices / indices
   创建 VAO/VBO/EBO + 属性指针
   glEnable(GL_DEPTH_TEST)

④ while 主循环
   glClear 加上 DEPTH
   算 MVP → useProgram → bind VAO → draw
   SwapBuffers / PollEvents

⑤ 退出清理
   Delete* + DestroyWindow + Terminate
```

---

## 7. 稍后可以封装（本章不强制）

等立方体转起来了，再考虑：

```text
src/shader.hpp / shader.cpp   — 加载、setMat4
src/mesh.hpp                  — VAO + draw()
```

后面 Chunk 会大量复用同一套 Shader，只换顶点数据。

## 本章检查点

- [x] `include/glm/` 已放好，能编过带 GLM 的 `#include`
- [x] 存在 `shaders/basic.vert` 与 `shaders/basic.frag`
- [x] 能看到旋转的彩色立方体  
- [x] 近处遮挡正确（深度缓冲生效）  
- [x] 知道：Shader 文件在 `shaders/`；VAO 创建在主循环外；绘制在主循环内  

下一章：[03-camera.md](03-camera.md) — 第一人称自由观察（主要改 `main.cpp` 里相机相关的那几处）。主人一步步来，本女仆陪着喵♡