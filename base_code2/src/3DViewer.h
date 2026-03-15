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
    glm::vec3 Tangent;
};

struct SubMesh {
    std::string name;
    std::vector<unsigned int> indices;
    unsigned int indexCount = 0;
    glm::vec3 diffuseColor = glm::vec3(0.7f);
    // TRANSFORMACIONES LOCALES
    glm::vec3 initialPosition = glm::vec3(0.0f);
    glm::vec3 localPosition = glm::vec3(0.0f);
    glm::vec3 localScale = glm::vec3(1.0f);
    glm::quat localRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    int animType = 0;
    bool visible = true;
    glm::vec3 min = glm::vec3(FLT_MAX);
    glm::vec3 max = glm::vec3(-FLT_MAX);
    GLuint diffuseMap = 0;
    GLuint specularMap = 0;
    GLuint ambientMap = 0;
    //Bump
    GLuint bumpMap = 0;
    bool isReflective = false;
    int uvMappingMode = 0; // 0: OBJ Original, 1: S-Mapping (Esférico), 2: O-Mapping (Cilíndrico)
};

class C3DViewer {
public:
    C3DViewer();
    virtual ~C3DViewer();
    bool setup();
    void mainLoop();
    void exportOBJ(const std::string& filename);
    void resetView();
    void createParametricTable();
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
    bool loadOBJ(const std::string& path, bool clearScene = true, glm::vec3 pos = glm::vec3(0.0f), int animType = 1);
    void loadSceneFromString(const std::string& objList);
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
    GLuint loadTexture(const char* path);
    void computeTangents();
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
    // Iluminacion
    struct Light {
        bool enabled = true;
        glm::vec3 position;
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        int shadingModel = 0; // 0 = Phong, 1 = Blinn-Phong, 2 = Flat Shading
        // Datos para su trayectoria
        float radius;
        float speedOffset;
        float height;
    };
    Light m_lights[3];
    float m_lightAnimSpeed = 1.0f;
    bool m_enableAttenuation = true;
    // Buffer para dibujar las esferitas de luz
    GLuint m_vao_sphere = 0, m_vbo_sphere = 0, m_ebo_sphere = 0;
    int m_sphereIndexCount = 0;
    void setupSphereBuffer();
    void drawLightSphere(const glm::vec3& pos, const glm::vec3& color);
    char m_texDiffuseInput[128] = "madera_difusa.jpg";
    char m_texBumpInput[128] = "madera_bump.jpg";
    // Variables del Skybox 
    GLuint m_skyboxVAO = 0, m_skyboxVBO = 0;
    GLuint m_cubemapTexture = 0;
    GLuint m_skyboxShader = 0;
    void setupSkybox();
    GLuint loadCubemap(std::vector<std::string> faces);
    // Shaders Sources
    const char* vertexShaderSrc = R"glsl(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec2 aTexCoords;
        layout(location = 3) in vec3 aTangent;
        uniform mat4 model; uniform mat4 view; uniform mat4 projection;
        out vec3 vFragPos; out vec2 vTexCoords; out vec3 vNormal; out mat3 vTBN; 
        out vec3 vLocalPos; // Para el mapeo Esférico/Cilíndrico
        void main() {
            vLocalPos = aPos; 
            vFragPos = vec3(model * vec4(aPos, 1.0));
            vTexCoords = aTexCoords;
            mat3 normalMatrix = mat3(transpose(inverse(model)));
            vec3 T = normalize(normalMatrix * aTangent);
            vec3 N = normalize(normalMatrix * aNormal);
            T = normalize(T - dot(T, N) * N); 
            vec3 B = cross(T, N); 
            vTBN = mat3(T, B, N); vNormal = N;
            gl_Position = projection * view * vec4(vFragPos, 1.0);
        }
    )glsl";
    const char* fragmentShaderSrc = R"glsl(
        #version 330 core
        in vec3 vFragPos; in vec2 vTexCoords; in vec3 vNormal; in mat3 vTBN; in vec3 vLocalPos;
        struct Light { 
            bool enabled; 
            vec3 position; 
            vec3 ambient; 
            vec3 diffuse; 
            vec3 specular; 
            int shadingModel; 
        };
        uniform Light lights[3]; 
        uniform vec3 viewPos; 
        uniform vec3 uColor; 
        uniform bool useFlatColor; 
        uniform bool useAttenuation; 
        uniform bool isPicking;
        uniform sampler2D mapDiffuse; 
        uniform sampler2D mapSpecular; 
        uniform sampler2D mapAmbient; 
        uniform sampler2D mapBump;
        uniform samplerCube skybox; // El mapa del entorno para el reflejo
        uniform bool hasMapDiffuse; uniform bool hasMapSpecular; 
        uniform bool hasMapAmbient; uniform bool hasMapBump;
        uniform int uvMappingMode; // 0=OBJ, 1=Esferico, 2=Cilindrico
        uniform bool isReflective; // True = Bronce brillante
        out vec4 FragColor;
        void main() {
            if (isPicking || useFlatColor) { FragColor = vec4(uColor, 1.0); return; }
            vec3 result = vec3(0.0);
            vec3 totalSpecular = vec3(0.0);
            vec3 viewDir = normalize(viewPos - vFragPos);
            vec2 finalUV = vTexCoords;
            if (uvMappingMode == 1) { 
                // S-Mapping 1: Esférico
                vec3 p = normalize(vLocalPos);
                finalUV = vec2(atan(p.z, p.x) / 6.2831853 + 0.5, asin(p.y) / 3.14159265 + 0.5);
            } 
            else if (uvMappingMode == 2) { 
                // S-Mapping 2: Plano XY (Proyección ortogonal)
                finalUV = vLocalPos.xy * 0.5 + 0.5;
            } 
            else if (uvMappingMode == 3) { 
                // O-Mapping 1: Cilíndrico
                vec3 p = normalize(vLocalPos);
                finalUV = vec2(atan(p.z, p.x) / 6.2831853 + 0.5, vLocalPos.y * 0.5 + 0.5);
            } 
            else if (uvMappingMode == 4) { 
                // O-Mapping 2: Mapeo por Normales (El entorno influye en la textura)
                finalUV = normalize(vNormal).xy * 0.5 + 0.5;
            }
            vec3 texDiffuse = hasMapDiffuse ? texture(mapDiffuse, finalUV).rgb : uColor;
            vec3 texSpecular = hasMapSpecular ? texture(mapSpecular, finalUV).rgb : vec3(1.0);
            vec3 texAmbient = hasMapAmbient ? texture(mapAmbient, finalUV).rgb : texDiffuse;
            vec3 norm = normalize(vNormal);
            if (hasMapBump && uvMappingMode == 0) { // Bump solo si usa UVs originales
                norm = normalize(vTBN * normalize(texture(mapBump, finalUV).rgb * 2.0 - 1.0));
            }
            for(int i = 0; i < 3; i++) {
                if(!lights[i].enabled) continue;
                vec3 currentNorm = norm; 
                if (lights[i].shadingModel == 2) { 
                    currentNorm = normalize(cross(dFdx(vFragPos), dFdy(vFragPos)));
                }
                vec3 lightDir = normalize(lights[i].position - vFragPos);
                vec3 ambient = lights[i].ambient * texAmbient;
                // Usar currentNorm aquí en adelante
                float diff = max(dot(currentNorm, lightDir), 0.0);
                vec3 diffuse = lights[i].diffuse * diff * texDiffuse;
                float spec = 0.0;
                if (lights[i].shadingModel == 1) { 
                    spec = pow(max(dot(currentNorm, normalize(lightDir + viewDir)), 0.0), 32.0);
                } else { 
                    spec = pow(max(dot(viewDir, reflect(-lightDir, currentNorm)), 0.0), 32.0);
                }
                vec3 specular = lights[i].specular * spec * texSpecular; 
                float dist = length(lights[i].position - vFragPos);
                float att = useAttenuation ? 1.0 / (1.0 + 0.09 * dist + 0.032 * (dist * dist)) : 1.0;
                result += (ambient + diffuse + specular) * att;
                totalSpecular += specular * att;
            }
            if (isReflective) {
                vec3 R = reflect(-viewDir, normalize(vNormal));
                vec3 reflection = texture(skybox, R).rgb;
                result = (reflection * vec3(1.0, 0.75, 0.4)) + totalSpecular; 
            }
            FragColor = vec4(result, 1.0);
        }
    )glsl";
    // SHADERS DEL SKYBOX
    const char* skyboxVertexShaderSrc = R"glsl(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        out vec3 TexCoords;
        uniform mat4 projection; uniform mat4 view;
        void main() {
            TexCoords = aPos;
            vec4 pos = projection * view * vec4(aPos, 1.0);
            gl_Position = pos.xyww;
        }
    )glsl";
    const char* skyboxFragmentShaderSrc = R"glsl(
        #version 330 core
        out vec4 FragColor;
        in vec3 TexCoords;
        uniform samplerCube skybox;
        void main() {    
            FragColor = texture(skybox, TexCoords);
        }
    )glsl";
};