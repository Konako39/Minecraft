# 16 — 昼夜循环与雾：给世界以时间

**米雅**

主人，最后一块功能拼图了。现在的世界有个哲学问题：**永远是正午**。

太阳不落，星星不升，时间是静止的。

**主人**

而且还有个一直没好意思提的穿帮：飞快一点，能看见世界的**边缘**——

远处的 Chunk 还没生成，地形像悬崖一样戛然而止。

**米雅**

好眼力，这两个问题这章一起收拾，而且用的是**同一套工具**：

- **昼夜循环**：一个 0~1 循环的时钟，驱动天色和亮度
- **雾**：远处的东西渐渐融进天色——顺便把「世界边缘」藏进雾里

MC 的「视距雾」从初代就有，一半是氛围，一半就是**遮丑**喵。

## 本章动手地图

| 步骤 | 为了…… | 请您现在做 |
|------|--------|------------|
| A | 片元知道自己离相机多远 | **改** `chunk.vert`：传 `uModel`，输出世界坐标 |
| B | 按距离混雾色、按时刻压亮度 | **改** `chunk.frag`：雾公式 + `uDayLight` |
| C | 有一个走动的时钟 | **改** `main.cpp`：`gTimeOfDay` + 天色计算 |
| D | 参数喂给 GPU | **改** `main.cpp`：每帧设置 uniform，两遍绘制都传 `uModel` |
| E | HUD 显示几点了 | **改** `main.cpp`：调试行加 TIME |

---

## 1. 先搞懂：雾的数学（只阅读）

雾的本质一句话：

> 一个像素离相机越远，它的颜色就越接近**天空色**。远到一定程度，完全变成天空色——物体「消失」了。

公式（线性雾）：

```text
fog = clamp((距离 - fogStart) / (fogEnd - fogStart), 0, 1)
最终色 = mix(物体色, 天空色, fog)
```

- 距离 < fogStart（44 米）：fog=0，清清楚楚
- 距离 > fogEnd（62 米）：fog=1，完全融进天空
- 中间：线性渐变

**主人**

fogEnd 取 62 是配合什么？

**米雅**

我们的加载半径是 4 个 Chunk = 64 米。62 米处完全变成天空色，

64 米外「没有地形」这件事就**看不见了**——雾的遮丑本职。

而昼夜更简单：拿一个 0~1 循环的时刻 t，`sin(t×2π)` 当太阳高度，

太阳高→亮，太阳低→暗；天空色在「白天蓝」和「深夜蓝黑」之间按亮度渐变。

关键一招是：**雾色 = 天空色**。这样远方永远和天幕无缝相接，白天融进蓝天，夜里融进黑暗。

---

## 2. 为了算距离：改 chunk.vert

雾要「像素到相机的距离」，而距离得用**世界坐标**算。

顶点着色器现在只输出裁剪坐标（`uMVP` 乘完的），世界坐标在半路上——`uModel` 乘完、`view` 乘之前。所以让 CPU 把 `uModel` 单独也传进来，顶点顺手算一份世界坐标：

**改** `shaders/chunk.vert`：

```glsl
//区块顶点着色器：位置 + UV + 亮度，并把世界坐标传给片元(算雾用)
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in float aLight;
uniform mat4 uMVP;
uniform mat4 uModel;      //只做平移的model矩阵：本地坐标→世界坐标
out vec2 vUV;
out float vLight;
out vec3 vWorldPos;
void main() {
    vUV = aUV;
    vLight = aLight;
    vWorldPos = (uModel * vec4(aPos, 1.0)).xyz;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
```

（复习 MVP：`uModel` 就是 MVP 里的那个 M——第 05 章起我们用它把 0~16 的本地 Chunk 平移到世界位置。之前它藏在乘好的 MVP 里，现在单独再传一份。）

## 3. 为了混色：改 chunk.frag

