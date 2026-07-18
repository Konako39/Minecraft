#pragma once
#include "../world/world.hpp"
#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>

//玩家：一个隐藏箱子（AABB 轴对齐包围盒 不旋转，棱边平行于坐标轴的长方体箱子）
struct Player {
	glm::vec3 pos{ 8.f,70.f,8.f };//脚底中心
	glm::vec3 vel{ 0.f };//速度
	bool onGround = false;
	bool flying = false;

	static constexpr float HALF_W = 0.3f;  // 半宽：箱子0.6米宽
	static constexpr float HEIGHT = 1.8f;  // 身高1.8米
	static constexpr float EYE = 1.62f; // 眼睛离脚底1.62米

	glm::vec3 eyePos() const { return pos + glm::vec3(0.f, EYE, 0.f); }
};

inline bool collides(const World& world, const glm::vec3& pos) {
	int x0 = (int)std::floor(pos.x - Player::HALF_W);
	int x1 = (int)std::floor(pos.x + Player::HALF_W);
	int y0 = (int)std::floor(pos.y);
	int y1 = (int)std::floor(pos.y + Player::HEIGHT);
	int z0 = (int)std::floor(pos.z - Player::HALF_W);
	int z1 = (int)std::floor(pos.z + Player::HALF_W);
	//这是一个三维坐标轴的三条线的坐标(六个形成边界范围)，刚好在脚底中心是一个箱子的大小
	for (int y = y0; y <= y1; ++y)
		for (int z = z0; z <= z1; ++z)
			for (int x = x0; x <= x1; ++x)
				if (isSolid(world.getBlock(x, y, z))) return true;
	//在此边界内如果遇到实体就碰撞true
	return false;
}

// 沿单轴(axis: 0=x 1=y 2=z)移动 amount 米，撞墙就停。返回 true=撞了
inline bool moveAxis(const World& world, glm::vec3& pos, int axis, float amout) {
	const float STEP = 0.05f; //每次只前进怎么多（玩家） 1是方块大小 太大了容易漏
	float dir = (amout > 0) ? 1.f : -1.f;
	float remaining = std::abs(amout);//取绝对值
	while (remaining > 0.f) {
		float s = std::min(STEP, remaining);//可能这个距离比步数还小
		pos[axis] += s * dir;//算出检查的距离，每一步每一步的检查

		if (collides(world,pos)) {
			pos[axis] -= s * dir;//退后到最后一个安全位置
								//这里就做完碰撞后不能继续过去的逻辑了
			return true;
		}
		remaining -= s;
	}
	return false;
	//如果remaining的长度 循环完了没有碰撞到 就flase
}

//重力 把速度积分成唯一，逐轴移动+碰撞
inline void  UpdatePlayer(const World& world, Player& p, float dt) {
	if (!p.flying) p.vel.y -= 32.f * dt;//重力加速度 dt时间增量
	p.vel.y = std::max(p.vel.y, -60.f); //最大下落速度

	moveAxis(world, p.pos, 0, p.vel.x * dt);//0是数组第一个 就是x
	bool hitY = moveAxis(world, p.pos, 1, p.vel.y * dt);
	moveAxis(world, p.pos, 2, p.vel.z * dt);//只要他帮我移动
	//这里分轴移动 不然撞墙卡墙上

	p.onGround = hitY && p.vel.y < 0.f; // 往下撞到了=踩到地
	if (hitY) p.vel.y = 0.f;            // 撞天花板/地板都清掉竖直速度
}

