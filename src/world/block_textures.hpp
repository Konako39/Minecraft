//判断方块哪一面画什么

#pragma once
#include "block.hpp"

//目前使用的图 256px、每格16
static constexpr int ATLAS_TILES = 16;

//编号
enum Tile {
	T_GRASS_TOP = 0,
    T_STONE = 1,
    T_DIRT = 2,
    T_GRASS_SIDE = 3,
    T_PLANKS = 4,
    T_LOG_SIDE = 5,
    T_LOG_TOP = 6,
    T_LEAVES = 7,
    T_SAND = 8,
    T_WATER = 9,
    T_GLASS = 10,
    T_COBBLE = 11,
    T_BEDROCK = 12,
    T_GRAVEL = 13,
    T_SNOW = 14,
    T_BRICKS = 15,
};

//给方块各面赋予材质 (和 FACE 顺序一致：+X -X +Y -Y +Z -Z)，返回图块编号
inline int tileOf(Block b, int face) {
    switch (b) {
    case Block::Grass:
        if (face == 2) return T_GRASS_TOP;//上面
        if (face == 3) return T_DIRT; //底面
        return T_GRASS_SIDE;//侧边
    case Block::Dirt:   return T_DIRT;
    case Block::Stone:  return T_STONE;
    case Block::Log:
        return (face == 2 || face == 3) ? T_LOG_TOP : T_LOG_SIDE;
    case Block::Leaves: return T_LEAVES;
    case Block::Sand:return T_SAND;
    case Block::Water:return T_WATER;
    case Block::Planks: return T_PLANKS;
    case Block::Glass:  return T_GLASS;
    case Block::Cobble: return T_COBBLE;
    case Block::Bricks: return T_BRICKS;
    case Block::Snow:   return T_SNOW;
    case Block::Gravel: return T_GRAVEL;
    default:            return T_STONE;
    }
}

//图块编号
inline void tileUV(int tile, float& u0, float& v0, float& u1, float& v1) {
    int col = tile % ATLAS_TILES;
    int row = tile / ATLAS_TILES;
    //求行列
    float s = 1.f / ATLAS_TILES;
    //一个格子占多大UV UV永远是0-1 这里是16 一个格子就占1/16
    u0 = col * s;
    u1 = u0 + s;
    //加载的时候上下反转过，所以从底部数
    v0 = 1.f - (row + 1) * s;
    v1 = v0 + s;
    //这里uv01 就是左右上下边界了，就定位到此材质了
}