# 06 — 纹理与方块图集

**米雅**

主人，现在的世界是「色块世界」——草是一坨绿，石头是一坨灰。

这一章给它换上**真正的像素贴图**：草方块顶上有草纹、侧面带泥边，树干有年轮喵。

**主人**

贴图从哪来？我可不会画像素画。

**米雅**

米雅已经替主人备好了两样东西，请先确认它们在：

1. `assets/textures/atlas.png` —— 一张 256×256 的**图集**，16×16 个小格，每格 16 像素
2. `include/stb_image.h` —— 读 PNG 用的单头文件库（stb 出品，公有领域）

这两样属于「素材和第三方库」，不算我们要学的代码，直接用。

剩下的**每一行代码**照旧由主人亲手写喵。

---

## 本章动手地图

| 步骤 | 为了…… | 请您现在做 |
|------|--------|------------|
| A | stb 有一份实现体 | **新建** `src/render/texture.cpp`（就两行）+ 加进 CMake |
| B | 把 PNG 上传到 GPU | **新建** `src/render/texture.hpp`：`loadTexture` |
| C | 每个面知道贴哪块图 | **新建** `src/world/block_textures.hpp`：方块×面 → 图块编号 |
| D | 顶点带 UV 而不是颜色 | **改** `chunk.hpp`：`rebuildMesh` 和 `upload` |
| E | GPU 会采样贴图 | **新建** `shaders/chunk.vert` / `chunk.frag`，`main.cpp` 换加载路径 |
| F | 运行目录找得到图 | **改** `CMakeLists.txt`：POST_BUILD 复制 `assets/` |

---

## 1. 先搞懂：图集 + UV 是怎么回事（只阅读）

**米雅**

主人猜猜，为什么不是「一种方块一张图」，而是全拼在一张大图上？

**主人**

嗯……16 种方块 16 张图，GPU 要换 16 次贴图？

**米雅**

正解喵。GPU 换贴图（bind texture）是有开销的，

而把所有小图拼成一张**图集（atlas）**，整个世界一次绑定、一次画完。

MC 老版本那张著名的 `terrain.png` 就是这么干的。

然后是 **UV 坐标**——告诉每个顶点「去大图的哪个位置取色」：

- UV 范围是 0~1：`(0,0)` 是图的**左下角**，`(1,1)` 是右上角
- 想贴第几号小格，就把这个面的 UV 限制在那个小格的子矩形里

> **人话：** 位置 `(x,y,z)` 决定顶点画在**屏幕哪里**；UV `(u,v)` 决定它从**贴图哪里**取色。从这章起每个顶点两样都带。

我们的图集第一行 16 格，编号从左上角数起（0,1,2,...）：

| 编号 | 内容 | 编号 | 内容 |
|------|------|------|------|
| 0 | 草·顶面 | 8 | 沙子 |
| 1 | 石头 | 9 | 水（半透明） |
| 2 | 泥土 | 10 | 玻璃 |
| 3 | 草·侧面 | 11 | 圆石 |
| 4 | 木板 | 12 | 基岩 |
| 5 | 原木·侧面 | 13 | 沙砾 |
| 6 | 原木·顶面 | 14 | 雪 |
| 7 | 树叶（带透明孔） | 15 | 砖块 |

本章先用 0~7；后面的水、沙、木板……是给第 11、13 章预留的库存喵。

---

## 2. 为了读 PNG：texture.cpp 与 texture.hpp

### 2.1 stb 的「一处实现」规矩

**新建** `src/render/texture.cpp`（先建文件夹 `src/render/`），整个文件就这两行：

```cpp
// 整个工程只允许这一处写 STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
```

**主人**

为什么要单独开个 `.cpp` 就写两行？

**米雅**

stb_image 是「单头文件库」：函数**声明和实现**都在一个 `.h` 里。

`#define STB_IMAGE_IMPLEMENTATION` 是开关——定义了它再 include，实现代码才会展开。

这个开关**全工程只能开一次**，不然两个 `.cpp` 里各展开一份实现，链接时就会撞车（重复定义）。

所以惯例是：专门给它一个孤零零的 `.cpp` 当家。

**改** `CMakeLists.txt`，把它加进编译名单（复习第 01 章：每多一个 `.cpp` 都要登记）：

