#pragma once
#include "ui.hpp"
#include "texture.hpp"
#include <iostream>

//准备的font是16x6行 每格16x16像素
struct Font {
	GLuint tex = 0;
	static constexpr int COLS = 16, ROWS = 6, FIRST = 32;

	bool init() {
		tex = loadTexture("assets/textures/font.png");
		return tex != 0;
	}

	//一行有多宽
	static float measure(float size, const std::string& text) {
		return size * 0.6f * (float)text.size();
		//宽度 = 字距 × 字数
	}

	//画一行字。x,y是左上角，size是单个字的显示高度
	void draw(UIRenderer& ui, float x, float y, float size, const std::string& text,
		glm::vec4 color = { 1,1,1,1 }) {
		float advance = size * 0.6f;//等宽字体 宽约是高的0.6倍
		for (char ch : text) {
			int c = (unsigned char)ch;
			if (c < FIRST || c >= FIRST + COLS * ROWS) { x += advance;continue; }
			int idx = c - FIRST;
			int col = idx % COLS, row = idx / COLS;
			float du = 1.f / COLS, dv = 1.f / ROWS;
			float u0 = col * du, u1 = u0 + du;
			//行从图的顶部往下数；加载时上下翻转过，第0行的v在最上面
			float v1 = 1.f - row * dv, v0 = v1 - dv;
			ui.texRect(x, y, size, size, tex, u0, v0, u1, v1, color);
			x += advance;
		}

	}


};