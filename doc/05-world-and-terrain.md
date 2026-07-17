# 05 — 世界、Perlin 地形与种树

**米雅**（猫娘女仆）

主人，欢迎回来喵。

第 04 章结束时，您的世界是——一块 16×16×16 的方形石头，孤零零漂在虚空里。

**主人**

说是「世界」，其实是一块豆腐。

**米雅**

对，豆腐。

所以这一章我们干三件大事：

一，让世界能装下**很多很多块豆腐**（`World` 管理一堆 Chunk）；

二，让豆腐自己长成**连绵的山丘**（Perlin 噪声地形）；

三，在山丘的草地上**种树**。

**主人**

一章干三件事，不会消化不良吗？

**米雅**

放心，每一步都是「为了什么 → 所以在哪个文件写什么」，

跟着走就行。

而且这一章路上会有妖怪出没……先不剧透喵。

---

## 本章动手地图

| 步骤 | 为了…… | 请您现在做 |
|------|--------|------------|
| A | 有树干和树叶这两种方块 | **改** `src/world/block.hpp`：enum 加 `Log`、`Leaves`；**改** `chunk.hpp` 的 `colorOf` |
| B | 有一台「随机但可复现」的噪声机 | **新建** `src/world/noise.hpp`（Perlin 实现） |
| C | 把噪声变成地面高度 + 长树 + 填土石草 | **新建** `src/world/terrain.hpp` |
| D | 用坐标当钥匙管很多 Chunk | **新建** `src/world/world.hpp`（`World` 类） |
| E | 玩家飞到哪就加载到哪 | **改** `main.cpp`：删掉手工豆腐，换成 `World` + 遍历绘制 |
| F | 出生点别埋在土里 | **改** `camera.hpp`：初始位置抬高到地形上空 |

> 开工前确认您现在的状态（第 04 章检查点）：`main.cpp` 里有 `Chunk chunk;` 加一个三重循环手工填下半石头，主循环里 `if (chunk.dirty) chunk.rebuildMesh();` 然后 `chunk.draw()`。是这样就没问题喵。

---

## 1. 为了有树的方块：改 block.hpp

**米雅**

树 = 树干 + 树叶。它们对引擎来说就是两种新「标签」。

复习一下第 04 章：`Block` 只是个标签，**不含任何图形信息**，

画成什么颜色是 `colorOf` 的事，生不生成面是 `rebuildMesh` 的事。

**改** `src/world/block.hpp`，enum 里补两种：

```cpp
enum class Block : uint8_t {
    Air = 0,
    Grass,
    Dirt,
    Stone,
    Log,      // 树干
    Leaves,   // 树叶
};
```

**改** `src/world/chunk.hpp` 的 `colorOf`，`case Block::Stone` 那行后面补两行：

```cpp
case Block::Log:    return { 0.45f, 0.32f, 0.18f }; // 深棕树干
case Block::Leaves: return { 0.20f, 0.55f, 0.18f }; // 深绿树叶
```

**主人**

`isSolid` 不用改吗？

**米雅**

不用喵。`isSolid` 写的是「不是空气就算实心」，

Log 和 Leaves 自动就是实心，会被正常剔除、正常画。

问得很好，这说明您真的理解了第 04 章。

---

## 2. 为了「随机但可复现」：新建 noise.hpp

### 2.1 先想清楚：为什么不能用 rand()？

**主人**

地形要随机起伏……那我每列用 `rand()` 取个高度不就行了？

**米雅**

主人可以试试，然后您会看到一片**针毡**。

`rand()` 有两个致命伤：

一，**不连续**——相邻两列的高度毫无关系，乱跳，没有「山坡」这种东西；

二，**不可复现**——您飞走再飞回来，同一个地方重新生成时高度全变了，世界会「碎掉」。

**主人**

所以要一个「喂它坐标、吐出平滑数值、而且每次答案都一样」的函数。

**米雅**

正是！这就是 **Perlin 噪声**——MC 用的正是这一类。

> **人话：** Perlin 噪声是一个函数 `noise(x, z)`：喂一个坐标，吐一个 −1~1 之间、随位置**平滑**变化的数。同样的输入永远得到同样的输出。把这个数当地面高度，就有山有谷了。

### 2.2 完整实现（可整份粘贴）

