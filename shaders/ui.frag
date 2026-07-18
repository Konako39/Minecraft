

//片元着色器：贴图颜色 x 顶点颜色

#version 330 core

in vec2 vUV;
in vec4 vColor;

out vec4 FragColor;
uniform sampler2D uTex;

void main(){
	FragColor=texture(uTex,vUV)* vColor;
}