//============================================================================
// Name        : MarchingCubes.cpp
// Author      : lupe
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

// Include standard headers
#include<iostream>
#include<fstream>
#include <windows.h>
#include <psapi.h>

using namespace std;

// Include GLEW
#include<gl/glew.h>

// Include GLFW
#include<GLFW/glfw3.h>

// Include GLM
#include<glm/glm.hpp>
#include<glm/gtx/transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<glm/gtx/norm.hpp>

using namespace glm;

// Include ImGui
#include "imgui.h"
#include "imgui_impl_glfw_gl3.h"

using namespace ImGui;
// Include self-define headers
#include "Controls.hpp"
#include "LoadShader.h"

GLFWwindow *window;

GLuint g_vertex_obj;
GLuint g_vertex_buffer;
GLuint g_vertex_index_buffer;
GLuint g_GLSL_program_id;
GLint g_uniform_loc;
GLuint g_uniform_obj;

int subdivide_cube_num = 16; // 细分立方体数目
float target_value = 48.0; // 等值面值
int g_view_mode = 0; // 显示模式，0：Fill，1：Lines
// 顶点位置
static const vec3 cube_vertex_position[] = {{0.0, 0.0, 0.0},
                                            {1.0, 0.0, 0.0},
                                            {1.0, 1.0, 0.0},
                                            {0.0, 1.0, 0.0},
                                            {0.0, 0.0, 1.0},
                                            {1.0, 0.0, 1.0},
                                            {1.0, 1.0, 1.0},
                                            {0.0, 1.0, 1.0}};
// 组成12条边的顶点
static const int cube_edges_indices[12][2] = {{0, 1},
                                              {1, 2},
                                              {2, 3},
                                              {
                                               3, 0},
                                              {4, 5},
                                              {5, 6},
                                              {6, 7},
                                              {7, 4},
                                              {0, 4},
                                              {1, 5},
                                              {2,
                                                  6},
                                              {3, 7}};

static const vec3 cube_edge_direction[] = {{1.0,  0.0,  0.0},
                                           {0.0,  1.0,  0.0},
                                           {-1.0, 0.0,  0.0},
                                           {0.0,  -1.0, 0.0},
                                           {1.0,  0.0,
                                                        0.0},
                                           {0.0,  1.0,  0.0},
                                           {-1.0, 0.0,  0.0},
                                           {0.0,  -1.0, 0.0},
                                           {0.0,  0.0,  1.0},
                                           {0.0,  0.0,  1.0},
                                           {0.0,  0.0,
                                                        1.0},
                                           {0.0,  0.0,  1.0}};

// 绘制立方体线框
void createCubeFrame(float size) {
    // 立方体6个面的顶点
    vec3 *cube_vertices = new vec3[24];
    // 所需绘制的三角形顶点索引
    unsigned int *cube_lines_indices = new unsigned int[24];
    size *= 0.5;

    int k = 0;
    for (int i = 0; i < 12; ++i) {
        for (int j = 0; j < 2; ++j) {
            cube_vertices[k] = size
                               * cube_vertex_position[cube_edges_indices[i][j]];
            k++;
        }
    }
    k = 0;
    unsigned int j = 0;
    for (unsigned int i = 0; i < 12; ++i) {
        j = i * 2;
        cube_lines_indices[k++] = j;
        cube_lines_indices[k++] = j + 1;
    }

    glGenVertexArrays(1, &g_vertex_obj);
    glBindVertexArray(g_vertex_obj);

    glGenBuffers(1, &g_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, g_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * 24, cube_vertices,
                 GL_STATIC_DRAW);

    glGenBuffers(1, &g_vertex_index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vertex_index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * 24,
                 cube_lines_indices, GL_STATIC_DRAW);

    glUseProgram(g_GLSL_program_id);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);

    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, (void *) 0);

    delete[] cube_vertices;
    delete[] cube_lines_indices;
    glDeleteVertexArrays(1, &g_vertex_obj);
    glDeleteBuffers(1, &g_vertex_buffer);
    glDeleteBuffers(1, &g_vertex_index_buffer);

}

void marchingCube(float x, float y, float z, float scale) {
    extern int aiCubeEdgeFlags[256];
    extern int a2iTriangleConnectionTable[256][16];
    vec3 *iossurface_vertices = new vec3[12];
    float *cube_vertex_value = new float[8];
    int flag_index = 0;
    int edge_flag;
    float a_offset;
    int triangles_num = 0;
    //生成样本数据源点
    vec3 source_point = vec3(0.5f, 0.5f, 0.5f);
    vec3 subdivie_point = vec3(x, y, z);

    // 创建样本并查找顶点在表面内部或外部
    for (int i = 0; i < 8; ++i) {
        cube_vertex_value[i] = 2 / distance2(subdivie_point + cube_vertex_position[i] * scale, source_point);
        if (cube_vertex_value[i] <= target_value)
            flag_index |= 1 << i;
    }

    edge_flag = aiCubeEdgeFlags[flag_index];
    if (edge_flag == 0) {
        delete[] iossurface_vertices;
        delete[] cube_vertex_value;
        return;
    }
    for (int i = 0; i < 12; ++i) {
        if (edge_flag & (1 << i)) {
            float delta = cube_vertex_value[cube_edges_indices[i][1]]
                          - cube_vertex_value[cube_edges_indices[i][0]];
            if (delta == 0.0)
                a_offset = 0.5;
            else
                a_offset = (target_value
                            - cube_vertex_value[cube_edges_indices[i][0]]) / delta;

            iossurface_vertices[i].x = x
                                       + (cube_vertex_position[cube_edges_indices[i][0]][0]
                                          + a_offset * cube_edge_direction[i][0]) * scale;
            iossurface_vertices[i].y = y
                                       + (cube_vertex_position[cube_edges_indices[i][0]][1]
                                          + a_offset * cube_edge_direction[i][1]) * scale;
            iossurface_vertices[i].z = z
                                       + (cube_vertex_position[cube_edges_indices[i][0]][2]
                                          + a_offset * cube_edge_direction[i][2]) * scale;
        }
    }


        for (int i = 0; i < 5; ++i) {
            if (a2iTriangleConnectionTable[flag_index][3 * i] < 0)
                break;
            triangles_num++;
        }
        int triangles_indices_num = triangles_num * 3;

        unsigned int *triangles_indices = new unsigned int[triangles_indices_num];

        for (int i = 0; i < triangles_indices_num; ++i)
            triangles_indices[i] = a2iTriangleConnectionTable[flag_index][i];

        glGenVertexArrays(1, &g_vertex_obj);
        glBindVertexArray(g_vertex_obj);

        glGenBuffers(1, &g_vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, g_vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * 12, iossurface_vertices, GL_STATIC_DRAW);

        glGenBuffers(1, &g_vertex_index_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vertex_index_buffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * triangles_indices_num, triangles_indices,
                     GL_STATIC_DRAW);

        glUseProgram(g_GLSL_program_id);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);

        glDrawElements((g_view_mode == 0 ? GL_TRIANGLES : GL_LINES), triangles_indices_num, GL_UNSIGNED_INT,
                       (void *) 0);

        delete[] iossurface_vertices;
        delete[] cube_vertex_value;
        delete[] triangles_indices;
        glDeleteVertexArrays(1, &g_vertex_obj);
        glDeleteBuffers(1, &g_vertex_buffer);
        glDeleteBuffers(1, &g_vertex_index_buffer);
}


