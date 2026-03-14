#include "3DViewer.h"
#include <iostream>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <glm/gtc/type_ptr.hpp> 

C3DViewer::C3DViewer() {}

C3DViewer::~C3DViewer() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_shaderProgram) glDeleteProgram(m_shaderProgram);
    if (m_window) glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool C3DViewer::setup() {
    if (!glfwInit()) return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    m_window = glfwCreateWindow(width, height, "Proyecto 2 - Grafica", NULL, NULL);
    if (!m_window) { glfwTerminate(); return false; }
    glfwMakeContextCurrent(m_window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return false;
    // Configuración Inicial
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LINE_SMOOTH);
    // ImGui Setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* w, int width, int height) {
        auto p = (C3DViewer*)glfwGetWindowUserPointer(w); if (p) p->resize(width, height);
        });
    if (!setupShader()) return false;
    // Cargar Modelo
    //if (!loadOBJ("Blender_2.obj")) std::cout << "Error cargando OBJ." << std::endl;
    // Callbacks
    glfwSetKeyCallback(m_window, keyCallbackStatic);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallbackStatic);
    glfwSetCursorPosCallback(m_window, cursorPosCallbackStatic);
    return true;
}

void C3DViewer::update() {
    // Grados por segundo
    float cameraSpeed = 2.5f * 0.016f;
    float rotateSpeed = 100.0f * 0.016f; 
    // MOVERSE ADELANTE/ATRAS 
    if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS)
        m_cameraPos += cameraSpeed * m_cameraFront;
    if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS)
        m_cameraPos -= cameraSpeed * m_cameraFront;
    // ROTAR VISTA (LEFT/RIGHT) 
    if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS)
        m_cameraYaw -= rotateSpeed;
    if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        m_cameraYaw += rotateSpeed;
    // Recalcular vector Front
    glm::vec3 front;
    front.x = cos(glm::radians(m_cameraYaw)) * cos(glm::radians(m_cameraPitch));
    front.y = sin(glm::radians(m_cameraPitch));
    front.z = sin(glm::radians(m_cameraYaw)) * cos(glm::radians(m_cameraPitch));
    m_cameraFront = glm::normalize(front);
}

void C3DViewer::mainLoop() {
    while (!glfwWindowShouldClose(m_window)) {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - m_lastFrame;
        m_lastFrame = currentFrame;
        glfwPollEvents();
        // Procesa movimiento cámara continuo
        update(); 
        // Render
        render();
        glfwSwapBuffers(m_window);
    }
}

void C3DViewer::onKey(int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

void C3DViewer::onMouseButton(int button, int action, int mods) {
    // Si ImGui está usando el ratón, ignoramos la lógica 3D
    if (ImGui::GetIO().WantCaptureMouse) return;
    if (action == GLFW_PRESS) {
        double x, y;
        glfwGetCursorPos(m_window, &x, &y);
        lastMouseX = x; lastMouseY = y;
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            bool forceRotation = (glfwGetKey(m_window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS);
            int picked = pickObject(x, y);
            if (forceRotation) {
                m_selectedSubMeshIndex = -1;
                m_showBoundingBox = false;
                isDragging = true;
            }
            else {
                int picked = pickObject(x, y);
                if (picked != -1) {
                    m_selectedSubMeshIndex = picked;
                    m_showBoundingBox = true;
                    isDragging = false;
                }
                else {
                    m_selectedSubMeshIndex = -1;
                    m_showBoundingBox = false;
                    isDragging = true;
                }
            }
        }
    }
    else if (action == GLFW_RELEASE) {
        isDragging = false;
    }
}

void C3DViewer::onCursorPos(double xpos, double ypos) {
    if (isDragging) {
        float dx = (float)(xpos - lastMouseX);
        float dy = (float)(ypos - lastMouseY);
        float sensitivity = 0.005f;
        glm::quat rotY = glm::angleAxis(dx * sensitivity, glm::vec3(0, 1, 0));
        glm::quat rotX = glm::angleAxis(dy * sensitivity, glm::vec3(1, 0, 0));
        m_globalRotation = rotY * rotX * m_globalRotation;
        m_globalRotation = glm::normalize(m_globalRotation);
    }
    else if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        float xoffset = (float)(xpos - lastMouseX);
        float yoffset = (float)(lastMouseY - ypos);
        float sensitivity = 0.1f;
        m_cameraYaw += xoffset * sensitivity;
        m_cameraPitch += yoffset * sensitivity;
        if (m_cameraPitch > 89.0f) m_cameraPitch = 89.0f;
        if (m_cameraPitch < -89.0f) m_cameraPitch = -89.0f;
        glm::vec3 front;
        front.x = cos(glm::radians(m_cameraYaw)) * cos(glm::radians(m_cameraPitch));
        front.y = sin(glm::radians(m_cameraPitch));
        front.z = sin(glm::radians(m_cameraYaw)) * cos(glm::radians(m_cameraPitch));
        m_cameraFront = glm::normalize(front);
    }
    lastMouseX = xpos;
    lastMouseY = ypos;
}

