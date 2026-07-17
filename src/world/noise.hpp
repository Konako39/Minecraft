//这里是柏林噪声的实现
//输入坐标，输出一个 -1 到 1 之间，随位置平滑变化的数。
//同样的输入永远获得同样的输出
//暂时先放在这

#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

//2D柏林噪声 构造的时候给一个Seed,同一Seed得到同一世界
struct Perlin {
    int perm[512];   // 打乱后的排列表，噪声的“随机源”

    Perlin(unsigned seed = 1337) {
        int p[256];
        for (int i = 0; i < 256; ++i) p[i] = i;
        // 用 seed 驱动一个简单的伪随机，把 0..255 洗牌
        uint32_t s = seed ? seed : 1;
        for (int i = 255; i > 0; --i) {
            s = s * 1664525u + 1013904223u;      // 线性同余，够用了
            int j = (int)(s % (uint32_t)(i + 1));
            std::swap(p[i], p[j]);
        }
        for (int i = 0; i < 512; ++i) perm[i] = p[i & 255];
    }

    static float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); } // 平滑曲线
    static float lerp(float a, float b, float t) { return a + t * (b - a); }   // 线性插值

    // 根据格点哈希值给一个方向梯度，再和偏移做点积
    static float grad(int h, float x, float y) {
        switch (h & 7) {
        case 0: return  x + y; case 1: return -x + y;
        case 2: return  x - y; case 3: return -x - y;
        case 4: return  x;     case 5: return -x;
        case 6: return  y;     default: return -y;
        }
    }

    // 返回约 [-1, 1] 的平滑噪声
    float noise(float x, float y) const {
        int X = (int)std::floor(x) & 255;
        int Y = (int)std::floor(y) & 255;
        x -= std::floor(x);
        y -= std::floor(y);
        float u = fade(x), v = fade(y);
        int A = perm[X] + Y, B = perm[X + 1] + Y;
        return lerp(
            lerp(grad(perm[A], x, y), grad(perm[B], x - 1, y), u),
            lerp(grad(perm[A + 1], x, y - 1), grad(perm[B + 1], x - 1, y - 1), u),
            v);
    }
};

// 分形叠加（fBm）：把多层不同频率的噪声叠起来，
// 大层管大山脉，小层管小起伏，地形更自然。
inline float fbm(const Perlin& n, float x, float z,
    int octaves = 4, float lacunarity = 2.f, float gain = 0.5f) {
    float sum = 0.f, amp = 1.f, freq = 1.f, norm = 0.f;
    for (int i = 0; i < octaves; ++i) {
        sum += amp * n.noise(x * freq, z * freq);
        norm += amp;
        amp *= gain;         // 每层振幅减半
        freq *= lacunarity;   // 每层频率翻倍
    }
    return sum / norm;        // 归一回约 [-1, 1]
}