void marchingCubes() {
    float step_size = 1.0 / subdivide_cube_num;
// 循环遍历每个立方体，立方体数由细分数决定
    for (int i = 0; i < subdivide_cube_num; ++i)
        for (int j = 0; j < subdivide_cube_num; ++j)
            for (int k = 0; k < subdivide_cube_num; ++k){
                HANDLE handle = GetCurrentProcess();
                PROCESS_MEMORY_COUNTERS pmc;
                GetProcessMemoryInfo(handle, &pmc, sizeof(pmc));
                marchingCube(i * step_size, j * step_size, k * step_size,
                             step_size);
        }
}

static void errorCallback(int error, const char *description) {
    cerr << "Error " << error << ":" << description << endl;
}

int main() {

    glfwSetErrorCallback(errorCallback);

    // Intialize GLFW
    if (!glfwInit()) {
        cerr << "无法初始化glfw！" << endl;
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(640, 480, "MarchingCubes", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);

    // Initialize GLEW
    glewExperimental = GL_TRUE; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        cerr << "无法初始化glew" << endl;
        glfwTerminate();
        return -1;
    }

    // 初始化ImGui
    static bool show_window = true;
    ImGui_ImplGlfwGL3_Init(window, true);

    glClearColor(0.8, 0.8, 0.8, 1.0);
    g_GLSL_program_id = LoadShaders("vertex.glsl", "fragment.glsl");


    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS
           && !glfwWindowShouldClose(window)) {

        // 创建ImGui辅助窗口
        {
            // 窗口样式
            ImGuiWindowFlags window_flags = 0;
            window_flags |= ImGuiWindowFlags_NoCollapse;
            window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
            // 创建窗口并设置大小
            ImGui_ImplGlfwGL3_NewFrame();
            // 绘制窗口
            Begin("MarchingCubes Data", &show_window, window_flags);
            LabelText("", "Isometric surface value:");
            InputFloat("", &target_value, 1.0f, 10.0f);
            LabelText("", "Subdivide nums in each axis:");
            InputInt("", &subdivide_cube_num,1);
            LabelText("", "View mode:");
            RadioButton("Fill", &g_view_mode, 0);
            SameLine();
            RadioButton("Lines", &g_view_mode, 1);
            End();
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // 执行MarchingCube算法
        createCubeFrame(2);
        marchingCubes();

        computerMatricesFromInput();
        mat4 projection_matrix = getProjectionMatrix();
        mat4 view_matrix = getViewMatrix();

        mat4 model_matrix = mat4(1.0f);

        g_uniform_loc = glGetUniformLocation(g_GLSL_program_id, "model_matrix");
        glUniformMatrix4fv(g_uniform_loc, 1, GL_FALSE, &model_matrix[0][0]);
        g_uniform_loc = glGetUniformLocation(g_GLSL_program_id, "view_matrix");
        glUniformMatrix4fv(g_uniform_loc, 1, GL_FALSE, &view_matrix[0][0]);
        g_uniform_loc = glGetUniformLocation(g_GLSL_program_id,
                                             "projection_matrix");
        glUniformMatrix4fv(g_uniform_loc, 1, GL_FALSE,
                           &projection_matrix[0][0]);


        // 渲染ImGui
        ImGui::Render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &g_vertex_obj);
    glDeleteBuffers(1, &g_vertex_buffer);
    glDeleteBuffers(1, &g_vertex_index_buffer);

    // 释放ImGui资源
    ImGui_ImplGlfwGL3_Shutdown();

    glfwTerminate();
    return 0;
}

