#include "3DViewer.h"
#include <iostream>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
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
    m_window = glfwCreateWindow(width, height, "Proyecto 3 - Grafica", NULL, NULL);
    if (!m_window) { glfwTerminate(); return false; }
    glfwMakeContextCurrent(m_window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return false;
    // Configuración Inicial
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
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
    // Inicializar Luces 
    // LUZ 0: ROJA
    m_lights[0].diffuse = glm::vec3(1.0f, 50.0f / 255.0f, 25.0f / 255.0f);
    m_lights[0].ambient = m_lights[0].diffuse * 0.2f;
    m_lights[0].specular = glm::vec3(1.0f);
    m_lights[0].radius = 4.0f; m_lights[0].height = 2.0f; m_lights[0].speedOffset = 0.0f;
    // LUZ 1: VERDE
    m_lights[1].diffuse = glm::vec3(50.0f / 255.0f, 1.0f, 25.0f / 255.0f);
    m_lights[1].ambient = m_lights[1].diffuse * 0.2f;
    m_lights[1].specular = glm::vec3(1.0f);
    m_lights[1].radius = 5.5f; m_lights[1].height = 3.5f; m_lights[1].speedOffset = 2.09f; 
    // LUZ 2: AZUL
    m_lights[2].diffuse = glm::vec3(25.0f / 255.0f, 50.0f / 255.0f, 1.0f);
    m_lights[2].ambient = m_lights[2].diffuse * 0.2f;
    m_lights[2].specular = glm::vec3(1.0f);
    m_lights[2].radius = 3.0f; m_lights[2].height = 1.0f; m_lights[2].speedOffset = 4.18f;
    // Crear la geometría de la esferita
    setupSphereBuffer();
    setupSkybox();
    std::string miEscena = "space_station_3.obj, spaceship.obj, space_cephalopod.obj, spaceship_longford.obj, planet_of_phoenix.obj";
    loadSceneFromString(miEscena);
    createParametricTable();
    return true;
}

