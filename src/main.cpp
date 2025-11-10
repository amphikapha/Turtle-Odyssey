#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Player.h"
#include "Car.h"

#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>

// Settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Input - Initialize arrays
bool keys[1024] = { false };
bool keysProcessed[1024] = { false };

// Camera - อยู่ด้านหลังและสูงขึ้น
Camera camera(glm::vec3(0.0f, 6.0f, 12.0f));

// Game state
bool gameOver = false;
int score = 0;

// Function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void processInput(GLFWwindow* window, Player* player);
unsigned int createGroundPlane();
void renderGround(unsigned int VAO, Shader* shader, glm::mat4 view, glm::mat4 projection);

int main()
{
    // Initialize random seed
    srand(static_cast<unsigned>(time(0)));

    // GLFW initialization
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Turtle Odyssey", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    // GLAD: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // OpenGL configuration
    glEnable(GL_DEPTH_TEST);

    // Build and compile shaders
    Shader shader("shaders/vertex_shader.glsl", "shaders/fragment_shader.glsl");

    // Create game objects
    Player* player = new Player(glm::vec3(0.0f, 0.5f, 15.0f));
    
    // Set camera to follow player from start
    camera.FollowTarget(player->position);
    
    // Create cars in different lanes
    std::vector<Car*> cars;
    const int NUM_LANES = 5;
    const float LANE_WIDTH = 3.0f;
    
    // Create multiple cars - กระจายตามเลน (Z axis) พร้อมการเว้นช่องห่าง
    for (int i = 0; i < 8; i++) {
        int lane = (i % NUM_LANES) - 2; // Lanes from -2 to 2 (Z axis)
        bool movingRight = (rand() % 2 == 0);
        Car* car = new Car(lane, LANE_WIDTH, movingRight);
        
        // สเปรดรถให้กระจายตัวตามแกน X เพื่อลดการซ้อนกัน (เพิ่มระยะห่างให้มากขึ้น)
        if (movingRight) {
            car->position.x = -30.0f + (i * 12.0f); // เว้นห่าง 12 หน่วยต่อคัน (เพิ่มจาก 8)
        } else {
            car->position.x = 30.0f - (i * 12.0f);
        }
        
        // ปรับ Y ให้รถแต่ละคันอยู่ที่ความสูงเดียวกัน เพื่อไม่ให้ซ้อนตามแกน Y
        car->position.y = 0.3f + (lane * 0.1f); // ปรับความสูงตามแนวเล็กน้อยตามแต่ละเลน
        
        // ไม่ต้องปรับ Z อีก เพราะ constructor จัดการแล้ว
        cars.push_back(car);
    }

    // Create ground
    unsigned int groundVAO = createGroundPlane();

    // Lighting
    glm::vec3 lightPos(0.0f, 20.0f, 0.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 0.9f);

    std::cout << "=== Turtle Odyssey ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "W/A/S/D - Move" << std::endl;
    std::cout << "SPACE - Jump" << std::endl;
    std::cout << "LEFT SHIFT - Speed Boost (5 sec)" << std::endl;
    std::cout << "ESC - Exit" << std::endl;
    std::cout << "Goal: Cross the road without getting hit!" << std::endl;
    std::cout << "======================" << std::endl;

    // Game loop
    while (!glfwWindowShouldClose(window))
    {
        // Per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input
        processInput(window, player);

        // Update
        if (!gameOver) {
            player->Update(deltaTime);

            // Update cars
            for (auto car : cars) {
                car->Update(deltaTime);

                // Check collision with player
                if (player->CheckCollision(car)) {
                    gameOver = true;
                    std::cout << "\n=== GAME OVER ===" << std::endl;
                    std::cout << "You got hit by a car!" << std::endl;
                    std::cout << "Final Score: " << score << std::endl;
                    std::cout << "Press ESC to exit" << std::endl;
                }
            }

            // Update score based on forward progress
            // คะแนนเพิ่มเมื่อเดินไปข้างหน้า (Z ลดลง)
            if (player->position.z < -score * 2.0f) {
                score++;
                std::cout << "Score: " << score << std::endl;
            }

            // Camera follows player
            camera.FollowTarget(player->position);
        }

        // Render
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f); // Sky blue
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Activate shader
        shader.use();

        // View/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);

        // Lighting uniforms
        shader.setVec3("lightPos", lightPos);
        shader.setVec3("lightColor", lightColor);
        shader.setVec3("viewPos", camera.Position);

        // Bind default texture unit (will use white if no texture loaded)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0); // White texture as default
        shader.setInt("ourTexture", 0);

        // Render ground
        renderGround(groundVAO, &shader, view, projection);

        // Render player
        shader.setMat4("model", player->GetModelMatrix());
        if (player->hasSpeedBoost) {
            shader.setVec3("objectColor", glm::vec3(0.5f, 1.0f, 0.5f)); // Bright green when boosted
        } else {
            shader.setVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f)); // White - ให้ texture แสดง
        }
        player->Draw();

        // Render cars
        for (auto car : cars) {
            shader.setMat4("model", car->GetModelMatrix());
            shader.setVec3("objectColor", car->color);
            car->Draw();
        }

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    delete player;
    for (auto car : cars) {
        delete car;
    }
    glDeleteVertexArrays(1, &groundVAO);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window, Player* player)
{
    if (gameOver) return;

    glm::vec3 movement(0.0f);

    // สลับแกน Z ให้ W = ไปข้างหน้า (เพิ่ม Z), S = ถอยหลัง (ลด Z)
    if (keys[GLFW_KEY_W])
        movement.z -= 1.0f;  // เดินไปข้างหน้า (ขึ้นบน)
    if (keys[GLFW_KEY_S])
        movement.z += 1.0f;  // ถอยหลัง (ลงล่าง)
    if (keys[GLFW_KEY_A])
        movement.x -= 1.0f;  // ไปซ้าย
    if (keys[GLFW_KEY_D])
        movement.x += 1.0f;  // ไปขวา

    if (glm::length(movement) > 0.0f) {
        movement = glm::normalize(movement);
        player->Move(movement, deltaTime);
    }

    // Space - Jump
    if (keys[GLFW_KEY_SPACE] && !keysProcessed[GLFW_KEY_SPACE]) {
        player->Jump();
        keysProcessed[GLFW_KEY_SPACE] = true;
    }

    // Shift - Speed Boost
    if (keys[GLFW_KEY_LEFT_SHIFT] && !keysProcessed[GLFW_KEY_LEFT_SHIFT]) {
        player->ActivateSpeedBoost();
        keysProcessed[GLFW_KEY_LEFT_SHIFT] = true;
        std::cout << "Speed Boost Activated! (5 seconds)" << std::endl;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE) {
            keys[key] = false;
            keysProcessed[key] = false;
        }
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

unsigned int createGroundPlane()
{
    // Create a large ground plane
    float groundVertices[] = {
        // positions          
        -50.0f, 0.0f, -50.0f,
         50.0f, 0.0f, -50.0f,
         50.0f, 0.0f,  50.0f,

         50.0f, 0.0f,  50.0f,
        -50.0f, 0.0f,  50.0f,
        -50.0f, 0.0f, -50.0f,
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), groundVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

void renderGround(unsigned int VAO, Shader* shader, glm::mat4 view, glm::mat4 projection)
{
    glm::mat4 model = glm::mat4(1.0f);
    shader->setMat4("model", model);
    shader->setVec3("objectColor", glm::vec3(0.3f, 0.3f, 0.3f)); // Gray road

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
