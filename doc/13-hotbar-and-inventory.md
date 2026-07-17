# 13 — 快捷栏与创造背包

**米雅**

主人，您现在是不是右键只能放石头？

**主人**

是啊，`world.setBlock(..., Block::Stone)` 写死的。

想造木屋，结果搬出来一栋毛坯房。

**米雅**

这一章就给您发装备喵：

- 底部 **9 格快捷栏**，数字键 1~9 和滚轮切换，右键放「手里拿的」
- 按 **E** 打开**创造背包**，鼠标点选方块装进手里
- 顺便进货：木板、玻璃、圆石、砖块、雪、沙砾——**6 种新方块**

而且这一章有个隐藏关卡：**第一次用鼠标指针点 UI**。

指针、窗口、帧缓冲……三套坐标系在这里第一次碰头，有妖怪守着喵。

## 本章动手地图

| 步骤 | 为了…… | 请您现在做 |
|------|--------|------------|
| A | 有新方块可放 | **改** `block.hpp`：enum 加 6 种；玻璃从 `isOpaque` 除名 |
| B | 新方块有贴图 | **改** `block_textures.hpp`：`tileOf` 补 6 行 |
| C | 玻璃拼一起不穿帮 | **改** `chunk.hpp`：同类相贴不生成面 |
| D | 有物品清单和快捷栏数据 | **新建** `src/player/inventory.hpp` |
| E | 键盘滚轮选、右键放选中的 | **改** `main.cpp`：输入部分 |
| F | 画快捷栏和背包界面 | **改** `main.cpp`：UI 部分 |

---

## 1. 为了进货：改 block.hpp

**改** `src/world/block.hpp`，enum 里 `Water` 后面加：

```cpp
	Planks,   // 木板
	Glass,    // 玻璃
	Cobble,   // 圆石
	Bricks,   // 砖块
	Snow,     // 雪块
	Gravel,   // 沙砾
```

然后是重头戏——玻璃。`isSolid` **不动**（玻璃挡人），`isOpaque` 改成：

```cpp
//「不透明」：挡视线。面剔除用它——邻居不透明这面才不用画
//玻璃实心但透明：挡人不挡视线，所以两个函数从此正式分家
inline bool isOpaque(Block b) {
	return b != Block::Air && b != Block::Water && b != Block::Glass;
}
```

**米雅**

上一章拆 `isSolid`/`isOpaque` 时说「以后会分家」，现在兑现了：

玻璃是第一个「实心但透明」的方块——撞得到、看得穿。

当时要是图省事只留一个函数，现在就要大改；

**好的抽象是给未来留的门**喵。

**改** `block_textures.hpp` 的 `tileOf`，`case Block::Water` 后面加（图块编号还是第 06 章那张表，早就备好了）：

```cpp
case Block::Planks: return T_PLANKS;
case Block::Glass:  return T_GLASS;
case Block::Cobble: return T_COBBLE;
case Block::Bricks: return T_BRICKS;
case Block::Snow:   return T_SNOW;
case Block::Gravel: return T_GRAVEL;
```

**改** `chunk.hpp` `rebuildMesh` 里普通方块的剔除判断，补一个条件：

```cpp
} else {
    //普通方块：邻居不透明就不画（水是透明的，所以水下的地面照画）
    //同类相贴也不画（两块玻璃拼一起，中间不该有面）
    if (isOpaque(nb) || nb == b) continue;
}
```

**主人**

`nb == b` ……不加会怎样？

**米雅**

想象两块玻璃并排：玻璃不 opaque，所以彼此的贴合面都会生成——

从侧面看，两块玻璃**中间悬着一层玻璃纹**，穿帮喵。

MC 的玻璃、水都有这条「同类相邻不画」规则，我们的水在上一章已经这么做了（水贴水不画），现在推广到所有方块。

---

## 2. 为了有清单：新建 inventory.hpp

**新建** `src/player/inventory.hpp`：