bool C3DViewer::loadOBJ(const std::string& filename) {
    std::string modelsDir = "objetos3D/";
    std::string fullPath = modelsDir + filename;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    std::string baseDir = fullPath.substr(0, fullPath.find_last_of("/\\") + 1);
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, fullPath.c_str(), baseDir.c_str());
    if (!warn.empty()) std::cout << "OBJ Warning: " << warn << std::endl;
    if (!err.empty()) std::cerr << "OBJ Error: " << err << std::endl;
    if (!ret) return false;
        if (materials.empty()) {
            std::cout << "[AVISO] No se encontró MTL. Se usará gris por defecto." << std::endl;
        }
    m_vertices.clear();
    m_subMeshes.clear();
    for (const auto& shape : shapes) {
        SubMesh subMesh;
        subMesh.name = shape.name;
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex;
            //Lectura de Posicion, Normal, TexCoords, etc
            vertex.Position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            // Actualizar limites
            subMesh.min = glm::min(subMesh.min, vertex.Position);
            subMesh.max = glm::max(subMesh.max, vertex.Position);
        }
            if (!shape.mesh.material_ids.empty() && shape.mesh.material_ids[0] >= 0) {
                int matId = shape.mesh.material_ids[0];
                if (matId < materials.size()) {
                    subMesh.diffuseColor = glm::vec3(
                        materials[matId].diffuse[0],
                        materials[matId].diffuse[1],
                        materials[matId].diffuse[2]
                    );
                }
            }
            else {
                // Gris default
                subMesh.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f); 
            }
        // Aplanado de vértices 
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex;
            // Posición
            vertex.Position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            // Normales
            if (index.normal_index >= 0) {
                vertex.Normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
            }
            else {
                vertex.Normal = glm::vec3(0.0f);
            }
            if (index.texcoord_index >= 0) {
                vertex.TexCoords = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    attrib.texcoords[2 * index.texcoord_index + 1]
                };
            }
            else {
                vertex.TexCoords = glm::vec2(0.0f);
            }
            m_vertices.push_back(vertex);
            subMesh.indices.push_back(m_vertices.size() - 1);
        }
        subMesh.indexCount = subMesh.indices.size();
        m_subMeshes.push_back(subMesh);
    }
    calculateBoundingBox();
    computeNormals();
    setupMeshBuffers();
    updateNormalBuffers();
    resetView();
    return true;
}

void C3DViewer::calculateBoundingBox() {
    if (m_vertices.empty()) return;
    glm::vec3 minV(1e9), maxV(-1e9);
    for (const auto& v : m_vertices) {
        minV = glm::min(minV, v.Position);
        maxV = glm::max(maxV, v.Position);
    }
    m_center = (minV + maxV) * 0.5f; 
    glm::vec3 size = maxV - minV;
    // Escala para cubo unitario
    float maxDim = std::max({ size.x, size.y, size.z });
    m_scaleFactor = (maxDim > 0) ? (2.0f / maxDim) : 1.0f;
    std::cout << "Modelo Normalizado. Escala: " << m_scaleFactor << std::endl;
    m_boundingBoxDiagonal = glm::length(maxV - minV);
    // Si m_boundingBoxDiagonal es 0 (punto), evitar errores
    if (m_boundingBoxDiagonal < 0.0001f) m_boundingBoxDiagonal = 1.0f;
}