// 对于任何边，如果一个顶点在表面内，另一个顶点在表面外，则此边与表面相交
// 对于立方体的8个顶点中的每一个可以是两个可能的状态：表面内部或外部
// 对于任何立方体，都是2^8=256个可能的顶点状态集合
// 该表列出了边与表面相交的所有256个可能的顶点状态
// 这有12条边。对于表中的每个条目，如果边#id是相交的，则位#id被设置为1
int aiCubeEdgeFlags[256] = {0x000, 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c, 0x80c, 0x905, 0xa0f, 0xb06, 0xc0a,
                            0xd03, 0xe09, 0xf00,
                            0x190, 0x099, 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c, 0x99c, 0x895, 0xb9f, 0xa96, 0xd9a,
                            0xc93, 0xf99, 0xe90,
                            0x230, 0x339, 0x033, 0x13a, 0x636, 0x73f, 0x435, 0x53c, 0xa3c, 0xb35, 0x83f, 0x936, 0xe3a,
                            0xf33, 0xc39, 0xd30,
                            0x3a0, 0x2a9, 0x1a3, 0x0aa, 0x7a6, 0x6af, 0x5a5, 0x4ac, 0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa,
                            0xea3, 0xda9, 0xca0,
                            0x460, 0x569, 0x663, 0x76a, 0x066, 0x16f, 0x265, 0x36c, 0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a,
                            0x963, 0xa69, 0xb60,
                            0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0x0ff, 0x3f5, 0x2fc, 0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa,
                            0x8f3, 0xbf9, 0xaf0,
                            0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x055, 0x15c, 0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a,
                            0xb53, 0x859, 0x950,
                            0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0x0cc, 0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca,
                            0xac3, 0x9c9, 0x8c0,
                            0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc, 0x0cc, 0x1c5, 0x2cf, 0x3c6, 0x4ca,
                            0x5c3, 0x6c9, 0x7c0,
                            0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c, 0x15c, 0x055, 0x35f, 0x256, 0x55a,
                            0x453, 0x759, 0x650,
                            0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc, 0x2fc, 0x3f5, 0x0ff, 0x1f6, 0x6fa,
                            0x7f3, 0x4f9, 0x5f0,
                            0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c, 0x36c, 0x265, 0x16f, 0x066, 0x76a,
                            0x663, 0x569, 0x460,
                            0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac, 0x4ac, 0x5a5, 0x6af, 0x7a6, 0x0aa,
                            0x1a3, 0x2a9, 0x3a0,
                            0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c, 0x53c, 0x435, 0x73f, 0x636, 0x13a,
                            0x033, 0x339, 0x230,
                            0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c, 0x69c, 0x795, 0x49f, 0x596, 0x29a,
                            0x393, 0x099, 0x190,
                            0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c, 0x70c, 0x605, 0x50f, 0x406, 0x30a,
                            0x203, 0x109, 0x000};