```cpp
#pragma once
#include "../world/block.hpp"
#include <vector>

// 创造模式物品清单：所有能放置的方块
inline const std::vector<Block>& placeableBlocks() {
	static std::vector<Block> list = {
		Block::Grass, Block::Dirt, Block::Stone, Block::Cobble,
		Block::Log, Block::Planks, Block::Leaves, Block::Sand,
		Block::Glass, Block::Bricks, Block::Snow, Block::Gravel,
		Block::Water,
	};
	return list;
}

// 方块的英文名（HUD和背包里显示）
inline const char* blockName(Block b) {
	switch (b) {
		case Block::Grass:  return "Grass";
		case Block::Dirt:   return "Dirt";
		case Block::Stone:  return "Stone";
		case Block::Log:    return "Log";
		case Block::Leaves: return "Leaves";
		case Block::Sand:   return "Sand";
		case Block::Water:  return "Water";
		case Block::Planks: return "Planks";
		case Block::Glass:  return "Glass";
		case Block::Cobble: return "Cobblestone";
		case Block::Bricks: return "Bricks";
		case Block::Snow:   return "Snow";
		case Block::Gravel: return "Gravel";
		default:            return "?";
	}
}

// 快捷栏：9格 + 当前选中的格子
struct Hotbar {
	Block slots[9] = {
		Block::Grass, Block::Dirt, Block::Stone, Block::Log,
		Block::Planks, Block::Glass, Block::Cobble, Block::Sand, Block::Bricks,
	};
	int selected = 0;
	Block current() const { return slots[selected]; }
};
```

数据毫无花头：9 个格子一个光标。UI 永远是「数据简单、画起来啰嗦」，一会儿见识喵。

---

## 3. 为了能选：改 main.cpp 输入部分

**① include**：

```cpp
#include "player/inventory.hpp"
```

**② 全局区**（`gPlayer` 下面）加三个：

```cpp
static Hotbar gHotbar;//快捷栏
static bool gInventoryOpen = false;//背包开着吗
static float gScroll = 0.f;//滚轮攒的量
```

**③ 滚轮回调**：`mouse_callback` 前面加一个新回调，并在 `main` 里注册（`glfwSetCursorPosCallback` 那行后面）：

```cpp
static void scroll_callback(GLFWwindow*, double, double yoff) {
    gScroll += (float)yoff;//滚轮事件来了先攒着，主循环里再消费
}
```

```cpp
glfwSetScrollCallback(window, scroll_callback);//滚轮切快捷栏
```

**米雅**

为什么「先攒着」？复习第 03 章：滚轮和鼠标移动一样是**事件回调**，

它响的时机不归主循环管。回调里只做最小的事（记下来），

主循环里统一处理——这是回调世界的铁律，逻辑散进回调里迟早失控喵。

**④ 视角回调加一道闸**：`mouse_callback` 开头加：

```cpp
if (gInventoryOpen) return;//背包开着时鼠标是指针，不转视角
```

**⑤ E 键开关背包**：F 键那段后面加：

```cpp
//E键开关背包：切换鼠标模式
bool eNow = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
static bool eWas = false;
if (eNow && !eWas) {
    gInventoryOpen = !gInventoryOpen;
    glfwSetInputMode(window, GLFW_CURSOR,
        gInventoryOpen ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    if (!gInventoryOpen) firstMouse = true;//回游戏时重置，防视角猛跳
}
eWas = eNow;

//背包开着时不走路（但重力照常，MC也这样）
if (gInventoryOpen) { gPlayer.vel.x = 0.f; gPlayer.vel.z = 0.f; }

//数字键1~9和滚轮选快捷栏
if (!gInventoryOpen) {
    for (int i = 0; i < 9; ++i)
        if (glfwGetKey(window, GLFW_KEY_1 + i) == GLFW_PRESS)
            gHotbar.selected = i;
    int s = (int)gScroll;
    if (s != 0) {
        gHotbar.selected = ((gHotbar.selected - s) % 9 + 9) % 9;//往下滚选下一格
        gScroll -= (float)s;
    }
}
```

两处细节：

- **`firstMouse = true`**：背包打开时指针满屏跑，关背包的瞬间指针在哪，视角就会往哪猛甩一下。重置 `firstMouse` 让下一次视角计算从头来——第 03 章埋的这个变量，今天第二次救场；
- **`((x - s) % 9 + 9) % 9`**：又是负数取模！`-1 % 9` 在 C++ 是 `-1` 不是 `8`。撕裂妖的余党无处不在，同一招收拾它喵。