void C3DViewer::computeNormals() {
    if (m_vertices.empty()) return;
    for (size_t i = 0; i < m_vertices.size(); i += 3) {
        // Verificar que no nos salimos del vector 
        if (i + 2 >= m_vertices.size()) break;
        Vertex& v0 = m_vertices[i];
        Vertex& v1 = m_vertices[i + 1];
        Vertex& v2 = m_vertices[i + 2];
        // Si la normal ya existe. Si es cero, calculamos.
        if (glm::length(v0.Normal) < 0.01f) {
            glm::vec3 edge1 = v1.Position - v0.Position;
            glm::vec3 edge2 = v2.Position - v0.Position;
            glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
            v0.Normal = normal;
            v1.Normal = normal;
            v2.Normal = normal;
        }
    }
}

void C3DViewer::setupMeshBuffers() {
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), m_vertices.data(), GL_STATIC_DRAW);
    // Location 0: Posición
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
    glEnableVertexAttribArray(0);
    // Location 1: Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

int C3DViewer::pickObject(double mouseX, double mouseY) {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(m_shaderProgram);
    glm::mat4 view = glm::lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 100.0f);
    glm::mat4 globalModel = glm::mat4(1.0f);
    globalModel = glm::translate(globalModel, m_globalPos);
    globalModel = globalModel * glm::toMat4(m_globalRotation);
    globalModel = glm::scale(globalModel, m_globalScale * m_scaleFactor);
    globalModel = glm::translate(globalModel, -m_center);
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1i(glGetUniformLocation(m_shaderProgram, "isPicking"), 1);
    unsigned int offset = 0;
    for (int i = 0; i < m_subMeshes.size(); i++) {
        SubMesh& sub = m_subMeshes[i];
        if (!sub.visible) { offset += sub.indices.size(); continue; }
        glm::mat4 localModel = glm::translate(globalModel, sub.localPosition);
        glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(localModel));
        // Color ID
        float r = (float)i / 255.0f;
        glUniform3f(glGetUniformLocation(m_shaderProgram, "uColor"), r, 0.0f, 0.0f);
        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLES, offset, sub.indices.size());
        offset += sub.indices.size();
    }
    glUniform1i(glGetUniformLocation(m_shaderProgram, "isPicking"), 0);
    unsigned char data[4];
    glReadPixels((int)mouseX, height - (int)mouseY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
    int id = (int)data[0];
    if (id == 255 || id >= m_subMeshes.size()) return -1;
    return id;
}

