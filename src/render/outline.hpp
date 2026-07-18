#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "../shader.hpp"

//瞄准方块的黑色描边
//shader用basic

struct OutlineRenderer {
	GLuint vao = 0, vbo = 0, prog = 0;
	int vertexCount = 0;

	bool init() {
		prog = loadShaderProgram("shaders/basic.vert","shaders/basic.frag");
		if (!prog) return false;
		
		//比立方体稍微大一点，一共12条线
		const float a = -0.003f, b = 1.003f;
		const float col[3] = {0.05f,0.05f,0.05f};//近黑色
		std::vector<float> v;
		auto edge = [&](float x1, float y1, float z1, float x2, float y2, float z2) {
			v.insert(v.end(), { x1,y1,z1,col[0],col[1],col[2] });
			v.insert(v.end(), { x2,y2,z2,col[0],col[1],col[2] });
			};
		//在v的end后面插入后面六个
		//底面4条
		edge(a, a, a, b, a, a); edge(b, a, a, b, a, b); edge(b, a, b, a, a, b); edge(a, a, b, a, a, a);
		//顶面4条
		edge(a, b, a, b, b, a); edge(b, b, a, b, b, b); edge(b, b, b, a, b, b); edge(a, b, b, a, b, a);
		//竖着4条
		edge(a, a, a, a, b, a); edge(b, a, a, b, b, a); edge(b, a, b, b, b, b); edge(a, a, b, a, b, b);
		vertexCount = (int)v.size() / 6;

		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		return true;
	}
	//mvp = proj * view * translate(方块坐标)
	void draw(const glm::mat4& mvp) {
		glUseProgram(prog);
		glUniformMatrix4fv(glGetUniformLocation(prog, "uMVP"), 1, GL_FALSE, glm::value_ptr(mvp));
		glBindVertexArray(vao);
		glLineWidth(3.f);//粗细
		glDrawArrays(GL_LINES, 0, vertexCount);
	}



};