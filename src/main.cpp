#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <sys/stat.h>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Render_Utils.h"
#include "Shader_Loader.h"
#include "Snow.h"
#include "objload.h"

#include "Pathfinder.h"
#include "Snow.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Zmienne globalne
GLuint program;
GLuint snowProgram;
GLuint vaoMatterhorn;
int numIndicesMatterhorn = 0;
GLuint textureMatterhorn;
GLuint textureNormalMap;

SnowSimulation *snowSim = nullptr;
Pathfinder *pathfinder = nullptr;
GLuint pathProgram;

float globalSnowLevel = 0.0f;
int selectedCorner = 0;

// Snow simulation settings
bool enableSnowSimulation = false;
int maxSnowParticles = 20000;

// Kamera
glm::vec3 cameraPos = glm::vec3(0.0f, 1.2f, 2.5f);
glm::vec3 cameraFront = glm::normalize(glm::vec3(0.0f, 0.2f, 0.0f) - cameraPos);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float yaw = -90.0f;
float pitch = -21.8f;

// Funkcja sprawdzająca czy plik istnieje
inline bool file_exists(const std::string &name) {
  struct stat buffer;
  return (stat(name.c_str(), &buffer) == 0);
}

// Funkcja pomocnicza do pobierania właściwej ścieżki pliku (wspiera
// uruchamianie z katalogu głównego i build/)
std::string get_path(const std::string &relative_path) {
  if (file_exists(relative_path)) {
    return relative_path;
  }
  std::string fallback = "../" + relative_path;
  if (file_exists(fallback)) {
    return fallback;
  }
  return relative_path; // domyślny powrót
}

// Funkcja wywoływana przy zmianie rozmiaru okna
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

// Obsługa wejścia
void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  static float lastKeyPressTime = 0.0f;
  static bool fastMode = false;

  int keys[] = {GLFW_KEY_W, GLFW_KEY_A, GLFW_KEY_S,
                GLFW_KEY_D, GLFW_KEY_Q, GLFW_KEY_E};
  static bool wasPressed[6] = {false};
  bool anyJustPressed = false;
  bool anyPressed = false;

  for (int i = 0; i < 6; i++) {
    bool isPressed = glfwGetKey(window, keys[i]) == GLFW_PRESS;
    if (isPressed && !wasPressed[i])
      anyJustPressed = true;
    if (isPressed)
      anyPressed = true;
    wasPressed[i] = isPressed;
  }

  if (anyJustPressed) {
    float currentTime = static_cast<float>(glfwGetTime());
    if (currentTime - lastKeyPressTime < 0.4f) {
      fastMode = true;
    }
    lastKeyPressTime = currentTime;
  }

  if (!anyPressed) {
    fastMode = false;
  }

  float speedMultiplier = fastMode ? 5.0f : 1.0f;
  float cameraSpeed = static_cast<float>(0.5 * speedMultiplier * deltaTime);
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    cameraPos += cameraSpeed * cameraFront;
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    cameraPos -= cameraSpeed * cameraFront;
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    cameraPos -=
        glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    cameraPos +=
        glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
  if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    cameraPos += cameraSpeed * cameraUp;
  if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    cameraPos -= cameraSpeed * cameraUp;

  float rotationSpeed = 60.0f * deltaTime;
  if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    pitch += rotationSpeed;
  if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    pitch -= rotationSpeed;
  if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
    yaw -= rotationSpeed;
  if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
    yaw += rotationSpeed;

  if (pitch > 89.0f)
    pitch = 89.0f;
  if (pitch < -89.0f)
    pitch = -89.0f;

  glm::vec3 front;
  front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
  front.y = sin(glm::radians(pitch));
  front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
  cameraFront = glm::normalize(front);
}