**新建** `src/world/noise.hpp`。这是经典 Perlin 的标准实现。米雅说句实话：这份代码的数学细节（fade 曲线、梯度点积）**不必现在全懂**，您只要会用 `noise(x, z)` 和知道 seed 是干嘛的就够了，就像会用 `sin` 不必会推导泰勒展开：

```cpp
#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

// 2D Perlin 噪声。构造时给一个 seed，同一个 seed 得到同一个世界。
struct Perlin {
    int perm[512];   // 打乱后的排列表，噪声的“随机源”

    Perlin(unsigned seed = 1337) {
        int p[256];
        for (int i = 0; i < 256; ++i) p[i] = i;
        // 用 seed 驱动一个简单的伪随机，把 0..255 洗牌
        uint32_t s = seed ? seed : 1;
        for (int i = 255; i > 0; --i) {
            s = s * 1664525u + 1013904223u;      // 线性同余，够用了
            int j = (int)(s % (uint32_t)(i + 1));
            std::swap(p[i], p[j]);
        }
        for (int i = 0; i < 512; ++i) perm[i] = p[i & 255];
    }

    static float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); } // 平滑曲线
    static float lerp(float a, float b, float t) { return a + t * (b - a); }   // 线性插值

    // 根据格点哈希值给一个方向梯度，再和偏移做点积
    static float grad(int h, float x, float y) {
        switch (h & 7) {
            case 0: return  x + y; case 1: return -x + y;
            case 2: return  x - y; case 3: return -x - y;
            case 4: return  x;     case 5: return -x;
            case 6: return  y;     default: return -y;
        }
    }

    // 返回约 [-1, 1] 的平滑噪声
    float noise(float x, float y) const {
        int X = (int)std::floor(x) & 255;
        int Y = (int)std::floor(y) & 255;
        x -= std::floor(x);
        y -= std::floor(y);
        float u = fade(x), v = fade(y);
        int A  = perm[X] + Y, B = perm[X + 1] + Y;
        return lerp(
            lerp(grad(perm[A],     x,     y    ), grad(perm[B],     x - 1, y    ), u),
            lerp(grad(perm[A + 1], x,     y - 1), grad(perm[B + 1], x - 1, y - 1), u),
            v);
    }
};

// 分形叠加（fBm）：把多层不同频率的噪声叠起来，
// 大层管大山脉，小层管小起伏，地形更自然。
inline float fbm(const Perlin& n, float x, float z,
                 int octaves = 4, float lacunarity = 2.f, float gain = 0.5f) {
    float sum = 0.f, amp = 1.f, freq = 1.f, norm = 0.f;
    for (int i = 0; i < octaves; ++i) {
        sum  += amp * n.noise(x * freq, z * freq);
        norm += amp;
        amp  *= gain;         // 每层振幅减半
        freq *= lacunarity;   // 每层频率翻倍
    }
    return sum / norm;        // 归一回约 [-1, 1]
}
```

**主人**

`fbm` 又是什么？一层噪声还不够吗？

**米雅**

一层噪声出来的山「太圆润」，像果冻。

`fbm` 把好几层叠起来：第一层管大山脉的走势，

第二层频率翻倍、幅度减半，管小丘陵，

再往上几层管地表的小坑洼。大骨架 + 小细节，才像真山喵。

> **注意 seed 的设计**：`Perlin` 把 seed 存在构造函数里。后面第 14 章做「创建世界」界面时，玩家输入的种子就是从这里进去的——我们现在就把管道铺好，将来不用返工。

---

## 3. 为了自动铺地和长树：新建 terrain.hpp

**新建** `src/world/terrain.hpp`。这个文件从上往下写四个函数，顺序别乱（C++ 要先声明后使用）：

```
heightAt     → 这一列地面多高？
hash2        → 确定性哈希（种树用的“随机源”）
isTreeBase   → 这一列长不长树？
plantTrees   → 把树盖进 Chunk
generateTerrain → 总指挥：填土石草，再调 plantTrees
```

### 3.1 heightAt：从噪声算出「这一列地面多高」

文件开头写：

```cpp
#pragma once
#include "chunk.hpp"
#include "noise.hpp"
#include <cstdlib>

// 给定世界坐标的一列 (wx, wz)，返回地面高度（草方块所在的 y）
inline int heightAt(const Perlin& noise, int wx, int wz) {
    // 0.01f 控制山的“宽窄”；数越小山越缓
    float e = fbm(noise, wx * 0.01f, wz * 0.01f, 4);
    e = e * 0.5f + 0.5f;              // 从 [-1,1] 映射到 [0,1]
    return 40 + (int)(e * 24);        // 基准高度 40，最多再抬 24 格
}
```

