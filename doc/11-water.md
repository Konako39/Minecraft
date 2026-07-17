# 11 — 水：半透明的大挑战

**米雅**

主人，现在的世界什么都好，就是太**干**了。

山谷里应该有湖，低地应该有海，湖边应该有沙滩——这一章全部安排上喵。

**主人**

水嘛……不就是加一种蓝色方块？

**米雅**

如果水是不透明的，确实五分钟就完事。

可水要**半透明**——透过水面能看见水底的沙子。半透明是渲染世界的一道分水岭：

它会同时挑战我们的**面剔除**、**深度缓冲**和**绘制顺序**三套系统。

做完这章，主人对渲染的理解会上一个台阶。

## 本章动手地图

| 步骤 | 为了…… | 请您现在做 |
|------|--------|------------|
| A | 有水和沙两种方块 | **改** `block.hpp`：enum + 拆出 `isSolid` / `isOpaque` 两个判断 |
| B | 低地自动有海、湖、沙滩 | **改** `terrain.hpp`：海平面 `SEA_LEVEL` |
| C | 水单独一份网格 | **改** `chunk.hpp`：双顶点列表 + `drawWater` |
| D | 正确的半透明绘制 | **改** `main.cpp`：先不透明后水、开混合、关深度写 |
| E | 准星穿水、人能游泳 | **改** `raycast.hpp` / `player.hpp`：浮力与踩水 |
| F | 头进水里有沉浸感 | **改** `main.cpp` UI：全屏蓝色滤镜 |

---

## 1. 为了区分两种「挡」：改 block.hpp

**米雅**

先想清楚水的哲学问题：水「挡」什么？

**主人**

呃……挡视线？不对，能看穿。挡人？也不对，人能泡进去……

**米雅**

正是这个纠结，说明一个 `isSolid` 不够用了。「挡」其实是两件事：

- **挡身体**（碰撞、射线）→ 石头挡、水**不挡**
- **挡视线**（面剔除：邻居挡住了这面就不用画）→ 石头挡、水**不挡**——水下的沙地要照画！

对现有方块两者恰好一致，但语义不同，以后还会分家（下下章的玻璃：挡身体、**不**挡视线）。

**改** `src/world/block.hpp` 整个文件：

```cpp
#pragma once
#include <cstdint>

enum class Block : uint8_t {
	Air = 0,
	Grass,
	Dirt,
	Stone,
	Log,      // 树干
	Leaves,   // 树叶
	Sand,     // 沙子（水边）
	Water,    // 水
};

//「实心」：挡人、挡射线。水不实心——人能泡进去，准星能穿过去
inline bool isSolid(Block b) { return b != Block::Air && b != Block::Water;
}

//「不透明」：挡视线。面剔除用它——邻居不透明这面才不用画
inline bool isOpaque(Block b) { return b != Block::Air && b != Block::Water;
}
```

**改** `src/world/block_textures.hpp` 的 `tileOf`，`case Block::Leaves` 后面加两行（图块 8 和 9 早在第 06 章的图集里备着了）：

```cpp
case Block::Sand:   return T_SAND;
case Block::Water:  return T_WATER;
```

---

## 2. 为了有海有沙滩：改 terrain.hpp

规则一句话：**海平面以下的空地填水；地表贴近或低于海平面的地方，草和土换成沙。**

**①** `terrain.hpp` 里 `heightAt` 之前加常量：

```cpp
// 海平面：这个高度以下的空地会被水填满
static constexpr int SEA_LEVEL = 44;
```

（复习：地面高度范围是 40~64，所以 44 意味着最低洼的地方会淹出湖海。）

**②** `generateTerrain` 的内层填充改成：

```cpp
bool beach = (h <= SEA_LEVEL + 1);            // 水边/水下的地表换成沙

for (int ly = 0; ly < N; ++ly) {
    int wy = coordY * N + ly;      // 这一格的世界高度
    Block b;
    if      (wy > h)      b = (wy <= SEA_LEVEL) ? Block::Water : Block::Air;
                                              // 地面以上：海平面以下填水，否则空气
    else if (wy == h)     b = beach ? Block::Sand : Block::Grass; // 最顶一格
    else if (wy > h - 4)  b = beach ? Block::Sand : Block::Dirt;  // 往下 3 格
    else                  b = Block::Stone;   // 再往下：石头
    chunk.set(lx, ly, lz, b);                 // 存进本地坐标
}
```

**③** `plantTrees` 里，算出 `ground` 之后加一行（树不长在水里和沙滩上）：

```cpp
if (ground <= SEA_LEVEL + 1) continue;          // 水里和沙滩上不长树
```

---

## 3. 半透明的三条军规（只阅读，重要！）

**米雅**

改渲染之前，先听我讲半透明的三条军规，不然一会儿妖怪出来您接不住喵。

**军规一：先画所有不透明的，再画半透明的。**

混合（blend）的公式是「新颜色 × α + **已经画在画布上的颜色** × (1−α)」。

