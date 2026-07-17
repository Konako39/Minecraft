# 10 — 玩家实体：重力、跳跃与碰撞

**米雅**

主人，现在您在这个世界里是什么？

**主人**

嗯……一台会飞的照相机？

**米雅**

对，一台没有体积的照相机——能钻进石头里、能穿过树干、松开按键就悬停。

这一章给您造一副**身体**：

- 有**大小**（一个 0.6×1.8×0.6 米的隐形箱子）
- 有**重量**（松手会掉下去，落地会停住）
- 有**弹跳**（空格跳 1 格高）
- 还保留**飞行模式**（按 F 切换，毕竟是造物主，特权要留）

**主人**

……听起来要写物理引擎？会不会很难？

**米雅**

MC 级别的物理其实小得可怜喵：不需要旋转、不需要斜面、不需要弹力。

整个物理系统 = **一个箱子 + 一堆格子的重叠测试**。今天一章写完。

## 本章动手地图

| 步骤 | 为了…… | 请您现在做 |
|------|--------|------------|
| A | 玩家有身体、有速度 | **新建** `src/player/player.hpp`：`Player` 结构体 |
| B | 知道「这里站不站得下」 | 同文件：`collides`（AABB vs 方块重叠测试） |
| C | 移动时撞墙会停 | 同文件：`moveAxis`（逐轴小步移动） |
| D | 重力把这一切串起来 | 同文件：`updatePlayer` |
| E | 换掉幽灵飞行 | **改** `main.cpp`：WASD 改成驱动 Player，相机住进眼睛 |
| F | 不能把方块放进自己身体 | **改** `main.cpp` 右键逻辑：放完检查，卡住就反悔 |

---

## 1. 先搞懂：AABB 是什么（只阅读）

**米雅**

**AABB** = Axis-Aligned Bounding Box，「轴对齐包围盒」。

人话：一个**永远不旋转**、棱边平行于坐标轴的长方体箱子。

MC 里的玩家就是一个 0.6 宽、1.8 高的 AABB——你转视角时，身体箱子根本不转，转的只有相机。

为什么执着于「不旋转」？因为轴对齐的箱子判断重叠**只需要比大小**：

> 箱子占据 x ∈ [pos.x−0.3, pos.x+0.3]，y ∈ [pos.y, pos.y+1.8]，z ∈ [pos.z−0.3, pos.z+0.3]。
> 把这段范围向下取整，就得到它**碰到的所有格子**。
> 里面只要有一格是实心 → 重叠了，这里站不下。

方块世界本身就是轴对齐的，AABB 和它是天作之合喵。

**主人**

那「移动撞墙」呢？

**米雅**

用您第 08 章就见过的老朋友——**小步走**。

想移动 0.3 米？拆成 6 小步、每步 5 厘米：走一步，测一次重叠，

撞上了就退回上一步、宣布「此路不通」。和 raycast 一小步步找方块是同一个思想。

还有一个**关键设计**：x、y、z **三个轴分开移动、分开碰撞**。

好处马上就能体会到：往前跑着撞上墙时，x 被墙拦下，但 y 上的重力照常生效——

于是您会**贴着墙滑落**，而不是整个人「粘」在墙上。所有 3D 游戏都这么干。

---

## 2. 为了有身体：新建 player.hpp

**新建** `src/player/player.hpp`，从数据开始：

```cpp
#pragma once
#include "../world/world.hpp"
#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>

// 玩家：一个会受重力、能碰撞的隐形箱子(AABB)
struct Player {
	glm::vec3 pos{ 8.f, 70.f, 8.f };  // 脚底中心
	glm::vec3 vel{ 0.f };             // 速度(米/秒)
	bool onGround = false;            // 脚下踩着东西吗
	bool flying = false;              // 飞行模式(F切换)

	static constexpr float HALF_W = 0.3f;  // 半宽：箱子0.6米宽
	static constexpr float HEIGHT = 1.8f;  // 身高1.8米
	static constexpr float EYE    = 1.62f; // 眼睛离脚底1.62米

	glm::vec3 eyePos() const { return pos + glm::vec3(0.f, EYE, 0.f); }
};
```

**主人**

`pos` 是「脚底中心」而不是箱子正中？

**米雅**

习惯问题，但脚底中心最好用：`pos.y` 就是「站的高度」，

跳跃、落地、找出生点全都直觉。眼睛在脚上方 1.62 米（MC 原版数值），

相机以后就挂在 `eyePos()` 上。

### 2.1 collides：这里站得下吗

