# 15 — 存档与读档：把世界写进硬盘

**主人**

米雅，我昨天盖了一下午的玻璃城堡，QUIT TO TITLE 一点——

**米雅**

灰飞烟灭了吧。节哀喵。

内存里的世界，程序一关就是南柯一梦。想让城堡活过关机，

就得把它变成**硬盘上的字节**——这个过程叫**序列化（serialization）**。

这一章：暂停菜单的 SAVE AND QUIT 真的保存，主菜单多一个 LOAD WORLD 真的读回来。

**主人**

要存的东西很多吧？几百个 Chunk，一个 4096 格……

**米雅**

算算账：一格 1 字节，一个 Chunk 4096 字节，几百个 Chunk ≈ 一两 MB，不大。

但我们还能便宜十倍——因为方块世界有个特点：**大片大片的重复**。

一个 Chunk 里往往整层整层都是石头或空气。对付重复，压缩界的入门神器是——

> **RLE（Run-Length Encoding，游程编码）**：连续 500 个石头，不存 500 字节，存两个数：「500 × 石头」。

## 本章动手地图

| 步骤 | 为了…… | 请您现在做 |
|------|--------|------------|
| A | 定义存档格式 + 读写函数 | **新建** `src/world/save.hpp` |
| B | 暂停时能存 | **改** `main.cpp`：QUIT TO TITLE → SAVE AND QUIT |
| C | 标题界面能读 | **改** `main.cpp`：主菜单加 LOAD WORLD |

---

## 1. 先搞懂：二进制文件（只阅读）

**米雅**

主人写过 `std::cout << 123`，那是**文本**：数字 123 变成 `'1' '2' '3'` 三个字符。

二进制文件不翻译：int 在内存里是 4 个字节，就**原样拷** 4 个字节进文件。

| | 文本 | 二进制 |
|--|------|--------|
| 数字 123456 | 6 字节 `"123456"` | 4 字节原始位 |
| 人能直接读吗 | 能 | 不能（要按格式解读） |
| 读写代码 | 要解析 | 原样拷贝，快而简单 |

存档用二进制，配一份**格式说明书**——文件里第几字节到第几字节是什么。我们的格式：

```text
"OMC1"        4字节   魔数：认门牌用
seed          4字节
玩家 x,y,z    12字节
flying        1字节
chunk数量 n   4字节
n 组 { 区块坐标 x,y,z (12字节) + RLE数据若干 }
```

**主人**

「魔数」是什么迷信？

**米雅**

文件开头的固定标记喵。读档第一步先查这 4 个字节是不是 `OMC1`，

不是就立刻拒收——防止把别的文件（或者旧版本存档）当存档硬读，读出一堆乱象。

PNG 文件开头是 `\x89PNG`、zip 是 `PK`，全世界的文件格式都这么干。

---

## 2. 为了读写：新建 save.hpp

**新建** `src/world/save.hpp`：

```cpp
#pragma once
#include "world.hpp"
#include "../player/player.hpp"
#include <fstream>
#include <filesystem>
#include <string>

// 存档文件格式（二进制）：
//   魔数"OMC1"(4字节)  ——  认门牌：不是我们的文件就拒收
//   seed(4)  玩家pos(3×4)  flying(1)
//   chunk数量(4)
//   每个chunk：区块坐标xyz(3×4) + RLE压缩的4096个方块
// RLE = Run-Length Encoding：连续N个一样的方块记成「N个×编号」两个数

//写任意「平坦」类型（int/float/uint8...）的二进制小工具
template<class T> void writeBin(std::ostream& f, const T& v) {
	f.write((const char*)&v, sizeof(T));
}
template<class T> bool readBin(std::istream& f, T& v) {
	f.read((char*)&v, sizeof(T));
	return (bool)f;//读失败会让流进入错误状态
}
```

**主人**

`template<class T>`……模板函数？

**米雅**

对，第一次正式登场喵。`writeBin(f, seed)` 时 T 是 uint32_t，

`writeBin(f, p.pos.x)` 时 T 是 float——**一份代码，所有「平坦」类型通吃**。

「平坦」指内存里就是纯数据、没有指针的类型（int、float、bool、enum 都算）。

`std::string`、`vector` 这种内部藏着指针的**不能**这样存——存下去的是指针地址，读回来指向虚空。

接着写保存：