水要和「水底的沙子」混合，那沙子必须**先**在画布上。顺序反了就没得混。

**军规二：画半透明时，关掉深度写入（depth write）。**

**主人**

深度写入和深度测试……不是一回事？

**米雅**

两个开关喵。**测试** = 画之前问「我被挡住了吗」；**写入** = 画完后登记「这里被我占了」。

水要**参加测试**（水在山后面时不该画出来），

但**不该写入**——水面登记了深度，另一片更远的水面就会被它「挡住」直接消失，

透过一层水看第二层水，是很正常的需求。

**军规三：同类透明物之间，理论上要按距离排序。**

我们只有水一种半透明物、且水面基本共面，不排序也看不出破绽，这条先记账不执行。

---

## 4. 为了水单独成网格：改 chunk.hpp

由三条军规推出结构：**水的面和不透明的面必须分成两份网格，分开时机画**。

**①** `struct Chunk` 里，紧跟原有 `vao/vbo/vertexCount` 加：

```cpp
	//水单独一套网格：不透明的先画、水最后画（半透明的规矩）
	GLuint waterVao = 0, waterVbo = 0;
	int waterVertexCount = 0;
```

**②** 声明区：把 `void upload(...)` 那行**替换**成：

```cpp
	void drawWater();
	//通用上传：往指定的 vao/vbo 传顶点（不透明和水两套都用它）
	static void uploadMesh(GLuint& vao, GLuint& vbo, int& count, const std::vector<float>& verts);
```

**③** 把 `inline void Chunk::upload(...)` 的**函数头**改成通用版（函数体一个字不用动，只是 `vertexCount` 改叫 `count`）：

```cpp
inline void Chunk::uploadMesh(GLuint& vao, GLuint& vbo, int& count, const std::vector<float>& verts) {
	count = (int)verts.size() / 6;//6个float一个顶点：x,y,z,u,v,light
	// ……以下原封不动（vao/vbo 现在是参数，正好同名）……
```

**米雅**

这叫**参数化重构**：原来它焊死在成员 `vao/vbo/vertexCount` 上，

现在让调用者指定往哪套缓冲传。同一份逻辑，服务两份网格，一行都不重复喵。

**④** `draw()` 后面加：

```cpp
inline void Chunk::drawWater() {
	if (waterVertexCount == 0) return;
	glBindVertexArray(waterVao);
	glDrawArrays(GL_TRIANGLES, 0, waterVertexCount);
}
```

**⑤** 改 `rebuildMesh`。三处变化：开头两个列表；面剔除分家；结尾两次上传：

```cpp
inline void Chunk::rebuildMesh() {
	std::vector<float> verts;      //不透明方块的顶点
	std::vector<float> waterVerts; //水的顶点（半透明，要分开最后画）
	static const int order[6] = { 0,1,2,0,2,3 };
```

遍历里，原来的 `if (!isSolid(b)) continue;` 改成：

```cpp
Block b = get(x, y, z);
if (b == Block::Air) continue;//空气不生成面

bool isWater = (b == Block::Water);
```

面剔除处，原来的 `if (isSolid(get(nx,ny,nz))) continue;` 改成：

```cpp
Block nb = get(nx, ny, nz);
if (isWater) {
    //水的面：只有贴着空气才画（水和水之间、水贴石头都不画）
    if (nb != Block::Air) continue;
} else {
    //普通方块：邻居不透明就不画（水是透明的，所以水下的地面照画）
    if (isOpaque(nb)) continue;
}
```

**主人**

等等我理一下……沙子贴着水：水不透明吗？不，`isOpaque(水)=false`，所以沙子这面**要画**——

对哦！不画的话，透过水看下去，水底就是空洞了！

**米雅**

完全正确！这就是 `isSolid`/`isOpaque` 分家的第一笔回报喵。

顶点生成前选好去哪个列表：

```cpp
std::vector<float>& out = isWater ? waterVerts : verts;
```

（后面 6 个 `verts.push_back` 全改成 `out.push_back`。）

函数结尾，`upload(verts)` 换成两次上传：

```cpp
	uploadMesh(vao, vbo, vertexCount, verts);
	uploadMesh(waterVao, waterVbo, waterVertexCount, waterVerts);
	dirty = false;
}
```

---

## 5. 为了画出来：改 main.cpp —— 妖怪登场

主循环里，画完所有 Chunk 的 `for` 循环之后、描边框之前，加第二遍水渲染：

```cpp
//第二遍画水：开混合(半透明)、关深度写入(水面不挡后面的水面)
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glDepthMask(GL_FALSE);
for (auto& [coord, chunkPtr] : world.chunks) {
    glm::mat4 model = glm::translate(glm::mat4(1.f),
        glm::vec3(coord.x, coord.y, coord.z) * (float)Chunk::N);
    glm::mat4 mvp = proj * view * model;
    glUniformMatrix4fv(glGetUniformLocation(program, "uMVP"),
                       1, GL_FALSE, glm::value_ptr(mvp));
    chunkPtr->drawWater();
}
glDepthMask(GL_TRUE);
glDisable(GL_BLEND);
```

