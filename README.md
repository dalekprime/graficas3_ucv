# **Proyecto 3: Iluminación Avanzada, Texturizado y Mapeo de Entorno (OpenGL 3.3+)**

Este proyecto expande el motor gráfico 3D implementando modelos de iluminación física, mapeo de texturas multicapa, renderizado de entornos inmersivos (Skybox) y coreografía de escenas animadas múltiples utilizando C++ y OpenGL Moderno.

## Funcionalidades Principales

* Iluminación Dinámica: Implementación de 3 fuentes de luz (RGB) con trayectorias animadas mediante funciones trigonométricas. Soporta atenuación matemática y selección en tiempo real de modelos de sombreado (Phong, Blinn-Phong y Flat Shading vía derivadas espaciales).

* Texturizado y Bump Mapping: Soporte para carga de mapas Diffuse, Specular y Ambient. Inclusión de un sistema de Normal/Bump Mapping para simulación de micro-relieves interactivos a la luz.

* Environment Mapping: Renderizado de un Skybox cúbico infinito con fusión de bordes suavizada. 

* Implementación de materiales: Simulación de metal/bronce que reflejan el entorno mediante la función reflect().

* Escena Animada Inteligente: Sistema de parsing capaz de cargar múltiples archivos .obj desde un std::string, asignando escalas dinámicas, posiciones iniciales y animaciones temáticas.

## Decisiones de Diseño

* Cálculo del Espacio Tangente: Para el Bump Mapping, se genera la matriz TBN en el Vertex Shader utilizando el proceso de ortogonalización de Gram-Schmidt. Se ajustó el orden del producto cruz ($B = T \times N$) para alinear la profundidad matemática con los estándares de texturas normales de OpenGL.

* Arquitectura de Animación Segregada: Se expandió la estructura SubMesh inyectando propiedades de estado ancla (initialPosition, animType). Esto desacopla la lógica de animación de la matriz global, permitiendo lógicas de movimiento asíncronas e independientes para cada objeto de la escena.

* Prevención de Fallos de Memoria: La extracción de vértices a través del parser (TinyObjLoader) se envolvió en validaciones de límites estrictos. Esto previene violaciones de acceso al cargar modelos corrompidos.

* Aislamiento del Especular: Se modificó la ecuación de iluminación del Fragment Shader aislando el acumulador de brillo ($Specular$) de la base de color. Esto evita la contaminación difusa en materiales reflectivos, logrando un efecto espejo/bronce físicamente correcto.

## Asunciones del Enunciado

* Se asume que el S-Mapping (Surface) se fundamenta en la posición puramente geométrica de los vértices ($V_{local}$), mientras que el O-Mapping (Object) opera sobre propiedades morfológicas como las coordenadas cilíndricas o el vector normal propio del objeto.

* Se asume que la implementación del Environment Mapping requiere la arquitectura de un Cubemap tradicional (6 imágenes discretas) para garantizar la compatibilidad directa con los muestreadores samplerCube del API de OpenGL.

* Acorde al enunciado, se asume la desactivación del picking por búfer de color mediante clics en la escena. La selección y edición de sub-mallados queda delegada exclusivamente al componente ImGui::Combo (Group Box).


## Controles

* Rotar Cámara: Clic Derecho + Arrastrar.

* Mover Cámara: Flechas Dirrecionales.

* Seleccionar: Control por Interfaz.

* Interfaz y Selección: Uso del ratón sobre el panel flotante ImGui para seleccionar objetos, alterar UVs, cargar texturas e interactuar con las luces.

## Librerías y Dependencias

* GLFW: Gestión de ventana y contexto OpenGL.

* GLAD: Carga de punteros a funciones de OpenGL (3.3 Core).

* GLM: Matemáticas para gráficos (Vectores, Matrices, Cuaterniones).

* ImGui: Interfaz gráfica de usuario inmediata.

* TinyObjLoader: Parser ligero para archivos Wavefront (.obj).

* stb_image: Decodificador ligero para la inyección de texturas y caras del Skybox.

# **Autor**

*Desarrollado para la cátedra de Introducción a la Computación Gráfica. 
Universidad Central de Venezuela (UCV). Por Bryan Silva, 2026.*