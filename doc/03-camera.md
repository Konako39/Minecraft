# 03 — 第一人称相机

喵～把写死的 `lookAt` 变成「跟着主人走」：WASD + 鼠标，驱动 **view 矩阵**。

## 本章动手地图


| 步骤  | 为了……    | 请您现在做                                                          |
| --- | ------- | -------------------------------------------------------------- |
| A   | 把相机逻辑收拢 | **新建** `src/camera.hpp`                                        |
| B   | 在程序里用上它 | `main.cpp` 顶部 `#include "camera.hpp"`，初始化处声明 `Camera camera`   |
| C   | 鼠标转头    | 窗口创建后：禁用光标 + 注册光标回调                                            |
| D   | WASD 飞  | **主循环开头**读按键，改 `camera.position`（乘 `dt`）                       |
| E   | 画面跟视角走  | **主循环算 MVP 处**：`view` 改成 `camera.view()`，`proj` 用 `camera.fov` |


**几乎不改**第 02 章的 VAO/立方体数据；主要动「输入 + view/proj」。

---

## 1. 为了收拢相机逻辑：新建 src/camera.hpp

**为了**不把 yaw/pitch 散落在 `main` 各处，**请新建文件：** `src/camera.hpp`

写入（可整份粘贴后微调默认位置）：

```cpp
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Camera {
    glm::vec3 position{0.f, 0.f, 3.f}; // 先对着立方体；以后改 64 看地形
    float yaw   = -90.f;
    float pitch = 0.f;
    float speed = 10.f;
    float sensitivity = 0.1f;
    float fov = 70.f;

    glm::vec3 front() const {
        glm::vec3 f;
        f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.y = sin(glm::radians(pitch));
        f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        return glm::normalize(f);
    }

    glm::mat4 view() const {
        return glm::lookAt(position, position + front(), glm::vec3(0, 1, 0));
    }
};
```

人话：**yaw** 左右转头，**pitch** 抬头低头；`view()` 仍内部调用 `lookAt`。

限制 pitch ∈ [-89, 89]，避免翻转。

---



## 2. 为了用上 Camera：改 main.cpp 头部与初始化

**请在** `src/main.cpp`**：**

1. `#include` 区加上：`#include "camera.hpp"`
2. 在 `gladLoadGLLoader` 成功之后（主循环前）加：

```cpp
Camera camera;
float lastTime = (float)glfwGetTime();
float lastX = 640.f, lastY = 360.f; // 大致半个窗口；首次鼠标回调会校正
bool firstMouse = true;
```

若 Camera 要给回调用，可做成 `static Camera*` 或 `glfwSetWindowUserPointer`（入门可先 `static Camera gCamera`）。

---



## 3. 为了鼠标转视角：窗口创建成功后注册回调

**为了**移动鼠标改 yaw/pitch，**请在** `glfwCreateWindow` 成功、`MakeContextCurrent` 附近：

```cpp
glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

glfwSetCursorPosCallback(window, [](GLFWwindow*, double xpos, double ypos) {
    // 这里访问到您的 camera（用 UserPointer 或文件作用域静态变量）
    // firstMouse 时只记录 lastX/Y，不跳视角
    // 否则：
    //   float xoffset = xpos - lastX;
    //   float yoffset = lastY - ypos;  // Y 常反号
    //   lastX = xpos; lastY = ypos;
    //   camera.yaw += xoffset * sensitivity;
    //   camera.pitch = clamp(pitch + yoffset * sens, -89, 89);
});
```

---



## 4. 为了 WASD 飞行：主循环开头加输入

**请插在** `while` 里、`glClear` **之前**（或紧后）：

```cpp
float now = (float)glfwGetTime();
float dt = now - lastTime;
lastTime = now;

glm::vec3 f = camera.front();
glm::vec3 r = glm::normalize(glm::cross(f, glm::vec3(0, 1, 0)));

if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.position += f * camera.speed * dt;
if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.position -= f * camera.speed * dt;
if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.position -= r * camera.speed * dt;
if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.position += r * camera.speed * dt;
if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) camera.position.y += camera.speed * dt;
if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camera.position.y -= camera.speed * dt;
```

**dt**：本帧秒数，乘上后移动不跟帧率绑死。

---



## 5. 为了画面跟着转：改算 MVP 那几行

**找到**第 02 章写死的：

```cpp
glm::mat4 view = glm::lookAt(glm::vec3(2,2,2), ...);
glm::mat4 proj = glm::perspective(..., 70.f, 1280.f/720.f, ...);
```

**改成：**

```cpp
int fbw, fbh;
glfwGetFramebufferSize(window, &fbw, &fbh);
float aspect = fbh ? (float)fbw / (float)fbh : 1.f;

glm::mat4 model = glm::mat4(1.f); // 立方体可先不转，或仍用 rotate
glm::mat4 view  = camera.view();
glm::mat4 proj  = glm::perspective(glm::radians(camera.fov), aspect, 0.1f, 500.f);
glm::mat4 mvp   = proj * view * model;
```

VAO / `glDrawArrays` **不用动**。

---



## main.cpp 变更位置总览

```text
① #include + camera.hpp
② 窗口创建后：CursorDisabled + CursorPosCallback
③ glad 后：Camera camera; lastTime...
④ while 开头：dt + WASD
⑤ while 里算 MVP：用 camera.view() / fov
⑥ 其余绘制逻辑保持第 02 章
```



## 本章检查点

- [ ] 能飞着看立方体  
- [ ] 抬头不翻转  
- [ ] 卡顿时速度仍大致稳定（用了 dt）  

下一章：[04-voxel-chunk.md](04-voxel-chunk.md)