```cmake
add_executable(openglmc
    src/main.cpp
    src/glad.c
    src/render/texture.cpp
)
```

### 2.2 loadTexture：PNG → GPU 纹理

**新建** `src/render/texture.hpp`：

```cpp
#pragma once
#include <glad/glad.h>
#include <stb_image.h>
#include <iostream>

inline GLuint loadTexture(const char* path) {
    int w, h, channels;
    stbi_set_flip_vertically_on_load(true);       // OpenGL 的 UV 原点在左下，需翻转
    unsigned char* data = stbi_load(path, &w, &h, &channels, 4); // 强制 RGBA
    if (!data) { std::cerr << "load texture failed: " << path << '\n'; return 0; }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    // 像素风关键：放大缩小都用 NEAREST，别糊
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    stbi_image_free(data);
    return tex;
}
```

这套流程和第 04 章的 VBO 上传如出一辙，复习一下对照着看：

| VBO（顶点） | 纹理（像素） |
|-------------|--------------|
| `glGenBuffers` 申请 | `glGenTextures` 申请 |
| `glBindBuffer` 绑定 | `glBindTexture` 绑定 |
| `glBufferData` 上传 | `glTexImage2D` 上传 |

**主人**

`flip_vertically` 是在翻转什么？

**米雅**

PNG 文件从**上往下**存像素，OpenGL 的 UV 却以**左下角**为原点。

不翻的话，所有贴图上下颠倒——草长在方块底下，成倒栽葱了喵。

这行让 stb 加载时就把图倒过来，一劳永逸。

`GL_NEAREST` 也很重要：MC 的像素感全靠它。若用默认的 LINEAR，16 像素的小图被拉大后会糊成水彩画。

---

## 3. 为了每个面查对图：block_textures.hpp

**米雅**

草方块顶面是草、底面是土、侧面是「草边土身」——

同一个方块**不同面贴不同图**。所以要一张「方块 × 面 → 图块编号」的查询表。

**新建** `src/world/block_textures.hpp`：

```cpp
#pragma once
#include "block.hpp"

// 图集是 TILES × TILES 个小格。我们的图是 256px、每格 16px → 16
static constexpr int ATLAS_TILES = 16;

// 图块编号：从左上角起、从左到右数（0,1,2,...）
enum Tile {
    T_GRASS_TOP  = 0,
    T_STONE      = 1,
    T_DIRT       = 2,
    T_GRASS_SIDE = 3,
    T_PLANKS     = 4,
    T_LOG_SIDE   = 5,
    T_LOG_TOP    = 6,
    T_LEAVES     = 7,
    T_SAND       = 8,
    T_WATER      = 9,
    T_GLASS      = 10,
    T_COBBLE     = 11,
    T_BEDROCK    = 12,
    T_GRAVEL     = 13,
    T_SNOW       = 14,
    T_BRICKS     = 15,
};

// 给定方块 + 面(0..5 与第 04 章 FACE 顺序一致：+X -X +Y -Y +Z -Z)，返回图块编号
inline int tileOf(Block b, int face) {
    switch (b) {
        case Block::Grass:
            if (face == 2) return T_GRASS_TOP;    // 上面
            if (face == 3) return T_DIRT;         // 底面是土
            return T_GRASS_SIDE;                  // 四个侧面
        case Block::Dirt:   return T_DIRT;
        case Block::Stone:  return T_STONE;
        case Block::Log:
            return (face == 2 || face == 3) ? T_LOG_TOP : T_LOG_SIDE;
        case Block::Leaves: return T_LEAVES;
        default:            return T_STONE;
    }
}

// 图块编号 → 它在图集里的 UV 子矩形 [u0,v0] 到 [u1,v1]
inline void tileUV(int tile, float& u0, float& v0, float& u1, float& v1) {
    int col = tile % ATLAS_TILES;
    int row = tile / ATLAS_TILES;
    float s = 1.f / ATLAS_TILES;
    u0 = col * s;  u1 = u0 + s;
    // 因为加载时上下翻转过，行要从底部数
    v0 = 1.f - (row + 1) * s;  v1 = v0 + s;
}
```

**主人**

`face == 2` 是上面……这个 0~5 的编号哪来的？

**米雅**

