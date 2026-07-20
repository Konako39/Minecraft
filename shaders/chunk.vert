


//区块顶点着色器：位置 + UV + 亮度，并把世界坐标传给片元(算雾用)
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in float aLight;

uniform mat4 uMVP;
uniform mat4 uModel; //只做平移的model矩阵：本地坐标→世界坐标
out vec2 vUV;
out float vLight;
out vec3 vWorldPos;

void main() {
    vUV = aUV;
    vLight=aLight;
    vWorldPos=(uModel*vec4(aPos,1.0)).xyz;
    gl_Position = uMVP * vec4(aPos, 1.0);
}