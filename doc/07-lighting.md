# 07 — 基础光照：一分钟的立体感

**主人**

贴图是有了，但总觉得……方块像纸糊的贴纸，没有「体积」。

**米雅**

眼光很毒喵。问题在于：**每个面一样亮**。

现实里，朝天的面被太阳直射最亮，侧面斜着受光暗一些，朝地的面基本背光。

人眼就是靠这种明暗差在「读」立体感的。

**主人**

那要算光线和面的夹角？听起来要上向量点积了……

**米雅**

真·光照确实要（第 5 节会给您尝一口）。

但 MC 早期用了一个聪明到近乎偷懒的近似——

> 一个面朝哪个方向，就直接决定它多亮：**顶面 1.0、侧面 0.7~0.8、底面 0.5**。

不用算，查表。零成本，效果立竿见影。这叫把光照**烘焙**进顶点。

## 本章动手地图

| 步骤 | 为了…… | 请您现在做 |
|------|--------|------------|
| A | 每个面带一个亮度值 | **改** `chunk.hpp`：加 `FACE_LIGHT` 表，顶点尾巴加 1 个 float |
| B | 属性指针跟着变 | **改** `upload`：步长 5→6，新增属性 2 |
| C | GPU 把颜色乘上亮度 | **改** `shaders/chunk.vert` / `chunk.frag` |

三处，十分钟的量。但中途会有一只非常经典的妖怪拦路，先剧透名字：**花屏**。

---

## 1. 为了每个面带亮度：改 rebuildMesh

**改** `src/world/chunk.hpp`。第 06 章加 `UVCORNER` 的地方，下面再加一张表：

```cpp
// 按面朝向给固定亮度：上最亮、侧中等、下最暗（顺序和 FACE 一致）
static const float FACE_LIGHT[6] = {
	0.8f, // +X 侧
	0.8f, // -X 侧
	1.0f, // +Y 顶
	0.5f, // -Y 底
	0.7f, // +Z 侧
	0.7f, // -Z 侧
};
```

**米雅**

复习喵：这个 0~5 的顺序哪来的？

**主人**

第 04 章 `FACE` 表：+X、−X、+Y、−Y、+Z、−Z。上一章 `tileOf` 的 face 也是它。

**米雅**

对。同一个 `f` 走天下。

顺便，±X 给 0.8、±Z 给 0.7 是故意**不一样**的——

两组侧面亮度错开，方块的棱角才分明；全给 0.75 反而糊在一起。

然后在 `rebuildMesh` 内层、`push_back` 完两个 UV 之后，补一行：

```cpp
//亮度：按面朝向查表
verts.push_back(FACE_LIGHT[f]);
```

现在每个顶点是 **6 个 float**：`x,y,z, u,v, light`。

---

## 2. 为了属性指针对得上：改 upload

**主人**

（改完 rebuildMesh，兴冲冲按下 F5）

……喂！！画面全花了！三角形满天乱飞！像电视没信号！

**花屏妖·斯特莱德** 登场

**花屏妖·斯特莱德**

哈——哈哈哈哈！又一个只改一半的家伙！

**主人**

你又是谁！

**花屏妖·斯特莱德**

Stride——**步长**！GPU 读顶点数组时「一步迈几个字节」！

你往每个顶点塞了 6 个 float，

可你的 `upload` 还在告诉 GPU「一个顶点 5 个 float」！

于是 GPU 把你的**亮度**当成下一个顶点的 **x 坐标**，

把 x 当 y、把 y 当 z——整锅数据错位，

满屏的疯狂三角形，全、是、我、的、舞、台！

**米雅**

主人别慌，这只妖怪看着凶，其实一句话就能收拾：

> **顶点格式改了，`upload` 里所有 5 都要改成 6，一个都不能漏。**

**改** `upload`，共四处数字 + 新增一个属性：

```cpp
vertexCount = (int)verts.size() / 6;//6个float一个顶点：x,y,z,u,v,light
```

