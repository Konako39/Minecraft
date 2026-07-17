# 12 — 文字渲染：让世界开口说话

**米雅**

考主人一个冷知识：OpenGL 会画字吗？

**主人**

……这么问，答案肯定是「不会」。

**米雅**

对，OpenGL 眼里只有三角形和贴图，**根本没有「文字」这个概念**。

您在所有游戏里见过的每一个字，都是引擎自己想办法画出来的。

而我们要用的办法，是游戏界祖传的**位图字体（bitmap font）**：

> 把 95 个 ASCII 字符预先画在**一张图**上，排成网格。
> 想显示 "FPS"，就查出 F、P、S 三个格子，贴三个小矩形。

**主人**

等等，这不就是……第 06 章的图集 + 第 09 章的 texRect？

**米雅**

主人已经悟了喵。**字体就是字符的图集**，文字渲染就是连着画一排贴图矩形。

我们已有的系统一行都不用改，只要加一个「查格子」的封装。

## 本章动手地图

| 步骤 | 为了…… | 请您现在做 |
|------|--------|------------|
| A | 有一张字符图集 | 确认 `assets/textures/font.png`（米雅已备好） |
| B | 会画一行字 | **新建** `src/render/font.hpp`（`Font` 结构体） |
| C | 屏幕上有点有用的信息 | **改** `main.cpp`：左上角 FPS + 坐标（MC 的 F3 既视感） |

---

## 1. 字体图长什么样

`assets/textures/font.png` 是一张 256×96 的图：

- **16 列 × 6 行** = 96 个格子，每格 **16×16 像素**
- 按 ASCII 码从 32（空格）排到 127，白字透明底
- 第 0 格是空格，`'A'`(65) 在第 65−32 = 33 格 → 第 2 行第 1 列

**主人**

为什么白字？游戏里的字有各种颜色啊。

**米雅**

复习第 09 章的白贴图技巧喵：UI shader 输出 = 贴图色 × 顶点色。

白色 (1,1,1) 乘什么色就变什么色——一张白字图，染出万紫千红。

> 汉字呢？汉字有几万个，位图字体装不下，正经做法是 FreeType 那类「按需光栅化」库。我们的菜单和 HUD 用英文+数字完全够（原版 MC 界面也是英文），汉字留作第 17 章的选修喵。

---

## 2. 为了会写字：新建 font.hpp

**新建** `src/render/font.hpp`：

```cpp
#pragma once
#include "ui.hpp"
#include "texture.hpp"
#include <string>

// 用字体图集画ASCII文字。图是16列×6行，字符32~127，每格16×16像素
struct Font {
	GLuint tex = 0;
	static constexpr int COLS = 16, ROWS = 6, FIRST = 32;

	bool init() {
		tex = loadTexture("assets/textures/font.png");
		return tex != 0;
	}

	//一行字有多宽？(排版用：居中、右对齐都靠它)
	static float measure(float size, const std::string& text) {
		return size * 0.6f * (float)text.size();
	}

	//画一行字。x,y是左上角(像素)，size是单个字的显示高度(像素)
	void draw(UIRenderer& ui, float x, float y, float size, const std::string& text,
	          glm::vec4 color = { 1,1,1,1 }) {
		float advance = size * 0.6f;   //字距：等宽字体宽约是高的0.6倍
		for (char ch : text) {
			int c = (unsigned char)ch;
			if (c < FIRST || c >= FIRST + COLS * ROWS) { x += advance; continue; }
			int idx = c - FIRST;
			int col = idx % COLS, row = idx / COLS;
			float du = 1.f / COLS, dv = 1.f / ROWS;
			float u0 = col * du, u1 = u0 + du;
			//行从图的顶部往下数；加载时上下翻转过，第0行的v在最上面
			float v1 = 1.f - row * dv, v0 = v1 - dv;
			ui.texRect(x, y, size, size, tex, u0, v0, u1, v1, color);
			x += advance;
		}
	}
};
```

逐段拆解：