**主人**

为什么要乘 0.01？

**米雅**

Perlin 噪声大约「走 1 格变一个样」。

方块坐标一格才 1，直接喂进去，隔一个方块地形就大变，还是针毡。

乘 0.01 等于说「走 100 格才变一个样」——山就宽了、缓了。

这些数字以后全可以调：`0.01f` 管山的宽窄，`24` 管山的高矮，`40` 管海拔基准。先照抄，跑通再玩。

### 3.2 hash2 / isTreeBase：哪一列长树？

**米雅**

种树的第一个问题：**哪里长树**？

要求和地形一样——随机，但**同一个位置永远同一个答案**。

不然您回头一看，树挪窝了，多吓人喵。

**主人**

那再造一台 Perlin？

**米雅**

杀鸡不用宰牛刀。树不需要「平滑」，只需要「确定」，

一个整数哈希就够了——喂 `(x, z, seed)`，吐一个乱七八糟但固定的数：

```cpp
// 一个确定性哈希：同样的 (x,z,seed) 永远返回同样的数
inline uint32_t hash2(int x, int z, uint32_t seed) {
    uint32_t h = (uint32_t)(x * 73856093) ^ (uint32_t)(z * 19349663) ^ seed;
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return h;
}

// 判断世界列 (wx, wz) 是不是一棵树的“树干根部”
inline bool isTreeBase(int wx, int wz, uint32_t seed) {
    return (hash2(wx, wz, seed) % 40u) == 0u;   // 约 1/40 的列长树
}
```

那串魔法数字（73856093 之类）是常用的哈希质数，作用就是把坐标搅得足够乱。会用即可。

### 3.3 plantTrees：树要怎么「跨区块」长？

—— ✦ 妖怪来袭的预感 ✦ ——

**米雅**

在写代码之前，主人先想一个问题：

一棵树，树干 1 格宽，叶冠 5×5。

如果树正好长在 Chunk 的边缘，叶子会伸进**隔壁的** Chunk。

可是隔壁 Chunk 生成的时候，根本不知道邻居这边有棵树……

**主人**

那树就会被区块边界切成半棵？

**米雅**

如果傻写的话，是的。满世界的「半棵树」，惨不忍睹喵。

我们的解法叫**确定性生成**，一句话：

> 每个 Chunk 生成时，**多扫一圈**（四周各多看 2 格）。对范围里每一列，用坐标+种子**算出**「这里有没有树、多高」，然后**只把落在自己家里的那部分方块**盖上去。

因为「有没有树」由世界坐标唯一决定（3.2 节的 `isTreeBase`），相邻两个 Chunk 对同一棵边界树会得出**一模一样**的结论——你画你那半边，我画我这半边，拼起来天衣无缝。真实体素引擎就是这么干的。

继续在 `terrain.hpp` 里写：

```cpp
// 往 chunk 里种树。只写落在本 chunk 内的方块。
inline void plantTrees(Chunk& chunk, int coordX, int coordY, int coordZ,
                       const Perlin& noise, uint32_t seed) {
    const int N = Chunk::N;
    const int MARGIN = 2;  // 叶冠半径，向外多扫 2 格，接住邻区伸进来的树

    for (int wz = coordZ * N - MARGIN; wz < coordZ * N + N + MARGIN; ++wz)
    for (int wx = coordX * N - MARGIN; wx < coordX * N + N + MARGIN; ++wx) {
        if (!isTreeBase(wx, wz, seed)) continue;

        int ground = heightAt(noise, wx, wz);           // 地面高度（草所在 y）
        int trunkH = 4 + (int)(hash2(wx, wz, seed) >> 8) % 3; // 树干高 4~6
        int baseY  = ground + 1;                        // 树干从草上面一格开始

        // —— 一个把“世界坐标方块”盖进本 chunk 的小工具 ——
        auto place = [&](int wx2, int wy2, int wz2, Block b) {
            int lx = wx2 - coordX * N;
            int ly = wy2 - coordY * N;
            int lz = wz2 - coordZ * N;
            if (lx < 0 || ly < 0 || lz < 0 || lx >= N || ly >= N || lz >= N)
                return;                        // 不在本 chunk → 交给别的 chunk 去画
            chunk.set(lx, ly, lz, b);
        };

        // 树干
        for (int i = 0; i < trunkH; ++i)
            place(wx, baseY + i, wz, Block::Log);

        // 叶冠：下两层 5×5（去四角）、顶上一层 3×3，简单的橡树形状
        int topY = baseY + trunkH - 1;
        for (int dy = 0; dy <= 1; ++dy)
        for (int dz = -2; dz <= 2; ++dz)
        for (int dx = -2; dx <= 2; ++dx) {
            if (std::abs(dx) == 2 && std::abs(dz) == 2) continue; // 抹掉四角更圆
            place(wx + dx, topY - 1 + dy, wz + dz, Block::Leaves);
        }
        for (int dz = -1; dz <= 1; ++dz)
        for (int dx = -1; dx <= 1; ++dx)
            place(wx + dx, topY + 1, wz + dz, Block::Leaves);
    }
}
```