// Inicjalizacja
void init(GLFWwindow *window) {
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glClearColor(0.08f, 0.12f, 0.18f,
               1.0f); // Klimatyczne, ciemnoniebieskie tło nieba
  glEnable(GL_DEPTH_TEST);

  // Wczytanie shaderów z obsługą ścieżki względnej
  Core::Shader_Loader shaderLoader;
  std::string vertPath = get_path("shaders/shader_3_2.vert");
  std::string fragPath = get_path("shaders/shader_3_2.frag");

  // Konwertujemy std::string na char* potrzebny dla Shader_Loader
  std::vector<char> vertPathBuf(vertPath.begin(), vertPath.end());
  vertPathBuf.push_back('\0');
  std::vector<char> fragPathBuf(fragPath.begin(), fragPath.end());
  fragPathBuf.push_back('\0');

  program = shaderLoader.CreateProgram(vertPathBuf.data(), fragPathBuf.data());

  // Wczytanie modelu landscape
  std::string modelPath = get_path("models/landscape_mid.obj");
  std::cout << "Loading model: " << modelPath << std::endl;
  obj::Model model = obj::loadModelFromFile(modelPath);

  if (model.faces.empty()) {
    std::cerr << "Failed to load model faces or model is empty!" << std::endl;
  } else {
    vaoMatterhorn = Core::initVAOForModel(model);
    numIndicesMatterhorn = model.faces.begin()->second.size();
    std::cout << "Loaded Matterhorn model successfully with "
              << numIndicesMatterhorn << " indices." << std::endl;

    // Initialize Grid Data for Pathfinding
    snowSim = new SnowSimulation(0); // We only need the grid, not particles
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -20.0f, 0.0f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(30.0f, 30.0f, 30.0f));
    snowSim->initGridFromModel(model, modelMatrix);

    // Wczytanie shaderów dla symulacji śniegu
    std::string snowVertPath = get_path("shaders/snow.vert");
    std::string snowFragPath = get_path("shaders/snow.frag");
    std::vector<char> snowVertBuf(snowVertPath.begin(), snowVertPath.end());
    snowVertBuf.push_back('\0');
    std::vector<char> snowFragBuf(snowFragPath.begin(), snowFragPath.end());
    snowFragBuf.push_back('\0');
    snowProgram = shaderLoader.CreateProgram(snowVertBuf.data(), snowFragBuf.data());

    snowSim->initParticles(maxSnowParticles);
    snowSim->initRendering(snowProgram);

    // Initialize Pathfinder
    pathfinder = new Pathfinder(snowSim);
    std::string pathVertPath = get_path("shaders/path.vert");
    std::string pathFragPath = get_path("shaders/path.frag");
    std::vector<char> pathVertBuf(pathVertPath.begin(), pathVertPath.end());
    pathVertBuf.push_back('\0');
    std::vector<char> pathFragBuf(pathFragPath.begin(), pathFragPath.end());
    pathFragBuf.push_back('\0');
    pathProgram =
        shaderLoader.CreateProgram(pathVertBuf.data(), pathFragBuf.data());

    pathfinder->initRendering(pathProgram);
  }

  // Wczytywanie tekstury
  std::string texPath = get_path("models/landscape_texture.jpg");
  glGenTextures(1, &textureMatterhorn);
  glBindTexture(GL_TEXTURE_2D, textureMatterhorn);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  stbi_set_flip_vertically_on_load(true);
  int width, height, nrChannels;
  unsigned char *data =
      stbi_load(texPath.c_str(), &width, &height, &nrChannels, 0);
  if (data) {
    GLenum format = GL_RGB;
    if (nrChannels == 1)
      format = GL_RED;
    else if (nrChannels == 3)
      format = GL_RGB;
    else if (nrChannels == 4)
      format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    std::cout << "Failed to load texture" << std::endl;
  }
  stbi_image_free(data);

  // Wczytywanie mapy normalnych (Normal Map)
  std::string normalPath = get_path("models/landscape_normal.jpg");
  glGenTextures(1, &textureNormalMap);
  glBindTexture(GL_TEXTURE_2D, textureNormalMap);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  unsigned char *normalData =
      stbi_load(normalPath.c_str(), &width, &height, &nrChannels, 0);
  if (normalData) {
    GLenum format = GL_RGB;
    if (nrChannels == 1)
      format = GL_RED;
    else if (nrChannels == 3)
      format = GL_RGB;
    else if (nrChannels == 4)
      format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, normalData);
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    std::cout << "Failed to load normal map" << std::endl;
  }
  stbi_image_free(normalData);

  // Inicjalizacja ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");
}

