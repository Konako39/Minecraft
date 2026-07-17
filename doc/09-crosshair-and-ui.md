# 09 — 准星、选中框与 UI 渲染基础

**米雅**

主人，上一章您挖方块的时候，是不是一直在「盲狙」？

**主人**

……是。屏幕中央没有准星，我只能凭感觉猜正中在哪。

而且瞄没瞄到、瞄到的是哪一块，全靠挖下去才知道。

**米雅**

这一章就治这两个毛病：

1. 屏幕正中画一个**十字准星**；
2. 给瞄准的方块画一圈**黑色描边框**（MC 那个经典的黑线框）。

顺便，这章要建的 `UIRenderer` 是个**战略级投资**——

以后的血条、快捷栏、背包、主菜单、按钮、文字，**全部**用它来画喵。

## 本章动手地图

| 步骤 | 为了…… | 请您现在做 |
|------|--------|------------|
| A | 在屏幕上按像素画矩形 | **新建** `shaders/ui.vert` / `ui.frag` |
| B | 封装好用的 2D 绘制器 | **新建** `src/render/ui.hpp`（`UIRenderer`） |
| C | 瞄准的方块有描边 | **新建** `src/render/outline.hpp`（复用 basic shader！） |
| D | 用上它们 | **改** `main.cpp`：初始化 + 主循环末尾绘制 |

---

## 1. 先搞懂：UI 和 3D 画法有什么不同（只阅读）

**米雅**

考主人一个问题：准星要一直显示在屏幕正中。

用我们现有的 3D 那套（MVP 矩阵）画它，行不行？

**主人**

呃……理论上可以算一个「永远在相机正前方」的位置……但听着就别扭。

**米雅**

对，别扭就不要做。UI 的本质是：**直接往屏幕上画，跟 3D 世界无关**。

所以三个规矩：

1. **不用 MVP**。UI 用「像素坐标」：左上角 (0,0)，右下角 (宽,高)——跟美工的习惯一致；
2. **关掉深度测试**。UI 永远压在 3D 画面上面，不参加「谁挡谁」的比赛；
3. **开混合（blend）**。UI 经常半透明（暗一点的背景板、80% 不透明的准星）。

但 GPU 只认 NDC 坐标（复习第 02 章：屏幕中心 (0,0)、四角 ±1）。

所以 UI 顶点着色器的全部工作，就是把像素坐标换算成 NDC：

```
ndc.x = 像素x / 屏幕宽 × 2 − 1
ndc.y = 1 − 像素y / 屏幕高 × 2      ← 注意翻转！
```

**主人**

为什么 y 要翻？

**米雅**

屏幕坐标 y **向下**长（第 0 行在顶上），NDC 的 y **向上**长。

两个世界的上下是反的，一个减法搞定。记住这件事，

第 13 章鼠标点背包格子的时候，还有一只妖怪会拿它做文章喵。

---

## 2. 为了像素画矩形：ui.vert / ui.frag

**新建** `shaders/ui.vert`：

```glsl
//UI顶点着色器：像素坐标 → NDC
#version 330 core
layout (location = 0) in vec2 aPos;    // 像素坐标，原点在屏幕左上
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec4 aColor;
uniform vec2 uScreen;                  // 屏幕宽高（像素）
out vec2 vUV;
out vec4 vColor;
void main() {
    vUV = aUV;
    vColor = aColor;
    float x = aPos.x / uScreen.x * 2.0 - 1.0;
    float y = 1.0 - aPos.y / uScreen.y * 2.0;   // 屏幕y向下，NDC的y向上，要翻
    gl_Position = vec4(x, y, 0.0, 1.0);
}
```

**新建** `shaders/ui.frag`：

```glsl
//UI片元着色器：贴图颜色 × 顶点颜色
#version 330 core
in vec2 vUV;
in vec4 vColor;
out vec4 FragColor;
uniform sampler2D uTex;
void main() {
    FragColor = texture(uTex, vUV) * vColor;
}
```

**主人**

为什么 UI 顶点连 UV 和颜色都要带？准星不就是俩白矩形吗。

**米雅**

投资未来喵。这套顶点格式（位置+UV+颜色）能画：

- **纯色矩形**：贴一张 1×1 的纯白图，颜色全靠顶点色 —— 准星、背景板、按钮
- **贴图矩形**：贴图集 → 背包里的方块图标；贴字体图 → 所有文字

一套 shader 通吃整个 UI 系统。「贴白图画纯色」是老牌图形技巧，学走不亏。

---

## 3. 为了好用：封装 UIRenderer