**主人**

那个 `auto place = [&](...)` 是什么写法？

**米雅**

**lambda**，就地定义的小函数喵。`[&]` 表示它能直接用外面的变量（`chunk`、`coordX` 这些）。

它干的事很朴素：把世界坐标换算成本地坐标，**出界就默默放弃**——

出界的那部分，隔壁 Chunk 自己会算出来并画上，这正是确定性生成的精髓。

### 3.4 generateTerrain：总指挥

最后写总指挥。一列从下往上是：**深处石头 → 表层 3 格泥土 → 最顶 1 格草 → 再上面空气**，铺完再种树：

```cpp
// 把一个 Chunk 填成地形。coordX/Y/Z 是这个 Chunk 在世界里的“区块坐标”。
inline void generateTerrain(Chunk& chunk, int coordX, int coordY, int coordZ,
                            const Perlin& noise, uint32_t seed) {
    const int N = Chunk::N;
    for (int lz = 0; lz < N; ++lz)
    for (int lx = 0; lx < N; ++lx) {
        // 本地坐标 → 世界坐标
        int wx = coordX * N + lx;
        int wz = coordZ * N + lz;
        int h  = heightAt(noise, wx, wz);

        for (int ly = 0; ly < N; ++ly) {
            int wy = coordY * N + ly;      // 这一格的世界高度
            Block b;
            if      (wy > h)      b = Block::Air;     // 地面以上：空气
            else if (wy == h)     b = Block::Grass;   // 最顶一格：草
            else if (wy > h - 4)  b = Block::Dirt;    // 往下 3 格：泥土
            else                  b = Block::Stone;   // 再往下：石头
            chunk.set(lx, ly, lz, b);                 // 存进本地坐标
        }
    }
    // 地形铺好后种树
    plantTrees(chunk, coordX, coordY, coordZ, noise, seed);
    chunk.dirty = true;
}
```

**米雅**

这里有全章**最容易翻车**的一个点，主人把它刻在脑子里：

> 数据用**本地坐标** `(lx, ly, lz)` 存进 Chunk 数组，
> 但「这格该填什么」要用**世界坐标** `wy` 和 `h` 来判断。
>
> 换算式：**世界坐标 = 区块坐标 × 16 + 本地坐标**

「本地存、世界判」，这六个字贯穿整个体素引擎。

**主人**

为什么种树不会把地形盖坏？

**米雅**

顺序！`plantTrees` 在土石草**填完之后**才调用，

它只往地面**上方**的空气里塞 Log 和 Leaves，地皮动都不动喵。

---

## 4. 为了管很多 Chunk：新建 world.hpp

**米雅**

现在一个 Chunk 会自己长地形了，但只有一块还是豆腐。

世界要（伪）无限大，就得**要多少块、生成多少块**，还得能快速找到「坐标 (3, 2, -1) 的那块在哪」。

**主人**

听起来像一张查询表。钥匙是坐标，值是 Chunk。

**米雅**

完全正确，`std::unordered_map` 就是干这个的。

只是钥匙要用「**区块坐标**」——世界坐标 ÷ 16 向下取整。

**新建** `src/world/world.hpp`：

```cpp
#pragma once
#include "chunk.hpp"
#include "terrain.hpp"
#include <memory>
#include <unordered_map>
#include <cmath>
#include <glm/glm.hpp>

// 区块坐标（不是方块坐标）。相邻两个 Chunk 的坐标差 1。
struct ChunkCoord {
    int x, y, z;
    bool operator==(const ChunkCoord& o) const { return x==o.x && y==o.y && z==o.z; }
};
struct ChunkCoordHash {
    size_t operator()(const ChunkCoord& c) const {
        return (size_t)(c.x * 73856093) ^ (size_t)(c.y * 19349663) ^ (size_t)(c.z * 83492791);
    }
};
```

