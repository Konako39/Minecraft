#pragma once
#include "chunk.hpp"
#include "terrain.hpp"
#include <memory>
#include <unordered_map>
#include <cmath>
#include <glm/glm.hpp>

//区块坐标
struct ChunkCoord {
    int x, y, z;
    //现在无法比 ChunkCoord a 和 ChunkCoord b
    //下面是重载(布尔运算重载) 如果现在比 a==b 会被换成
    // a.operator==(b)
    //就变成 a.x a.y a.z   o.x o.y o.z(b换成o) 然后return三个比较 就是个比较重载
    bool operator==(const ChunkCoord& o) const {
        return x == o.x && y == o.y && z == o.z;
    }
    //两个坐标什么时候算同一个坐标（比较啦）

};
struct ChunkCoordHash {
    size_t operator()(const ChunkCoord& c) const {
        return (size_t)(c.x * 73856093) ^ (size_t)(c.y * 19349663) ^ (size_t)(c.z * 83492791);
    }//一个哈希值，能把上面那个区块坐标快速变成一个数字
};

////维持负坐标的正常运作
inline int floorDiv(int a, int b) {
    return (a >= 0) ? a / b : -((-a + b - 1) / b);
}//把负数往下取整

inline int floorMod(int a, int b) {
    int m = a % b;
    return m < 0 ? m + b : m;
}//把余数拉回 [0, 16)

class World {
public:
    uint32_t seed;
    Perlin noise;
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> chunks;
    //unordered_map就是大字典，也就是chunks大表里，key是ChunkCoord也就是区块坐标，值是Chunk 哈希
    //坐标 内容 哈希（我定义的ChunkCoord该怎么存呢？就用这个哈希存）

    World(uint32_t s = 2026) :seed(s), noise(s) {}
    //先默认搞一个这个世界 种子2026的

    static ChunkCoord toChunk(int wx, int wy, int wz) {
        return { floorDiv(wx, Chunk::N), floorDiv(wy, Chunk::N), floorDiv(wz, Chunk::N) };
        //世界坐标定位到区块坐标
    }

    //取或者创建一个区块（新建的时候立刻生成地形和树）
    Chunk* getOrCreate(ChunkCoord c) {
        auto it = chunks.find(c);//传进来的世界坐标寻找区块
        if (it != chunks.end()) return it->second.get();//end是找到最后了，不是就算找到了。
        //找到了就读取第二个，第二个是值 就算那个区块的索引
        //不然就↓
        auto up = std::make_unique<Chunk>();//创建个新区块
        generateTerrain(*up, c.x, c.y, c.z, noise, seed);
        Chunk* ptr = up.get();//upget，保存原始指针Chunk，up指向chunk
        chunks[c] = std::move(up);//生成好的区块存到刚刚的大字典chunks里
        return ptr;
    }
    void markNeighborDirty(int wx,int wy,int wz){
        //修改一个方块后，周围需要重新生成网格
        const int N = Chunk::N;

        auto tryDirty = [&](int x, int y, int z) {
            auto it = chunks.find(toChunk(x, y, z));
            if (it != chunks.end()) it->second->dirty = true;
            };//进来的世界坐标找区块，只要找到了就设置为脏

        if (floorMod(wx, N) == 0) tryDirty(wx-1,wy,wz);
        //如果余N是0，就在区块边上，那左边的那个区块就需要重建
        //六个方向
        if (floorMod(wx, N) == N - 1) tryDirty(wx + 1, wy, wz);
        if (floorMod(wz, N) == 0)   tryDirty(wx, wy, wz - 1);
        if (floorMod(wz, N) == N - 1) tryDirty(wx, wy, wz + 1);
        if (floorMod(wy, N) == 0)   tryDirty(wx, wy - 1, wz);
        if (floorMod(wy, N) == N - 1) tryDirty(wx, wy + 1, wz);
    }

    Block getBlock(int wx, int wy, int wz)const {
        auto it = chunks.find(toChunk(wx, wy, wz));
        if (it == chunks.end()) return Block::Air;//如果此世界坐标的区块是空那就是空气
            return it->second->get(floorMod(wx, Chunk::N), floorMod(wy, Chunk::N), floorMod(wz, Chunk::N));
        //不然就 it -> 第二个(区块的值) ->get(世界坐标余出来，获得区块里那个方块的坐标)
    }

    void setBlock(int wx, int wy, int wz, Block b) {
        Chunk* c = getOrCreate(toChunk(wx, wy, wz));
        c->set(floorMod(wx, Chunk::N), floorMod(wy, Chunk::N), floorMod(wz, Chunk::N), b);
        // 若改的是紧贴边界的格子，邻居 Chunk 的接缝面也要重画
        markNeighborDirty(wx, wy, wz);
    }

    //玩家周围radius个区块没有的，就生成出来
    void updateLoadedChunks(glm::vec3 playerPos, int radius) {
        ChunkCoord center = toChunk((int)std::floor(playerPos.x),
            (int)std::floor(playerPos.y),
            (int)std::floor(playerPos.z));//floor向下取整

            for (int dz = -radius;dz <= radius;++dz)
                for (int dx = -radius;dx <= radius;++dx)
                    for (int dy = -2;dy <= 2;++dy)//垂直没必要很多
                        getOrCreate({center.x+dx,center.y+dy,center.z+dz});
    }


};   