#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <sys/stat.h>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Camera.h"
#include "objload.h"

// Zmienne globalne
GLuint program;
GLuint vaoMatterhorn;
int numIndicesMatterhorn = 0;
GLuint textureMatterhorn;

// Kamera
glm::vec3 cameraPos   = glm::vec3(0.0f, 1.2f, 2.5f);
glm::vec3 cameraFront = glm::normalize(glm::vec3(0.0f, 0.2f, 0.0f) - cameraPos);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float yaw   = -90.0f;
float pitch = -21.8f;

// Funkcja sprawdzająca czy plik istnieje
inline bool file_exists(const std::string& name) {
    struct stat buffer;   
    return (stat(name.c_str(), &buffer) == 0); 
}

// Funkcja pomocnicza do pobierania właściwej ścieżki pliku (wspiera uruchamianie z katalogu głównego i build/)
std::string get_path(const std::string& relative_path) {
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
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Obsługa wejścia
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = static_cast<float>(0.5 * deltaTime);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
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
void init(GLFWwindow* window)
{
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glClearColor(0.08f, 0.12f, 0.18f, 1.0f); // Klimatyczne, ciemnoniebieskie tło nieba
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

    // Wczytanie modelu Matterhorn
    std::string modelPath = get_path("models/matterhorn.obj");
    std::cout << "Loading model: " << modelPath << std::endl;
    obj::Model model = obj::loadModelFromFile(modelPath);
    
    if (model.faces.empty()) {
        std::cerr << "Failed to load model faces or model is empty!" << std::endl;
    } else {
        vaoMatterhorn = Core::initVAOForModel(model);
        numIndicesMatterhorn = model.faces.begin()->second.size();
        std::cout << "Loaded Matterhorn model successfully with " << numIndicesMatterhorn << " indices." << std::endl;
    }

    // Wczytywanie tekstury
    std::string texPath = get_path("models/matterhorn-disneyland-pbr-model-low-poly/textures/M_low_poly_phong1SG_BaseColor.png");
    glGenTextures(1, &textureMatterhorn);
    glBindTexture(GL_TEXTURE_2D, textureMatterhorn); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrChannels;
    unsigned char *data = stbi_load(texPath.c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
}

// Funkcja renderująca scenę
void renderScene(GLFWwindow* window)
{
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (numIndicesMatterhorn > 0) {
        glUseProgram(program);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureMatterhorn);
        glUniform1i(glGetUniformLocation(program, "textureSampler"), 0);

        glm::mat4 projection = Core::createPerspectiveMatrix(0.1f, 100.0f, 1.0f);
        glm::mat4 view = Core::createViewMatrix(cameraPos, cameraFront, cameraUp);

        // Macierz modelu - dopasowanie skali i pozycji dla nowego modelu
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
        model = glm::translate(model, glm::vec3(0.0f, -1.7f, 0.0f));

        glm::mat4 transformation = projection * view * model;

        GLuint transLoc = glGetUniformLocation(program, "transformation");
        glUniformMatrix4fv(transLoc, 1, GL_FALSE, &transformation[0][0]);

        Core::drawVAOIndexedUShort(vaoMatterhorn, numIndicesMatterhorn);
    }

    glfwSwapBuffers(window);
}

// Główna pętla
void renderLoop(GLFWwindow* window)
{
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);
        renderScene(window);
        glfwPollEvents();
    }
}

void shutdown(GLFWwindow* window)
{
    // Czyszczenie zasobów
    if (vaoMatterhorn != 0) {
        Core::deleteVAO(vaoMatterhorn);
    }
    if (program != 0) {
        Core::Shader_Loader shaderLoader;
        shaderLoader.DeleteProgram(program);
    }
}

int main(int argc, char ** argv)
{
    // Inicjalizacja GLFW
    if (!glfwInit())
    {
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
    GLFWwindow* window = glfwCreateWindow(800, 600, "Projekt - Grafika Komputerowa (Matterhorn)", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Inicjalizacja GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    init(window);
    renderLoop(window);
    shutdown(window);
    glfwTerminate();

    return 0;
}