复习第 04 章喵：`FACE` 表的顺序是 `+X, -X, +Y, -Y, +Z, -Z`。

所以 2 号是 +Y（顶面）、3 号是 −Y（底面），其余四个都是侧面。

`tileOf` 的 `face` 参数将来就传 `rebuildMesh` 里那个 `f`，一一对应。

---

## 4. 为了顶点带 UV：改 chunk.hpp

现在动第 04 章的心脏。顶点格式从 `x,y,z, r,g,b`（6 float）改成 `x,y,z, u,v`（5 float）。要动四处。

**① 顶部 include**（`#include "block.hpp"` 下面加一行）：

```cpp
#include "block_textures.hpp"
```

**② 加 UV 角表**：在 `NEIGHBOR` 表后面加：

```cpp
// 4 个角对应到图块的四角(0/1)。顺序和 FACE 的角顺序对齐。
static const float UVCORNER[4][2] = { {0,0},{1,0},{1,1},{0,1} };
```

**主人**

这表什么意思？

**米雅**

第 04 章 `FACE` 里每个面有 4 个角，`order = {0,1,2,0,2,3}` 把它们连成两个三角。

贴图的小格也有 4 个角。`UVCORNER` 说的是：**面的第 0 个角贴图块的左下、第 1 个角贴右下、第 2 个角贴右上、第 3 个角贴左上**。角对角，图就不歪。

**③ 改 `rebuildMesh` 内层**：找到 `glm::vec3 col = colorOf(b);` 这行——**删掉**（颜色退休了）。然后把「检查每个面的邻居」那个 `for (int f...)` 循环里、剔除判断之后的顶点生成段改成：

```cpp
//这个面用图集里哪个图块？算出它的UV子矩形
float u0, v0, u1, v1;
tileUV(tileOf(b, f), u0, v0, u1, v1);

for (int k = 0;k < 6;++k) {
    int ci = order[k];//角的编号 012023
    const float* corner = FACE[f][ci];
    //加上xyz得到方块这个面6个点的本地坐标
    verts.push_back(x + corner[0]);
    verts.push_back(y + corner[1]);
    verts.push_back(z + corner[2]);
    //UV：把角的0/1映射进这块图块的子矩形
    float tu = UVCORNER[ci][0], tv = UVCORNER[ci][1];
    verts.push_back(u0 + tu * (u1 - u0));
    verts.push_back(v0 + tv * (v1 - v0));
}
```

和原来的差别：原来 `FACE[f][order[k]]` 一步到位，现在先把 `order[k]` 存成 `ci`——因为**位置和 UV 要用同一个角号**，得存下来用两次。

**④ 改 `upload`**：顶点从 6 float 变 5 float，三处数字要同步：

```cpp
vertexCount = (int)verts.size() / 5;//5个float一个顶点：x,y,z,u,v
```

属性指针（`glBufferData` 之后那两组）：

```cpp
// 属性 0：位置(3 个 float)，从每个顶点第 0 个 float 开始
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);

// 属性 1：UV(2 个 float)，偏移3个float
glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
glEnableVertexAttribArray(1);
```

`colorOf` 函数本身可以留着不删（万一想切回调试用），只是没人调用了。

---

## 5. 为了 GPU 采样：新建 chunk.vert / chunk.frag

**米雅**

复习第 02 章：**顶点着色器**决定顶点落在屏幕哪，**片元着色器**决定每个像素什么色。

原来的 `basic.*` 收位置+颜色；现在要收位置+UV，所以新开一套（旧的留着，以后画辅助线还有用）。

**新建** `shaders/chunk.vert`：

```glsl
//区块顶点着色器：位置 + UV
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
uniform mat4 uMVP;
out vec2 vUV;
void main() {
    vUV = aUV;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
```

**新建** `shaders/chunk.frag`：

```glsl
//区块片元着色器：从图集取色
#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uAtlas;
void main() {
    vec4 c = texture(uAtlas, vUV);
    if (c.a < 0.5) discard;      // 树叶的透明像素直接丢弃，露出后面
    FragColor = c;
}
```

—— ✦ 空气突然变得阴森 ✦ ——

**主人**

等等，`discard` 那行是干嘛的？没有它会怎样？

