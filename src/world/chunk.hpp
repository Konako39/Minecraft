#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>//需要vec3存颜色
#include <vector>
#include "block.hpp"
#include "block_textures.hpp"

// 每个方块占据从 (x,y,z) 到 (x+1,y+1,z+1) 的 1×1×1 空间。
// 下面是 6 个面、每面 4 个角，相对方块原点的 0/1 偏移量。
// 4 个角的顺序是「从外面看逆时针」，这样正面朝外。
static const float FACE[6][4][3] = {
	// +X 右面 0
	{{1,0,1},{1,0,0},{1,1,0},{1,1,1}},
	// -X 左面 1
	{{0,0,0},{0,0,1},{0,1,1},{0,1,0}},
	// +Y 上面 2
	{{0,1,1},{1,1,1},{1,1,0},{0,1,0}},
	// -Y 下面 3
	{{0,0,0},{1,0,0},{1,0,1},{0,0,1}},
	// +Z 前面 4
	{{0,0,1},{1,0,1},{1,1,1},{0,1,1}},
	// -Z 后面 5 材质那要用
	{{1,0,0},{0,0,0},{0,1,0},{1,1,0}},
};

// 每个面「朝向的邻居」在哪个方向。顺序和 FACE 一一对应。就是面的法线啦
// 例如第 0 个面(+X)的邻居在 (x+1, y, z)。
static const int NEIGHBOR[6][3] = {
	{ 1, 0, 0}, {-1, 0, 0},
	{ 0, 1, 0}, { 0,-1, 0},
	{ 0, 0, 1}, { 0, 0,-1},
};

static const float UVCORNER[4][2] = { {0,0},{1,0},{1,1},{0,1} };
//UV切出来的图的小格的四个角 右下右上左上左下
static const float FACE_LIGHT[6] = {
	0.8f, // +X 侧
	0.8f, // -X 侧
	1.0f, // +Y 顶
	0.5f, // -Y 底
	0.7f, // +Z 侧
	0.7f, // -Z 侧
};



// 给不同方块一个纯色（贴图之前先靠颜色区分）。
static glm::vec3 colorOf(Block b) {
	switch (b) {
	case Block::Grass: return { 0.35f, 0.70f, 0.30f }; // 绿
	case Block::Dirt:  return { 0.55f, 0.40f, 0.25f }; // 棕
	case Block::Stone: return { 0.55f, 0.55f, 0.58f }; // 灰
	case Block::Log:    return { 0.45f, 0.32f, 0.18f }; // 深棕树干
	case Block::Leaves: return { 0.20f, 0.55f, 0.18f }; // 深绿树叶
	default:           return { 1.0f, 0.0f, 1.0f };    // 洋红=出错提示
	}
}



struct Chunk
{
	static constexpr int N = 16;
	Block blocks[N * N * N]{};
	bool dirty = true;//脏了 也就是数据变了要重建

	GLuint vao = 0, vbo = 0;//vbo存顶点数据 vao存顶点数据的解释
	int vertexCount = 0;//draw时要知道画几个点

	//水单独一套网格：不透明的先画、水最后画（半透明的规矩）
	GLuint waterVao = 0, waterVbo = 0;
	int waterVertexCount = 0;

	static int index(int x, int y, int z) { return x + N * (y + N * z); }
	//三维坐标 (x,y,z) 再折成一维下标,N*z 等于跳过了z个N +y代表到那个二维了
	//然后*N等于跳过多少个y，+x索引到目标
	//反正我理解了，但是不好写出来. N 张 N×N 的纸摞起来,第z张

	Block get(int x, int y, int z)const {
		if (x < 0 || y < 0 || z < 0 || x >= N || y >= N || z >= N)
			return Block::Air;
		return blocks[index(x,y,z)];
	}

	void set(int x, int y, int z, Block b) {
		blocks[index(x, y, z)] = b;
		dirty = true;
	}

	void rebuildMesh();
	void draw();

	void drawWater();
	//通用上传：往指定的 vao/vbo 传顶点（不透明和水两套都用它）
	static void uploadMesh(GLuint& vao, GLuint& vbo, int& count, const std::vector<float>& verts);


};