```cpp
// 玩家箱子在 pos 时，有没有和实心方块重叠？
inline bool collides(const World& world, const glm::vec3& pos) {
	int x0 = (int)std::floor(pos.x - Player::HALF_W);
	int x1 = (int)std::floor(pos.x + Player::HALF_W);
	int y0 = (int)std::floor(pos.y);
	int y1 = (int)std::floor(pos.y + Player::HEIGHT);
	int z0 = (int)std::floor(pos.z - Player::HALF_W);
	int z1 = (int)std::floor(pos.z + Player::HALF_W);
	for (int y = y0; y <= y1; ++y)
	for (int z = z0; z <= z1; ++z)
	for (int x = x0; x <= x1; ++x)
		if (isSolid(world.getBlock(x, y, z))) return true;
	return false;
}
```

就是第 1 节那段话的直译：箱子范围 → 向下取整成格子区间 → 逐格问 `isSolid`。

箱子 0.6×1.8×0.6，最多跨 2×3×2 = 12 格，循环极小。

### 2.2 moveAxis：小步走，撞墙停

```cpp
// 沿单轴(axis: 0=x 1=y 2=z)移动 amount 米，撞墙就停。返回 true=撞了
inline bool moveAxis(const World& world, glm::vec3& pos, int axis, float amount) {
	const float STEP = 0.05f;              // 一小步5厘米，防止一大步穿墙
	float dir = (amount >= 0) ? 1.f : -1.f;
	float remaining = std::abs(amount);
	while (remaining > 0.f) {
		float s = std::min(STEP, remaining);
		pos[axis] += s * dir;
		if (collides(world, pos)) {
			pos[axis] -= s * dir;          // 退回最后一个安全位置
			return true;
		}
		remaining -= s;
	}
	return false;
}
```

`pos[axis]`：glm 的 vec3 可以像数组一样用下标取分量（0=x, 1=y, 2=z），这样一个函数就能伺候三个轴。

### 2.3 updatePlayer：重力总指挥

```cpp
// 每帧：重力 → 把速度积分成位移，逐轴移动+碰撞
inline void updatePlayer(const World& world, Player& p, float dt) {
	if (!p.flying) p.vel.y -= 32.f * dt;               // 重力加速度
	p.vel.y = std::max(p.vel.y, -60.f);                // 终端速度，别无限加速

	moveAxis(world, p.pos, 0, p.vel.x * dt);
	bool hitY = moveAxis(world, p.pos, 1, p.vel.y * dt);
	moveAxis(world, p.pos, 2, p.vel.z * dt);

	p.onGround = hitY && p.vel.y < 0.f;                // 往下撞到了=踩到地
	if (hitY) p.vel.y = 0.f;                           // 撞天花板/地板都清掉竖直速度
}
```

**主人**

32 这个数哪来的？地球重力不是 9.8 吗？

**米雅**

MC 的世界比地球「紧凑」，原版重力就是约 32 m/s²。

用 9.8 的话跳起来轻飘飘的像在月球喵。游戏数值以手感为王，不必效忠现实。

`onGround` 的判定也很讲究：**只有向下撞**（`vel.y < 0` 时撞了）才算踩地——

向上顶到天花板可不算「落地」，不然顶着房顶也能连跳。

---

## 3. 为了换掉幽灵：改 main.cpp

### 3.1 声明玩家

**① include**：

```cpp
#include "player/player.hpp"
```

**② 全局区**，`static Camera gCamera;` 下面加：

```cpp
static Player gPlayer;//玩家的身体（相机住在它眼睛里）
```

### 3.2 替换整段 WASD

找到主循环里从 `glm::vec3 f = gCamera.front();` 到 SHIFT 下降那一整段（第 03 章写的幽灵飞行），**整段替换**为：

```cpp
//水平方向的前和右（走路时视线抬头不该让人往天上走）
glm::vec3 f = gCamera.front();
f.y = 0.f;
if (glm::length(f) > 0.001f) f = glm::normalize(f);
glm::vec3 r = glm::normalize(glm::cross(f, glm::vec3(0, 1, 0)));

//把WASD合成一个「想走的方向」，再统一乘速度
float speed = gPlayer.flying ? 10.f : 4.3f;
glm::vec3 wish(0.f);
if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) wish += f;
if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) wish -= f;
if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) wish -= r;
if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) wish += r;
if (glm::length(wish) > 0.001f) wish = glm::normalize(wish) * speed;
gPlayer.vel.x = wish.x;
gPlayer.vel.z = wish.z;

if (gPlayer.flying) {
    gPlayer.vel.y = 0.f;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)      gPlayer.vel.y = speed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) gPlayer.vel.y = -speed;
} else {
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && gPlayer.onGround)
        gPlayer.vel.y = 9.f;//起跳
}

//F键切换飞行（边沿触发，连发妖的教训）
bool fNow = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
static bool fWas = false;
if (fNow && !fWas) { gPlayer.flying = !gPlayer.flying; gPlayer.vel.y = 0.f; }
fWas = fNow;

updatePlayer(world, gPlayer, dt);//重力+碰撞
gCamera.position = gPlayer.eyePos();//相机住进玩家的眼睛
```

