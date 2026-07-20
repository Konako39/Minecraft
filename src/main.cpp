#include <glad/glad.h>   // OpenGL 函数声明
#include <GLFW/glfw3.h>  // 开窗口、读输入
#include <cstdio>
// 数学库：旋转、透视、矩阵
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader.hpp"
#include "camera.hpp"
#include "world/terrain.hpp"
#include "world/world.hpp"
#include "render/texture.hpp"
#include "player/raycast.hpp"
#include "render/ui.hpp"
#include "render/outline.hpp"
#include "player/player.hpp"
#include "render/font.hpp"
#include "player/inventory.hpp"
#include "world/save.hpp"

#include <memory> //unique_ptr管理世界的生死
#include <string>
#include <ctime>//随机种子用当前时间

#include <iostream>
//把相机先弄出来
static Camera gCamera;
static Player gPlayer;//玩家的身体（相机在眼睛里）
static float gScroll = 0.f;//滚轮攒的量
static Hotbar gHotbar;//快捷栏
static bool gInventoryOpen;//开背包了
static const char* SAVE_PATH = "save/world.dat";//存档位置，单存档位
static float gTimeOfDay = 0.2f;//一天中的时刻 0-1循环 0.25正午 0.75午夜
static const float DAY_LENGTH = 240.f;//240秒一天

//状态——游戏状态机
enum class GameState {
    MainMenu,
    CreateWorld,
    Playing,
    Paused
};
static GameState gState = GameState::MainMenu;
static std::unique_ptr<World> gWorld;//世界，进了游戏才存在，退出到标题就销毁
static std::string gSeedInput;//创建世界界面正在输入的种子


float lastX = 640.f, lastY = 360.f;
bool firstMouse = true;
static void scroll_callback(GLFWwindow*, double, double yoff) {
    gScroll += (float)yoff;//滑轮后攒着 回调里只做最小的事 具体消费在main里
}

//打字回调 创建世界的时候输种子的时候用
static void char_callback(GLFWwindow*, unsigned int cp) {
    if (gState == GameState::CreateWorld && cp >= '0' && cp <= '9' && gSeedInput.size() < 9)
        gSeedInput += (char)cp;
    //1-9 最多9位 打字一律用char回调
}

static void mouse_callback(GLFWwindow*,double xops,double yops) {
    if (gState != GameState::Playing || gInventoryOpen) return;//背包开着时鼠标是指针，不转视角
    if(firstMouse){
        lastX = (float)xops;
        lastY = (float)yops;
        firstMouse = false;
        return;
    }//避免第一帧计算偏移量，不然会突然转很大

    float xoffset = (float)xops - lastX;
    float yoffset = lastY - (float)yops;// 屏幕 Y 和抬头常相反
    lastX = (float)xops;
    lastY = (float)yops;

    gCamera.yaw += xoffset * gCamera.sensitivity;//敏感度
    gCamera.pitch += yoffset * gCamera.sensitivity;

    if (gCamera.pitch > 89.f) gCamera.pitch = 89.f;
    if (gCamera.pitch < -89.f) gCamera.pitch = -89.f;
    //不准转太厉害
}

// 窗口被用户拖拽改变大小时，告诉 OpenGL：画画的区域也要变
static void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}

