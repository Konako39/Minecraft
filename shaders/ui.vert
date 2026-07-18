

//UI的顶点着色器，像素坐标转化为NDC坐标

#version 330 core

layout (location = 0) in vec2 aPos; //像素原点
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec4 aColor;

uniform vec2 uScreen; //uniform：这是从 CPU 传给 GPU 的数据
out vec2 vUV; //out把数据发给下一个着色器 frag文件里有个in来接收
out vec4 vColor;

void main(){
	vUV=aUV;
	vColor = aColor;
	float x = aPos.x/uScreen.x * 2.0-1.0;//像素转换 屏幕左到右 -1 - 1
	//屏幕大小1280，传进去640，就得出0，也就是屏幕中心
	float y = 1.0 - aPos.y / uScreen.y * 2.0;
	//NDC的y向上增加 屏幕y向下增加 所以1-
	gl_Position=vec4(x,y,0.0,1.0);
}