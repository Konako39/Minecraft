#pragma once
#include "../world/world.hpp"
#include<glm/glm.hpp>
#include<cmath>

//从origin朝着dir射出，最远maxDist
//命中返回true，hit=撞到的格子，prev=撞之前的最后一个空气格子
inline bool raycast(const World& world, glm::vec3 origin, glm::vec3 dir,
	float maxDist, glm::ivec3& hit, glm::ivec3& prev) {
	dir = glm::normalize(dir);

	glm::ivec3 last = glm::ivec3(glm::floor(origin));//起点格子
	for (float t = 0.f;t < maxDist;t += 0.05f) {
		glm::vec3 p = origin + dir * t;	//射出去的落点 一点一点的循环遍历过去
		glm::ivec3 cell((int)std::floor(p.x), (int)std::floor(p.y), (int)std::floor(p.z));

		if (cell == last) continue;
		if (world.getBlock(cell.x, cell.y, cell.z) != Block::Air) {
			hit = cell;
			prev = last;//命中的前一格
			return true;
		}
		last = cell;
	}
	return false;

}