//切换状态 并且设置鼠标状态 切换状态走这里
static void setState(GLFWwindow* window, GameState s) {
    gState = s;
    bool grab = (s == GameState::Playing) && !gInventoryOpen;
    glfwSetInputMode(window, GLFW_CURSOR, grab ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (grab) firstMouse = true;//回游戏的时候重置鼠标初始坐标不然会瞬移
}

//创建新世界
static void startNewWorld(uint32_t seed) {
    gWorld = std::make_unique<World>(seed);
    gPlayer = Player{};//重置玩家
    int h = heightAt(gWorld->noise, 8, 8);//算出高度
    gPlayer.pos = { 8.5f,(float)(h + 1),8.5f };//出生后站到地面上
    gInventoryOpen = false;
}

//画一个按钮，返回此帧是否被点击
static bool uiButton(UIRenderer& ui, Font& font, float x, float y, float w, float h,
    const std::string& label, float mfx, float mfy, bool click) {
    bool hover = mfx >= x && mfx < x + w && mfy >= y && mfy < y + h;
    ui.rect(x, y, w, h, hover ? glm::vec4{ 0.6f,0.6f,0.6f,0.95f } : glm::vec4{ 0.35f,0.35f,0.35f,0.9f });
    ui.rect(x + 2, y + 2, w - 4, h - 4,
        hover ? glm::vec4{ 0.32f,0.32f,0.32f,0.95f } : glm::vec4{ 0.18f,0.18f,0.18f,0.9f });
    //俩UI按钮，鼠标移动到上面悬浮了，就变色调
    font.draw(ui, x + w * 0.5f - Font::measure(36, label) * 0.5f, y + h * 0.5f - 18, 36, label);
    //写字 在按钮中间
    return hover && click;
}



int main() {
    // 1) 启动 GLFW 库
    if (!glfwInit()) return 1;

    // 2) 申请「OpenGL 3.3 Core」上下文 —— 相当于说：我要用这种版本的画笔规则
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 3) 真正创建窗口（并附带一个可用的 OpenGL 上下文）
    GLFWwindow* window = glfwCreateWindow(1280, 720, "OPENGLMC", nullptr, nullptr);
    if (!window) {
        glfwTerminate(); // 与 glfwInit 成对：关掉 GLFW
        return 1;
    }
    glfwMakeContextCurrent(window);  // 之后的 glXXX 都画到这个窗口上    
    //窗口初始化

    glfwSetCursorPosCallback(window,mouse_callback);
    //windows窗口，第二个参数是回调函数，鼠标一动就要调用这个 []匿名函数，就地写一个传进去
    
    glfwSetScrollCallback(window,scroll_callback);
    //滚动初始化

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    //帧大小回调初始化

    glfwSetCharCallback(window, char_callback);
    //输入初始化

    // 4) 用 GLFW 提供的「取函数地址」能力，让 GLAD 把所有 gl 函数接通
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        return 1;
    }

    // 加载 shaders/ 里的 GLSL，编译链接成 GPU 程序
    unsigned int program = loadShaderProgram("shaders/chunk.vert", "shaders/chunk.frag");
    if (!program) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    //加载图集
    GLuint atlas = loadTexture("assets/textures/atlas.png");
    if (!atlas) {
        std::cerr << "材质/图集，加载失败\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    //UI绘制，方块描边,HUD初始化
    UIRenderer ui;
    Font font;
    OutlineRenderer outline;
    if (!ui.init() || !outline.init()||!font.init()) {
        std::cerr << "UI初始化失败 ui/font/outline\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glEnable(GL_DEPTH_TEST);// 开启深度测试：近的挡住远的

    float lastTime = (float)glfwGetTime();

    setState(window, GameState::MainMenu);
    //初始化结束，进入主菜单

    // 5) 主循环：每转一圈 ≈ 刷新一帧画面
    while (!glfwWindowShouldClose(window)) {
        //帧缓冲 窗口大小
        int fbw, fbh;
        glfwGetFramebufferSize(window, &fbw, &fbh);
        float aspect = fbh ? (float)fbw / (float)fbh : 1.f;
        float speed = gPlayer.flying ? 10.f : 4.3f;//速度

        float cx = fbw * 0.5f, cy = fbh * 0.5f;
        //定位数据等公用部分

        //鼠标状态整个UI共用：位置换算成帧缓冲坐标(防高DPI错位)，点击取边沿
        bool left = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        bool right = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        static bool wasLeft = false, wasRight = false;
        bool click = left && !wasLeft;
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        int winW, winH;
        glfwGetWindowSize(window, &winW, &winH);
        float mfx = (float)mx * fbw / (winW ? winW : 1);
        float mfy = (float)my * fbh / (winH ? winH : 1);


        //一帧经过多久
        float  now = (float)glfwGetTime();
        float dt = now - lastTime;
        lastTime = now;

        //昼夜：只在游戏进行的时候行走的时间，主菜单和暂停的时候不走
        if (gState == GameState::Playing) {
            gTimeOfDay += dt / DAY_LENGTH;//除以一天长度
            if (gTimeOfDay >= 1.f)gTimeOfDay -= 1.f;//超过1说明1天过去了 -1开始第二天
        }
        //太阳高度→亮度：正午1.0，深夜0.15，黎明黄昏平滑过渡
        float sun = sinf(gTimeOfDay * 6.2831853f);//2派 前面那个是0-1，就可以出0-360度
        //sinf一下就可以获得-1到-之间的波形值 也就是太阳高低变化
        //用sin原值的话一整天都在爬坡，sin的是波形

        float dayLight = glm::clamp(sun * 2.f + 0.5f, 0.15f, 1.f);
        //晚上最低亮度0.5，clamp是（数值，最小值，最大值）
        //↓天空色在白天蓝和深夜蓝黑之间渐变，雾色和天空一致
        glm::vec3 daySky{0.53f,0.81f,0.92f},nightSky{ 0.02f, 0.03f, 0.08f };
        glm::vec3 skyColor = glm::mix(nightSky, daySky, (dayLight - 0.15f) / 0.85f);
        //混合起来

        glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);//清空成当前天色
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);//深度


        //ESC逻辑
        //ESC按状态各有含义：游戏中→暂停(先关背包)，暂停→回游戏
        bool escNow = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        static bool escWas = false;
        if (escNow && !escWas) {
            if (gState == GameState::Playing) {
                if (gInventoryOpen) {
                    gInventoryOpen = false;
                    setState(window, GameState::Playing);//重设鼠标锁定
                    //如果开着背包就是关背包
                }
                else {
                    setState(window, GameState::Paused);
                    //暂停
                }
            }
            else if (gState == GameState::Paused) {
                setState(window, GameState::Playing);
                //恢复暂停
            }
        }
        escWas = escNow;
        //ESC结束 上面是边缘触发


        glm::vec3 f = gCamera.front();
        f.y = 0.f;//走路不该被y影响
        if (glm::length(f) > 0.001f)
            f = glm::normalize(f);//归一化，出方向
        glm::vec3 r = glm::normalize(glm::cross(f, glm::vec3(0, 1, 0)));
        //水平方向的前和右，走路的时候抬头不会朝天上走
        //这里是正上方 叉乘 方向，同时垂直A和B的向量 就出右边


        if (gState == GameState::Playing || gState == GameState::Paused) 
        {//进入状态机 能进这里了肯定有world

        World& world = *gWorld;
        glm::ivec3 hit{}, prev{};
        bool hitOK = raycast(world, gCamera.position, gCamera.front(), 8.f, hit, prev);
        //这两个后面的描边之类的要用就放到外面


        if (gState == GameState::Playing || gState == GameState::Paused) {
            //暂停状态 或者 游戏状态
            //暂停状态 或者 游戏状态

            world.updateLoadedChunks(gPlayer.pos, 4);//4个区块半径内自动生成

            glm::mat4 view = gCamera.view();
            glm::mat4 proj = glm::perspective(glm::radians(gCamera.fov), aspect, 0.1f, 500.f);
            //                                          透视效果，fov广角 宽高 近的多少不画 远的多少不画

            glUseProgram(program);//用刚刚创建的shader小程序

            glActiveTexture(GL_TEXTURE0);//激活0号纹理 GPU的贴图插槽
            glBindTexture(GL_TEXTURE_2D, atlas);//把图集绑上去
            glUniform1i(glGetUniformLocation(program, "uAtlas"), 0);//用0号
            //世界渲染

            //昼夜和雾的参数
            glUniform3f(glGetUniformLocation(program, "uCamPos"),
                gCamera.position.x, gCamera.position.y, gCamera.position.z);
            glUniform3f(glGetUniformLocation(program, "uFogColor"),
                skyColor.r, skyColor.g, skyColor.b);
            glUniform1f(glGetUniformLocation(program, "uFogStart"), 44.f);
            glUniform1f(glGetUniformLocation(program, "uFogEnd"), 62.f);
            glUniform1f(glGetUniformLocation(program, "uDayLight"), dayLight);
            //传给shader，也就是frag和vert那几个


            //渲染区块
            for (auto& [coord, chunkPtr] : world.chunks) {
                Chunk& c = *chunkPtr;
                if (c.dirty) c.rebuildMesh();//脏了才重建
                //这里不停的遍历所有区块，所有区块的dirty

                //每个Chunk顶点是本地0~16坐标，靠model矩阵平移到世界正确位置
                glm::mat4 model = glm::translate(glm::mat4(1.f),
                    glm::vec3(coord.x, coord.y, coord.z) * (float)Chunk::N);
                //单位矩阵 平移到 xyz*16变世界坐标 translate平移

                glm::mat4 mvp = proj * view * model;
                //m模型坐标 * v世界坐标 * p摄像机坐标透视 = 屏幕

                glUniformMatrix4fv(glGetUniformLocation(program, "uMVP"),
                    1, GL_FALSE, glm::value_ptr(mvp));
                glUniformMatrix4fv(glGetUniformLocation(program, "uModel"),
                    1, GL_FALSE, glm::value_ptr(model));//算雾要世界坐标

                c.draw();

            }

            //画水 开混合、关深度写入（和深度测试不一样，深度测试是我被挡住了吗，深度写入是画完后我要挡住别人）
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            for (auto& [coord, chunkPtr] : world.chunks) {
                glm::mat4 model = glm::translate(glm::mat4(1.f),
                    glm::vec3(coord.x, coord.y, coord.z) * (float)Chunk::N);
                glm::mat4 mvp = proj * view * model;
                glUniformMatrix4fv(glGetUniformLocation(program, "uMVP"),
                    1, GL_FALSE, glm::value_ptr(mvp));
                glUniformMatrix4fv(glGetUniformLocation(program, "uModel"),
                    1, GL_FALSE, glm::value_ptr(model));//算雾要世界坐标

                chunkPtr->drawWater();
            }
            glDepthMask(GL_TRUE);//谁弄乱谁还原 一直关着写入画区块就完蛋了
            //这里是打开深度写入了
            glDisable(GL_BLEND);

            //画瞄准方块时的描边框
            if (hitOK) {
                glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(hit));
                outline.draw(proj * view * model);
            }

            ////////////////////////////上 世界///////////////////
            ///////////////////////////下 UI/////////////////////

            //这里开掉深度写入，画完世界了后，后面跟UI
            ui.begin(fbw, fbh);

            //头在水里的时候有个全屏蓝色的滤镜（画一个蓝色UI矩形 透明度0.35）
            glm::ivec3 eyeCell = glm::ivec3(glm::floor(gCamera.position));
            if (world.getBlock(eyeCell.x, eyeCell.y, eyeCell.z) == Block::Water)
                ui.rect(0, 0, (float)fbw, (float)fbh, { 0.2f, 0.4f, 0.9f, 0.35f });

            //暂停菜单
            if (gState == GameState::Paused) {
                ui.rect(0, 0, (float)fbw, (float)fbh, { 0,0,0,0.6f });//压暗世界
                std::string t = "GAME PAUSED";
                font.draw(ui, cx - Font::measure(50, t) * 0.5f, cy - 120, 50, t);

                if (uiButton(ui, font, cx - 260, cy - 50, 520, 80, "BACK TO GAME", mfx, mfy, click))
                    setState(window, GameState::Playing);
                if (uiButton(ui, font, cx - 260, cy + 50, 520, 80, "QUIT TO TITLE", mfx, mfy, click)) {
                    saveWorld(SAVE_PATH, *gWorld, gPlayer);//写盘
                    gWorld.reset();//世界销毁 之后不要调用了
                    setState(window, GameState::MainMenu);
                }
            }//暂停菜单结束

        if (gState == GameState::Playing) {
            //只在游戏内
        //把WASD变为想走的方向，再统一乘速度
        glm::vec3 wish(0.f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) wish += f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) wish -= f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) wish -= r;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) wish += r;
        if (glm::length(wish) > 0.001f)
            wish = glm::normalize(wish) * speed;//不归一就经典WD一起按走1.4倍速了
        gPlayer.vel.x = wish.x;
        gPlayer.vel.z = wish.z;

        //开始挖和放的逻辑
        //准星射线
        if (!gInventoryOpen) {
            if (left && !wasLeft && hitOK)
                world.setBlock(hit.x, hit.y, hit.z, Block::Air);
            if (right && !wasRight && hitOK){
                world.setBlock(prev.x,prev.y,prev.z,gHotbar.current());
            if(collides(world,gPlayer.pos))
                world.setBlock(prev.x, prev.y, prev.z, Block::Air);
            //如果放自己身体里面了，给他弄成空气
            }
        }

        //飞行和跳跃逻辑
        if (gPlayer.flying) {
            gPlayer.vel.y = 0.f;
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
                gPlayer.vel.y = speed;
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
                gPlayer.vel.y = -speed;
        }
        else {
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                if (inWater(world, gPlayer))
                    gPlayer.vel.y = 4.f;//踩水上浮
                else if (gPlayer.onGround)
                    gPlayer.vel.y = 9.f;//起跳
            }
        }

        UpdatePlayer(world, gPlayer, dt);//重力 碰撞

        }//只在playing状态
        }//playing和pause两方都有

        //画调试信息 0.25=正午对应12:00 → 小时 = 时刻*24 + 6
        int clockH = (int)fmodf(gTimeOfDay * 24.f + 6.f, 24.f);
        int clockM = (int)(fmodf(gTimeOfDay * 24.f + 6.f, 1.f) * 60.f);


        static float fpsTimer = 0.f;
        static int fpsFrames = 0, fpsShow = 0;
        fpsTimer += dt; fpsFrames++; //0.5s跑了多少帧
        if (fpsTimer >= 0.5f)//0.5f刷新一次
        {
            fpsShow = (int)(fpsFrames / fpsTimer);//1除了变成1秒多少帧
            fpsTimer = 0.f;
            fpsFrames = 0;
        }
        char dbg[160];
        snprintf(dbg, sizeof(dbg), "FPS %d  XYZ %.1f / %.1f / %.1f  TIME %02d:%02d%s",
            fpsShow, gPlayer.pos.x, gPlayer.pos.y, gPlayer.pos.z,
            clockH, clockM, gPlayer.flying ? "  [FLY]" : "");
        font.draw(ui, 8, 8, 64, dbg);



        //F键切换飞行 fWas防止一帧读取很多次
        bool fNow = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
        static bool fWas = false;
        if (fNow && !fWas) {
            gPlayer.flying = !gPlayer.flying;
            gPlayer.vel.y = 0.f;
        }
            fWas = fNow;

        //E键开关背包
         bool eNow = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
         static bool eWas = false;
         if (eNow && !eWas) {
             gInventoryOpen = !gInventoryOpen;
             setState(window, GameState::Playing);
                //指针模式
             if (!gInventoryOpen) firstMouse = true;//回游戏时重置，防视角猛跳
         }
         eWas = eNow;
         //背包开时不走路
         if (gInventoryOpen) { gPlayer.vel.x = 0.f;gPlayer.vel.z = 0.f; }

         //数字键和滚轮切换快捷栏
         if (!gInventoryOpen&&gState!=GameState::Paused) {
             for (int i = 0;i < 9;++i)
                 if (glfwGetKey(window, GLFW_KEY_1 + i) == GLFW_PRESS)
                     gHotbar.selected = i;
             int s = (int)gScroll;
             if (s != 0) {
                gHotbar.selected= ((gHotbar.selected - s) % 9 + 9) % 9;//负数取模 往下滚选下一格
                gScroll -= (float)s;
             }
         
         }

        gCamera.position = gPlayer.eyePos();
        //UI画准心
        if (gState == GameState::Playing&& !gInventoryOpen){
        glm::vec4 crossCol{ 1.f,1.f,1.f,0.8f };
        ui.rect(cx - 10, cy - 1, 20, 2, crossCol);//横
        ui.rect(cx - 1, cy - 10, 2, 20, crossCol);//竖
        }
        //准心结束

        //画快捷栏
        const float SLOT = 120.f;//1格子宽度
        float barX = cx - SLOT * 4.5f,
            barY = fbh - SLOT - 8.f;
        for (int i = 0;i < 9;++i) {
            float x = barX + i * SLOT;//选中格
            bool sel = (i == gHotbar.selected);
            //选中格边框亮白，其余半透明黑
            ui.rect(x, barY, SLOT, SLOT, sel ? glm::vec4{ 1,1,1,0.9f } : glm::vec4(0, 0, 0, 0.55f));
            ui.rect(x + 10, barY + 10, SLOT - 30, SLOT - 30, { 0.15f,0.15f,0.15f,0.85f });//选中外边框
            //方块图标 face=0那个
            float u0, v0, u1, v1;
            tileUV(tileOf(gHotbar.slots[i], 0), u0, v0, u1, v1);
            ui.texRect(x + 7, barY + 7, SLOT - 30, SLOT - 30, atlas, u0, v0, u1, v1, { 1,1,1,1 });
        }
            //手里拿的方块名，显示在快捷栏上方
        {
            std::string name = blockName(gHotbar.current());
            font.draw(ui,cx - Font::measure(50,name)*0.5f,barY-50,50,name);
        }

        //背包
        if (gInventoryOpen) {
            ui.rect(0, 0, (float)fbw, (float)fbh, { 0,0,0,0.5f });//给背景加个暗滤镜

            const auto& items = placeableBlocks();
            //获得可放置方块
            const int COLS_INV = 7;
            const float CELL = 110.f;
            //每行7格每格110像素
            int rows = ((int)items.size() + COLS_INV - 1) / COLS_INV;
            //算出需要多少行 +COLS_INV-1 是向上取整
            float gx = cx - COLS_INV * CELL * 0.5f;
            float gy = cy - rows * CELL * 0.5f;
            //获得背包左上角坐标

            std::string title = "INVENTORY  (click = put in hand)";
            font.draw(ui, cx - Font::measure(52, title) * 0.5f, gy - 40, 50, title);
            //画在中间

            for (int i = 0; i < (int)items.size(); ++i) {
                float x = gx + (i % COLS_INV) * CELL;
                float y = gy + (i / COLS_INV) * CELL;
                //遍历每个物品，算出行高
                bool hover = mfx >= x && mfx < x + CELL && mfy >= y && mfy < y + CELL;
                //鼠标是否悬停

                ui.rect(x + 2, y + 2, CELL - 4, CELL - 4,
                    hover ? glm::vec4{ 0.45f,0.45f,0.45f,0.95f } : glm::vec4{ 0.2f,0.2f,0.2f,0.95f });
                //悬停了格子背景绘制亮灰色否则暗灰色
                float u0, v0, u1, v1;
                tileUV(tileOf(items[i], 0), u0, v0, u1, v1);
                ui.texRect(x + 20, y + 20, CELL - 20, CELL - 20, atlas, u0, v0, u1, v1, { 1,1,1,1 });
                //绘制方块图标 内边距10 大小-20
                if (hover) {//悬停显示名字；点击放进手里(当前选中的快捷栏格)
                    font.draw(ui, cx - Font::measure(40, blockName(items[i])) * 0.5f,
                        gy + rows * CELL + 25, 40, blockName(items[i]));
                    if (left && !wasLeft)
                        gHotbar.slots[gHotbar.selected] = items[i];
                }
            }
        }//背包绘制结束

        //UI结束
        ui.end();
        }
        else{
        //游戏主菜单
         //———— 菜单界面（主菜单/创建世界）————
         ui.begin(fbw, fbh);
         //泥土平铺背景，MC主菜单的经典味道
         float u0, v0, u1, v1;
         tileUV(T_DIRT, u0, v0, u1, v1);
         for (float ty = 0; ty < fbh; ty += 64)
             for (float tx = 0; tx < fbw; tx += 64)
                 ui.texRect(tx, ty, 64, 64, atlas, u0, v0, u1, v1, { 0.4f,0.4f,0.4f,1.f });
         //游戏主菜单状态
         if (gState == GameState::MainMenu) {
             std::string title = "OPENGLMC";
             font.draw(ui, cx - Font::measure(140, title) * 0.5f, cy - 240, 140, title);
             std::string sub = "Minecraft";
             font.draw(ui, cx - Font::measure(90, sub) * 0.5f, cy - 140, 90, sub, { 0.8f,0.8f,0.8f,1.f });

             if (uiButton(ui, font, cx - 260, cy - 40, 520, 80, "NEW WORLD", mfx, mfy, click)) {
                 gSeedInput.clear();
                 setState(window, GameState::CreateWorld);
                 //新世界按钮切换到创建世界状态
             }
             bool hasSave = std::filesystem::exists(SAVE_PATH);
             if (uiButton(ui, font, cx - 260, cy + 60, 520, 80,
                 hasSave ? "LOAD WORLD" : "NO SAVE", mfx, mfy, click)) {
                 if (hasSave && loadWorld(SAVE_PATH, gWorld, gPlayer)) {
                     gInventoryOpen = false;
                     setState(window, GameState::Playing);
                 }
             }
             if (uiButton(ui, font, cx - 260, cy + 160, 520, 80, "QUIT", mfx, mfy, click))
                 glfwSetWindowShouldClose(window, true);
         }      //这里关闭窗口

         else if (gState == GameState::CreateWorld) {
             std::string title = "CREATE NEW WORLD";
             font.draw(ui, cx - Font::measure(56, title) * 0.5f, cy - 220, 56, title);

             //种子输入框（就是一个矩形+一行字，没输入时显示提示）
             font.draw(ui, cx - 260, cy - 120, 32, "SEED (numbers, empty = random):");
             ui.rect(cx - 260, cy - 80, 520, 64, { 0,0,0,0.7f });
             ui.rect(cx - 256, cy - 76, 512, 56, { 0.1f,0.1f,0.1f,0.9f });
             if (gSeedInput.empty())
                 font.draw(ui, cx - 240, cy - 64, 36, "(random)", { 0.5f,0.5f,0.5f,1.f });
             else
                 font.draw(ui, cx - 240, cy - 64, 36, gSeedInput);

             //退格删种子（边沿触发）
             bool bsNow = glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS;
             static bool bsWas = false;
             if (bsNow && !bsWas && !gSeedInput.empty()) gSeedInput.pop_back();
             bsWas = bsNow;

             if (uiButton(ui, font, cx - 260, cy + 20, 520, 80, "CREATE", mfx, mfy, click)) {
                 uint32_t seed = gSeedInput.empty()
                     ? (uint32_t)time(nullptr)               //没输就用当前时间当种子
                     : (uint32_t)std::stoul(gSeedInput);
                 startNewWorld(seed);
                 setState(window, GameState::Playing);
             }
             if (uiButton(ui, font, cx - 260, cy + 120, 520, 80, "BACK", mfx, mfy, click))
                 setState(window, GameState::MainMenu);
         }//创建世界分支结束
         ui.end();
        }//主菜单结束

        //帧末统一记录鼠标状态（上面的都用完了）
        wasLeft = left; wasRight = right;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }//循环结束


    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
