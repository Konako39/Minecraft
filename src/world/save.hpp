#pragma once
#include "world.hpp"
#include "../player/player.hpp"
#include <fstream>
#include <filesystem>
#include <string>

//////////////////////////////////////////////////////////////
// 永远不要信任从磁盘读进来的数字
// 永远不要信任从磁盘读进来的数字
// 永远不要信任从磁盘读进来的数字
// 永远不要信任从磁盘读进来的数字
//////////////////////////////////////////////////////////////
// 
// 存档文件格式：
// 单位字节
// 
// 魔数"OMC1"(4字节)  ——  认门牌：格式不对就拒收
// seed(4)  玩家pos(3×4)  flying(1)
// chunk数量(4)
// 每个chunk：区块坐标xyz(3×4) + RLE压缩的4096个方块
// RLE = Run-Length Encoding：连续N个一样的方块记成「N个×编号」两个数

//只存档读取方块状态玩家位置

//写任意平坦类型（无指针 纯数据）的二进制小工具
template<class T> void writeBin(std::ostream& f, const T& v) {
	f.write((const char*)&v, sizeof(T));
}

template<class T> bool readBin(std::istream& f, T& v) {
	f.read((char*)&v, sizeof(T));
	return (bool)f;//读失败会进入错误状态
}

inline bool saveWorld(const std::string& path, const World& world, const Player& p) {
	std::filesystem::create_directories(std::filesystem::path(path).parent_path());
	std::ofstream f(path, std::ios::binary);
	//Windows 下文本模式会偷偷把 \n 改成 \r\n
	//创建文件夹并准备写入文件
	if (!f) return false;

	f.write("OMC1", 4);
	writeBin(f, world.seed);
	writeBin(f, p.pos.x);writeBin(f, p.pos.y); writeBin(f, p.pos.z);
	writeBin(f, p.flying);
	uint32_t n = (uint32_t)world.chunks.size();
	writeBin(f, n);
	//世界中区块数量算出来后转换成t 写进文件
	const int TOTAL = Chunk::N * Chunk::N * Chunk::N;
	//计算区块中多少方块
	for (auto& [c, ch] : world.chunks) {
		writeBin(f, c.x); writeBin(f, c.y); writeBin(f, c.z);
		//此区块位置
		//RLE压缩：数一数连续多少个同样的方块
		int i = 0;
		while (i < TOTAL) {
			uint8_t id = (uint8_t)ch->blocks[i];
			uint16_t run = 1;
			while (i + run < TOTAL && (uint8_t)ch->blocks[i + run] == id && run < 65535)
				run++;//算从当前位置开始有多少一样的方块
			writeBin(f, run);
			writeBin(f, id);
			i += run;//跳过已经存了的重复的方块
		}
	}
	return (bool)f;
}

inline bool loadWorld(const std::string& path, std::unique_ptr<World>& worldOut, Player& p) {
	std::ifstream f(path, std::ios::binary);
	if (!f) return false;

	char magic[4];
	f.read(magic, 4);
	if (!f || magic[0] != 'O' || magic[1] != 'M' || magic[2] != 'C' || magic[3] != '1')
		return false;//不是存档

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
			if (i + run > TOTAL) return false;//存档损坏，越界
			for (int j = 0; j < run; ++j)
				ch->blocks[i + j] = (Block)id;//方块全放回去
			i += run;
		}
		ch->dirty = true;//读进来的都要重建网格
		worldOut->chunks[c] = std::move(ch);
	}
	return true;

}