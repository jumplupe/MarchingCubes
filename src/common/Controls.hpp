#include<GLFW/glfw3.h>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
using namespace glm;

extern GLFWwindow* window;

// 初始化位于z正方向的位置
vec3 position = vec3(0, 0, 5);
// 朝向-z方向的水平角
float horizontal_angle = 3.14f;
// 垂直角：0，看向水平角
float vertical_angle = 0.0f;
// 初始化视角
// fov是缩放级别。 80°：大广角，巨大的变形。 60° - 45°：标准。 20°：大变焦。
float init_fov = 45.0f;

float speed = 0.1f; // 每秒3单位
float mouse_speed = 0.005f;

mat4 projection_matrix;
mat4 view_matrix;

mat4 getProjectionMatrix() {
	return projection_matrix;
}

mat4 getViewMatrix() {
	return view_matrix;
}

void computerMatricesFromInput() {
	double x_pos, y_pos;
	int width,height;
	float fov;
	glfwGetWindowSize(window,&width,&height);
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
		// 获取光标位置
		glfwGetCursorPos(window, &x_pos, &y_pos);
		glfwSetCursorPos(window, width/2, height/2);
		// 计算新的方向
		horizontal_angle += mouse_speed * float(width/2 - x_pos);
		vertical_angle += mouse_speed * float(height/2 - y_pos);
	}
	// 方向：球面坐标转换为直角坐标
	vec3 direction(cos(vertical_angle) * sin(horizontal_angle),
			sin(vertical_angle), cos(vertical_angle) * cos(horizontal_angle));
	// 右向量
	vec3 right = vec3(sin(horizontal_angle - 3.14f / 2.0f), 0,
			cos(horizontal_angle - 3.14f / 2.0f));
	// 上方向向量
	vec3 up = cross(right, direction);
	// 前移
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		position += direction *speed;
	}
	// 后移
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		position -= direction * speed;
	}
	// 右移
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		position += right * speed;
	}
	// 左移
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		position -= right * speed;
	}

	// 滚轮缩放
	fov = init_fov;
	// 重置窗口大小
	glViewport(0, 0, width, height);
	float aspect = (float) width / (float) height;
	// 窗口最小化时
	if(width==0&&height==0)
		aspect=0.0f;

	// 投影矩阵：
	projection_matrix = perspective(45.0f, aspect, 0.1f, 100.0f);
	view_matrix = lookAt(position, position + direction, up);

}