**新建** `src/render/ui.hpp`。结构和第 04 章 Chunk 的 VAO/VBO 套路完全一样（复习：VAO 存「顶点怎么解读」，VBO 存数据），只是顶点每帧都变，所以用 `GL_DYNAMIC_DRAW`：

```cpp
#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "../shader.hpp"

// 极简2D绘制器：以像素为单位往屏幕上画矩形（纯色或贴图）。
// 用法：ui.begin(宽,高) → ui.rect(...)/ui.texRect(...) 若干 → ui.end()
struct UIRenderer {
	GLuint vao = 0, vbo = 0, prog = 0, whiteTex = 0;
	glm::vec2 screen{ 0,0 };

	bool init() {
		prog = loadShaderProgram("shaders/ui.vert", "shaders/ui.frag");
		if (!prog) return false;

		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		//每顶点8个float：x,y, u,v, r,g,b,a
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glBindVertexArray(0);

		//1x1的纯白贴图：画纯色矩形时就贴它（颜色全靠顶点色）
		unsigned char px[4] = { 255,255,255,255 };
		glGenTextures(1, &whiteTex);
		glBindTexture(GL_TEXTURE_2D, whiteTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		return true;
	}

	//开始画UI：关深度(UI永远在最上层)、开混合(半透明)
	void begin(int w, int h) {
		screen = { (float)w, (float)h };
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glUseProgram(prog);
		glUniform2f(glGetUniformLocation(prog, "uScreen"), screen.x, screen.y);
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(prog, "uTex"), 0);
	}

	//画一块贴图矩形：屏幕像素坐标(x,y是左上角)，UV子矩形，tint是染色
	void texRect(float x, float y, float w, float h, GLuint tex,
	             float u0, float v0, float u1, float v1, glm::vec4 c) {
		//屏幕的上边对应贴图v1(上)，下边对应v0(下)
		float v[6][8] = {
			{ x,     y,     u0, v1,  c.r,c.g,c.b,c.a },
			{ x + w, y,     u1, v1,  c.r,c.g,c.b,c.a },
			{ x + w, y + h, u1, v0,  c.r,c.g,c.b,c.a },
			{ x,     y,     u0, v1,  c.r,c.g,c.b,c.a },
			{ x + w, y + h, u1, v0,  c.r,c.g,c.b,c.a },
			{ x,     y + h, u0, v0,  c.r,c.g,c.b,c.a },
		};
		glBindTexture(GL_TEXTURE_2D, tex);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	//画一块纯色矩形
	void rect(float x, float y, float w, float h, glm::vec4 c) {
		texRect(x, y, w, h, whiteTex, 0, 0, 1, 1, c);
	}

	//收工：把3D绘制需要的状态还原
	void end() {
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glBindVertexArray(0);
	}
};
```

**主人**

`begin` / `end` 这对是什么讲究？

**米雅**

OpenGL 是台**状态机**——你拨了开关它就一直保持，直到有人再拨。

UI 要「关深度、开混合」，可下一帧的 3D 世界要「开深度、关混合」。

`begin` 拨过去、`end` 拨回来，谁弄乱谁负责还原，这是状态机世界的公德喵。

哪天您发现「3D 世界突然透明了/UI 被地形挡住了」，八成是有人忘了还原状态。

---

## 4. 为了瞄准描边：outline.hpp（复用旧 shader！）

**米雅**

描边框是**3D 物体**（它要跟着方块走、被正确透视），但它不贴图——

12 条黑线而已。位置 + 颜色……主人想起谁了吗？

**主人**

……第 02 章的 `basic.vert` / `basic.frag`！贴图化之后它们不是失业了吗！

**米雅**

返聘喵！这就是当时说「旧 shader 留着别删」的原因。

**新建** `src/render/outline.hpp`：