```cpp
inline bool saveWorld(const std::string& path, const World& world, const Player& p) {
	std::filesystem::create_directories(std::filesystem::path(path).parent_path());
	std::ofstream f(path, std::ios::binary);
	if (!f) return false;

	f.write("OMC1", 4);
	writeBin(f, world.seed);
	writeBin(f, p.pos.x); writeBin(f, p.pos.y); writeBin(f, p.pos.z);
	writeBin(f, p.flying);

	uint32_t n = (uint32_t)world.chunks.size();
	writeBin(f, n);
	const int TOTAL = Chunk::N * Chunk::N * Chunk::N;
	for (auto& [c, ch] : world.chunks) {
		writeBin(f, c.x); writeBin(f, c.y); writeBin(f, c.z);
		//RLE压缩：数一数连续多少个同样的方块
		int i = 0;
		while (i < TOTAL) {
			uint8_t id = (uint8_t)ch->blocks[i];
			uint16_t run = 1;
			while (i + run < TOTAL && (uint8_t)ch->blocks[i + run] == id && run < 65535)
				run++;
			writeBin(f, run);
			writeBin(f, id);
			i += run;
		}
	}
	return (bool)f;
}
```

三个点：

1. `create_directories`：`saves/` 文件夹不存在就建（C++17 的 `<filesystem>`，第一次用，很好用喵）；
2. `std::ios::binary` 必须写——Windows 下文本模式会偷偷把 `\n` 改成 `\r\n`，二进制数据里恰好有 0x0A 字节就被改烂了，这坑深不见底；
3. RLE 循环就是嘴上说的那句话直译：从 i 开始数连续同款，写「几个 × 什么」，跳过去接着数。

然后读档——**严格按写的顺序倒着来**：

```cpp
inline bool loadWorld(const std::string& path, std::unique_ptr<World>& worldOut, Player& p) {
	std::ifstream f(path, std::ios::binary);
	if (!f) return false;

	char magic[4];
	f.read(magic, 4);
	if (!f || magic[0] != 'O' || magic[1] != 'M' || magic[2] != 'C' || magic[3] != '1')
		return false;//不是我们的存档

	uint32_t seed;
	if (!readBin(f, seed)) return false;
	worldOut = std::make_unique<World>(seed);

	p = Player{};//先归零(速度等)，再填存档里的值
	readBin(f, p.pos.x); readBin(f, p.pos.y); readBin(f, p.pos.z);
	readBin(f, p.flying);

	uint32_t n = 0;
	if (!readBin(f, n)) return false;
	const int TOTAL = Chunk::N * Chunk::N * Chunk::N;
	for (uint32_t k = 0; k < n; ++k) {
		ChunkCoord c{};
		readBin(f, c.x); readBin(f, c.y); readBin(f, c.z);
		auto ch = std::make_unique<Chunk>();
		int i = 0;
		while (i < TOTAL) {
			uint16_t run; uint8_t id;
			if (!readBin(f, run) || !readBin(f, id)) return false;
			if (i + run > TOTAL) return false;//存档损坏，别写越界
			for (int j = 0; j < run; ++j)
				ch->blocks[i + j] = (Block)id;
			i += run;
		}
		ch->dirty = true;//读进来的都要重建网格
		worldOut->chunks[c] = std::move(ch);
	}
	return true;
}
```

—— ✦ 空气凝重了起来 ✦ ——

**字节妖·奥夫赛特**

嘻嘻嘻嘻……写和读，一个字节都不能错位……

来，人类，我考考你。要是保存时写了 `flying`，读档时**忘了读**它呢？

**主人**

呃……后面所有数据都往前错 1 个字节？chunk 数量读出来是个天文数字……

然后循环疯狂 new Chunk 直到内存爆炸？！

**字节妖·奥夫赛特**

哈哈哈哈对！错一个字节，全盘皆输！

这就是二进制的世界——没有容错，没有提示，只有沉默的崩坏！

**米雅**

所以这份代码里到处是它的克星，主人数数看：

一，**魔数**把「根本不是存档」的文件拦在门口；

二，每个 `readBin` 都查返回值，文件提前截断就立刻撤退；

三，`if (i + run > TOTAL)` 拦住损坏数据的数组越界——

**永远不要信任从磁盘读进来的数字**，它说 run=60000 你就真写 60000 格吗喵？

