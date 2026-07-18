
#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "shader.hpp"

// 极简2D绘制器：以像素为单位往屏幕上画矩形（纯色或贴图）。
// 用法：ui.begin(宽,高) → ui.rect(...)/ui.texRect(...) 若干 → ui.end()

struct UIRenderer {
	GLuint vao = 0, vbo = 0, prog = 0, whiteTex = 0;
	glm::vec2 screen{ 0,0 };

	bool init() {
		prog = loadShaderProgram("shaders/ui.vert", "shaders/ui.frag");
		if (!prog) return false;

		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		//每顶点8个float：x,y, u,v, r,g,b,a
		//VAO 存「顶点怎么解读」，VBO 存数据 这里和画面的shader
		// chunk里画方块的那个一个道理
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glBindVertexArray(0);

		//1x1的纯白贴图：画纯色矩形时就贴它（颜色全靠顶点色）
		unsigned char px[4] = { 255,255,255,255 };
		glGenTextures(1, &whiteTex);
		glBindTexture(GL_TEXTURE_2D, whiteTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		return true;
		}

	//开始画UI：关深度(UI永远在最上层)、开混合(半透明)
	void begin(int w, int h) {
		screen = { (float)w, (float)h };
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glUseProgram(prog);
		glUniform2f(glGetUniformLocation(prog, "uScreen"), screen.x, screen.y);
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(prog, "uTex"), 0);
	}


	//画一块贴图矩形：屏幕像素坐标(x,y是左上角)，UV子矩形，tint是染色
	void texRect(float x, float y, float w, float h, GLuint tex,
		float u0, float v0, float u1, float v1, glm::vec4 c) {
		//屏幕的上边对应贴图v1(上)，下边对应v0(下)
		float v[6][8] = {
			{ x,     y,     u0, v1,  c.r,c.g,c.b,c.a },
			{ x + w, y,     u1, v1,  c.r,c.g,c.b,c.a },
			{ x + w, y + h, u1, v0,  c.r,c.g,c.b,c.a },
			{ x,     y,     u0, v1,  c.r,c.g,c.b,c.a },
			{ x + w, y + h, u1, v0,  c.r,c.g,c.b,c.a },
			{ x,     y + h, u0, v0,  c.r,c.g,c.b,c.a },
		};
		glBindTexture(GL_TEXTURE_2D, tex);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	//画一个白色矩形
	void rect(float x, float y, float w, float h, glm::vec4 c) {
		texRect(x,y,w,h,whiteTex,0,0,1,1,c);
	}

	void end() {
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glBindVertexArray(0);
	}//还原3D绘制需要的状态


};