void C3DViewer::update() {
    // Grados por segundo
    float cameraSpeed = 2.5f * 0.016f;
    float rotateSpeed = 100.0f * 0.016f; 
    // Camara FPS/GOD
    glm::vec3 moveFront = m_cameraFront;
    if (m_cameraFPSMode) {
        // En modo FPS, ignoramos la componente Y para caminar sobre la mesa
        moveFront.y = 0.0f;
        // Volvemos a normalizar para mantener la misma velocidad al caminar
        if (glm::length(moveFront) > 0.001f) {
            moveFront = glm::normalize(moveFront);
        }
    }
    if (m_isFallingToFPS) {
        // Interpolación suave. El 0.1f es la velocidad de caida
        m_cameraPos = glm::mix(m_cameraPos, m_savedFPSPos, 0.05f);
        // Si ya estamos muy cerca del punto guardado, detenemos la caída
        if (glm::distance(m_cameraPos, m_savedFPSPos) < 0.05f) {
            m_cameraPos = m_savedFPSPos; // Aseguramos posición exacta
            m_isFallingToFPS = false;
        }
    }
    // MOVERSE ADELANTE/ATRAS 
    if (!m_isFallingToFPS) {
        if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS)
            m_cameraPos += cameraSpeed * moveFront;
        if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS)
            m_cameraPos -= cameraSpeed * moveFront;
    }
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
    // Animacion de Luces
    float time = glfwGetTime() * m_lightAnimSpeed;
    for (int i = 0; i < 3; i++) {
        // Trayectoria armoniosa tipo luciérnaga (Seno y Coseno)
        m_lights[i].position.x = cos(time + m_lights[i].speedOffset) * m_lights[i].radius;
        m_lights[i].position.y = m_lights[i].height + sin(time * 0.5f + m_lights[i].speedOffset) * 1.5f; // Sube y baja suavemente
        m_lights[i].position.z = sin(time + m_lights[i].speedOffset) * m_lights[i].radius;
    }
    // ANIMACIONES MÚLTIPLES 
    float timeObj = glfwGetTime();
    for (auto& sub : m_subMeshes) {
        if (sub.animType == 1) {
            // 1: Clásico (flota y gira rápido)
            sub.localRotation = glm::angleAxis(timeObj * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
            sub.localPosition.y = sub.initialPosition.y + sin(timeObj * 2.0f) * 0.5f;
            sub.localScale = glm::vec3(1.0f + sin(timeObj * 4.0f) * 0.02f);
        }
        else if (sub.animType == 2) {
            // 2: Rebote continuo
            sub.localPosition.y = sub.initialPosition.y + abs(sin(timeObj * 3.0f)) * 3.0f;
        }
        else if (sub.animType == 3) {
            // 3: Transporte 1 (Ruta: Planeta <-> Estación Derecha Lejana)
            glm::vec3 start = glm::vec3(6.5f, 6.0f, 2.0f);   // Afuera de la atmósfera del planeta gigante
            glm::vec3 end = glm::vec3(22.0f, 8.0f, 9.0f);  // Justo antes de chocar con la estación
            float t = (sin(timeObj * 0.5f) + 1.0f) * 0.5f;
            sub.localPosition = glm::mix(start, end, t);
            sub.localRotation = glm::angleAxis(sin(timeObj * 0.5f) * 0.3f, glm::vec3(0.0f, 0.0f, 1.0f)) * glm::angleAxis(glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        else if (sub.animType == 4) {
            // 4: Rotación Planetaria (Planeta Reflectivo)
            sub.localRotation = glm::angleAxis(timeObj * 0.2f, glm::vec3(0.0f, 1.0f, 0.0f));
        }
        else if (sub.animType == 5) {
            // 5: Gravedad Cero (Estaciones y Cefalópodo)
            sub.localPosition.y = sub.initialPosition.y + sin(timeObj * 0.5f + sub.initialPosition.x) * 1.2f;
            // Pequeño balanceo lateral
            sub.localRotation = glm::angleAxis(sin(timeObj * 0.3f) * 0.1f, glm::vec3(1.0f, 0.0f, 0.0f));
        }
        else if (sub.animType == 6) {
            // 6: Transporte 2 (Ruta: Planeta <-> Estación Izquierda Lejana)
            glm::vec3 start = glm::vec3(-6.5f, 6.0f, -2.0f);
            glm::vec3 end = glm::vec3(-22.0f, 2.0f, -9.0f);
            float t = (sin(timeObj * 0.6f + 1.5f) + 1.0f) * 0.5f;
            sub.localPosition = glm::mix(start, end, t);
            sub.localRotation = glm::angleAxis(sin(timeObj * 0.6f + 1.5f) * 0.3f, glm::vec3(0.0f, 0.0f, 1.0f)) * glm::angleAxis(glm::radians(135.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        }
    }
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
            //Picking Desactivado
            /*
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
            */
			//Desactivada la rotacion global
            //isDragging = true;
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

bool C3DViewer::loadOBJ(const std::string& filename, bool clearScene, glm::vec3 pos, int animType) {
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
    // Solo Limpia la primera vez
    if (clearScene) {
        m_vertices.clear();
        m_subMeshes.clear();
    }
    // Extraer el nombre base del archivo (sin ".obj" ni rutas)
    std::string baseName = filename;
    size_t lastSlash = baseName.find_last_of("/\\");
    if (lastSlash != std::string::npos) baseName = baseName.substr(lastSlash + 1);
    size_t lastDot = baseName.find_last_of('.');
    if (lastDot != std::string::npos) baseName = baseName.substr(0, lastDot);
    int partCounter = 0;
    //Submesh
    for (const auto& shape : shapes) {
        SubMesh subMesh;
        subMesh.name = baseName + "_" + std::to_string(partCounter++);
        subMesh.initialPosition = pos;
        subMesh.localPosition = pos;
        subMesh.animType = animType;
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
                    subMesh.diffuseColor = glm::vec3(materials[matId].diffuse[0], materials[matId].diffuse[1], materials[matId].diffuse[2]);
                    // Cargar Texturas usando baseDir
                    if (!materials[matId].diffuse_texname.empty()) {
                        std::string texPath = baseDir + materials[matId].diffuse_texname;
                        subMesh.diffuseMap = loadTexture(texPath.c_str());
                    }
                    if (!materials[matId].specular_texname.empty()) {
                        std::string texPath = baseDir + materials[matId].specular_texname;
                        subMesh.specularMap = loadTexture(texPath.c_str());
                    }
                    if (!materials[matId].ambient_texname.empty()) {
                        std::string texPath = baseDir + materials[matId].ambient_texname;
                        subMesh.ambientMap = loadTexture(texPath.c_str());
                    }
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
            if (index.vertex_index >= 0 && (3 * index.vertex_index + 2) < attrib.vertices.size()) {
                vertex.Position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };
            }
            else {
                vertex.Position = glm::vec3(0.0f); // OBJ está roto
            }
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
            vertex.Tangent = glm::vec3(0.0f);
            m_vertices.push_back(vertex);
            subMesh.indices.push_back(m_vertices.size() - 1);
        }
        subMesh.indexCount = subMesh.indices.size();
        m_subMeshes.push_back(subMesh);
    }
    if (clearScene) {
        calculateBoundingBox();
        computeNormals();
        computeTangents();
        createParametricTable();
        //setupMeshBuffers();
        updateNormalBuffers();
        resetView();
    }
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
    // Location 2: TexCoords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    glEnableVertexAttribArray(2);
    // Location 3: Tangent 
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
    glEnableVertexAttribArray(3);
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
    // Enviar Iluminacion
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "viewPos"), 1, glm::value_ptr(m_cameraPos));
    glUniform1i(glGetUniformLocation(m_shaderProgram, "useAttenuation"), m_enableAttenuation);
    for (int i = 0; i < 3; i++) {
        std::string prefix = "lights[" + std::to_string(i) + "].";
        glUniform1i(glGetUniformLocation(m_shaderProgram, (prefix + "enabled").c_str()), m_lights[i].enabled);
        glUniform3fv(glGetUniformLocation(m_shaderProgram, (prefix + "position").c_str()), 1, glm::value_ptr(m_lights[i].position));
        glUniform3fv(glGetUniformLocation(m_shaderProgram, (prefix + "ambient").c_str()), 1, glm::value_ptr(m_lights[i].ambient));
        glUniform3fv(glGetUniformLocation(m_shaderProgram, (prefix + "diffuse").c_str()), 1, glm::value_ptr(m_lights[i].diffuse));
        glUniform3fv(glGetUniformLocation(m_shaderProgram, (prefix + "specular").c_str()), 1, glm::value_ptr(m_lights[i].specular));
        glUniform1i(glGetUniformLocation(m_shaderProgram, (prefix + "shadingModel").c_str()), m_lights[i].shadingModel);
        // Dibujar físicamente la esferita de luz en su posición actual
        if (m_lights[i].enabled) {
            drawLightSphere(m_lights[i].position, m_lights[i].diffuse);
        }
    }
    //glBindVertexArray(m_vao);
    unsigned int offset = 0;
    // SubMesh
    for (int i = 0; i < m_subMeshes.size(); i++) {
        glBindVertexArray(m_vao);
        SubMesh& sub = m_subMeshes[i];
        if (!sub.visible) { offset += sub.indices.size(); continue; }
        glm::mat4 localModel;
        if (sub.name == "Mesa_Madera") {
            localModel = glm::mat4(1.0f); // La mesa se queda completamente quieta
        }
        else {
            localModel = globalModel;
            // Aplicar animación al objeto
            localModel = glm::translate(localModel, sub.localPosition);
            localModel = localModel * glm::toMat4(sub.localRotation);
            localModel = glm::scale(localModel, sub.localScale);
        }
        glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(localModel));
        glUniform1i(glGetUniformLocation(m_shaderProgram, "uvMappingMode"), sub.uvMappingMode);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "isReflective"), sub.isReflective);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTexture);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "skybox"), 4);
        // Color
        glm::vec3 color = sub.diffuseColor;
        // Resaltar selección 
        //if (i == m_selectedSubMeshIndex && !m_showWireframe) color = glm::vec3(1.0f, 0.2f, 0.2f);
        glUniform3fv(glGetUniformLocation(m_shaderProgram, "uColor"), 1, glm::value_ptr(color));
        // Configurar texturas activas
        glUniform1i(glGetUniformLocation(m_shaderProgram, "hasMapDiffuse"), sub.diffuseMap != 0);
        //Bump
        glUniform1i(glGetUniformLocation(m_shaderProgram, "hasMapBump"), sub.bumpMap != 0);
        if (sub.bumpMap != 0) {
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, sub.bumpMap);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "mapBump"), 3);
        }
        if (sub.diffuseMap != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sub.diffuseMap);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "mapDiffuse"), 0);
        }
        glUniform1i(glGetUniformLocation(m_shaderProgram, "hasMapSpecular"), sub.specularMap != 0);
        if (sub.specularMap != 0) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, sub.specularMap);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "mapSpecular"), 1);
        }
        glUniform1i(glGetUniformLocation(m_shaderProgram, "hasMapAmbient"), sub.ambientMap != 0);
        if (sub.ambientMap != 0) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, sub.ambientMap);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "mapAmbient"), 2);
        }
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
    glDepthFunc(GL_LEQUAL);
    glUseProgram(m_skyboxShader);
    glm::mat4 skyView = glm::mat4(glm::mat3(view));
    glUniformMatrix4fv(glGetUniformLocation(m_skyboxShader, "view"), 1, GL_FALSE, glm::value_ptr(skyView));
    glUniformMatrix4fv(glGetUniformLocation(m_skyboxShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1i(glGetUniformLocation(m_skyboxShader, "skybox"), 0);
    glBindVertexArray(m_skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
    drawInterface();
}
void C3DViewer::drawInterface() {
    // Inicio de Frame ImGui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    // Configuración de la Ventana Principal
    ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin("Panel de Control - Proyecto 3 UCV");
    // Carga Desactivada 
    /*
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
    */
    ImGui::Separator();
    //  SISTEMA Y ESTADÍSTICAS 
    ImGui::Text("Rendimiento: %.1f FPS", ImGui::GetIO().Framerate);
    //ImGui::ColorEdit3("Color de Fondo", glm::value_ptr(m_bgColor));
    ImGui::Separator();
    // TRANSFORMACIONES GLOBALES 
    if (ImGui::CollapsingHeader("Transformacion Global", ImGuiTreeNodeFlags_DefaultOpen)) {
        //ImGui::DragFloat3("Posicion Mundo", glm::value_ptr(m_globalPos), 0.05f);
        //ImGui::DragFloat3("Escala (XYZ)", glm::value_ptr(m_globalScale), 0.01f, 0.01f, 100.0f);
        if (ImGui::Button("CENTRAR VISTA Y OBJETO")) {
            resetView();
        }
        //Centrar no Necesario
        /*
        if (ImGui::Button("Centrar Objeto en (0,0,-3)")) {
            m_globalPos = glm::vec3(0.0f, 0.0f, -3.0f);
            m_globalRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            m_globalScale = glm::vec3(1.0f);
        }
        */
        /*
        ImGui::Separator();
        static char exportName[128] = "modelo_exportado.obj";
        ImGui::InputText("Archivo de Salida", exportName, sizeof(exportName));
        if (ImGui::Button("Exportar OBJ")) {
            exportOBJ(exportName);
        }
        */
    }
    //Opciones de Iluminacion
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Iluminacion")) {
        ImGui::Checkbox("Activar Atenuacion (f_att)", &m_enableAttenuation);
        ImGui::SliderFloat("Velocidad de Animacion", &m_lightAnimSpeed, 0.0f, 5.0f);
        ImGui::Separator();

        const char* lightNames[3] = { "Luz 1 (Roja)", "Luz 2 (Verde)", "Luz 3 (Azul)" };
        const char* shadingModels[3] = { "Phong", "Blinn-Phong", "Flat Shading" };

        for (int i = 0; i < 3; i++) {
            ImGui::PushID(i); // Para que ImGui no mezcle los botones de diferentes luces
            if (ImGui::TreeNode(lightNames[i])) {
                ImGui::Checkbox("Encendida", &m_lights[i].enabled);
                ImGui::Combo("Modelo", &m_lights[i].shadingModel, shadingModels, 3);

                ImGui::ColorEdit3("Difuso (RGB)", glm::value_ptr(m_lights[i].diffuse));
                ImGui::ColorEdit3("Ambiente (RGB)", glm::value_ptr(m_lights[i].ambient));
                ImGui::ColorEdit3("Especular (RGB)", glm::value_ptr(m_lights[i].specular));

                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Mapeo y Reflejos", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (!m_subMeshes.empty()) {
            // Llenar la lista con los nombres
            std::vector<const char*> names;
            for (const auto& s : m_subMeshes) names.push_back(s.name.c_str());
            // Selector "Group Box" 
            ImGui::Combo("Seleccionar Parte", &m_selectedSubMeshIndex, names.data(), names.size());
            if (m_selectedSubMeshIndex >= 0 && m_selectedSubMeshIndex < m_subMeshes.size()) {
                SubMesh& sub = m_subMeshes[m_selectedSubMeshIndex];
                ImGui::Separator();
                const char* mapModes[] = {
                    "0: UVs Originales (OBJ)",
                    "1: S-Mapping (Esferico)",
                    "2: S-Mapping (Plano XY)",
                    "3: O-Mapping (Cilindrico)",
                    "4: O-Mapping (Normales)"
                };
                ImGui::Combo("Generacion UVs", &sub.uvMappingMode, mapModes, 5);
                ImGui::Checkbox("Material Reflectivo (Bronce)", &sub.isReflective);
            }
        }
    }
    ImGui::Separator();
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
            /*
            ImGui::Separator();
            // Botón rojo para indicar acción destructiva
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
            
            if (ImGui::Button("ELIMINAR SUB-MALLADO", ImVec2(-1, 0))) {
                sub.visible = false;         
                m_selectedSubMeshIndex = -1;
            }
            */
            //Control de Bump
            ImGui::Separator();
            ImGui::Text("Texturas Personalizadas (Punto B)");
            ImGui::InputText("Difusa (.jpg/.png)", m_texDiffuseInput, sizeof(m_texDiffuseInput));
            if (ImGui::Button("Cargar Difusa")) {
                std::string full = "objetos3D/" + std::string(m_texDiffuseInput);
                sub.diffuseMap = loadTexture(full.c_str());
            }

            ImGui::InputText("Bump Map (.jpg/.png)", m_texBumpInput, sizeof(m_texBumpInput));
            if (ImGui::Button("Cargar Bump Map")) {
                std::string full = "objetos3D/" + std::string(m_texBumpInput);
                sub.bumpMap = loadTexture(full.c_str());
            }
        }
        else {
            ImGui::TextWrapped("Haz Clic Izquierdo sobre el objeto 3D para seleccionar una parte y editarla.");
        }
    }
    // Interfaz de Cámara
    if (ImGui::CollapsingHeader("Camara", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Modo de Navegacion:");
        // Si el usuario selecciona FPS, la variable se vuelve true
        if (ImGui::RadioButton("FPS (Caminar)", m_cameraFPSMode == true)) {
            if (!m_cameraFPSMode) {
                m_cameraFPSMode = true;
                m_isFallingToFPS = true; 
            }
        }
        ImGui::SameLine();
        // Si el usuario selecciona GOD, la variable se vuelve false
        if (ImGui::RadioButton("GOD (Volar)", m_cameraFPSMode == false)) {
            if (m_cameraFPSMode) {
                m_savedFPSPos = m_cameraPos;
                m_cameraFPSMode = false;
                m_isFallingToFPS = false;
            }
        }
        ImGui::Text("Altura (Y): %.2f", m_cameraPos.y);
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
    std::ofstream outObj("objetos3D/" + filename);
    std::ofstream outMtl("objetos3D/" + mtlFilename);
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
//Iluminacion
// Generador trigonométrico de esfera (Longitud/Latitud)
void C3DViewer::setupSphereBuffer() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    int sectors = 15;
    int stacks = 15;
    float radius = 0.2f;
    for (int i = 0; i <= stacks; ++i) {
        float V = i / (float)stacks;
        float phi = V * glm::pi<float>();
        for (int j = 0; j <= sectors; ++j) {
            float U = j / (float)sectors;
            float theta = U * (glm::pi<float>() * 2.0f);
            float x = cos(theta) * sin(phi);
            float y = cos(phi);
            float z = sin(theta) * sin(phi);
            vertices.push_back(x * radius);
            vertices.push_back(y * radius);
            vertices.push_back(z * radius);
        }
    }
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            int first = (i * (sectors + 1)) + j;
            int second = first + sectors + 1;
            indices.push_back(first); indices.push_back(second); indices.push_back(first + 1);
            indices.push_back(second); indices.push_back(second + 1); indices.push_back(first + 1);
        }
    }
    m_sphereIndexCount = indices.size();
    glGenVertexArrays(1, &m_vao_sphere);
    glGenBuffers(1, &m_vbo_sphere);
    glGenBuffers(1, &m_ebo_sphere);
    glBindVertexArray(m_vao_sphere);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo_sphere);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo_sphere);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void C3DViewer::drawLightSphere(const glm::vec3& pos, const glm::vec3& color) {
    if (m_vao_sphere == 0) return;
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(glGetUniformLocation(m_shaderProgram, "useFlatColor"), 1);
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "uColor"), 1, glm::value_ptr(color)); // Color de su luz
    glBindVertexArray(m_vao_sphere);
    glDrawElements(GL_TRIANGLES, m_sphereIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "useFlatColor"), 0);
}