逐段拆解，有四个新讲究：

1. **`f.y = 0` 拍平前方向**：走路时抬头看天，W 也不该带您上天。把 front 的 y 拍成 0 再归一化，得到「水平的前方」。（复习：`normalize` 是把向量长度变成 1，方向不变——不归一的话斜向移动会变慢。）
2. **`wish` 合成再归一**：W+D 斜着走时，两个方向直接相加长度是 √2 ≈ 1.41——不处理的话**斜走比直走快 41%**，老游戏的著名 bug（雷神之锤玩家的最爱）。归一化后所有方向等速。
3. **速度是「设置」不是「累加」**：`vel.x = wish.x` 而非 `+=`——松开按键立刻停下，MC 的脆手感。想要滑冰惯性才用累加。
4. **跳跃 = 给一次向上的初速度**：9 m/s 起跳、32 m/s² 重力，物理课公式 h = v²/2g ≈ 1.27 米——刚好能跳上一格方块，这两个数是配套的喵。

### 3.3 三处小改

**①** `world.updateLoadedChunks` 的参数从 `gCamera.position` 改成 `gPlayer.pos`（以身体为中心加载）。

**②** `camera.hpp` 里第 05 章设置的初始位置不用改也没关系——现在每帧都会被 `eyePos()` 覆盖，它只决定第一帧在哪。

**③** 右键放方块处，包一层「反悔」逻辑：

```cpp
if (right && !wasRight && hitOk) {//右键：在命中面外侧放一块
    world.setBlock(prev.x, prev.y, prev.z, Block::Stone);
    if (collides(world, gPlayer.pos))//放进自己身体里了？反悔
        world.setBlock(prev.x, prev.y, prev.z, Block::Air);
}
```

第 08 章遗留的「方块放进自己身体」这就修好了：先放，发现把自己卡住了就撤销。简单粗暴但完全正确。

---

## 4. 开跑！——以及一只压轴妖怪

构建运行。出生在半空，**咚**——落到草地上。WASD 走路，空格跳跃，撞树会停，跳一格上台阶。按 F 起飞，再按 F……

**主人**

啊啊啊啊从天上摔下来了！！

**米雅**

重力嘛。原版 MC 关掉飞行也是这么摔的，还原度满分喵。

**主人**

不过说起来……刚才我跑着跑着穿进一座刚生成的山里了？就一瞬间。

**穿墙妖·隧穿** 登场

**穿墙妖·隧穿**

嘻……嘻嘻……被你发现了……

我是量子隧穿的远房亲戚……专钻「一大步」的空子……

要是哪天你图快，把 `moveAxis` 的小步改成一步到位——

一帧卡顿，dt 变成 0.5 秒，位移 = 10×0.5 = **5 米**——

一步跨过整面墙，碰撞测试两头都是空气，我就直接**穿**过去了！

**米雅**

它说的正是取消小步走的下场，术语叫 **tunneling（隧穿）**。

我们的 `moveAxis` 每 5 厘米测一次，比最薄的墙（1 米）细 20 倍，它钻不了空子。

刚才主人「一瞬间进山」其实是另一回事：新 Chunk 生成前那格还是空气，

身体先站进去、地形后长出来。第 14 章做「创建世界」的出生点安置时会顺手治它。

**穿墙妖·隧穿**

哼……那我等着你写「冲刺技能」的那天……8 倍速移动，帧率再一低……

**米雅**

那天就把 STEP 再调小点，或者换成扫掠检测。总有得治，回去吧您喵。

**穿墙妖·隧穿** 被击败

---

## 概念小抄

| 词 | 人话 |
|----|------|
| AABB | 永不旋转的包围箱，重叠测试只需比大小 |
| 逐轴移动 | x/y/z 分开撞，才有「贴墙滑」的手感 |
| 隧穿 tunneling | 一步太大跨过障碍物；用小步走或扫掠防它 |
| 终端速度 | 下落速度封顶，防止越掉越离谱 |
| onGround | 「向下撞到了」才算踩地，顶天花板不算 |
| 归一化 normalize | 向量长度变 1；斜向移动不再偷跑 |

---

## 本章检查点

- [ ] 出生会掉到地面上站稳，不再悬浮
- [ ] 走路撞墙会停，但贴着墙还能滑动（逐轴分离生效）
- [ ] 空格恰好跳上 1 格高的台阶；2 格高跳不上去
- [ ] 斜着走和直着走速度相同
- [ ] F 切飞行，再切回来会自由落体
- [ ] 无法再把方块放进自己身体里

**米雅**

有了身体，就该有倒影。下一章往世界里注**水**——

顺便迎战一位半透明的大妖怪喵。→ [11-water.md](11-water.md)