void C3DViewer::render() {
    // Configuración de Estados
    glClearColor(m_bgColor.r, m_bgColor.g, m_bgColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (m_enableZBuffer) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (m_enableCulling) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (m_enableAntiAliasing) {
		// Ahora si esta el Blending para suavizar líneas
        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else {
        glDisable(GL_LINE_SMOOTH);
        glDisable(GL_BLEND);
    }
    glUseProgram(m_shaderProgram);
    // Matrices
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_cameraUp);
    // Matriz Global del Objeto
    glm::mat4 globalModel = glm::mat4(1.0f);
    globalModel = glm::translate(globalModel, m_globalPos);
    globalModel = globalModel * glm::toMat4(m_globalRotation);
    globalModel = glm::scale(globalModel, m_globalScale * m_scaleFactor);
    globalModel = glm::translate(globalModel, -m_center);
    // Uniforms básicos
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1i(glGetUniformLocation(m_shaderProgram, "isPicking"), 0);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "useFlatColor"), 0);
    //glBindVertexArray(m_vao);
    unsigned int offset = 0;
    // SubMesh
    for (int i = 0; i < m_subMeshes.size(); i++) {
        glBindVertexArray(m_vao);
        SubMesh& sub = m_subMeshes[i];
        if (!sub.visible) { offset += sub.indices.size(); continue; }
        glm::mat4 localModel = globalModel;
        localModel = glm::translate(localModel, sub.localPosition);
        glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(localModel));
        // Color
        glm::vec3 color = sub.diffuseColor;
        // Resaltar selección 
        //if (i == m_selectedSubMeshIndex && !m_showWireframe) color = glm::vec3(1.0f, 0.2f, 0.2f);
        glUniform3fv(glGetUniformLocation(m_shaderProgram, "uColor"), 1, glm::value_ptr(color));
        // Dibujar Relleno
        if (m_showTriangles) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawArrays(GL_TRIANGLES, offset, sub.indices.size());
        }
        if (m_showWireframe) {
            glUniform1i(glGetUniformLocation(m_shaderProgram, "useFlatColor"), 1);
            glUniform3fv(glGetUniformLocation(m_shaderProgram, "uColor"), 1, glm::value_ptr(m_wireframeColor));
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonOffset(-1.0, -1.0); 
                glDrawArrays(GL_TRIANGLES, offset, sub.indices.size());
            glDisable(GL_POLYGON_OFFSET_LINE);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "useFlatColor"), 0);
        }
        if (m_showVertices) {
            glUniform1i(glGetUniformLocation(m_shaderProgram, "useFlatColor"), 1);
            glUniform3fv(glGetUniformLocation(m_shaderProgram, "uColor"), 1, glm::value_ptr(m_vertexColor));
            glPointSize(m_pointSize);
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            glEnable(GL_POLYGON_OFFSET_POINT);
            glPolygonOffset(-1.0, -1.0);
            glDrawArrays(GL_POINTS, offset, sub.indices.size());
            glDisable(GL_POLYGON_OFFSET_POINT);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "useFlatColor"), 0);
        }
        if (m_showNormals) {
            if (m_vao_normals == 0) updateNormalBuffers();
            glUniform1i(glGetUniformLocation(m_shaderProgram, "useFlatColor"), 1);
            glUniform3fv(glGetUniformLocation(m_shaderProgram, "uColor"), 1, glm::value_ptr(m_normalsColor));
            glBindVertexArray(m_vao_normals);
            // Dibujamos solo las normales de este sub-mallado
            glDrawArrays(GL_LINES, offset * 2, sub.indices.size() * 2);
            glBindVertexArray(m_vao); // Restaurar vao principal
            glUniform1i(glGetUniformLocation(m_shaderProgram, "useFlatColor"), 0);
        }
        if (i == m_selectedSubMeshIndex && m_showBoundingBox) {
            drawBoundingBox(sub.min, sub.max, localModel, view, projection, m_boundingBoxColor);
        }
        offset += sub.indices.size();
    }
    glBindVertexArray(0);
    drawInterface();
}
void C3DViewer::drawInterface() {
    // Inicio de Frame ImGui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    // Configuración de la Ventana Principal
    ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin("Panel de Control - Proyecto 2 UCV");
    if (ImGui::CollapsingHeader("Cargar Modelo", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputText("Archivo (.obj)", m_objFileName, sizeof(m_objFileName));
        // Botón de Cargar
        if (ImGui::Button("CARGAR OBJETO")) {
            if (loadOBJ(m_objFileName)) {
                std::cout << "Carga exitosa: " << m_objFileName << std::endl;
            }
            else {
                std::cerr << "Error al cargar: " << m_objFileName << std::endl;
            }
        }
        // Mensaje de estado
        if (m_subMeshes.empty()) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Estado: Esperando carga...");
        }
        else {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Estado: Modelo cargado (%d partes)", (int)m_subMeshes.size());
        }
    }
    ImGui::Separator();
    //  SISTEMA Y ESTADÍSTICAS 
    ImGui::Text("Rendimiento: %.1f FPS", ImGui::GetIO().Framerate);
    ImGui::ColorEdit3("Color de Fondo", glm::value_ptr(m_bgColor));
    ImGui::Separator();
    // TRANSFORMACIONES GLOBALES 
    if (ImGui::CollapsingHeader("Transformacion Global", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat3("Posicion Mundo", glm::value_ptr(m_globalPos), 0.05f);
        ImGui::DragFloat3("Escala (XYZ)", glm::value_ptr(m_globalScale), 0.01f, 0.01f, 100.0f);
        if (ImGui::Button("CENTRAR VISTA Y OBJETO")) {
            resetView();
        }
        if (ImGui::Button("Centrar Objeto en (0,0,-3)")) {
            m_globalPos = glm::vec3(0.0f, 0.0f, -3.0f);
            m_globalRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            m_globalScale = glm::vec3(1.0f);
        }
        ImGui::Separator();
        static char exportName[128] = "modelo_exportado.obj";
        ImGui::InputText("Archivo de Salida", exportName, sizeof(exportName));
        if (ImGui::Button("Exportar OBJ")) {
            exportOBJ(exportName);
        }
        ImGui::Separator();
    }
    // OPCIONES DE RENDERIZADO GLOBAL 
    if (ImGui::CollapsingHeader("Opciones de Visualizacion")) {
        // Relleno y Optimizaciones
        ImGui::Text("Renderizado Base:");
        ImGui::Checkbox("Relleno (Fill)", &m_showTriangles); 
        ImGui::SameLine();
        ImGui::Checkbox("Z-Buffer", &m_enableZBuffer); 
        ImGui::Checkbox("Back-Face Culling", &m_enableCulling); 
        ImGui::SameLine();
        ImGui::Checkbox("Antialiasing", &m_enableAntiAliasing); 
        ImGui::Separator();
        ImGui::Checkbox("Mostrar Wireframe", &m_showWireframe);
        if (m_showWireframe) {
            ImGui::Indent();
            ImGui::ColorEdit3("Color Lineas", glm::value_ptr(m_wireframeColor));
            ImGui::Unindent();
        }
        ImGui::Separator();
        ImGui::Checkbox("Mostrar Vertices", &m_showVertices);
        if (m_showVertices) {
            ImGui::Indent();
            ImGui::ColorEdit3("Color Puntos", glm::value_ptr(m_vertexColor));
            ImGui::SliderFloat("Tamanno Puntos", &m_pointSize, 1.0f, 20.0f);
            ImGui::Unindent();
        }
        ImGui::Separator();
        ImGui::Checkbox("Mostrar Normales", &m_showNormals);
        if (m_showNormals) {
            ImGui::Indent();
            ImGui::ColorEdit3("Color Normales", glm::value_ptr(m_normalsColor));
            // Slider para longitud 
            if (ImGui::SliderFloat("Largo (%)", &m_normalLengthPercent, 0.01f, 0.5f, "%.2f")) {
                updateNormalBuffers(); 
            }
            ImGui::Unindent();
        }
    }
    // EDICIÓN DE SUB-MALLADO
    if (ImGui::CollapsingHeader("Edicion Sub-Mallado (Picking)", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (m_selectedSubMeshIndex != -1 && m_selectedSubMeshIndex < m_subMeshes.size()) {
            SubMesh& sub = m_subMeshes[m_selectedSubMeshIndex];
            // Mostrar nombre e ID
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "SELECCIONADO: %s (ID: %d)", sub.name.c_str(), m_selectedSubMeshIndex);
            ImGui::ColorEdit3("Material (Kd)", glm::value_ptr(sub.diffuseColor));
            ImGui::DragFloat3("Posicion Local", glm::value_ptr(sub.localPosition), 0.05f);
            ImGui::Checkbox("Ver Bounding Box", &m_showBoundingBox);
            if (m_showBoundingBox) {
                ImGui::SameLine();
                ImGui::ColorEdit3("Color BB", glm::value_ptr(m_boundingBoxColor), ImGuiColorEditFlags_NoInputs);
            }
            ImGui::Separator();
            // Botón rojo para indicar acción destructiva
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
            if (ImGui::Button("ELIMINAR SUB-MALLADO", ImVec2(-1, 0))) { 
                sub.visible = false;         
                m_selectedSubMeshIndex = -1;
            }
            ImGui::PopStyleColor(3);
        }
        else {
            ImGui::TextWrapped("Haz Clic Izquierdo sobre el objeto 3D para seleccionar una parte y editarla.");
        }
    }
    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void C3DViewer::resize(int new_width, int new_height) {
    width = new_width;
    height = new_height;
    glViewport(0, 0, width, height);
}

bool C3DViewer::setupShader() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSrc, nullptr);
    glCompileShader(vertexShader);
    if (!checkCompileErrors(vertexShader, "VERTEX")) return false;
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSrc, nullptr);
    glCompileShader(fragmentShader);
    if (!checkCompileErrors(fragmentShader, "FRAGMENT")) return false;
    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);
    if (!checkCompileErrors(m_shaderProgram, "PROGRAM")) return false;
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return true;
}