// Texturas
GLuint C3DViewer::loadTexture(const char* path) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format = (nrComponents == 4) ? GL_RGBA : GL_RGB;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
        std::cout << "Textura cargada: " << path << std::endl;
    }
    else {
        std::cout << "Error al cargar textura: " << path << std::endl;
        stbi_image_free(data);
        return 0;
    }
    return textureID;
}

void C3DViewer::createParametricTable() {
    SubMesh tableMesh;
    tableMesh.name = "Mesa_Madera";
    int resolution = 50;
    float size = 40.0f;
    // Generar Vértices Paramétricos (Aplanados para glDrawArrays)
    for (int i = 0; i < resolution; ++i) {
        for (int j = 0; j < resolution; ++j) {
            float u0 = (float)j / resolution;
            float v0 = (float)i / resolution;
            float u1 = (float)(j + 1) / resolution;
            float v1 = (float)(i + 1) / resolution;
            Vertex p1, p2, p3, p4;
            // Coordenadas Espaciales
            p1.Position = glm::vec3((u0 - 0.5f) * size, -1.0f, (v0 - 0.5f) * size);
            p2.Position = glm::vec3((u1 - 0.5f) * size, -1.0f, (v0 - 0.5f) * size);
            p3.Position = glm::vec3((u0 - 0.5f) * size, -1.0f, (v1 - 0.5f) * size);
            p4.Position = glm::vec3((u1 - 0.5f) * size, -1.0f, (v1 - 0.5f) * size);
            // Normales y Tangentes apuntando correctamente
            glm::vec3 normal(0.0f, 1.0f, 0.0f);
            glm::vec3 tangent(1.0f, 0.0f, 0.0f);
            p1.Normal = p2.Normal = p3.Normal = p4.Normal = normal;
            p1.Tangent = p2.Tangent = p3.Tangent = p4.Tangent = tangent;
            // Coordenadas de Textura (UVs)
            p1.TexCoords = glm::vec2(u0 * 10.0f, v0 * 10.0f);
            p2.TexCoords = glm::vec2(u1 * 10.0f, v0 * 10.0f);
            p3.TexCoords = glm::vec2(u0 * 10.0f, v1 * 10.0f);
            p4.TexCoords = glm::vec2(u1 * 10.0f, v1 * 10.0f);
            // Definir los 2 triángulos (6 vértices)
            Vertex quad[6] = { p1, p3, p2, p2, p3, p4 };
            for (int k = 0; k < 6; k++) {
                tableMesh.min = glm::min(tableMesh.min, quad[k].Position);
                tableMesh.max = glm::max(tableMesh.max, quad[k].Position);
                m_vertices.push_back(quad[k]);
                tableMesh.indices.push_back(m_vertices.size() - 1);
            }
        }
    }
    tableMesh.diffuseColor = glm::vec3(0.5f, 0.3f, 0.1f);
    // Cargar texturas automáticamente
    std::string diffPath = "objetos3D/" + std::string(m_texDiffuseInput);
    std::string bumpPath = "objetos3D/" + std::string(m_texBumpInput);
    std::cout << "Cargando texturas de la mesa por defecto..." << std::endl;
    tableMesh.diffuseMap = loadTexture(diffPath.c_str());
    tableMesh.bumpMap = loadTexture(bumpPath.c_str());
    tableMesh.indexCount = tableMesh.indices.size();
    tableMesh.animType = 0;
    m_subMeshes.push_back(tableMesh);
    // Recargar buffers a la GPU
    setupMeshBuffers();
    std::cout << "Mesa Parametrica generada." << std::endl;
}

