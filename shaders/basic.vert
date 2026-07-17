//顶点着色器，决定顶点出现在什么地方



#version 330 core
layout (location = 0) in vec3 aPos;    // 属性 0：位置
layout (location = 1) in vec3 aColor;  // 属性 1：颜色

uniform mat4 uMVP;
out vec3 vColor;

void main() {
    vColor = aColor;
    gl_Position = uMVP * vec4(aPos, 1.0);
}