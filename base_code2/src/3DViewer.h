#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <sstream>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct SubMesh {
    std::string name;
    std::vector<unsigned int> indices;
    unsigned int indexCount = 0;
    glm::vec3 diffuseColor = glm::vec3(0.7f);
    glm::vec3 localPosition = glm::vec3(0.0f);
    bool visible = true;
    glm::vec3 min = glm::vec3(FLT_MAX);
    glm::vec3 max = glm::vec3(-FLT_MAX);
};

class C3DViewer {
public:
    C3DViewer();
    virtual ~C3DViewer();
    bool setup();
    void mainLoop();
    void exportOBJ(const std::string& filename);
    void resetView();
private:
    // Callbacks
    virtual void onKey(int key, int scancode, int action, int mods);
    virtual void onMouseButton(int button, int action, int mods);
    virtual void onCursorPos(double xpos, double ypos);
    virtual void update();
    virtual void render();
    virtual void drawInterface();
    void setupBBoxBuffer();
    // Helpers
    void resize(int new_width, int new_height);
    bool setupShader();
    bool checkCompileErrors(GLuint shader, const char* type);
    bool loadOBJ(const std::string& path);
    void calculateBoundingBox();
    void computeNormals(); 
    void setupMeshBuffers();
    // Picking
    int pickObject(double x, double y); 
    // Dibujo auxiliar
    void drawBoundingBox(const glm::vec3& min, const glm::vec3& max, const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj, glm::vec3 color);
    static void keyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallbackStatic(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallbackStatic(GLFWwindow* window, double xpos, double ypos);
    void updateNormalBuffers();
protected:
    char m_objFileName[128] = "pig.obj";
    int width = 1280;
    int height = 720;
    GLFWwindow* m_window = nullptr;
    // OpenGL handles
    GLuint m_vao = 0, m_vbo = 0;
    GLuint m_shaderProgram = 0;
    // Datos del Modelo
    std::vector<Vertex> m_vertices;
    std::vector<SubMesh> m_subMeshes;
    glm::vec3 m_center = glm::vec3(0.0f);
    float m_scaleFactor = 1.0f;
    // Para longitud de normales
    float m_boundingBoxDiagonal = 1.0f; 
    // Selección y Edición
    int m_selectedSubMeshIndex = -1;
    bool isDragging = false;
    double lastMouseX = 0, lastMouseY = 0;
    // Transformaciones Globales
    glm::vec3 m_globalPos = glm::vec3(0.0f, 0.0f, -3.0f);
    glm::vec3 m_globalScale = glm::vec3(1.0f);
    glm::quat m_globalRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    // Cámara FPS
    glm::vec3 m_cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 m_cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 m_cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    float m_cameraYaw = -90.0f;
    float m_cameraPitch = 0.0f;
    float m_lastFrame = 0.0f;
    bool m_cameraFPSMode = true;
    // Guarda dónde estábamos antes de volar
    glm::vec3 m_savedFPSPos = glm::vec3(0.0f); 
    bool m_isFallingToFPS = false;
    // Opciones de Visualización
    bool m_showWireframe = false;
    bool m_showNormals = false;
    bool m_showBoundingBox = false;
    bool m_enableZBuffer = true;
    bool m_enableCulling = true;
    bool m_enableAntiAliasing = true;
    glm::vec3 m_bgColor = glm::vec3(0.1f, 0.1f, 0.1f);
    glm::vec3 m_wireframeColor = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 m_normalsColor = glm::vec3(1.0f, 1.0f, 0.0f);
    float m_normalLengthPercent = 0.05f;
    GLuint m_vao_bbox = 0, m_vbo_bbox = 0;
    GLuint m_vao_normals = 0, m_vbo_normals = 0;
    int m_normalCount = 0;
    bool m_showTriangles = true; 
    bool m_showVertices = false; 
    float m_pointSize = 3.0f; 
    glm::vec3 m_vertexColor = glm::vec3(1.0f, 1.0f, 1.0f); 
    glm::vec3 m_boundingBoxColor = glm::vec3(1.0f, 0.0f, 1.0f); 
    // Shaders Sources
    const char* vertexShaderSrc = R"glsl(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        out vec3 vNormal;
        out vec3 vFragPos;
        void main() {
            vFragPos = vec3(model * vec4(aPos, 1.0));
            vNormal = mat3(transpose(inverse(model))) * aNormal; 
            gl_Position = projection * view * vec4(vFragPos, 1.0);
        }
    )glsl";
    const char* fragmentShaderSrc = R"glsl(
        #version 330 core
        in vec3 vNormal;
        in vec3 vFragPos;
        uniform vec3 uColor;
        uniform bool isPicking;
        uniform bool useFlatColor; 
        out vec4 FragColor;
        void main() {
            if (isPicking || useFlatColor) {
                FragColor = vec4(uColor, 1.0);
            } else {
                vec3 norm = normalize(vNormal);
                vec3 lightDir = normalize(vec3(0.2, 0.5, 0.8));
                float diff = max(dot(norm, lightDir), 0.3); 
                vec3 diffuse = diff * uColor;
                FragColor = vec4(diffuse, 1.0);
            }
        }
    )glsl";
};