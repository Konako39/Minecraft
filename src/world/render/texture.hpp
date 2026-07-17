/*
读取图片
创建 OpenGL Texture
上传到 GPU
设置采样方式
释放CPU内存
返回纹理ID
*/
#pragma once
#include<glad/glad.h>
#include<stb_image.h>
#include<iostream>

inline GLuint loadTexture(const char* path) {
	int w, h, channels;
	stbi_set_flip_vertically_on_load(true);// OpenGL 的 UV 原点在左下，需翻转
	//flip翻转一下
	unsigned char* data = stbi_load(path, &w, &h, &channels, 4);//强制RGBA 读取图片数据
	if (!data) { std::cerr << "读取不到材质" << path << '\n';return 0; }

	GLuint tex;
	glGenTextures(1, &tex);//找OpenGL要一个纹理
	glBindTexture(GL_TEXTURE_2D, tex);//绑定其纹理
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	
	//申请绑定上传


	//像素风放大缩小都用NEAREST
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//缩小放大时候的采样方式
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//UV超出时候的方式，取边缘颜色
	
	stbi_image_free(data);
	//释放
	return tex;
	//返回纹理ID
}