// Correccion de Bump
void C3DViewer::computeTangents() {
    if (m_vertices.empty()) return;
    for (size_t i = 0; i < m_vertices.size(); i += 3) {
        if (i + 2 >= m_vertices.size()) break;
        Vertex& v0 = m_vertices[i];
        Vertex& v1 = m_vertices[i + 1];
        Vertex& v2 = m_vertices[i + 2];
        // Calcular los bordes del triángulo
        glm::vec3 edge1 = v1.Position - v0.Position;
        glm::vec3 edge2 = v2.Position - v0.Position;
        // Calcular la diferencia en las coordenadas UV
        glm::vec2 deltaUV1 = v1.TexCoords - v0.TexCoords;
        glm::vec2 deltaUV2 = v2.TexCoords - v0.TexCoords;
        float determinant = (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        glm::vec3 tangent;
        // Evitar división por cero si el OBJ no tiene buenas coordenadas UV
        if (abs(determinant) < 0.0001f) {
            tangent = glm::vec3(1.0f, 0.0f, 0.0f); // Tangente segura por defecto
        }
        else {
            float f = 1.0f / determinant;
            tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
            tangent = glm::normalize(tangent);
        }
        v0.Tangent = tangent;
        v1.Tangent = tangent;
        v2.Tangent = tangent;
    }
}
//Skybox
GLuint C3DViewer::loadCubemap(std::vector<std::string> faces) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(false);
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            std::cout << "Error cargando cara del cubemap: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return textureID;
}