1. **`idx = c - FIRST`** → 字符码转格子号；再 `% COLS` / `/ COLS` 拆成列和行——**和第 06 章 `tileUV` 一模一样的算术**，第三次见了，应该已经像呼吸一样自然喵；
2. **v 的方向**：又是老朋友「贴图上下翻转」。字体图第 0 行在图的**顶部**，翻转加载后顶部对应 v≈1，所以 `v1 = 1 - row*dv` 从上往下数；
3. **`advance = size * 0.6`**：等宽字体的字宽约为字高的 0.6 倍。每画完一个字，画笔右移一格——这就是「排版」的全部秘密；
4. **`measure`**：要把文字**居中**（比如按钮上的字），得先知道整行多宽。宽度 = 字距 × 字数，小学乘法。下一章的按钮全靠它。

**主人**

不认识的字符就 `continue`，跳过但照样右移——为什么不直接不移？

**米雅**

细节品味不错喵。跳过不移的话，一个奇怪字符会让后面所有字**挤位**。

宁可留个空窗，版面不乱。防御性编程的小习惯。

---

## 3. 为了有用：在 main.cpp 画调试 HUD

**① include**（`ui.hpp` 后面）：

```cpp
#include "render/font.hpp"
```

顶部还要补一个（拼字符串用）：

```cpp
#include <cstdio>//snprintf拼调试文字用
```

**② 初始化**：把 UI 那段改成三兄弟一起：

```cpp
//UI绘制器、字体和方块描边框
UIRenderer ui;
Font font;
OutlineRenderer outline;
if (!ui.init() || !font.init() || !outline.init()) {
    std::cerr << "Failed to init UI/font/outline\n";
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
}
```

**③ 主循环 UI 段**：画完准星那两个矩形之后、`ui.end()` 之前，加：

```cpp
//左上角调试信息（每0.5秒刷新一次FPS，太快了看不清）
static float fpsTimer = 0.f; static int fpsFrames = 0, fpsShow = 0;
fpsTimer += dt; fpsFrames++;
if (fpsTimer >= 0.5f) { fpsShow = (int)(fpsFrames / fpsTimer); fpsTimer = 0.f; fpsFrames = 0; }
char dbg[128];
snprintf(dbg, sizeof(dbg), "FPS %d  XYZ %.1f / %.1f / %.1f%s",
         fpsShow, gPlayer.pos.x, gPlayer.pos.y, gPlayer.pos.z,
         gPlayer.flying ? "  [FLY]" : "");
font.draw(ui, 8, 8, 16, dbg);
```

两个小讲究：

- **FPS 要攒着算**：每帧的 `1/dt` 抖得没法看。攒半秒的帧数除以时间，稳定又准确；
- **`snprintf`**：C 的字符串格式化，`%.1f` 保留一位小数。比 `std::to_string` 拼接干净得多，HUD 这种「拼一行状态」的场景它最顺手。

构建运行！左上角：`FPS 144  XYZ 8.0 / 58.0 / 8.0`。

**主人**

有 F3 内味了！……话说这次居然没有妖怪？

**米雅**

嘘——它已经在您没看见的地方被打过了喵。

要是 `v1`/`v0` 上下写反，屏幕上会出现一排**上下颠倒的鬼画符**——

「乱码妖」的招牌绝技。我们在第 06 章就跟贴图翻转打过照面，

这次直接按正确方向写了。老怪重刷新图，一刀秒杀，不值得开打戏喵。

（真遇到了就检查两处：`stbi_set_flip_vertically_on_load` 有没有开、`v1 = 1 - row*dv` 的方向。）

---

## 概念小抄

| 词 | 人话 |
|----|------|
| 位图字体 | 字符预先画进一张图，显示 = 贴小矩形 |
| advance 字距 | 画完一个字，画笔往右挪多少 |
| measure | 算整行宽度，居中排版的前提 |
| snprintf | C 风格「安全地把数字拼进字符串」 |
| FPS 平滑 | 攒半秒算平均，别用单帧 1/dt |

---

## 本章检查点

- [ ] 左上角实时显示 FPS 和坐标，字迹清晰锐利
- [ ] 按 F 切飞行，`[FLY]` 标签出现/消失
- [ ] 走到负坐标区，坐标正常显示负数
- [ ] 能口算出 `'Z'`(90) 在字体图的第几行第几列（(90−32)=58 → 第 3 行第 10 列）

**米雅**

会写字、会画矩形、会贴图标……积木齐了。

下一章拼一个大件：**快捷栏 + 创造背包**，右键终于不用只放石头了喵。

→ [13-hotbar-and-inventory.md](13-hotbar-and-inventory.md)