```cpp
#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "../shader.hpp"

// 瞄准方块的黑色描边线框。复用第02章的 basic.vert/basic.frag（位置+颜色）。
struct OutlineRenderer {
	GLuint vao = 0, vbo = 0, prog = 0;
	int vertexCount = 0;

	bool init() {
		prog = loadShaderProgram("shaders/basic.vert", "shaders/basic.frag");
		if (!prog) return false;

		//单位立方体稍微放大一点点(防止和方块表面打架闪烁)，12条棱
		const float a = -0.003f, b = 1.003f;
		const float col[3] = { 0.05f, 0.05f, 0.05f };//近黑色
		std::vector<float> v;
		auto edge = [&](float x1, float y1, float z1, float x2, float y2, float z2) {
			v.insert(v.end(), { x1,y1,z1, col[0],col[1],col[2] });
			v.insert(v.end(), { x2,y2,z2, col[0],col[1],col[2] });
		};
		//底面4条
		edge(a,a,a, b,a,a); edge(b,a,a, b,a,b); edge(b,a,b, a,a,b); edge(a,a,b, a,a,a);
		//顶面4条
		edge(a,b,a, b,b,a); edge(b,b,a, b,b,b); edge(b,b,b, a,b,b); edge(a,b,b, a,b,a);
		//竖着4条
		edge(a,a,a, a,b,a); edge(b,a,a, b,b,a); edge(b,a,b, b,b,b); edge(a,a,b, a,b,b);
		vertexCount = (int)v.size() / 6;

		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		glBindVertexArray(0);
		return true;
	}

	//mvp = proj * view * translate(方块坐标)
	void draw(const glm::mat4& mvp) {
		glUseProgram(prog);
		glUniformMatrix4fv(glGetUniformLocation(prog, "uMVP"), 1, GL_FALSE, glm::value_ptr(mvp));
		glBindVertexArray(vao);
		glDrawArrays(GL_LINES, 0, vertexCount);
	}
};
```

三个小心思：

1. **`GL_LINES`**：每 2 个顶点画 1 条线段，不填面。第一次见的绘制模式，除了 `GL_TRIANGLES` 家族还有线家族喵；
2. **±0.003 的放大**：线框如果和方块表面**完全**重合，两者深度值几乎一样，GPU 判不清谁在前——画面会闪烁（术语叫 **Z-fighting**，深度打架）。稍微放大一圈就错开了；
3. **`GL_STATIC_DRAW`**：线框形状永远不变（变的只是 model 矩阵），和 Chunk 的 DYNAMIC 相反——正好复习这两个提示词的区别。

---

## 5. 为了用上：改 main.cpp

**① include**：

```cpp
#include "render/ui.hpp"
#include "render/outline.hpp"
```

**② 初始化**：`World world(2024);` 之前加：

```cpp
//UI绘制器和方块描边框
UIRenderer ui;
OutlineRenderer outline;
if (!ui.init() || !outline.init()) {
    std::cerr << "Failed to init UI/outline\n";
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
}
```

**③ 绘制**：主循环里，画完所有 Chunk 的 `for` 循环**之后**、`glfwSwapBuffers` **之前**，加：

```cpp
//画瞄准方块的描边框
if (hitOk) {
    glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(hit));
    outline.draw(proj * view * model);
}

//画UI：屏幕中央十字准星
ui.begin(fbw, fbh);
float cx = fbw * 0.5f, cy = fbh * 0.5f;
glm::vec4 crossCol{ 1.f, 1.f, 1.f, 0.8f };
ui.rect(cx - 10, cy - 1, 20, 2, crossCol);//横
ui.rect(cx - 1, cy - 10, 2, 20, crossCol);//竖
ui.end();
```

**主人**

描边框的 `model` 就是把线框平移到 `hit` 那格……和 Chunk 的道理一样嘛。

**米雅**

完全一样，一份单位立方体线框，靠矩阵摆到任何被瞄准的方块上。

注意顺序：**先 3D（世界、描边框），后 UI**——UI 关了深度测试，谁后画谁在上面。

构建运行！

**主人**

有准星了！瞄哪一块，哪一块就套上黑框……手感一下子就「MC」了。

**米雅**

「你瞄准的就是你会挖的」，这层确定感就是描边框存在的意义喵。

---

## 概念小抄

| 词 | 人话 |
|----|------|
| 像素坐标 | UI 用的坐标：左上 (0,0)，右下 (宽,高) |
| 混合 blend | 半透明像素和背景按 alpha 加权混色 |
| 状态机 | OpenGL 开关拨了就保持；begin/end 负责拨过去拨回来 |
| 白贴图技巧 | 1×1 白图 + 顶点色 = 纯色矩形，与贴图矩形共用一套 shader |
| GL_LINES | 每 2 个顶点一条线段 |
| Z-fighting | 两个面深度几乎相同来回闪；错开一点点就好 |

---

## 本章检查点

- [ ] 屏幕正中有十字准星，半透明白色
- [ ] 瞄准 8 格内的方块出现黑色线框；瞄天空线框消失
- [ ] 线框不闪烁（±0.003 生效）
- [ ] 说得出为什么 UI 要「后画 + 关深度」

**米雅**

下一章是大工程：给主人一副**身体**——

重力、跳跃、碰撞，从幽灵变成会走路的史蒂夫喵。

→ [10-player-physics.md](10-player-physics.md)
