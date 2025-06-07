#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

GLFWwindow* window = nullptr;
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

const char* vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;
out vec3 Color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    Color = aColor;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)glsl";

const char* fragmentShaderSource = R"glsl(
#version 330 core
struct Light {
    vec3 position;
    vec3 color;
};
#define NUM_LIGHTS 3

in vec3 FragPos;
in vec3 Normal;
in vec3 Color;

out vec4 FragColor;

uniform vec3 viewPos;
uniform Light lights[NUM_LIGHTS];

void main()
{
    vec3 ambient = vec3(0.1) * Color;
    vec3 result = ambient;

    for (int i = 0; i < NUM_LIGHTS; ++i) {
        vec3 lightDir = normalize(lights[i].position - FragPos);
        float diff = max(dot(Normal, lightDir), 0.0);
        vec3 diffuse = diff * lights[i].color * Color;

        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, Normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = spec * lights[i].color;

        result += diffuse + specular;
    }

    FragColor = vec4(result, 1.0);
}
)glsl";

GLuint compileShader() {
    int success;
    char infoLog[512];

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cerr << "Vertex Shader Error:\n" << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cerr << "Fragment Shader Error:\n" << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

int loadSimpleOBJ(const string& path, GLuint& vao, int& vertexCount) {
    vector<glm::vec3> vertices;
    vector<GLfloat> vBuffer;
    glm::vec3 color(1.0f, 0.5f, 0.2f);

    ifstream file(path);
    if (!file.is_open()) {
        cerr << "Erro abrindo o arquivo: " << path << endl;
        return -1;
    }

    string line;
    while (getline(file, line)) {
        istringstream ss(line);
        string word;
        ss >> word;

        if (word == "v") {
            glm::vec3 v;
            ss >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        } else if (word == "f") {
            vector<string> faceIndices(3);
            ss >> faceIndices[0] >> faceIndices[1] >> faceIndices[2];

            glm::vec3 v0 = vertices[stoi(faceIndices[0]) - 1];
            glm::vec3 v1 = vertices[stoi(faceIndices[1]) - 1];
            glm::vec3 v2 = vertices[stoi(faceIndices[2]) - 1];

            glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

            for (int i = 0; i < 3; ++i) {
                int idx = stoi(faceIndices[i]) - 1;
                glm::vec3 v = vertices[idx];
                vBuffer.push_back(v.x);
                vBuffer.push_back(v.y);
                vBuffer.push_back(v.z);
                vBuffer.push_back(color.r);
                vBuffer.push_back(color.g);
                vBuffer.push_back(color.b);
                vBuffer.push_back(normal.x);
                vBuffer.push_back(normal.y);
                vBuffer.push_back(normal.z);
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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    vertexCount = vBuffer.size() / 9;
    return 0;
}

float angleX = 0.0f;
float angleY = 0.0f;
void processInput() {
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) angleY -= 0.2f;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) angleY += 0.2f;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) angleX -= 0.2f;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) angleX += 0.2f;
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Suzanne Viewer", NULL, NULL);
    if (!window) {
        cerr << "Erro ao criar janela GLFW" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cerr << "Erro ao inicializar GLAD" << endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    GLuint shader = compileShader();

    GLuint vao;
    int vertexCount;
    if (loadSimpleOBJ("C:/Users/Kamar/OneDrive/Documentos/CGCCHibrido/assets/Modelos3D/Suzanne.obj", vao, vertexCount) < 0) return -1;

    glm::vec3 cameraPos(0.0f, 0.0f, 5.0f);

    glm::vec3 lightPositions[3] = {
        glm::vec3(5.0f, 5.0f, 5.0f),
        glm::vec3(-5.0f, 2.0f, 2.0f),
        glm::vec3(0.0f, 5.0f, -5.0f)
    };
    glm::vec3 lightColors[3] = {
        glm::vec3(1.0f),
        glm::vec3(0.3f),
        glm::vec3(0.5f, 0.5f, 1.0f)
    };

    while (!glfwWindowShouldClose(window)) {
        processInput();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0), glm::vec3(0, 1, 0));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(shader, "viewPos"), 1, glm::value_ptr(cameraPos));

        for (int i = 0; i < 3; ++i) {
            string base = "lights[" + to_string(i) + "]";
            glUniform3fv(glGetUniformLocation(shader, (base + ".position").c_str()), 1, glm::value_ptr(lightPositions[i]));
            glUniform3fv(glGetUniformLocation(shader, (base + ".color").c_str()), 1, glm::value_ptr(lightColors[i]));
        }

        glm::mat4 baseModel = glm::rotate(glm::mat4(1.0f), glm::radians(angleY), glm::vec3(0, 1, 0));
        baseModel = glm::rotate(baseModel, glm::radians(angleX), glm::vec3(1, 0, 0));

        glBindVertexArray(vao);

        // Primeiro objeto
        glm::mat4 model1 = glm::translate(baseModel, glm::vec3(-1.5f, 0.0f, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model1));
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);

        // Segundo objeto invertido
        glm::mat4 model2 = glm::translate(baseModel, glm::vec3(1.5f, 0.0f, 0.0f));
        model2 = glm::scale(model2, glm::vec3(-1, 1, 1));
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model2));
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);

        glBindVertexArray(0);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
