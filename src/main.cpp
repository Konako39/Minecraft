#include <glad/glad.h>   // OpenGL 函数声明
#include <GLFW/glfw3.h>  // 开窗口、读输入
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


#include <iostream>
//把相机先弄出来
static Camera gCamera;
static Player gPlayer;//玩家的身体（相机在眼睛里）

float lastX = 640.f, lastY = 360.f;
bool firstMouse = true;

static void mouse_callback(GLFWwindow*,double xops,double yops) {
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

    //获取鼠标改yaw和pitch
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window,mouse_callback);
    //windows窗口，第二个参数是回调函数，鼠标一动就要调用这个 []匿名函数，就地写一个传进去

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

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

    //UI绘制，方块描边
    UIRenderer ui;
    OutlineRenderer outline;
    if (!ui.init() || !outline.init()) {
        std::cerr << "UI初始化失败\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }


    World world(2026);

    glEnable(GL_DEPTH_TEST);// 开启深度测试：近的挡住远的

    float lastTime = (float)glfwGetTime();

    // 5) 主循环：每转一圈 ≈ 刷新一帧画面
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glClearColor(0.53f, 0.81f, 0.92f, 1.0f); // 清空成这个 天空蓝（RGBA，0~1）
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);//深度

        float  now = (float)glfwGetTime();
        float dt = now - lastTime;
        lastTime = now;
        //一帧经过多久


        glm::vec3 f = gCamera.front();
        f.y = 0.f;//走路不该被y影响
        if (glm::length(f) > 0.001f)
            f = glm::normalize(f);//归一化，出方向
        glm::vec3 r = glm::normalize(glm::cross(f, glm::vec3(0, 1, 0)));
        //水平方向的前和右，走路的时候抬头不会朝天上走
        //这里是正上方 叉乘 方向，同时垂直A和B的向量 就出右边

        //把WASD变为想走的方向，再统一乘速度
        float speed = gPlayer.flying ? 10.f : 4.3f;
        glm::vec3 wish(0.f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) wish += f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) wish -= f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) wish -= r;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) wish += r;
        if (glm::length(wish) > 0.001f)
            wish = glm::normalize(wish) * speed;//不归一就经典WD一起按走1.4倍速了
        gPlayer.vel.x = wish.x;
        gPlayer.vel.z = wish.z;

        if (gPlayer.flying) {
            gPlayer.vel.y = 0.f;
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)      
                gPlayer.vel.y = speed;
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) 
                gPlayer.vel.y = -speed;
        }
        else {
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && gPlayer.onGround)
                gPlayer.vel.y = 9.f;//起跳
        }

        //F键切换飞行 fWas防止一帧读取很多次
        bool fNow = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
        static bool fWas = false;
        if (fNow && !fWas) {
            gPlayer.flying = !gPlayer.flying;
            gPlayer.vel.y = 0.f;
        }
            fWas = fNow;

        UpdatePlayer(world, gPlayer, dt);//重力 碰撞
        gCamera.position = gPlayer.eyePos();


        int fbw, fbh;
        glfwGetFramebufferSize(window, &fbw, &fbh);
        float aspect = fbh ? (float)fbw / (float)fbh : 1.f;
        world.updateLoadedChunks(gPlayer.pos, 4);//4个区块半径内自动生成

        //开始挖和放的逻辑
        //准星射线
        glm::ivec3 hit{}, prev{};
        bool hitOK = raycast(world, gCamera.position, gCamera.front(), 8.f, hit, prev);
        bool left = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        bool right = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
       
        //只在按下这一帧的时候触发 记住上一帧的状态，上一帧按下这一帧也按下当作连续了
        static bool wasLeft = false, wasRight = false;//static用在Update里面，循环多少次也是上一次的值
        if (left && !wasLeft && hitOK)
            world.setBlock(hit.x, hit.y, hit.z, Block::Air);
        if (right && !wasRight && hitOK){
            world.setBlock(prev.x,prev.y,prev.z,Block::Stone);
            if(collides(world,gPlayer.pos))
                world.setBlock(prev.x, prev.y, prev.z, Block::Air);
            //如果放自己身体里面了，给他弄成空气
        }
        wasLeft = left;
        wasRight = right;


        glm::mat4 view = gCamera.view();
        glm::mat4 proj = glm::perspective(glm::radians(gCamera.fov), aspect, 0.1f, 500.f);
        //                                          透视效果，fov广角 宽高 近的多少不画 远的多少不画
        
        glUseProgram(program);//用刚刚创建的shader小程序

        glActiveTexture(GL_TEXTURE0);//激活0号纹理 GPU的贴图插槽
        glBindTexture(GL_TEXTURE_2D, atlas);//把图集绑上去
        glUniform1i(glGetUniformLocation(program, "uAtlas"), 0);//用0号

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

            c.draw();

        }
        //画瞄准方块时的描边框
        if (hitOK) {
            glm::mat4 model = glm::translate(glm::mat4(1.f),glm::vec3(hit));
            outline.draw(proj*view*model);
        }
        //画准心UI
        ui.begin(fbw,fbh);
        float cx = fbw * 0.5f, cy = fbh * 0.5f;
        glm::vec4 crossCol{ 1.f,1.f,1.f,0.8f };
        ui.rect(cx - 10, cy - 1, 20, 2, crossCol);//横
        ui.rect(cx - 1, cy - 10, 2, 20, crossCol);//竖
        ui.end();



        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