**⑥ 挖放改造**：找到鼠标那段，三个变化——加背包闸门、放 `gHotbar.current()`、**`wasLeft/wasRight` 的更新搬家**：

```cpp
//当前帧鼠标状态（挖放和背包点击共用，帧末统一记录上一帧状态）
bool left  = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)  == GLFW_PRESS;
bool right = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
static bool wasLeft = false, wasRight = false;

if (!gInventoryOpen) {//背包开着时点击是给背包用的，不挖不放
    if (left && !wasLeft && hitOk)   //左键：挖掉命中格
        world.setBlock(hit.x, hit.y, hit.z, Block::Air);
    if (right && !wasRight && hitOk) {//右键：放手里选中的方块
        world.setBlock(prev.x, prev.y, prev.z, gHotbar.current());
        if (collides(world, gPlayer.pos))//放进自己身体里了？反悔
            world.setBlock(prev.x, prev.y, prev.z, Block::Air);
    }
}
```

（原来结尾的 `wasLeft = left; wasRight = right;` **删掉**——一会儿背包点击也要用这对状态，要等 UI 画完、点击判完才能记录。新位置在第 5 节。）

---

## 4. 为了看得见：画快捷栏

UI 段里，画完准星之后（顺便给准星包一层 `if (!gInventoryOpen)`，开背包时准星没意义）：

```cpp
//快捷栏：底部9个格子
const float SLOT = 44.f;
float barX = cx - SLOT * 4.5f, barY = fbh - SLOT - 8.f;
for (int i = 0; i < 9; ++i) {
    float x = barX + i * SLOT;
    bool sel = (i == gHotbar.selected);
    //选中的格子边框亮白，其余半透明黑
    ui.rect(x, barY, SLOT, SLOT, sel ? glm::vec4{1,1,1,0.9f} : glm::vec4{0,0,0,0.55f});
    ui.rect(x + 3, barY + 3, SLOT - 6, SLOT - 6, { 0.15f,0.15f,0.15f,0.85f });
    //方块图标：借它侧面(face=0)的图块
    float u0, v0, u1, v1;
    tileUV(tileOf(gHotbar.slots[i], 0), u0, v0, u1, v1);
    ui.texRect(x + 7, barY + 7, SLOT - 14, SLOT - 14, atlas, u0, v0, u1, v1, { 1,1,1,1 });
}
//手里拿的方块名，居中显示在快捷栏上方
{
    std::string name = blockName(gHotbar.current());
    font.draw(ui, cx - Font::measure(16, name) * 0.5f, barY - 24, 16, name);
}
```

**主人**

「边框」原来是**大矩形上叠小矩形**画出来的？

**米雅**

UI 的祖传偷懒法喵：先画满格的边框色，再叠一个缩进 3 像素的底色——

露出来的 3 像素圈就是边框。图标 UV 直接复用 `tileUV`+`tileOf`：

给 UI 画方块图标和给世界贴方块，查的是同一张表——一份数据，处处受益。

居中公式也值得记：**`中心x − 文字宽度/2`**，`Font::measure` 上一章就是为这刻准备的。

---

## 5. 为了能点：背包界面 —— 三坐标系会战

接着写背包（快捷栏代码之后）：

```cpp
//背包界面
if (gInventoryOpen) {
    ui.rect(0, 0, (float)fbw, (float)fbh, { 0,0,0,0.5f });//压暗世界

    //鼠标位置：窗口坐标 → 帧缓冲坐标（高DPI缩放时两者不一样，必须换算）
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    int winW, winH;
    glfwGetWindowSize(window, &winW, &winH);
    float mfx = (float)mx * fbw / (winW ? winW : 1);
    float mfy = (float)my * fbh / (winH ? winH : 1);

    const auto& items = placeableBlocks();
    const int   COLS_INV = 7;
    const float CELL = 52.f;
    int rows = ((int)items.size() + COLS_INV - 1) / COLS_INV;
    float gx = cx - COLS_INV * CELL * 0.5f;
    float gy = cy - rows * CELL * 0.5f;

    std::string title = "INVENTORY  (click = put in hand)";
    font.draw(ui, cx - Font::measure(18, title) * 0.5f, gy - 40, 18, title);

    for (int i = 0; i < (int)items.size(); ++i) {
        float x = gx + (i % COLS_INV) * CELL;
        float y = gy + (i / COLS_INV) * CELL;
        bool hover = mfx >= x && mfx < x + CELL && mfy >= y && mfy < y + CELL;

        ui.rect(x + 2, y + 2, CELL - 4, CELL - 4,
                hover ? glm::vec4{0.45f,0.45f,0.45f,0.95f} : glm::vec4{0.2f,0.2f,0.2f,0.95f});
        float u0, v0, u1, v1;
        tileUV(tileOf(items[i], 0), u0, v0, u1, v1);
        ui.texRect(x + 10, y + 10, CELL - 20, CELL - 20, atlas, u0, v0, u1, v1, { 1,1,1,1 });

        if (hover) {//悬停显示名字；点击放进手里(当前选中的快捷栏格)
            font.draw(ui, cx - Font::measure(16, blockName(items[i])) * 0.5f,
                      gy + rows * CELL + 12, 16, blockName(items[i]));
            if (left && !wasLeft)
                gHotbar.slots[gHotbar.selected] = items[i];
        }
    }
}
```

