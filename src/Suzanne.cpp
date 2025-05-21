
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ==== VARIÁVEIS GLOBAIS ====
GLFWwindow* window = nullptr;
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// ==== SHADER (BASIC) ====
const char* vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragColor;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    fragColor = aColor;
}
)glsl";

const char* fragmentShaderSource = R"glsl(
#version 330 core
in vec3 fragColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(fragColor, 1.0);
}
)glsl";

GLuint compileShader()
{
    int success;
    char infoLog[512];

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex Shader Error:\n" << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment Shader Error:\n" << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

int loadSimpleOBJ(const std::string& path, GLuint& vao, int& vertexCount)
{
    std::vector<glm::vec3> vertices;
    std::vector<GLfloat> vBuffer;
    glm::vec3 color(1.0f, 0.5f, 0.2f);

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Erro abrindo o arquivo: " << path << std::endl;
        return -1;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string word;
        ss >> word;

        if (word == "v") {
            glm::vec3 v;
            ss >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        } else if (word == "f") {
            for (int i = 0; i < 3; ++i) {
                ss >> word;
                int idx = std::stoi(word) - 1;
                vBuffer.push_back(vertices[idx].x);
                vBuffer.push_back(vertices[idx].y);
                vBuffer.push_back(vertices[idx].z);
                vBuffer.push_back(color.r);
                vBuffer.push_back(color.g);
                vBuffer.push_back(color.b);
            }
        }
    }
    file.close();

    GLuint vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    vertexCount = vBuffer.size() / 6;
    return 0;
}

void processInput(glm::vec3& pos, glm::vec3& rot, glm::vec3& scale)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) pos.y += 0.01f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) pos.y -= 0.01f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) pos.x -= 0.01f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) pos.x += 0.01f;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) rot.y += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) scale += glm::vec3(0.01f);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) scale -= glm::vec3(0.01f);
}

int main()
{
    // Inicializa GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Suzanne Viewer", NULL, NULL);
    if (!window) {
        std::cerr << "Erro ao criar janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Erro ao inicializar GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    GLuint shader = compileShader();

    GLuint vao;
    int vertexCount;
    if (loadSimpleOBJ("/CGCCHibrido/assets/Modelos3D/Suzanne.obj", vao, vertexCount) < 0) return -1;

    glm::vec3 position(0.0f), rotation(0.0f), scale(1.0f);

    while (!glfwWindowShouldClose(window))
    {
        processInput(position, rotation, scale);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0, 1, 0));
        model = glm::scale(model, scale);

        glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);

        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