**黑斑妖·阿尔法** 登场

**黑斑妖·阿尔法**

哼哼哼……问得好啊人类。

把那行删了试试？你的树叶会变成**一坨坨漆黑的补丁**！

**主人**

你谁啊。

**黑斑妖·阿尔法**

树叶贴图上不是有透明的孔吗？

透明像素也是像素！它有颜色值（黑）和透明度（0）！

深度测试可**不管透明度**——树叶这个面画上去，

后面的东西就被我挡住了，透明的孔糊成黑斑！

嘻嘻，这就是无数新手的第一片「黑森林」！

**米雅**

所以才有 `if (c.a < 0.5) discard;` 喵。

`discard` 是片元着色器的特权：**这个像素我不画了，当我没来过**——

深度也不写，后面的东西照常可见。透明孔就真的透明了。

**主人**

半透明的东西（比如水）也这样处理吗？

**米雅**

水是「半透明」不是「全透明」，`discard` 这种一刀切就不够用了，

要真正的混合（blending）——那是第 11 章的怪，今天先打眼前这只。

**黑斑妖·阿尔法**

可恶……记住，哪天你换了张树叶贴图忘了留 alpha 通道……

**黑斑妖·阿尔法** 被击败

---

## 6. 为了用上这一切：改 main.cpp 和 CMake

**① include**（顶部）：

```cpp
#include "render/texture.hpp"
```

**② 换 shader 路径**：把 `loadShaderProgram("shaders/basic.vert", "shaders/basic.frag")` 改成：

```cpp
unsigned int program = loadShaderProgram("shaders/chunk.vert", "shaders/chunk.frag");
```

**③ 加载图集**：在 shader 加载成功的判断之后加：

```cpp
//加载方块图集（一张大图装所有方块的小贴图）
GLuint atlas = loadTexture("assets/textures/atlas.png");
if (!atlas) {
    std::cerr << "Failed to load atlas\n";
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
}
```

**④ 每帧绑定**：主循环里 `glUseProgram(program);` 之后、遍历 chunks 之前加三行：

```cpp
glActiveTexture(GL_TEXTURE0);//激活0号纹理单元
glBindTexture(GL_TEXTURE_2D, atlas);//把图集绑上去
glUniform1i(glGetUniformLocation(program, "uAtlas"), 0);//告诉shader用0号
```

**米雅**

「纹理单元」= GPU 的贴图插槽，编号 0、1、2……

我们把图集插在 0 号槽，再告诉 shader 里的 `uAtlas`「你去 0 号槽取图」。

只有一张图时永远用 0 号就行。

**⑤ CMake 复制 assets**：仿照复制 shaders 的写法，`CMakeLists.txt` 末尾加：

```cmake
# 运行时需要 assets/（贴图等）
add_custom_command(TARGET openglmc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:openglmc>/assets
)
```

改了 CMakeLists 后 VS 会自动重新配置；然后构建运行。

**主人**

……！草地有草纹了！树干上还有木纹！

**米雅**

飞近树叶看看孔洞后面——能透出天空吧？那就是主人刚打败的黑斑妖喵。

---

## 概念小抄

| 名词 | 人话 |
|------|------|
| 图集 atlas | 很多小贴图拼成一张大图，一次绑定画全世界 |
| UV | 顶点在贴图上的取色坐标，0~1，原点左下 |
| 纹理单元 | GPU 的贴图插槽（0号、1号…） |
| NEAREST | 取最近像素，保像素风；LINEAR 会糊 |
| discard | 片元着色器丢弃当前像素，全透明像素专用 |
| 单头文件库 | 声明+实现全在一个 .h，用 IMPLEMENTATION 宏在唯一一处展开 |

---

## 本章检查点

- [ ] 草方块：顶面草纹、侧面草边土身、底面纯土
- [ ] 树干侧面木纹、顶面年轮（挖开树冠俯视树桩能看到——哦不对，还不能挖，那就飞到树顶看）
- [ ] 树叶的透明孔真的透明（黑斑妖没有复活）
- [ ] 像素边缘锐利不糊（NEAREST 生效）

**米雅**

但是主人有没有觉得……每个面一样亮，像纸糊的？

下一章一分钟给它立体感喵。→ [07-lighting.md](07-lighting.md)