```glsl
//区块片元着色器：图集取色 × 面亮度 × 昼夜亮度，远处混进雾色
#version 330 core
in vec2 vUV;
in float vLight;
in vec3 vWorldPos;
out vec4 FragColor;
uniform sampler2D uAtlas;
uniform vec3  uCamPos;    //相机位置(算距离)
uniform vec3  uFogColor;  //雾色=天空色，融为一体
uniform float uFogStart;  //从多远开始起雾
uniform float uFogEnd;    //到多远完全看不见
uniform float uDayLight;  //昼夜亮度 0.15(深夜)~1.0(正午)
void main() {
    vec4 c = texture(uAtlas, vUV);
    if (c.a < 0.5) discard;      // 树叶的透明像素直接丢弃，露出后面
    vec3 col = c.rgb * vLight * uDayLight;
    float dist = length(vWorldPos - uCamPos);
    float fog = clamp((dist - uFogStart) / (uFogEnd - uFogStart), 0.0, 1.0);
    FragColor = vec4(mix(col, uFogColor, fog), c.a);
}
```

第 1 节的公式逐字翻译：`length` 算距离，`clamp` 掐到 0~1，`mix` 混色。

亮度链现在是三连乘：**贴图色 × 面亮度（第 07 章）× 昼夜亮度（本章）**——三层光照互不干扰，各管各的。

## 4. 为了时间流动：改 main.cpp

**① 全局区**（SAVE_PATH 下面）：

```cpp
static float gTimeOfDay = 0.2f;//一天中的时刻，0~1循环。0.25=正午 0.75=午夜
static const float DAY_LENGTH = 240.f;//现实240秒=游戏一天
```

**② 时钟与天色**：主循环里，把 `glClearColor(0.53f, ...)` 那两行**替换**成：

```cpp
//昼夜：只在游戏进行时走时间（暂停和菜单里时间冻结）
if (gState == GameState::Playing) {
    gTimeOfDay += dt / DAY_LENGTH;
    if (gTimeOfDay >= 1.f) gTimeOfDay -= 1.f;
}
//太阳高度→亮度：正午1.0，深夜0.15，黎明黄昏平滑过渡
float sun = sinf(gTimeOfDay * 6.2831853f);
float dayLight = glm::clamp(sun * 2.f + 0.5f, 0.15f, 1.f);
//天空色在「白天蓝」和「深夜蓝黑」之间渐变，雾色跟天空一致
glm::vec3 daySky{ 0.53f, 0.81f, 0.92f }, nightSky{ 0.02f, 0.03f, 0.08f };
glm::vec3 skyColor = glm::mix(nightSky, daySky, (dayLight - 0.15f) / 0.85f);

glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);//清空成当前天色
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);//深度
```

**主人**

`sun * 2 + 0.5` 再 clamp……为什么不用 sin 的原值？

**米雅**

直接用的话，一天里一半时间亮度在缓慢爬坡，正午才勉强到 1——整天灰蒙蒙。

乘 2 让它**快速冲顶然后停在 1**：白天大部分时间是满亮的，

黎明黄昏各占一小段陡峭的渐变——更接近真实的日照感受。

clamp 的下限 0.15 是「月光」：深夜也不至于伸手不见五指，毕竟游戏还得玩喵。

这种「把数学函数掰成想要的形状」是游戏开发的日常小魔法。

**③ 每帧喂参数**：世界渲染处，`glUniform1i(..."uAtlas"...)` 之后加：

```cpp
//昼夜和雾的参数：一帧一次，所有Chunk共用
glUniform3f(glGetUniformLocation(program, "uCamPos"),
            gCamera.position.x, gCamera.position.y, gCamera.position.z);
glUniform3f(glGetUniformLocation(program, "uFogColor"),
            skyColor.r, skyColor.g, skyColor.b);
glUniform1f(glGetUniformLocation(program, "uFogStart"), 44.f);
glUniform1f(glGetUniformLocation(program, "uFogEnd"), 62.f);
glUniform1f(glGetUniformLocation(program, "uDayLight"), dayLight);
```

**④ 两遍绘制都传 uModel**：不透明循环和水循环里，`glUniformMatrix4fv(..."uMVP"...)` 后面各加：