**幽湖妖·薄透** 登场

**幽湖妖·薄透**

呵呵呵……又一个想驯服「半透明」的凡人……

告诉我，人类。要是你偷懒，不分两遍，把水混在石头里**一遍画完**呢？

**主人**

呃……军规一：水先画的话，水底还没画上画布，混合混了个寂寞……

水会跟**天空**混色，然后沙子把水盖掉？总之全乱了。

**幽湖妖·薄透**

哼，背了书嘛。那——`glDepthMask(GL_FALSE)` 这行删掉呢？

**主人**

军规二：水面写了深度，后面的水面被它挡住……

湖面会缺一大块？或者角度一变，远处的水时隐时现！

**幽湖妖·薄透**

……啧。那最后一问：为什么水画完要 `glDepthMask(GL_TRUE)` 拨回去？！

**主人**

状态机的公德！第 09 章米雅讲的：谁弄乱谁还原——

不还原的话，下一帧连**不透明的世界**都不写深度了，整个画面前后乱套！

**幽湖妖·薄透**

三问全对……无趣，无趣！！沉了沉了……

**幽湖妖·薄透** 被击败

**米雅**

主人已经能自己推演渲染 bug 了，米雅有点欣慰喵。

> 顺带记账：您以后可能注意到**贴着区块边界**的水下，隐约多出一层竖着的水面。那是因为 `rebuildMesh` 越界查邻居时当作空气，边界两侧各画了一面。属于已知小瑕疵，等第 17 章「跨 Chunk 查邻居」一并根治。

---

## 6. 为了能游泳：改 raycast.hpp 和 player.hpp

**① 准星穿水**：`raycast.hpp` 里命中判断从 `!= Block::Air` 改成：

```cpp
if (isSolid(world.getBlock(cell.x, cell.y, cell.z))) {//水不算，准星穿水而过
```

不改的话，您对着湖面右键，方块会放在水面上——想捞水底的沙都捞不到喵。

**② 浮力**：`player.hpp` 里 `updatePlayer` 前面加：

```cpp
// 身体中心泡在水里吗
inline bool inWater(const World& world, const Player& p) {
	glm::ivec3 c = glm::ivec3(glm::floor(p.pos + glm::vec3(0.f, 0.9f, 0.f)));
	return world.getBlock(c.x, c.y, c.z) == Block::Water;
}
```

`updatePlayer` 开头的重力段替换成：

```cpp
	bool swimming = inWater(world, p);
	if (!p.flying) {
		p.vel.y -= (swimming ? 8.f : 32.f) * dt;       // 水里浮力抵掉大半重力
		p.vel.y = std::max(p.vel.y, swimming ? -4.f : -60.f); // 水里下沉慢
	}
```

**③ 踩水**：`main.cpp` 里跳跃那段改成：

```cpp
} else {
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if (inWater(world, gPlayer))     gPlayer.vel.y = 4.f;//踩水上浮
        else if (gPlayer.onGround)       gPlayer.vel.y = 9.f;//起跳
    }
}
```

**④ 水下滤镜**：`ui.begin(fbw, fbh);` 之后、画准星之前加：

```cpp
//头泡在水里时，全屏罩一层蓝
glm::ivec3 eyeCell = glm::ivec3(glm::floor(gCamera.position));
if (world.getBlock(eyeCell.x, eyeCell.y, eyeCell.z) == Block::Water)
    ui.rect(0, 0, (float)fbw, (float)fbh, { 0.2f, 0.4f, 0.9f, 0.35f });
```

一个 UI 矩形而已，但潜下水的一瞬间，蓝光罩下来——沉浸感直接拉满，这就是第 09 章那笔投资的又一次分红喵。

---

## 概念小抄

| 词 | 人话 |
|----|------|
| isSolid vs isOpaque | 挡身体 ≠ 挡视线；水：都不挡，玻璃(将来)：只挡身体 |
| 混合 blend | 新色 × α + 画布已有色 × (1−α) |
| 深度测试 vs 深度写入 | 「我被挡了吗」vs「登记我占了这里」，两个独立开关 |
| 两遍绘制 | 先全部不透明，后全部半透明 |
| 海平面 | 低于它的空地一律填水 |

---

## 本章检查点

- [ ] 低地有湖海，边缘一圈沙滩，水下地面是沙
- [ ] 透过水面能看见水底（军规一生效）
- [ ] 斜着看远处多层水面都在（军规二生效）
- [ ] 走进水里会缓缓下沉，按住空格能游上来
- [ ] 头没入水面，全屏泛蓝
- [ ] 准星能穿过水选中水底的方块

**米雅**

世界有声有色了，但主人有没有发现——我们到现在**一个字都没在屏幕上写过**？

坐标、FPS、以后的菜单按钮，都需要文字。下一章就造字喵。

→ [12-text-rendering.md](12-text-rendering.md)
