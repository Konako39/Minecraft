


//区块片元着色器：从图集取色
#version 330 core

in vec2 vUV;
in float vLight;
out vec4 FragColor;
uniform sampler2D uAtlas;

void main() {
    vec4 c = texture(uAtlas, vUV);
    if (c.a < 0.5) discard;      // 树叶的透明像素直接丢弃，露出后面
    FragColor = vec4(c.rgb * vLight, c.a);//压暗颜色
}