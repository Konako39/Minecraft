#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//创建相机
//并计算出单位圆 朝向角度
struct Camera {
	glm::vec3 position{ 0.f,72.f,3.f };
	float yaw = -90.f;
	float pitch = 0.f;
	float speed = 26.f;
	float sensitivity = 0.03f;//灵敏度
	float fov = 70.f;

	glm::vec3 front() const {
		glm::vec3 f;
		f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		//这里*一个cos的意思是要加上下的分量，因为朝上看不能超前走了
		//sin cos 比如y，我抬头，y就从0-1
		//圆上一个点为(cos角，sin角)
		//yaw=0 平视 x=1 y=0 front就是1，0，0 朝向+x
		//yaw放大 到-90° front就是0，0，-1了，朝着屏幕内看OPENGL标准
		// 为了配合我刚刚写的-90yaw默认值，
		f.y = sin(glm::radians(pitch));
		return glm::normalize(f);
	}

	glm::mat4 view() const {
		return glm::lookAt(position, position + front(), glm::vec3(0, 1, 0));
		//lookAt里面参为：眼睛在哪里，看向哪，世界的 上方 是？
		//返回值是 view 矩阵：之后乘上它，相当于「把整个世界变到相机眼前」。
		//第二个+front() 是我算出来的front箭头，眼睛+朝向箭头
	}


};