inline void Chunk::uploadMesh(GLuint& vao, GLuint& vbo, int& count, const std::vector<float>& verts) {
	count = (int)verts.size() / 6;//6个float一个顶点 x,y,z,u,v,light

	if (vao == 0) {
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
	}//申请1个vao vbo
	glBindVertexArray(vao);//激活vao

	glBindBuffer(GL_ARRAY_BUFFER, vbo); // 把它当成「顶点数组缓冲」绑上
	glBufferData(GL_ARRAY_BUFFER,verts.size() * sizeof(float),verts.data(),GL_DYNAMIC_DRAW);
	//            ↑绑在哪一类槽位   ↑多大      ↑CPU数据从哪来  ↑提示：很少改，一次上传
	//上传到GPU
	
	// 属性 0：位置(3 个 float)，从每个顶点第 0 个 float 开始
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	//打开开关

	// 属性 1：UV(2 个 float)，后面是偏移3个float的量
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	//打开开关

	// 属性 2：亮度(1 个 float)，后面是偏移5个float的量
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);
	//打开开关



	glBindVertexArray(0);//关闭之前那个vao
}

inline void Chunk::draw() {
	if (vertexCount == 0) return;
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES,0,vertexCount);
	//按照当前绑定的 VAO 说明，从 VBO 里取顶点，然后让 GPU 画三角形。
}
inline void Chunk::drawWater() {
	if (waterVertexCount == 0) return;
	glBindVertexArray(waterVao);
	glDrawArrays(GL_TRIANGLES,0,waterVertexCount);
}



inline void Chunk::rebuildMesh() {
	std::vector<float> verts;	//不透明方块顶点
	std::vector<float> waterVerts;//水的顶点（半透明，要最后画）
	static const int order[6] = { 0,1,2,0,2,3 };
	//一面 2三角 6点 角1：012 角2：023 对应FACE来偏移的，以便画出面，这个记住吧
	for (int z = 0; z < N; ++z)
		for (int y = 0; y < N; ++y)
			for (int x = 0; x < N; ++x) {
				Block b = get(x, y, z);
				if (b==Block::Air) continue;//空气不生成面
				bool isWater = (b == Block::Water);

				//然后检查每个面的邻居
				for (int f = 0;f < 6;++f) {
				int nx = x + NEIGHBOR[f][0];
				int ny = y + NEIGHBOR[f][1];
				int nz = z + NEIGHBOR[f][2];//这里看上面，最后会各加减一个1，方块大小就是1
				Block nb = get(nx, ny, nz);
				if (isWater) {
				//水的面只有贴着空气才画
					if (nb != Block::Air) continue;
				}
				else
				{
					//邻居不透明 或者 邻居等于自身 就不画
					if (isOpaque(nb)|| nb == b) continue;
				}

				std::vector<float>& out = isWater ? waterVerts : verts;
				//决定是构造哪个列表 水的还是普通的
					for(int k=0;k<6;++k){
					//生成↓
					float u0, v0, u1, v1;
					tileUV(tileOf(b,f), u0, v0, u1, v1);
					//小格子在大图里的UV坐标（b的每面该是什么材质）
					//值给到uv01
						int ci = order[k];//角编号
						const float* corner = FACE[f][ci];//f 这个面 ci画完 012023
						//这里会把FACE[][]的传给corner 然后里面的3个数就是corner[012]
						//下面就是构造这个方块信息了
						//k去循环6个，这个面就出来了，这里+corner，是corner装的是这个角的偏移
						//画立方体，立方体的每个角总有1的偏移也就是FACE
						//加上xyz也就是得到方块的那个面的6个点的世界坐标，然后画出来
						out.push_back(x + corner[0]);
						out.push_back(y + corner[1]);
						out.push_back(z + corner[2]);
						//UV：把角的0/1映射进这块图块的子矩形
						float tu = UVCORNER[ci][0], tv = UVCORNER[ci][1];
						out.push_back(u0 + tu * (u1 - u0));
						out.push_back(v0 + tv * (v1 - v0));

						out.push_back(FACE_LIGHT[f]);
					}
					
				}
			
			}
	uploadMesh(vao,vbo,vertexCount,verts);
	uploadMesh(waterVao, waterVbo, waterVertexCount, waterVerts);
	dirty = false;
}