**主人**

`ChunkCoordHash` 是干嘛的？map 不是自己会查吗？

**米雅**

`unordered_map` 内部靠「把钥匙变成一个数字」来分抽屉（哈希桶）。

`int`、`string` 它天生会算，但 `ChunkCoord` 是我们自造的结构体，

得**教**它怎么算——就是这个仿函数。还得有 `operator==` 告诉它「两把钥匙什么时候算同一把」。

自定义类型当 map 钥匙，这两件套是固定搭配，记住喵。

接着写（还在 `world.hpp`）：

```cpp
// 负数也能正确「向下取整除」。floorDiv(-1, 16) 要得到 -1，而不是 0。
inline int floorDiv(int a, int b) { return (a >= 0) ? a / b : -((-a + b - 1) / b); }
// 取模到 [0, b)。世界坐标 → 本地坐标 用。
inline int floorMod(int a, int b) { int m = a % b; return m < 0 ? m + b : m; }
```

**主人**

等等，除法就除法，为什么要自己写个 `floorDiv`？

**米雅**

……来了。它听到了。主人，站到我身后喵。

—— ✦ 撕裂妖·负负 登场 ✦ ——

**撕裂妖·负负**

嘻嘻嘻……又一个用 `wx / 16` 算区块坐标的凡人。

**主人**

有什么问题吗？-17 除以 16……

**撕裂妖·负负**

你猜 C++ 会告诉你几？

**主人**

-17 ÷ 16 = -1.06…，向下取整，-2？

**撕裂妖·负负**

**-1！** C++ 的整数除法是「**向零取整**」，不是「向下取整」！

-1.06 会被截成 -1，不是 -2！

于是 -17 号方块和 -1 号方块会被你塞进**同一个区块**，

负半轴的世界从此错位、撕裂、支离破碎——

都是我的杰作，嘻嘻嘻嘻！

**米雅**

主人，它没说错，这是 C/C++ 埋了几十年的老坑：

`%` 也一样，`-17 % 16` 得 `-1` 而不是 `15`，本地坐标直接变负数，数组越界。

对付它的银弹就是刚才那两行：

`floorDiv` 把负数**往下**取整（-17→-2），

`floorMod` 把余数拉回 `[0, 16)`（-17→15）。

**主人**

所以 -17 号方块属于 -2 号区块的第 15 格。

……你可以碎了。

**撕裂妖·负负**

可恶……记住，只要你哪天图省事写回 `/` 和 `%`……

我随时会回来——

—— ✦ 撕裂妖·负负 被击败 ✦ ——

**米雅**

以后只要飞到负坐标发现地形撕裂，**第一个查这两个函数**。

继续写 `World` 本体（还在 `world.hpp`）：