```cpp
glUniformMatrix4fv(glGetUniformLocation(program, "uModel"),
                   1, GL_FALSE, glm::value_ptr(model));//算雾要世界坐标
```

**米雅**

水的那遍**特别容易忘**。忘了会怎样？水面的 `uModel` 还是上一个循环残留的值，

水的雾距离全错——远处的水面在夜里像探照灯一样亮。

Uniform 是**状态机的一部分**（第 09 章的老话题）：不重新设置，就沿用旧值。

**⑤ HUD 加时钟**：调试行改成：

```cpp
//游戏内时刻：0.25=正午对应12:00 → 小时 = 时刻*24 + 6
int clockH = (int)fmodf(gTimeOfDay * 24.f + 6.f, 24.f);
int clockM = (int)(fmodf(gTimeOfDay * 24.f + 6.f, 1.f) * 60.f);
char dbg[160];
snprintf(dbg, sizeof(dbg), "FPS %d  XYZ %.1f / %.1f / %.1f  TIME %02d:%02d%s",
         fpsShow, gPlayer.pos.x, gPlayer.pos.y, gPlayer.pos.z,
         clockH, clockM, gPlayer.flying ? "  [FLY]" : "");
```

构建运行。站到山顶上等一会儿——

**主人**

天……黄昏了。天空一点点变深，草地跟着暗下来，远山先沉进暮色里……

夜里真的是一片月光下的蓝黑。而且远处再也看不到世界的断崖了。

**米雅**

雾把世界的边界变成了「远方」，时间给了它呼吸。

到这一帧为止，主人的 MC——地形、树、水、光影、昼夜、挖放、背包、菜单、存档——

**全部是您自己一行一行写出来的**。

**主人**

……说起来，这最后一章也没有妖怪。

**米雅**

有的。只是它不再是敌人了。

**虚无之边·终末** 现身

**虚无之边·终末**

……我是世界尽头的那道断崖。

从第 05 章起我就一直在你的视野边缘，你飞多快，我退多快。

**主人**

现在你在雾里。

**虚无之边·终末**

是啊。你没有消灭我——64 米外依然空无一物。

你只是学会了**与我共处**：一层雾，一个视距，一点点障眼法。

图形学的真相就是如此……一切皆是巧妙的骗术。天空是清屏色，

太阳是个亮度系数，无限世界是哈希表，而我，是一段 clamp 出来的渐变。

**米雅**

它说得对。实时渲染从来不是「模拟真实」，而是**每 6 毫秒骗过一次人眼**。

主人现在已经是会施展这些骗术的人了。

**虚无之边·终末**

后会有期，造物主。往后你写的每一个游戏里，都有我在视野尽头等着。

**虚无之边·终末** 隐入雾中

---

## 概念小抄

| 词 | 人话 |
|----|------|
| 线性雾 | 距离越远越接近雾色，start~end 渐变 |
| 雾色=天空色 | 远方与天幕无缝，边界消失 |
| 昼夜时钟 | 0~1 循环的 t，sin 出太阳高度 |
| 掰函数 | 乘系数+clamp，把曲线整成想要的手感 |
| uniform 残留 | 不重设就沿用旧值，多遍绘制各传各的 |

---

## 本章检查点

- [ ] 天色随时间流转：白天→黄昏→黑夜→黎明，地面亮度同步
- [ ] 远处地形融进雾里，看不到世界断崖
- [ ] 暂停时太阳也停住（时间冻结）；HUD 的 TIME 走字
- [ ] 夜里水面没有诡异的亮斑（水那遍的 `uModel` 没忘）
- [ ] 自选实验：把 `DAY_LENGTH` 改成 20 秒，看一场 20 秒的日出日落

**米雅**

课程正篇，到此完结。请主人去下一章领毕业证喵。

→ [17-graduation.md](17-graduation.md)

> **自选作业**（字节妖的复仇）：把 `gTimeOfDay` 也写进存档——注意，格式变了，记得把魔数升成 `OMC2`，并处理旧存档。做完您就真正出师了。