bool C3DViewer::checkCompileErrors(GLuint shader, const char* type) {
    GLint success;
    GLchar infoLog[1024];
    if (strcmp(type, "PROGRAM") != 0) {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            fprintf(stderr, "ERROR::SHADER_COMPILATION_ERROR of type: %s\n%s\n", type, infoLog);
            return false;
        }
    }
    else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            fprintf(stderr, "ERROR::PROGRAM_LINKING_ERROR of type: %s\n%s\n", type, infoLog);
            return false;
        }
    }
    return true;
}

// Callbacks 
void C3DViewer::keyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods) {
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    C3DViewer* self = (C3DViewer*)glfwGetWindowUserPointer(window);
    if (self) self->onKey(key, scancode, action, mods);
}

void C3DViewer::mouseButtonCallbackStatic(GLFWwindow* window, int button, int action, int mods) {
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    C3DViewer* self = (C3DViewer*)glfwGetWindowUserPointer(window);
    if (self) self->onMouseButton(button, action, mods);
}

void C3DViewer::cursorPosCallbackStatic(GLFWwindow* window, double xpos, double ypos) {
    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
    C3DViewer* self = (C3DViewer*)glfwGetWindowUserPointer(window);
    if (self) self->onCursorPos(xpos, ypos);
}

void C3DViewer::setupBBoxBuffer() {
    float vertices[] = {
        // Cubo unitario (-0.5 a 0.5)
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,

        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,

        -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f
    };
    glGenVertexArrays(1, &m_vao_bbox);
    glGenBuffers(1, &m_vbo_bbox);
    glBindVertexArray(m_vao_bbox);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo_bbox);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

