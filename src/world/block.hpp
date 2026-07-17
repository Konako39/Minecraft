#pragma once
#include <cstdint>

enum class Block : uint8_t {
	Air = 0,
	Grass,
	Dirt,
	Stone,
	Log,
	Leaves,
};

inline bool isSolid(Block b) { return b != Block::Air;
}