// Funkcja renderująca scenę
void renderScene(GLFWwindow *window) {
  float currentFrame = glfwGetTime();
  deltaTime = currentFrame - lastFrame;
  lastFrame = currentFrame;

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGui::Begin("Snow Control");
  ImGui::SliderFloat("Snow Amount", &globalSnowLevel, 0.0f, 10.0f);

  ImGui::Separator();
  ImGui::Text("Snow Falling Animation");
  if (enableSnowSimulation) {
      if (ImGui::Button("Pause Animation")) {
          enableSnowSimulation = false;
      }
  } else {
      if (ImGui::Button("Play Animation")) {
          enableSnowSimulation = true;
      }
  }
  if (snowSim) {
      ImGui::SliderInt("Particles Count", &snowSim->activeParticleCount, 0, maxSnowParticles);
      ImGui::SliderFloat("Gravity", &snowSim->gravity, 0.0f, 5.0f);
      ImGui::SliderFloat("Wind Strength", &snowSim->windStrength, 0.0f, 5.0f);
  }

  ImGui::Separator();
  ImGui::Text("Pathfinding Options");
  const char *corners[] = {"North-West", "North-East", "South-West",
                           "South-East"};
  ImGui::Combo("Start Corner", &selectedCorner, corners, 4);

  if (ImGui::Button("Generate Route") && pathfinder) {
    pathfinder->generatePath(selectedCorner, globalSnowLevel);
  }
  ImGui::SameLine();
  if (ImGui::Button("Clear Routes") && pathfinder) {
    pathfinder->clearPaths();
  }

  if (pathfinder) {
    const auto &paths = pathfinder->getPaths();
    if (!paths.empty()) {
      ImGui::Separator();
      ImGui::Text("Generated Routes Info:");
      for (size_t i = 0; i < paths.size(); ++i) {
        const auto &p = paths[i];
        ImVec4 col(p.color.r, p.color.g, p.color.b, p.color.a);
        ImGui::TextColored(col, "Route %zu: %.2f km, %.0f m gain", i + 1,
                           p.distanceKm, p.elevationGainM);
      }
    }
  }

  ImGui::End();

  if (enableSnowSimulation && snowSim) {
    snowSim->update(deltaTime, currentFrame);
  }

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (numIndicesMatterhorn > 0) {
    glUseProgram(program);

    // Bind main texture to unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureMatterhorn);
    glUniform1i(glGetUniformLocation(program, "textureSampler"), 0);

    // Bind normal map to unit 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureNormalMap);
    glUniform1i(glGetUniformLocation(program, "normalSampler"), 1);

    glUniform1f(glGetUniformLocation(program, "snowLevel"), globalSnowLevel);

    glm::mat4 projection = Core::createPerspectiveMatrix(0.1f, 100.0f, 1.0f);
    glm::mat4 view = Core::createViewMatrix(cameraPos, cameraFront, cameraUp);

    // Macierz modelu - dopasowanie skali i pozycji dla nowego modelu
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, -6.0f, 0.0f));
    model = glm::scale(model, glm::vec3(30.0f, 30.0f, 30.0f));

    glm::mat4 transformation = projection * view * model;

    GLuint transLoc = glGetUniformLocation(program, "transformation");
    glUniformMatrix4fv(transLoc, 1, GL_FALSE, &transformation[0][0]);

    Core::drawVAOIndexedUInt(vaoMatterhorn, numIndicesMatterhorn);

    if (pathfinder) {
      pathfinder->render(view, projection);
    }

    if (enableSnowSimulation && snowSim) {
      snowSim->render(view, projection);
    }
  }

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  glfwSwapBuffers(window);
}

// Główna pętla
void renderLoop(GLFWwindow *window) {
  while (!glfwWindowShouldClose(window)) {
    processInput(window);
    renderScene(window);
    glfwPollEvents();
  }
}

void shutdown(GLFWwindow *window) {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  if (pathfinder) {
    delete pathfinder;
    pathfinder = nullptr;
  }
  if (snowSim) {
    delete snowSim;
    snowSim = nullptr;
  }

  // Czyszczenie zasobów
  if (vaoMatterhorn != 0) {
    Core::deleteVAO(vaoMatterhorn);
  }
  if (program != 0) {
    Core::Shader_Loader shaderLoader;
    shaderLoader.DeleteProgram(program);
  }
  if (snowProgram != 0) {
    Core::Shader_Loader shaderLoader;
    shaderLoader.DeleteProgram(snowProgram);
  }
}

int main(int argc, char **argv) {
  // Inicjalizacja GLFW
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return -1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  // Tworzenie okna
  GLFWwindow *window = glfwCreateWindow(
      800, 600, "Projekt - Grafika Komputerowa (Matterhorn)", NULL, NULL);
  if (window == NULL) {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  // Inicjalizacja GLEW
  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    std::cerr << "Failed to initialize GLEW" << std::endl;
    return -1;
  }

  init(window);
  renderLoop(window);
  shutdown(window);
  glfwTerminate();

  return 0;
}