```cpp
class World {
public:
    uint32_t seed;
    Perlin noise;
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> chunks;

    World(uint32_t s = 2024) : seed(s), noise(s) {}

    // 世界方块坐标 → 它属于哪个区块坐标
    static ChunkCoord toChunk(int wx, int wy, int wz) {
        return { floorDiv(wx, Chunk::N), floorDiv(wy, Chunk::N), floorDiv(wz, Chunk::N) };
    }

    // 取或创建一个 Chunk（新建时立刻生成地形+树）
    Chunk* getOrCreate(ChunkCoord c) {
        auto it = chunks.find(c);
        if (it != chunks.end()) return it->second.get();
        auto up = std::make_unique<Chunk>();
        generateTerrain(*up, c.x, c.y, c.z, noise, seed);
        Chunk* ptr = up.get();
        chunks[c] = std::move(up);
        return ptr;
    }

    Block getBlock(int wx, int wy, int wz) const {
        auto it = chunks.find(toChunk(wx, wy, wz));
        if (it == chunks.end()) return Block::Air;
        return it->second->get(floorMod(wx,Chunk::N), floorMod(wy,Chunk::N), floorMod(wz,Chunk::N));
    }

    void setBlock(int wx, int wy, int wz, Block b) {
        Chunk* c = getOrCreate(toChunk(wx, wy, wz));
        c->set(floorMod(wx,Chunk::N), floorMod(wy,Chunk::N), floorMod(wz,Chunk::N), b);
        // 若改的是紧贴边界的格子，邻居 Chunk 的接缝面也要重画
        markNeighborDirty(wx, wy, wz);
    }

    void markNeighborDirty(int wx, int wy, int wz) {
        const int N = Chunk::N;
        auto tryDirty = [&](int x, int y, int z) {
            auto it = chunks.find(toChunk(x, y, z));
            if (it != chunks.end()) it->second->dirty = true;
        };
        if (floorMod(wx,N) == 0)   tryDirty(wx-1, wy, wz);
        if (floorMod(wx,N) == N-1) tryDirty(wx+1, wy, wz);
        if (floorMod(wz,N) == 0)   tryDirty(wx, wy, wz-1);
        if (floorMod(wz,N) == N-1) tryDirty(wx, wy, wz+1);
        if (floorMod(wy,N) == 0)   tryDirty(wx, wy-1, wz);
        if (floorMod(wy,N) == N-1) tryDirty(wx, wy+1, wz);
    }

    // 玩家周围 radius 个区块内没有的，就生成出来
    void updateLoadedChunks(glm::vec3 playerPos, int radius) {
        ChunkCoord center = toChunk((int)std::floor(playerPos.x),
                                    (int)std::floor(playerPos.y),
                                    (int)std::floor(playerPos.z));
        for (int dz = -radius; dz <= radius; ++dz)
        for (int dx = -radius; dx <= radius; ++dx)
        for (int dy = -2; dy <= 2; ++dy)   // 竖直方向少量几层就够
            getOrCreate({ center.x + dx, center.y + dy, center.z + dz });
    }
};
```

逐个说人话：

| 成员 | 干嘛的 |
|------|--------|
| `seed` / `noise` | 世界的身份证。整个世界共用这一台噪声机（第 14 章「创建世界」时玩家输入的种子就传进构造函数） |
| `toChunk` | 方块坐标 → 区块坐标（÷16 向下取整，用的就是打败妖怪的 `floorDiv`） |
| `getOrCreate` | 查表；没有就 new 一个、立刻生成地形+树、登记进表 |
| `getBlock` / `setBlock` | 世界级读写：先找到 Chunk，再用 `floorMod` 算出本地坐标 |
| `markNeighborDirty` | 改了贴边的格子时，把邻居也标脏（复习：**dirty = 数据变了，网格该重建了**。第 08 章挖方块时这个函数会救您一命） |
| `updateLoadedChunks` | 以玩家为中心，横向 radius 圈、纵向 ±2 层，缺的补齐 |

**主人**

`unique_ptr` 是……？

**米雅**

「独占所有权的智能指针」喵。Chunk 里有 4096 字节数据 + GPU 句柄，

不适合在 map 扩容时被整个搬来搬去；用 `unique_ptr` 存，

map 里挪动的只是一根指针，而且 World 销毁时它们会自动释放，不用手写 delete。

---

## 5. 为了画出整个世界：改 main.cpp 和 camera.hpp

现在回到 `src/main.cpp`，做替换手术。对照您第 04 章写完的代码：

**① 顶部 include**：把

```cpp
#include "world/chunk.hpp"
#include "world/block.hpp"
```

换成一句（world.hpp 会把它俩连锁 include 进来）：

```cpp
#include "world/world.hpp"
```

**② 删掉手工豆腐**：`main` 里这一段——

```cpp
Chunk chunk;
for (int z = 0; z < Chunk::N; ++z)
    for (int y = 0; y < Chunk::N; ++y)
        for (int x = 0; x < Chunk::N; ++x)
            chunk.set(x, y, z, y < 8 ? Block::Stone : Block::Air);
```

整段删掉，换成：

```cpp
World world(2024);//种子，换个数字换个世界
```

**③ 改主循环**：找到算 `aspect` 之后、`glfwSwapBuffers` 之前的那一大段（从 `if (chunk.dirty)` 到 `chunk.draw();`），整体换成：