void C3DViewer::setupSkybox() {
    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f
    };
    glGenVertexArrays(1, &m_skyboxVAO);
    glGenBuffers(1, &m_skyboxVBO);
    glBindVertexArray(m_skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    std::vector<std::string> faces = {
        "objetos3D/skybox/right.jpg", "objetos3D/skybox/left.jpg",
        "objetos3D/skybox/top.jpg",   "objetos3D/skybox/bottom.jpg",
        "objetos3D/skybox/front.jpg", "objetos3D/skybox/back.jpg"
    };
    m_cubemapTexture = loadCubemap(faces);
    // Compilar el Shader del Skybox
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER); glShaderSource(vShader, 1, &skyboxVertexShaderSrc, NULL); glCompileShader(vShader);
    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(fShader, 1, &skyboxFragmentShaderSrc, NULL); glCompileShader(fShader);
    m_skyboxShader = glCreateProgram(); glAttachShader(m_skyboxShader, vShader); glAttachShader(m_skyboxShader, fShader); glLinkProgram(m_skyboxShader);
}
void C3DViewer::loadSceneFromString(const std::string& objList) {
    m_vertices.clear();
    m_subMeshes.clear();
    std::stringstream ss(objList);
    std::string item;
    while (std::getline(ss, item, ',')) {
        item.erase(0, item.find_first_not_of(" \t"));
        item.erase(item.find_last_not_of(" \t") + 1);
        if (item.empty()) continue;
        glm::vec3 pos(0.0f);
        glm::vec3 customScale(1.0f); //Control manual de tamaño
        int anim = 1;
        bool isMetal = false;
        size_t startMeshCount = m_subMeshes.size();
        if (item.find("planet") != std::string::npos) {
            pos = glm::vec3(0.0f, 6.0f, 0.0f); // Más alto para dominar la escena
            customScale = glm::vec3(4.5f);     // Planeta 4.5 veces más grande
            anim = 4;
            isMetal = true;
        }
        else if (item.find("space_station") != std::string::npos || item.find("cephalopod") != std::string::npos) {
            static int stationCount = 0;
            // Estaciones súper lejos a los extremos (-25 y 25)
            pos = (stationCount % 2 == 0) ? glm::vec3(-25.0f, 2.0f, -10.0f) : glm::vec3(25.0f, 8.0f, 10.0f);
            customScale = glm::vec3(2.5f); // Estaciones masivas
            anim = 5;
            stationCount++;
        }
        else if (item.find("spaceship") != std::string::npos) {
            static int shipCount = 0;
            pos = glm::vec3(0.0f);
            customScale = glm::vec3(1.2f); // Naves un poco más grandes para que se noten
            anim = (shipCount % 2 == 0) ? 3 : 6;
            shipCount++;
        }
        loadOBJ(item, false, pos, anim);
        // Aplicamos la reflectividad y el TAMAÑO a las partes recién cargadas
        for (size_t i = startMeshCount; i < m_subMeshes.size(); i++) {
            m_subMeshes[i].isReflective = isMetal;
            m_subMeshes[i].localScale = customScale; 
        }
    }
    calculateBoundingBox();
    computeNormals();
    computeTangents();
    createParametricTable();
    updateNormalBuffers();
    resetView();
    std::cout << "Escena espacial masiva cargada con exito." << std::endl;
}