最后，`ui.end();` 之后、`glfwSwapBuffers` 之前，补上搬家过来的：

```cpp
//帧末统一记录鼠标状态（挖放和背包点击都用完了）
wasLeft = left;
wasRight = right;
```

**错位妖·帕拉克斯** 登场

**错位妖·帕拉克斯**

呵呵呵……点吧点吧。要是这台机器开了 150% 缩放，

而你直接拿 `glfwGetCursorPos` 的数去跟 UI 坐标比——

鼠标指着「玻璃」，点到的却是「圆石」！

指哪不打哪，我最爱看人类抓狂调这种 bug 了！

**主人**

……为什么会差出来？

**米雅**

因为屏幕上有**三套坐标系**在开会喵：

| 坐标系 | 谁在用 | 单位 |
|--------|--------|------|
| 窗口坐标 | `glfwGetCursorPos`（鼠标） | 逻辑像素（受系统缩放影响） |
| 帧缓冲坐标 | `glfwGetFramebufferSize`、我们的 UI | 真实像素 |
| NDC | GPU | −1~1 |

100% 缩放时窗口=帧缓冲，人畜无害；150% 缩放时帧缓冲是窗口的 1.5 倍，

拿窗口坐标的鼠标去比帧缓冲坐标的按钮，全部错位 1.5 倍。

解药就是代码里那两行换算：**`鼠标 × 帧缓冲尺寸 ÷ 窗口尺寸`**。

**错位妖·帕拉克斯**

哼，这次防住了……但记住，只要你哪天写 UI 忘了我……

任何游戏、任何 GUI 框架、浏览器里的 canvas……我无处不在——

**错位妖·帕拉克斯** 被击败

构建运行！E 开背包，点一块砖，关背包，右键——墙上多了一块砖喵。

> 小提示：放**水**之后左键挖不掉它（射线穿水，这是上一章特性）。想清掉就往那格放个沙子再挖掉。原版 MC 的水要用桶收，属于第 17 章之后的自选题。

---

## 概念小抄

| 词 | 人话 |
|----|------|
| 事件攒着处理 | 回调只记账，主循环统一消费 |
| 负数取模 | `((x % n) + n) % n`，滚轮倒着滚不炸 |
| 边框画法 | 大色块上叠缩进的小色块 |
| hover 判定 | 点在矩形里 = 四个比较 |
| 窗口 vs 帧缓冲坐标 | 系统缩放下不相等，鼠标要换算 |

---

## 本章检查点

- [ ] 数字键和滚轮都能切换选中格，滚轮倒滚不会崩
- [ ] 右键放的是手里选中的方块，名字显示正确
- [ ] E 开背包：指针出现、视角不转、人停下；再按 E 回游戏视角不猛跳
- [ ] 背包里点方块，装进当前选中的快捷栏格
- [ ] 两块玻璃拼一起，中间没有多余的面；隔着玻璃看世界正常
- [ ] 造一面玻璃墙 + 砖房——享受一下建筑自由喵

**米雅**

主人有没有发现，我们的游戏还是「双击 exe 直接掉进世界」？

正经游戏得有**主菜单**：标题、开始、退出，创建世界时还能输种子。

下一章就把「程序」升格成「游戏」喵。→ [14-main-menu.md](14-main-menu.md)
