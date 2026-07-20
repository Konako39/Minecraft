


//区块片元着色器：图集取色 × 面亮度 × 昼夜亮度，远处混进雾色
#version 330 core

in vec2 vUV;
in float vLight;
in vec3 vWorldPos;

out vec4 FragColor;
uniform sampler2D uAtlas;

uniform vec3  uCamPos;    //相机位置(算距离)
uniform vec3  uFogColor;  //雾色=天空色，融为一体
uniform float uFogStart;  //从多远开始起雾
uniform float uFogEnd;    //到多远完全看不见
uniform float uDayLight;  //昼夜亮度 0.15(深夜)~1.0(正午)


void main() {
    vec4 c = texture(uAtlas, vUV);
    if (c.a < 0.5) discard;      // 树叶的透明像素直接丢弃，露出后面
    vec3 col=c.rgb* vLight *uDayLight;
    float dist = length(vWorldPos - uCamPos);
    float fog = clamp((dist-uFogStart)/(uFogEnd-uFogStart),0.0,1.0);
    //越近越清晰 离得越远越模糊
    FragColor = vec4( mix(col,uFogColor,fog), c.a);//压暗颜色
    //方块混合雾的颜色 根据距离
}
