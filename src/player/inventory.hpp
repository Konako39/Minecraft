#pragma once
#include"../world/block.hpp"
#include <vector>


//创造模式物品清单
inline const std::vector<Block>& placeableBlocks() {
	static std::vector<Block> list = {
		Block::Grass, Block::Dirt, Block::Stone, Block::Cobble,
		Block::Log, Block::Planks, Block::Leaves, Block::Sand,
		Block::Glass, Block::Bricks, Block::Snow, Block::Gravel,
		Block::Water,
	};
	return list;
}

//方块显示的英文名
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

//9格快捷栏+当前选中格

struct Hotbar {
	Block slots[9] = {
		Block::Grass, Block::Dirt, Block::Stone, Block::Log,
		Block::Planks, Block::Glass, Block::Cobble, Block::Sand, Block::Bricks,
	};
	int selected = 0;
	Block current() const { return slots[selected]; }

};
