#pragma once
#include <cstdint>

enum class Block : uint8_t {
	Air = 0,
	Grass,
	Dirt,
	Stone,
	Log,
	Leaves,
	Sand,
	Water,
	Planks,//木板
	Glass,
	Cobble,
	Bricks,//砖块
	Snow,
	Gravel//沙砾
};

//实心： 挡人档视线。
inline bool isSolid(Block b) { 
	return 
		b != Block::Air &&
		b !=Block::Water;
}
//不透明：挡视线。应用面剔除
inline bool isOpaque(Block b) {
	return
		b != Block::Air &&
		b != Block::Water&&
		b != Block::Glass;
}