```cpp
// 属性 0：位置(3 个 float)，从每个顶点第 0 个 float 开始
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);

// 属性 1：UV(2 个 float)，偏移3个float
glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
glEnableVertexAttribArray(1);

// 属性 2：亮度(1 个 float)，偏移5个float
glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float)));
glEnableVertexAttribArray(2);
```

**米雅**

念口诀：**步长 = 一个顶点总共几个 float；偏移 = 这个属性从第几个 float 开始**。

位置从 0 起、UV 从 3 起、亮度从 5 起，步长统一 6。

**花屏妖·斯特莱德**

唔啊——数字全对上了——

但我不会消失！只要你以后再改顶点格式——加法线、加动画权重——

只要漏改一个 5 和 6，我就立刻回来开派对！

**花屏妖·斯特莱德** 被击败

**米雅**

它说的是实话，这是图形编程最高频的翻车点之一。

以后看到「满屏乱飞的三角形」，**第一反应就是查 stride 和 offset**。

---

## 3. 为了 GPU 乘上亮度：改 Shader

**改** `shaders/chunk.vert`——收亮度，转交给片元：

```glsl
//区块顶点着色器：位置 + UV + 亮度
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in float aLight;
uniform mat4 uMVP;
out vec2 vUV;
out float vLight;
void main() {
    vUV = aUV;
    vLight = aLight;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
```

**改** `shaders/chunk.frag`——贴图颜色乘亮度：

```glsl
//区块片元着色器：从图集取色并乘亮度
#version 330 core
in vec2 vUV;
in float vLight;
out vec4 FragColor;
uniform sampler2D uAtlas;
void main() {
    vec4 c = texture(uAtlas, vUV);
    if (c.a < 0.5) discard;      // 树叶的透明像素直接丢弃，露出后面
    FragColor = vec4(c.rgb * vLight, c.a);   // 只压暗颜色，不动透明度
}
```

`layout (location = 2)` 对应刚才 `upload` 里的「属性 2」——两边编号必须一致，这是 CPU 和 GPU 之间的接头暗号喵。

`main.cpp` **一行都不用改**。构建运行！

**主人**

哦——！立体了！真的一瞬间就立体了！

**米雅**

顶面亮、侧面中、底面暗，直接骗过人眼。

MC 用几乎为零的计算量换来这个效果，这就是「好的近似」的力量。

---

## 4. 概念小抄

| 词 | 人话 |
|----|------|
| 烘焙 | 提前把亮度算好**写进顶点数据**，运行时不再算 |
| stride 步长 | 顶点数组里「一个顶点占几个字节」 |
| offset 偏移 | 某属性从顶点的第几个字节开始 |
| location | CPU 属性编号和 GPU in 变量的接头暗号 |

## 5. 加餐（可选读）：真正的方向光长什么样

哪天想让太阳「斜着照」，思路是：顶点再带一个**法线**（面朝向的单位向量，`NEIGHBOR` 表现成的就是），片元里算：

```glsl
uniform vec3 uLightDir;                        // 太阳方向，如 (0.4,-1,0.2)
float ndl = max(dot(normalize(vNormal), normalize(-uLightDir)), 0.0);
float light = 0.4 + 0.6 * ndl;                 // 0.4 底光避免全黑
```

`dot`（点积）算的就是「面正对光的程度」。今天的查表版其实是它的离散近似。想动手就当自选题，教程主线不需要它。

---

## 本章检查点

- [ ] 顶面 > ±X 侧 > ±Z 侧 > 底面，亮度四档分明
- [ ] 贴图颜色还在，只是变暗（不是整体变灰）
- [ ] 说得出「满屏乱三角」应该先查什么（斯特莱德的弱点）

**米雅**

看得见、摸不着——现在该让主人**碰**这个世界了。

下一章：挖掉一块、放上一块。→ [08-interaction.md](08-interaction.md)