// 对于aiCubeEdgeFlags中列出的每个可能的顶点状态，边缘交点都有一个特定的分割三角形。
// a2iTriangleConnectionTable以0-5边缘三元组的形式列出所有形式，列表以无效值-1终止。
// 例如：a2iTriangleConnectionTable[3]列出corner[0]接corner[1]在表面内部形成的2个三角形，
// 但立方体的其余corner则不是。
int a2iTriangleConnectionTable[256][16] = {{-1, -1, -1, -1, -1, -1, -1, -1,
                                                                            -1, -1, -1, -1, -1, -1, -1, -1},
                                           {0,  8,  3,  -1, -1, -1, -1, -1, -1, -1,
                                                                                    -1, -1, -1, -1, -1, -1},
                                           {0,  1,  9,  -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                                            -1, -1, -1, -1},
                                           {1,  8,  3,  9,  8,  1,  -1, -1, -1, -1, -1, -1, -1, -1,
                                                                                                    -1, -1},
                                           {1,  2,  10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {0,
                                                8,  3,  1,  2,  10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {9,
                                                2,  10, 0,  2,  9,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {2,
                                                8,  3,  2,  10, 8,  10, 9,  8,  -1, -1, -1, -1, -1, -1, -1},
                                           {3,  11,
                                                    2,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {0,
                                                11, 2,  8,  11, 0,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {1,
                                                9,  0,  2,  3,  11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {1,
                                                11, 2,  1,  9,  11, 9,  8,  11, -1, -1, -1, -1, -1, -1, -1},
                                           {3,
                                                10, 1,  11, 10, 3,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {0,
                                                10, 1,  0,  8,  10, 8,  11, 10, -1, -1, -1, -1, -1, -1, -1},
                                           {3,
                                                9,  0,  3,  11, 9,  11, 10, 9,  -1, -1, -1, -1, -1, -1, -1},
                                           {9,  8,
                                                    10, 10, 8,  11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {4,  7,
                                                    8,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {4,  3,
                                                    0,  7,  3,  4,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {0,  1,  9,
                                                        8,  4,  7,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {4,  1,  9,  4,
                                                            7,  1,  7,  3,  1,  -1, -1, -1, -1, -1, -1, -1},
                                           {1,  2,  10, 8,  4,
                                                                7,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {3,  4,  7,  3,  0,  4,
                                                                    1,  2,  10, -1, -1, -1, -1, -1, -1, -1},
                                           {9,  2,  10, 9,  0,  2,  8,
                                                                        4,  7,  -1, -1, -1, -1, -1, -1, -1},
                                           {2,  10, 9,  2,  9,  7,  2,  7,
                                                                            3,  7,  9,  4,  -1, -1, -1, -1},
                                           {8,  4,  7,  3,  11, 2,  -1, -1, -1,
                                                                                -1, -1, -1, -1, -1, -1, -1},
                                           {11, 4,  7,  11, 2,  4,  2,  0,  4,  -1,
                                                                                    -1, -1, -1, -1, -1, -1},
                                           {9,  0,  1,  8,  4,  7,  2,  3,  11, -1, -1,
                                                                                        -1, -1, -1, -1, -1},
                                           {4,  7,  11, 9,  4,  11, 9,  11, 2,  9,  2,  1,
                                                                                            -1, -1, -1, -1},
                                           {3,  10, 1,  3,  11, 10, 7,  8,  4,  -1, -1, -1,
                                                                                            -1, -1, -1, -1},
                                           {1,  11, 10, 1,  4,  11, 1,  0,  4,  7,  11, 4,  -1,
                                                                                                -1, -1, -1},
                                           {4,  7,  8,  9,  0,  11, 9,  11, 10, 11, 0,  3,  -1, -1,
                                                                                                    -1, -1},
                                           {4,  7,  11, 4,  11, 9,  9,  11, 10, -1, -1, -1, -1, -1,
                                                                                                    -1, -1},
                                           {9,  5,  4,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                                                    -1, -1},
                                           {9,  5,  4,  0,  8,  3,  -1, -1, -1, -1, -1, -1, -1, -1,
                                                                                                    -1, -1},
                                           {0,  5,  4,  1,  5,  0,  -1, -1, -1, -1, -1, -1, -1, -1,
                                                                                                    -1, -1},
                                           {8,  5,  4,  8,  3,  5,  3,  1,  5,  -1, -1, -1, -1, -1, -1,
                                                                                                        -1},
                                           {1,  2,  10, 9,  5,  4,  -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                                                        -1},
                                           {3,  0,  8,  1,  2,  10, 4,  9,  5,  -1, -1, -1, -1, -1, -1, -1},
                                           {5,  2,  10,
                                                        5,  4,  2,  4,  0,  2,  -1, -1, -1, -1, -1, -1, -1},
                                           {2,  10, 5,  3,
                                                            2,  5,  3,  5,  4,  3,  4,  8,  -1, -1, -1, -1},
                                           {9,  5,  4,  2,  3,  11,
                                                                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {0,  11, 2,  0,  8,  11,
                                                                    4,  9,  5,  -1, -1, -1, -1, -1, -1, -1},
                                           {0,  5,  4,  0,  1,  5,  2,  3,
                                                                            11, -1, -1, -1, -1, -1, -1, -1},
                                           {2,  1,  5,  2,  5,  8,  2,  8,  11,
                                                                                4,  8,  5,  -1, -1, -1, -1},
                                           {10, 3,  11, 10, 1,  3,  9,  5,  4,  -1,
                                                                                    -1, -1, -1, -1, -1, -1},
                                           {4,  9,  5,  0,  8,  1,  8,  10, 1,  8,  11,
                                                                                        10, -1, -1, -1, -1},
                                           {5,  4,  0,  5,  0,  11, 5,  11, 10, 11, 0,  3,
                                                                                            -1, -1, -1, -1},
                                           {5,  4,  8,  5,  8,  10, 10, 8,  11, -1, -1, -1,
                                                                                            -1, -1, -1, -1},
                                           {9,  7,  8,  5,  7,  9,  -1, -1, -1, -1, -1, -1,
                                                                                            -1, -1, -1, -1},
                                           {9,  3,  0,  9,  5,  3,  5,  7,  3,  -1, -1, -1, -1,
                                                                                                -1, -1, -1},
                                           {0,  7,  8,  0,  1,  7,  1,  5,  7,  -1, -1, -1, -1, -1,
                                                                                                    -1, -1},
                                           {1,  5,  3,  3,  5,  7,  -1, -1, -1, -1, -1, -1, -1, -1,
                                                                                                    -1, -1},
                                           {9,  7,  8,  9,  5,  7,  10, 1,  2,  -1, -1, -1, -1, -1, -1,
                                                                                                        -1},
                                           {10, 1,  2,  9,  5,  0,  5,  3,  0,  5,  7,  3,  -1, -1, -1, -1},
                                           {
                                            8,  0,  2,  8,  2,  5,  8,  5,  7,  10, 5,  2,  -1, -1, -1, -1},
                                           {2,  10,
                                                    5,  2,  5,  3,  3,  5,  7,  -1, -1, -1, -1, -1, -1, -1},
                                           {7,  9,  5,  7,
                                                            8,  9,  3,  11, 2,  -1, -1, -1, -1, -1, -1, -1},
                                           {9,  5,  7,  9,  7,
                                                                2,  9,  2,  0,  2,  7,  11, -1, -1, -1, -1},
                                           {2,  3,  11, 0,  1,  8,  1,
                                                                        7,  8,  1,  5,  7,  -1, -1, -1, -1},
                                           {11, 2,  1,  11, 1,  7,  7,  1,  5,
                                                                                -1, -1, -1, -1, -1, -1, -1},
                                           {9,  5,  8,  8,  5,  7,  10, 1,  3,  10,
                                                                                    3,  11, -1, -1, -1, -1},
                                           {5,  7,  0,  5,  0,  9,  7,  11, 0,  1,  0,  10,
                                                                                            11, 10, 0,  -1},
                                           {11, 10, 0,  11, 0,  3,  10, 5,  0,  8,  0,  7,  5,  7,
                                                                                                    0,  -1},
                                           {11, 10, 5,  7,  11, 5,  -1, -1, -1, -1, -1, -1, -1, -1,
                                                                                                    -1, -1},
                                           {10, 6,  5,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                                                -1, -1, -1},
                                           {0,  8,  3,  5,  10, 6,  -1, -1, -1, -1, -1, -1, -1,
                                                                                                -1, -1, -1},
                                           {9,  0,  1,  5,  10, 6,  -1, -1, -1, -1, -1, -1, -1,
                                                                                                -1, -1, -1},
                                           {1,  8,  3,  1,  9,  8,  5,  10, 6,  -1, -1, -1, -1, -1,
                                                                                                    -1, -1},
                                           {1,  6,  5,  2,  6,  1,  -1, -1, -1, -1, -1, -1, -1, -1,
                                                                                                    -1, -1},
                                           {1,  6,  5,  1,  2,  6,  3,  0,  8,  -1, -1, -1, -1, -1, -1,
                                                                                                        -1},
                                           {9,  6,  5,  9,  0,  6,  0,  2,  6,  -1, -1, -1, -1, -1, -1, -1},
                                           {5,  9,  8,  5,  8,  2,  5,  2,  6,  3,  2,  8,  -1, -1, -1, -1},
                                           {2,  3,  11, 10,
                                                            6,  5,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {11, 0,  8,  11,
                                                            2,  0,  10, 6,  5,  -1, -1, -1, -1, -1, -1, -1},
                                           {0,  1,  9,  2,  3,
                                                                11, 5,  10, 6,  -1, -1, -1, -1, -1, -1, -1},
                                           {5,  10, 6,  1,  9,  2,
                                                                    9,  11, 2,  9,  8,  11, -1, -1, -1, -1},
                                           {6,  3,  11, 6,  5,  3,  5,  1,
                                                                            3,  -1, -1, -1, -1, -1, -1, -1},
                                           {0,  8,  11, 0,  11, 5,  0,  5,  1,
                                                                                5,  11, 6,  -1, -1, -1, -1},
                                           {3,  11, 6,  0,  3,  6,  0,  6,  5,  0,  5,
                                                                                        9,  -1, -1, -1, -1},
                                           {6,  5,  9,  6,  9,  11, 11, 9,  8,  -1, -1, -1,
                                                                                            -1, -1, -1, -1},
                                           {5,  10, 6,  4,  7,  8,  -1, -1, -1, -1, -1, -1,
                                                                                            -1, -1, -1, -1},
                                           {4,  3,  0,  4,  7,  3,  6,  5,  10, -1, -1, -1, -1,
                                                                                                -1, -1, -1},
                                           {1,  9,  0,  5,  10, 6,  8,  4,  7,  -1, -1, -1, -1, -1,
                                                                                                    -1, -1},
                                           {10, 6,  5,  1,  9,  7,  1,  7,  3,  7,  9,  4,  -1, -1, -1, -1},
                                           {6,  1,  2,  6,
                                                            5,  1,  4,  7,  8,  -1, -1, -1, -1, -1, -1, -1},
                                           {1,  2,  5,  5,  2,  6,
                                                                    3,  0,  4,  3,  4,  7,  -1, -1, -1, -1},
                                           {8,  4,  7,  9,  0,  5,  0,  6,  5,
                                                                                0,  2,  6,  -1, -1, -1, -1},
                                           {7,  3,  9,  7,  9,  4,  3,  2,  9,  5,  9,  6,
                                                                                            2,  6,  9,  -1},
                                           {3,  11, 2,  7,  8,  4,  10, 6,  5,  -1, -1, -1, -1,
                                                                                                -1, -1, -1},
                                           {5,  10, 6,  4,  7,  2,  4,  2,  0,  2,  7,  11, -1, -1,
                                                                                                    -1, -1},
                                           {0,  1,  9,  4,  7,  8,  2,  3,  11, 5,  10, 6,  -1, -1, -1, -1},
                                           {9,  2,  1,  9,
                                                            11, 2,  9,  4,  11, 7,  11, 4,  5,  10, 6,  -1},
                                           {8,  4,  7,  3,  11, 5,
                                                                    3,  5,  1,  5,  11, 6,  -1, -1, -1, -1},
                                           {5,  1,  11, 5,  11, 6,  1,  0,
                                                                            11, 7,  11, 4,  0,  4,  11, -1},
                                           {0,  5,  9,  0,  6,  5,  0,  3,  6,  11,
                                                                                    6,  3,  8,  4,  7,  -1},
                                           {6,  5,  9,  6,  9,  11, 4,  7,  9,  7,  11, 9,  -1,
                                                                                                -1, -1, -1},
                                           {10, 4,  9,  6,  4,  10, -1, -1, -1, -1, -1, -1, -1,
                                                                                                -1, -1, -1},
                                           {4,  10, 6,  4,  9,  10, 0,  8,  3,  -1, -1, -1, -1, -1,
                                                                                                    -1, -1},
                                           {10, 0,  1,  10, 6,  0,  6,  4,  0,  -1, -1, -1, -1, -1, -1,
                                                                                                        -1},
                                           {8,  3,  1,  8,  1,  6,  8,  6,  4,  6,  1,  10, -1, -1, -1, -1},
                                           {
                                            1,  4,  9,  1,  2,  4,  2,  6,  4,  -1, -1, -1, -1, -1, -1, -1},
                                           {3,  0,
                                                    8,  1,  2,  9,  2,  4,  9,  2,  6,  4,  -1, -1, -1, -1},
                                           {0,  2,  4,  4,  2,
                                                                6,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {8,  3,  2,  8,  2,  4,
                                                                    4,  2,  6,  -1, -1, -1, -1, -1, -1, -1},
                                           {10, 4,  9,  10, 6,  4,  11,
                                                                        2,  3,  -1, -1, -1, -1, -1, -1, -1},
                                           {0,  8,  2,  2,  8,  11, 4,  9,
                                                                            10, 4,  10, 6,  -1, -1, -1, -1},
                                           {3,  11, 2,  0,  1,  6,  0,  6,  4,  6,
                                                                                    1,  10, -1, -1, -1, -1},
                                           {6,  4,  1,  6,  1,  10, 4,  8,  1,  2,  1,  11,
                                                                                            8,  11, 1,  -1},
                                           {9,  6,  4,  9,  3,  6,  9,  1,  3,  11, 6,  3,  -1, -1,
                                                                                                    -1, -1},
                                           {8,  11, 1,  8,  1,  0,  11, 6,  1,  9,  1,  4,  6,  4,  1,  -1},
                                           {3,  11, 6,  3,  6,  0,  0,  6,  4,  -1, -1, -1, -1, -1, -1, -1},
                                           {6,  4,  8,
                                                        11, 6,  8,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {7,  10, 6,
                                                        7,  8,  10, 8,  9,  10, -1, -1, -1, -1, -1, -1, -1},
                                           {0,  7,  3,  0,
                                                            10, 7,  0,  9,  10, 6,  7,  10, -1, -1, -1, -1},
                                           {10, 6,  7,  1,  10,
                                                                7,  1,  7,  8,  1,  8,  0,  -1, -1, -1, -1},
                                           {10, 6,  7,  10, 7,  1,  1,
                                                                        7,  3,  -1, -1, -1, -1, -1, -1, -1},
                                           {1,  2,  6,  1,  6,  8,  1,  8,  9,
                                                                                8,  6,  7,  -1, -1, -1, -1},
                                           {2,  6,  9,  2,  9,  1,  6,  7,  9,  0,  9,  3,
                                                                                            7,  3,  9,  -1},
                                           {7,  8,  0,  7,  0,  6,  6,  0,  2,  -1, -1, -1, -1, -1,
                                                                                                    -1, -1},
                                           {7,  3,  2,  6,  7,  2,  -1, -1, -1, -1, -1, -1, -1, -1,
                                                                                                    -1, -1},
                                           {2,  3,  11, 10, 6,  8,  10, 8,  9,  8,  6,  7,  -1, -1, -1,
                                                                                                        -1},
                                           {2,  0,  7,  2,  7,  11, 0,  9,  7,  6,  7,  10, 9,  10, 7,  -1},
                                           {
                                            1,  8,  0,  1,  7,  8,  1,  10, 7,  6,  7,  10, 2,  3,  11, -1},
                                           {11, 2,
                                                    1,  11, 1,  7,  10, 6,  1,  6,  7,  1,  -1, -1, -1, -1},
                                           {8,  9,  6,  8,
                                                            6,  7,  9,  1,  6,  11, 6,  3,  1,  3,  6,  -1},
                                           {0,  9,  1,  11, 6,  7,  -1,
                                                                        -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {7,  8,  0,  7,  0,  6,  3,  11,
                                                                            0,  11, 6,  0,  -1, -1, -1, -1},
                                           {7,  11, 6,  -1, -1, -1, -1, -1,
                                                                            -1, -1, -1, -1, -1, -1, -1, -1},
                                           {7,  6,  11, -1, -1, -1, -1,
                                                                        -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {3,  0,  8,  11, 7,  6,  -1,
                                                                        -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {0,  1,  9,  11, 7,  6,  -1,
                                                                        -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {8,  1,  9,  8,  3,  1,  11, 7,
                                                                            6,  -1, -1, -1, -1, -1, -1, -1},
                                           {10, 1,  2,  6,  11, 7,  -1, -1,
                                                                            -1, -1, -1, -1, -1, -1, -1, -1},
                                           {1,  2,  10, 3,  0,  8,  6,  11, 7,
                                                                                -1, -1, -1, -1, -1, -1, -1},
                                           {2,  9,  0,  2,  10, 9,  6,  11, 7,  -1,
                                                                                    -1, -1, -1, -1, -1, -1},
                                           {6,  11, 7,  2,  10, 3,  10, 8,  3,  10, 9,
                                                                                        8,  -1, -1, -1, -1},
                                           {7,  2,  3,  6,  2,  7,  -1, -1, -1, -1, -1, -1,
                                                                                            -1, -1, -1, -1},
                                           {7,  0,  8,  7,  6,  0,  6,  2,  0,  -1, -1, -1, -1,
                                                                                                -1, -1, -1},
                                           {2,  7,  6,  2,  3,  7,  0,  1,  9,  -1, -1, -1, -1, -1,
                                                                                                    -1, -1},
                                           {1,  6,  2,  1,  8,  6,  1,  9,  8,  8,  7,  6,  -1, -1, -1, -1},
                                           {10, 7,  6,  10,
                                                            1,  7,  1,  3,  7,  -1, -1, -1, -1, -1, -1, -1},
                                           {10, 7,  6,  1,  7,
                                                                10, 1,  8,  7,  1,  0,  8,  -1, -1, -1, -1},
                                           {0,  3,  7,  0,  7,  10, 0,
                                                                        10, 9,  6,  10, 7,  -1, -1, -1, -1},
                                           {7,  6,  10, 7,  10, 8,  8,  10,
                                                                            9,  -1, -1, -1, -1, -1, -1, -1},
                                           {6,  8,  4,  11, 8,  6,  -1, -1,
                                                                            -1, -1, -1, -1, -1, -1, -1, -1},
                                           {3,  6,  11, 3,  0,  6,  0,  4,  6,
                                                                                -1, -1, -1, -1, -1, -1, -1},
                                           {8,  6,  11, 8,  4,  6,  9,  0,  1,  -1,
                                                                                    -1, -1, -1, -1, -1, -1},
                                           {9,  4,  6,  9,  6,  3,  9,  3,  1,  11, 3,  6,
                                                                                            -1, -1, -1, -1},
                                           {6,  8,  4,  6,  11, 8,  2,  10, 1,  -1, -1, -1, -1,
                                                                                                -1, -1, -1},
                                           {1,  2,  10, 3,  0,  11, 0,  6,  11, 0,  4,  6,  -1, -1,
                                                                                                    -1, -1},
                                           {4,  11, 8,  4,  6,  11, 0,  2,  9,  2,  10, 9,  -1, -1, -1,
                                                                                                        -1},
                                           {10, 9,  3,  10, 3,  2,  9,  4,  3,  11, 3,  6,  4,  6,  3,  -1},
                                           {
                                            8,  2,  3,  8,  4,  2,  4,  6,  2,  -1, -1, -1, -1, -1, -1, -1},
                                           {0,  4,
                                                    2,  4,  6,  2,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {1,  9,  0,
                                                        2,  3,  4,  2,  4,  6,  4,  3,  8,  -1, -1, -1, -1},
                                           {1,  9,  4,  1,  4,  2,
                                                                    2,  4,  6,  -1, -1, -1, -1, -1, -1, -1},
                                           {8,  1,  3,  8,  6,  1,  8,  4,
                                                                            6,  6,  10, 1,  -1, -1, -1, -1},
                                           {10, 1,  0,  10, 0,  6,  6,  0,  4,
                                                                                -1, -1, -1, -1, -1, -1, -1},
                                           {4,  6,  3,  4,  3,  8,  6,  10, 3,  0,
                                                                                    3,  9,  10, 9,  3,  -1},
                                           {10, 9,  4,  6,  10, 4,  -1, -1, -1, -1, -1,
                                                                                        -1, -1, -1, -1, -1},
                                           {4,  9,  5,  7,  6,  11, -1, -1, -1, -1, -1,
                                                                                        -1, -1, -1, -1, -1},
                                           {0,  8,  3,  4,  9,  5,  11, 7,  6,  -1, -1, -1,
                                                                                            -1, -1, -1, -1},
                                           {5,  0,  1,  5,  4,  0,  7,  6,  11, -1, -1, -1, -1,
                                                                                                -1, -1, -1},
                                           {11, 7,  6,  8,  3,  4,  3,  5,  4,  3,  1,  5,  -1, -1, -1,
                                                                                                        -1},
                                           {9,  5,  4,  10, 1,  2,  7,  6,  11, -1, -1, -1, -1, -1, -1, -1},
                                           {6,  11, 7,
                                                        1,  2,  10, 0,  8,  3,  4,  9,  5,  -1, -1, -1, -1},
                                           {7,  6,  11, 5,  4,
                                                                10, 4,  2,  10, 4,  0,  2,  -1, -1, -1, -1},
                                           {3,  4,  8,  3,  5,  4,  3,
                                                                        2,  5,  10, 5,  2,  11, 7,  6,  -1},
                                           {7,  2,  3,  7,  6,  2,  5,  4,  9,  -1,
                                                                                    -1, -1, -1, -1, -1, -1},
                                           {9,  5,  4,  0,  8,  6,  0,  6,  2,  6,  8,  7,
                                                                                            -1, -1, -1, -1},
                                           {3,  6,  2,  3,  7,  6,  1,  5,  0,  5,  4,  0,  -1, -1,
                                                                                                    -1, -1},
                                           {6,  2,  8,  6,  8,  7,  2,  1,  8,  4,  8,  5,  1,  5,  8,  -1},
                                           {
                                            9,  5,  4,  10, 1,  6,  1,  7,  6,  1,  3,  7,  -1, -1, -1, -1},
                                           {1,  6,
                                                    10, 1,  7,  6,  1,  0,  7,  8,  7,  0,  9,  5,  4,  -1},
                                           {4,  0,  10, 4,  10,
                                                                5,  0,  3,  10, 6,  10, 7,  3,  7,  10, -1},
                                           {7,  6,  10, 7,  10, 8,  5,
                                                                        4,  10, 4,  8,  10, -1, -1, -1, -1},
                                           {6,  9,  5,  6,  11, 9,  11, 8,
                                                                            9,  -1, -1, -1, -1, -1, -1, -1},
                                           {3,  6,  11, 0,  6,  3,  0,  5,  6,
                                                                                0,  9,  5,  -1, -1, -1, -1},
                                           {0,  11, 8,  0,  5,  11, 0,  1,  5,  5,  6,
                                                                                        11, -1, -1, -1, -1},
                                           {6,  11, 3,  6,  3,  5,  5,  3,  1,  -1, -1, -1,
                                                                                            -1, -1, -1, -1},
                                           {1,  2,  10, 9,  5,  11, 9,  11, 8,  11, 5,  6,  -1,
                                                                                                -1, -1, -1},
                                           {0,  11, 3,  0,  6,  11, 0,  9,  6,  5,  6,  9,  1,  2,  10,
                                                                                                        -1},
                                           {11, 8,  5,  11, 5,  6,  8,  0,  5,  10, 5,  2,  0,  2,  5,  -1},
                                           {
                                            6,  11, 3,  6,  3,  5,  2,  10, 3,  10, 5,  3,  -1, -1, -1, -1},
                                           {5,  8,
                                                    9,  5,  2,  8,  5,  6,  2,  3,  8,  2,  -1, -1, -1, -1},
                                           {9,  5,  6,  9,  6,
                                                                0,  0,  6,  2,  -1, -1, -1, -1, -1, -1, -1},
                                           {1,  5,  8,  1,  8,  0,  5,
                                                                        6,  8,  3,  8,  2,  6,  2,  8,  -1},
                                           {1,  5,  6,  2,  1,  6,  -1, -1, -1,
                                                                                -1, -1, -1, -1, -1, -1, -1},
                                           {1,  3,  6,  1,  6,  10, 3,  8,  6,  5,
                                                                                    6,  9,  8,  9,  6,  -1},
                                           {10, 1,  0,  10, 0,  6,  9,  5,  0,  5,  6,  0,  -1,
                                                                                                -1, -1, -1},
                                           {0,  3,  8,  5,  6,  10, -1, -1, -1, -1, -1, -1, -1,
                                                                                                -1, -1, -1},
                                           {10, 5,  6,  -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                                            -1, -1, -1, -1},
                                           {11, 5,  10, 7,  5,  11, -1, -1, -1, -1, -1, -1,
                                                                                            -1, -1, -1, -1},
                                           {11, 5,  10, 11, 7,  5,  8,  3,  0,  -1, -1, -1,
                                                                                            -1, -1, -1, -1},
                                           {5,  11, 7,  5,  10, 11, 1,  9,  0,  -1, -1, -1,
                                                                                            -1, -1, -1, -1},
                                           {10, 7,  5,  10, 11, 7,  9,  8,  1,  8,  3,  1,  -1,
                                                                                                -1, -1, -1},
                                           {11, 1,  2,  11, 7,  1,  7,  5,  1,  -1, -1, -1, -1, -1,
                                                                                                    -1, -1},
                                           {0,  8,  3,  1,  2,  7,  1,  7,  5,  7,  2,  11, -1, -1, -1, -1},
                                           {9,  7,  5,  9,
                                                            2,  7,  9,  0,  2,  2,  11, 7,  -1, -1, -1, -1},
                                           {7,  5,  2,  7,  2,  11,
                                                                    5,  9,  2,  3,  2,  8,  9,  8,  2,  -1},
                                           {2,  5,  10, 2,  3,  5,  3,  7,  5,
                                                                                -1, -1, -1, -1, -1, -1, -1},
                                           {8,  2,  0,  8,  5,  2,  8,  7,  5,  10,
                                                                                    2,  5,  -1, -1, -1, -1},
                                           {9,  0,  1,  5,  10, 3,  5,  3,  7,  3,  10, 2,
                                                                                            -1, -1, -1, -1},
                                           {9,  8,  2,  9,  2,  1,  8,  7,  2,  10, 2,  5,  7,  5,
                                                                                                    2,  -1},
                                           {1,  3,  5,  3,  7,  5,  -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                                                        -1},
                                           {0,  8,  7,  0,  7,  1,  1,  7,  5,  -1, -1, -1, -1, -1, -1, -1},
                                           {9,  0,  3,  9,  3,  5,  5,  3,  7,  -1, -1, -1, -1, -1, -1, -1},
                                           {9,  8,  7,  5,
                                                            9,  7,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {5,  8,  4,  5,
                                                            10, 8,  10, 11, 8,  -1, -1, -1, -1, -1, -1, -1},
                                           {5,  0,  4,  5,
                                                            11, 0,  5,  10, 11, 11, 3,  0,  -1, -1, -1, -1},
                                           {0,  1,  9,  8,  4,
                                                                10, 8,  10, 11, 10, 4,  5,  -1, -1, -1, -1},
                                           {10, 11, 4,  10, 4,
                                                                5,  11, 3,  4,  9,  4,  1,  3,  1,  4,  -1},
                                           {2,  5,  1,  2,  8,  5,  2,  11,
                                                                            8,  4,  5,  8,  -1, -1, -1, -1},
                                           {0,  4,  11, 0,  11, 3,  4,  5,  11, 2,
                                                                                    11, 1,  5,  1,  11, -1},
                                           {0,  2,  5,  0,  5,  9,  2,  11, 5,  4,  5,  8,
                                                                                            11, 8,  5,  -1},
                                           {9,  4,  5,  2,  11, 3,  -1, -1, -1, -1, -1, -1, -1,
                                                                                                -1, -1, -1},
                                           {2,  5,  10, 3,  5,  2,  3,  4,  5,  3,  8,  4,  -1, -1, -1,
                                                                                                        -1},
                                           {5,  10, 2,  5,  2,  4,  4,  2,  0,  -1, -1, -1, -1, -1, -1, -1},
                                           {3,  10, 2,
                                                        3,  5,  10, 3,  8,  5,  4,  5,  8,  0,  1,  9,  -1},
                                           {5,  10, 2,  5,  2,  4,
                                                                    1,  9,  2,  9,  4,  2,  -1, -1, -1, -1},
                                           {8,  4,  5,  8,  5,  3,  3,  5,  1,
                                                                                -1, -1, -1, -1, -1, -1, -1},
                                           {0,  4,  5,  1,  0,  5,  -1, -1, -1,
                                                                                -1, -1, -1, -1, -1, -1, -1},
                                           {8,  4,  5,  8,  5,  3,  9,  0,  5,  0,  3,
                                                                                        5,  -1, -1, -1, -1},
                                           {9,  4,  5,  -1, -1, -1, -1, -1, -1, -1, -1,
                                                                                        -1, -1, -1, -1, -1},
                                           {4,  11, 7,  4,  9,  11, 9,  10, 11, -1, -1,
                                                                                        -1, -1, -1, -1, -1},
                                           {0,  8,  3,  4,  9,  7,  9,  11, 7,  9,  10, 11,
                                                                                            -1, -1, -1, -1},
                                           {1,  10, 11, 1,  11, 4,  1,  4,  0,  7,  4,  11, -1,
                                                                                                -1, -1, -1},
                                           {3,  1,  4,  3,  4,  8,  1,  10, 4,  7,  4,  11, 10, 11, 4,
                                                                                                        -1},
                                           {4,  11, 7,  9,  11, 4,  9,  2,  11, 9,  1,  2,  -1, -1, -1, -1},
                                           {9,  7,  4,  9,  11, 7,  9,  1,  11, 2,  11, 1,  0,  8,  3,  -1},
                                           {11, 7,  4,  11,
                                                            4,  2,  2,  4,  0,  -1, -1, -1, -1, -1, -1, -1},
                                           {11, 7,  4,  11, 4,
                                                                2,  8,  3,  4,  3,  2,  4,  -1, -1, -1, -1},
                                           {2,  9,  10, 2,  7,  9,  2,
                                                                        3,  7,  7,  4,  9,  -1, -1, -1, -1},
                                           {9,  10, 7,  9,  7,  4,  10, 2,  7,
                                                                                8,  7,  0,  2,  0,  7,  -1},
                                           {3,  7,  10, 3,  10, 2,  7,  4,  10, 1,  10,
                                                                                        0,  4,  0,  10, -1},
                                           {1,  10, 2,  8,  7,  4,  -1, -1, -1, -1, -1, -1,
                                                                                            -1, -1, -1, -1},
                                           {4,  9,  1,  4,  1,  7,  7,  1,  3,  -1, -1, -1, -1,
                                                                                                -1, -1, -1},
                                           {4,  9,  1,  4,  1,  7,  0,  8,  1,  8,  7,  1,  -1, -1, -1,
                                                                                                        -1},
                                           {4,  0,  3,  7,  4,  3,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {4,  8,  7,
                                                        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {9,  10,
                                                    8,  10, 11, 8,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {3,  0,
                                                    9,  3,  9,  11, 11, 9,  10, -1, -1, -1, -1, -1, -1, -1},
                                           {0,  1,
                                                    10, 0,  10, 8,  8,  10, 11, -1, -1, -1, -1, -1, -1, -1},
                                           {3,  1,
                                                    10, 11, 3,  10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {1,  2,
                                                    11, 1,  11, 9,  9,  11, 8,  -1, -1, -1, -1, -1, -1, -1},
                                           {3,  0,  9,
                                                        3,  9,  11, 1,  2,  9,  2,  11, 9,  -1, -1, -1, -1},
                                           {0,  2,  11, 8,  0,
                                                                11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {3,  2,  11, -1,
                                                            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {2,  3,  8,  2,
                                                            8,  10, 10, 8,  9,  -1, -1, -1, -1, -1, -1, -1},
                                           {9,  10, 2,  0,  9,
                                                                2,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {2,  3,  8,  2,  8,
                                                                10, 0,  1,  8,  1,  10, 8,  -1, -1, -1, -1},
                                           {1,  10, 2,  -1, -1, -1,
                                                                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {1,  3,  8,  9,  1,  8,
                                                                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {0,  9,  1,  -1, -1, -1,
                                                                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {0,  3,  8,  -1, -1, -1,
                                                                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                           {-1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};