// Implementación de la función de dibujo
void C3DViewer::drawBoundingBox(const glm::vec3& min, const glm::vec3& max, const glm::mat4& parentModel, const glm::mat4& view, const glm::mat4& proj, glm::vec3 color) {
    if (m_vao_bbox == 0) setupBBoxBuffer();
    glUniform1i(glGetUniformLocation(m_shaderProgram, "useFlatColor"), 1);
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "uColor"), 1, glm::value_ptr(color));
    glm::vec3 size = max - min;
    glm::vec3 center = (min + max) * 0.5f;
    glm::mat4 bboxModel = parentModel;
    bboxModel = glm::translate(bboxModel, center);
    bboxModel = glm::scale(bboxModel, size * 1.005f);
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(bboxModel));
    glBindVertexArray(m_vao_bbox);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, 24);
    glLineWidth(1.0f);
    glBindVertexArray(0);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "useFlatColor"), 0);
}

void C3DViewer::updateNormalBuffers() {
    if (m_vertices.empty()) return;
    std::vector<float> lineVertices;
    float len = m_boundingBoxDiagonal * m_normalLengthPercent;
    for (const auto& v : m_vertices) {
        // Punto inicio
        lineVertices.push_back(v.Position.x);
        lineVertices.push_back(v.Position.y);
        lineVertices.push_back(v.Position.z);
        // Punto fin 
        glm::vec3 end = v.Position + v.Normal * len;
        lineVertices.push_back(end.x);
        lineVertices.push_back(end.y);
        lineVertices.push_back(end.z);
    }
    m_normalCount = lineVertices.size() / 3;
    if (m_vao_normals == 0) {
        glGenVertexArrays(1, &m_vao_normals);
        glGenBuffers(1, &m_vbo_normals);
    }
    glBindVertexArray(m_vao_normals);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo_normals);
    glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float), lineVertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void C3DViewer::resetView() {
    // Resetear Cámara 
    m_cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
    m_cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    m_cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    m_cameraYaw = -90.0f;
    m_cameraPitch = 0.0f;
    // Resetear Transformaciones del Objeto
    m_globalPos = glm::vec3(0.0f, 0.0f, -3.0f); 
    m_globalScale = glm::vec3(1.0f);
    m_globalRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); 
    std::cout << "Vista y Objeto centrados." << std::endl;
}