```cpp
world.updateLoadedChunks(gCamera.position, 4);//玩家周围4个区块半径内自动生成

glm::mat4 view = gCamera.view();
glm::mat4 proj = glm::perspective(glm::radians(gCamera.fov), aspect, 0.1f, 500.f);
//透视效果，fov广角 宽高 近的多少不画 远的多少不画

glUseProgram(program);//用刚刚创建的shader小程序
for (auto& [coord, chunkPtr] : world.chunks) {
    Chunk& c = *chunkPtr;
    if (c.dirty) c.rebuildMesh();//脏了才重建

    //每个Chunk顶点是本地0~16坐标，靠model矩阵平移到世界正确位置
    glm::mat4 model = glm::translate(glm::mat4(1.f),
        glm::vec3(coord.x, coord.y, coord.z) * (float)Chunk::N);
    glm::mat4 mvp = proj * view * model;

    glUniformMatrix4fv(glGetUniformLocation(program, "uMVP"),
                       1, GL_FALSE, glm::value_ptr(mvp));
    c.draw();
}
```

**米雅**

复习时间。第 03 章的 MVP 还记得吗喵？

**主人**

Model 把物体摆到世界里，View 把世界变到眼前，Projection 压出透视。

**米雅**

满分！第 04 章只有一个 Chunk 时，`model` 是单位矩阵（摆在原点）。

现在的新花样是：**每个 Chunk 的顶点都是本地 0~16**，长得一模一样，

靠 `model = translate(区块坐标 × 16)` 把它们**摆到各自的位置**。

一份网格逻辑，画满整个世界——这就是矩阵的力量。

`view` 和 `proj` 每帧算一次就够，所以提到循环外面；`model` 每个 Chunk 不同，放循环里面。

**④ 出生点抬高**：**改** `src/camera.hpp`，把

```cpp
glm::vec3 position{ 0.f,0.f,3.f };//对准立方体
```

改成

```cpp
glm::vec3 position{ 8.f,72.f,8.f };//出生在地形上空，方便俯瞰
```

地面高度在 40~64 之间，出生在 72 正好俯瞰群山。不改的话，您会出生在 y=0——石头的最深处，眼前一片实心的黑，还以为程序坏了喵。

---

## 6. 编译运行！

CMake **不用改**（本章全是 `.hpp`，没有新 `.cpp`）。直接构建运行。

**主人**

……第一下卡了一秒多才出画面？

**米雅**

正常喵。第一帧要一口气生成 9×9×5 = 405 个 Chunk 的地形和网格。

以后每帧只补玩家新飞到的那几个，感觉不到的。

（第 17 章的进阶话题里有「多线程生成」，那是以后的事。）

**主人**

哦……哦！！山！是山！还有树！

**米雅**

恭喜主人，从豆腐店老板晋升为世界的造物主喵～

W 一直往前飞，山丘会源源不断地长出来；

回头飞回来，刚才的山**一模一样**——这就是「确定性」三个字的分量。

---

## 7. 两个「先知道，不用管」的小事

1. **接缝处的隐形面**：第 04 章的 `rebuildMesh` 只在本 Chunk 内查邻居（越界当空气），所以两个 Chunk 贴着的地方各自生成了一层朝对方的面。它们被埋在实心地形里**看不见**，只是白白多了点顶点。视觉零影响，优化留到第 17 章。
2. **飞太远内存会涨**：我们只加载不卸载。入门阶段内存完全够用，卸载策略也留到第 17 章。

---

## 8. 概念小抄

| 名词 | 人话 |
|------|------|
| Perlin 噪声 | 平滑、可复现的「随机」函数，喂坐标吐数值 |
| fBm / octave | 多层噪声叠加；大层管山脉小层管坑洼 |
| seed 种子 | 决定整个世界长相的那个数字 |
| 确定性生成 | 同坐标永远同结果 → 跨 Chunk 无缝、可复现 |
| 区块坐标 | 世界坐标 ÷ 16 **向下取整**（`floorDiv`！） |
| 本地存、世界判 | 数组下标用本地坐标，逻辑判断用世界坐标 |
| lambda `[&]` | 就地写的小函数，能借用外面的变量 |

---

## 本章检查点

- [ ] 一直往一个方向飞，连绵山丘源源不断（不是平板、不是针毡）
- [ ] 草地上零星有树；专门去找长在 Chunk 边缘的树，**没有被切成半棵**
- [ ] 把 `main.cpp` 里 `World world(2024)` 的种子改成别的数 → 整个世界大变样；改回来 → 世界复原
- [ ] 往任意方向飞过原点、进入负坐标区，地形依然连续不撕裂（撕裂妖没有复活）

**米雅**

下一章给这个纯色世界**贴上真正的 MC 材质**喵。

→ [06-textures.md](06-textures.md)
