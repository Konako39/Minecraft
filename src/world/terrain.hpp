#pragma once
#include"chunk.hpp"
#include"noise.hpp"
#include <iostream>
#include<cstdlib>

static constexpr int SEA_LEVEL = 50;
//常量 此高度下的空地被水占满

inline int heightAt(const Perlin& noise, int wx, int wz) {
	//这里noise里装了perm[512]的随机数组
	//给世界坐标的一列（wx,wz）,返回高度wy
	float e = fbm(noise, wx * 0.01f, wz * 0.01f, 4);
	//*0.01是因为柏林噪声 走1格变一个样子 方块也是1方块1格子，*0.01代表100个方块一个样子
	e = e * 0.5f + 0.5f; //e出来后是-1 - 1的噪声，映射到0-1
	return 40 + (int)(e * 24);//基准高度40 最高再抬24格
}

//哈希生成算法 用种子和xz来吐一个地图里，一个坐标的确定的哈希值
//然后到isTreeBase去用这个哈希值判断该不该出树
inline  uint32_t hash2(int x,int z, uint32_t seed){
	uint32_t h = (uint32_t)(x * 73856093) ^ (uint32_t)(z * 19349663) ^ seed;
	h ^= h >> 13;//shif运算 右移13位
	h *= 0x5bd1e995u; 
	h ^= h >> 15;
	//混合 乘常数 混合
	//就算相差1个数出来的哈希也不一样 这就是一个生成哈希的算法
	return h;
}

//判断树是不是一颗树的根部
inline bool isTreeBase(int wx, int wz, uint32_t seed) {
	return (hash2(wx, wz, seed) % 40u) == 0u;
	// 算出哈希值，取余40，结果肯定是0 - 39.
	// 只有余为0的时候才返回true。也就是概率为1/40
	// 地图上每一列(wx,wz)都会长，但平均40个格才会长出数
}



//种树，只生成本区块内该有的树的结构
//给区块 区块坐标 噪音和种子，通过种子和坐标用哈希计算出xz平面击中种树的方块
//然后根据种子和坐标计算出高，随后画随机出来的高度树干，然后画树叶
inline void plantTrees(Chunk& chunk, int coordX, int coordY, int coordZ, 
	const Perlin& noise, uint32_t seed) {

	const int N = Chunk::N;//大小 N*N*N的，怎么多个方块 压成了1维数组 在Chunk那个cpp里
	const int MARGIN = 2;//叶子半径 向外最多扫描2格，邻区有树伸过来了就找到

	for (int wz = coordZ * N - MARGIN;wz < coordZ * N + N + MARGIN; ++wz)	//cooedZ是区块在z上的编号，-MARGIN是前面2格，coordZ *N +N +MARGIN是中间和后面2格
		for (int wx = coordX * N - MARGIN;wx < coordX * N + N + MARGIN;++wx) {//这里再把x遍历了，就xz，这个区块周围全遍历了
			if (!isTreeBase(wx, wz, seed)) continue;
			//打中种树点 这里没命中就下一个
			int ground = heightAt(noise, wx, wz);//计算出地面高 也就是下面generateTerrain算出来的地形
			if (ground <= SEA_LEVEL + 1) continue;//不长在沙子和水里
			int trunkH = 4 + (int)(hash2(wx, wz, seed) >> 8) % 3;//树干高4-6 %3就是出012了 哈希随机高（这个坐标和种子来算）
			int baseY = ground + 1;

			auto place = [&](int wx2, int wy2, int wz2, Block b) {
				int lx = wx2 - coordX * N;
				int ly = wy2 - coordY * N;
				int lz = wz2 - coordZ * N;
				//世界坐标-区块坐标（xyz上第几个，*N代表到那个范围内）
				//这里把世界坐标转成局部坐标 如果大于N了或者小于0了就不在本区块
				if (lx < 0 || ly < 0 || lz < 0 || lx >= N || ly >= N || lz >= N)
					return;//不在本区块，不该我画
				chunk.set(lx, ly, lz, b);
				};
			//树干
			for (int i = 0;i < trunkH;++i)
				place(wx, baseY + i, wz, Block::Log);
			//树叶 下面两层去了四角的5x5 最上面一层3x3
			int topY = baseY + trunkH - 1;
			for (int dy = 0; dy <= 1; ++dy)
				for (int dz = -2; dz <= 2; ++dz)
					for (int dx = -2; dx <= 2; ++dx) {
						if (std::abs(dx) == 2 && std::abs(dz) == 2) continue; // 抹掉四角更圆
						place(wx + dx, topY - 1 + dy, wz + dz, Block::Leaves);
					}
			for (int dz = -1;dz <= 1;++dz)
				for (int dx = -1;dx <= 1;++dx)
					place(wx + dx, topY + 1, wz + dz,Block::Leaves);
		}
}

// 把一个 Chunk 填成地形。coordX/Y/Z 是这个 Chunk 在世界里的“区块坐标”。
inline void generateTerrain(Chunk& chunk, int coordX, int coordY, int coordZ,
	const Perlin& noise, uint32_t seed) {
	
	const int N = Chunk::N;
	for(int lz=0;lz<N;++lz)
		for (int lx = 0;lx < N;++lx) {
			int wx = coordX * N + lx;
			int wz = coordZ * N + lz;
			int h = heightAt(noise, wx, wz);//随机高度 平滑的
			//区块坐标改世界坐标，
			//这个就出来了这一个区块里这个方块的世界坐标了
			bool beach = (h < SEA_LEVEL + 1); //水边/水下的地表换成沙子

			for (int ly = 0;ly < N;++ly) {
				int wy = coordY * N + ly;//这一格的世界高度 方块高度是h
				Block b;
				if (wy > h)	b=(wy<=SEA_LEVEL)?Block::Water: b = Block::Air;
				//刚刚算出来的方块高度，地面以上海平面以下就水，不然就空气
				else if (wy == h)	 b = beach?Block::Sand: Block::Grass;//等于就是最高的草
				else if (wy > h - 4) b = beach?Block::Sand: Block::Dirt;//往下三格就是泥巴
				else b = Block::Stone;
				chunk.set(lx,ly,lz,b);//存进来,这里要区块坐标，但是block应该由世界坐标的y来算
			}
		}
	//地形画结束，开始种树
	plantTrees(chunk, coordX, coordY, coordZ, noise, seed);
	chunk.dirty = true;
}