void C3DViewer::exportOBJ(const std::string& filename) {
    std::string mtlFilename = filename.substr(0, filename.find_last_of('.')) + ".mtl";
    std::string mtlNameOnly = mtlFilename.substr(mtlFilename.find_last_of("/\\") + 1);
    std::ofstream outObj(filename);
    std::ofstream outMtl(mtlFilename);
    if (!outObj.is_open() || !outMtl.is_open()) return;
    outObj << "# Exportado por C3DViewer\n";
    outObj << "mtllib " << mtlNameOnly << "\n";
    glm::mat4 globalModel = glm::mat4(1.0f);
    globalModel = glm::translate(globalModel, m_globalPos);
    globalModel = globalModel * glm::toMat4(m_globalRotation);
    globalModel = glm::scale(globalModel, m_globalScale * m_scaleFactor);
    globalModel = glm::translate(globalModel, -m_center);
    int vertexOffset = 1;
    for (const auto& sub : m_subMeshes) {
        if (!sub.visible) continue;
        std::string matName = "Mat_" + sub.name;
        std::replace(matName.begin(), matName.end(), ' ', '_');
        // Escribir Material
        outMtl << "newmtl " << matName << "\n";
        outMtl << "Kd " << sub.diffuseColor.r << " " << sub.diffuseColor.g << " " << sub.diffuseColor.b << "\n";
        outMtl << "Ka 0.1 0.1 0.1\nKs 0.5 0.5 0.5\nNs 32\nd 1.0\nillum 2\n\n";
        outObj << "g " << sub.name << "\n";
        outObj << "usemtl " << matName << "\n";
        // Matrices
        glm::mat4 totalMatrix = glm::translate(globalModel, sub.localPosition);
        // Para normales
        glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(totalMatrix))); 
        // Escribir Vértices y Normales
        for (unsigned int idx : sub.indices) {
            Vertex v = m_vertices[idx];
            // Posición
            glm::vec4 pos = totalMatrix * glm::vec4(v.Position, 1.0f);
            outObj << "v " << pos.x << " " << pos.y << " " << pos.z << "\n";
            // Normal
            glm::vec3 norm = glm::normalize(normalMatrix * v.Normal);
            outObj << "vn " << norm.x << " " << norm.y << " " << norm.z << "\n";
        }
        // Escribir Caras
        int count = sub.indices.size();
        for (int i = 0; i < count; i += 3) {
            unsigned int i1 = vertexOffset + i;
            unsigned int i2 = vertexOffset + i + 1;
            unsigned int i3 = vertexOffset + i + 2;
            outObj << "f " << i1 << "//" << i1 << " " << i2 << "//" << i2 << " " << i3 << "//" << i3 << "\n";
        }
        vertexOffset += count;
    }
    std::cout << "Exportado correctamente con normales." << std::endl;
}