**字节妖·奥夫赛特**

哼……防得滴水不漏……但只要你哪天改了存档格式，

加了个字段却忘了改魔数版本号……旧存档读进新代码……我必回来！

**米雅**

所以魔数叫 `OMC1`——格式一变就改成 `OMC2`，旧档认不出来就拒收，安全喵。

**字节妖·奥夫赛特** 被击败

**主人**

还有个疑问：只存了坐标和方块，Chunk 的 VAO/VBO 那些 GPU 的东西呢？

**米雅**

好问题！**不存**。GPU 资源是「衍生品」——方块数据在，网格随时能重建。

所以读进来只要 `dirty = true`，第一帧绘制循环自然会 `rebuildMesh`。

存「源数据」，不存「缓存」，序列化的金科玉律喵。

没读到的 Chunk 呢？`getOrCreate` 在 map 里找不到就用 seed 重新生成——

和当年一模一样（确定性生成第三次立功）。所以存档只需要存**去过的地方**。

---

## 3. 为了接上 UI：改 main.cpp

**① include**：

```cpp
#include "world/save.hpp"
```

**② 全局区**加存档路径：

```cpp
static const char* SAVE_PATH = "saves/world.dat";//存档位置（单存档位）
```

**③ 暂停菜单**：把 QUIT TO TITLE 按钮改成：

```cpp
if (uiButton(ui, font, cx - 150, cy + 20, 300, 44, "SAVE AND QUIT", mfx, mfy, click)) {
    saveWorld(SAVE_PATH, *gWorld, gPlayer);//先写盘再销毁
    gWorld.reset();
    setState(window, GameState::MainMenu);
}
```

**④ 主菜单**：三个按钮重新排位（y 改成 −50 / +10 / +70），中间插 LOAD：

```cpp
if (uiButton(ui, font, cx - 150, cy - 50, 300, 44, "NEW WORLD", mfx, mfy, click)) {
    gSeedInput.clear();
    setState(window, GameState::CreateWorld);
}
bool hasSave = std::filesystem::exists(SAVE_PATH);
if (uiButton(ui, font, cx - 150, cy + 10, 300, 44,
             hasSave ? "LOAD WORLD" : "LOAD WORLD (no save)", mfx, mfy, click)) {
    if (hasSave && loadWorld(SAVE_PATH, gWorld, gPlayer)) {
        gInventoryOpen = false;
        setState(window, GameState::Playing);
    }
}
if (uiButton(ui, font, cx - 150, cy + 70, 300, 44, "QUIT", mfx, mfy, click))
    glfwSetWindowShouldClose(window, true);
```

构建运行，做一次完整的轮回测试：

1. NEW WORLD，随便挖个坑、盖个丑房子，记住样子；
2. ESC → SAVE AND QUIT；
3. 完全关掉程序，重新打开；
4. LOAD WORLD——房子、坑、你站的位置、甚至飞行状态，分毫不差。

**主人**

……我的城堡有救了。存档文件在哪？

**米雅**

exe 旁边的 `saves/world.dat`。用十六进制查看器打开它，

开头四个字节正是 `4F 4D 43 31`——"OMC1"。主人已经是会设计文件格式的人了喵。

---

## 概念小抄

| 词 | 人话 |
|----|------|
| 序列化 | 内存对象 → 磁盘字节；读档是反序列化 |
| 二进制 vs 文本 | 原样拷贝 vs 转成字符；存档用前者 |
| 魔数 | 文件开头的身份标记，认门牌 |
| RLE | 「连续 N 个同款」记成两个数，方块世界的天配 |
| 平坦类型 | 内存里没指针的类型才能整块读写 |
| 存源不存缓存 | 方块存，网格重建 |

---

## 本章检查点

- [ ] 盖房 → 存档退出 → **关程序重开** → 读档，一切原样
- [ ] 位置和飞行状态也被还原
- [ ] 没存档时按钮显示 (no save)，点了不炸
- [ ] 读档后走去没去过的远方，新地形照常生成（seed 复现）
- [ ] 能对着字节妖复述：为什么 run 要检查越界

**米雅**

最后一章了喵。现在的世界永远是正午——

下一章给它**时间**：日升日落、天色渐变、远山入雾。收尾之战！

→ [16-day-night-and-fog.md](16-day-night-and-fog.md)
