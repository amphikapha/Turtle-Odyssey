#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Player.h"
#include "Car.h"
#include "AudioManager.h"

#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <windows.h>
#include <gdiplus.h>
#include <cmath>
#include <set>
#include <algorithm>
#pragma comment(lib, "gdiplus.lib")

// Settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Input - Initialize arrays
bool keys[1024] = { false };
bool keysProcessed[1024] = { false };

// Audio manager (global for key callbacks)
AudioManager* g_audioManager = nullptr;

// Camera - อยู่ด้านหลังและสูงขึ้น
Camera camera(glm::vec3(0.0f, 6.0f, 12.0f));

// Game state
bool gameOver = false;
int score = 0;
float lastCarSpawnZ = 0.0f; // Track Z position where we last spawned cars
const float CAR_SPAWN_INTERVAL = 15.0f; // Spawn new cars every 15 units forward
const float CAR_DESPAWN_DISTANCE = 80.0f; // Remove cars that are far behind

// Function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void processInput(GLFWwindow* window, Player* player, AudioManager* audioManager);
unsigned int createGroundPlane();
void renderGround(unsigned int VAO, Shader* shader, glm::mat4 view, glm::mat4 projection);
unsigned int loadTexture(const char* path);
void resetGame(Player*& player, std::vector<Car*>& cars, std::set<int>& heartZonesUsed, std::vector<GameObject*>& hearts, int& playerHearts, int& score);

int main()
{
    // Initialize random seed
    srand(static_cast<unsigned>(time(0)));

    // Initialize GDI+
    using namespace Gdiplus;
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

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

    // Initialize audio system
    AudioManager audioManager;
    if (!audioManager.Initialize()) {
        std::cerr << "Warning: Failed to initialize audio system" << std::endl;
    }
    g_audioManager = &audioManager;

    // Build and compile shaders
    Shader shader("shaders/vertex_shader.glsl", "shaders/fragment_shader.glsl");

    // Create game objects
    Player* player = new Player(glm::vec3(0.0f, 0.5f, 15.0f));
    
    // Set camera to follow player from start
    camera.FollowTarget(player->position);
    
    // Create cars in different lanes
    std::vector<Car*> cars;
    // Use 10 lanes (wider street with many lanes)
    const int NUM_LANES = 10;
    // Increase lane width so lanes don't overlap visually
    const float LANE_WIDTH = 6.0f;
    // Each texture zone size (must match ground rendering later)
    // Reduced to 3/4 of original so zones change sooner (was 60 -> now 45)
    const float TEXTURE_ZONE_SIZE = 40.0f; // Each zone is 45 units

    // Helper: get the Z center of the next street zone (we will treat zone index mod 3 == 2 as street)
    const int STREET_ZONE_MOD = 2; // 0=grass,1=lake,2=street (we want start on grass)

    auto mod3 = [](int v) {
        int m = v % 3;
        if (m < 0) m += 3;
        return m;
    };

    // Return the integer zone index (floor-based) of the next street zone at least minAheadZones ahead
    auto getNearestStreetZoneIndex = [&](float referenceZ, int minAheadZones = 1) {
        int baseZone = static_cast<int>(std::floor(-referenceZ / TEXTURE_ZONE_SIZE));
        int targetZone = baseZone + minAheadZones; // choose a zone that is at least this far ahead
        while (mod3(targetZone) != STREET_ZONE_MOD) ++targetZone;
        return targetZone;
    };

    auto getNearestStreetZoneCenter = [&](float referenceZ, int minAheadZones = 1) {
        int zone = getNearestStreetZoneIndex(referenceZ, minAheadZones);
        float zoneCenterZ = - (zone * TEXTURE_ZONE_SIZE + TEXTURE_ZONE_SIZE * 0.5f);
        return zoneCenterZ;
    };

    // Hearts (life pickups)
    int playerHearts = 1; // player starts with one heart

    // Load bridge texture
    unsigned int bridgeTexture = loadTexture("assets/Bridge/textures/istockphoto-1145602814-170667a.jpg");

    // Load heart model (try OBJ then FBX)
    Model* heartModel = new Model();
    bool heartModelLoaded = heartModel->loadModel("assets/22_ Heart/Heart.obj");
    if (!heartModelLoaded) {
        heartModelLoaded = heartModel->loadModel("assets/22_ Heart/Heart.fbx");
        if (!heartModelLoaded) {
            delete heartModel;
            heartModel = nullptr;
            std::cout << "Warning: Could not load heart model; will fallback to simple marker." << std::endl;
        }
    }

    // One heart per grass zone: track which grass zones already had a heart spawned or consumed
    std::set<int> heartZonesUsed;
    std::vector<GameObject*> hearts; // GameObject per heart for collision and transform

    auto spawnHeartInZone = [&](int zoneIndex) {
        if (heartZonesUsed.count(zoneIndex)) return; // already used
        heartZonesUsed.insert(zoneIndex);

        float z = - (zoneIndex * TEXTURE_ZONE_SIZE + TEXTURE_ZONE_SIZE * 0.5f);
        GameObject* h = new GameObject();
        h->position = glm::vec3(0.0f, 4.0f, z); // slightly above ground
        h->scale = glm::vec3(1.0f, 1.0f, 1.0f);
        // If no model, shrink marker
        if (!heartModel) h->scale = glm::vec3(0.5f);
        hearts.push_back(h);
    };

    // Spawn initial hearts for upcoming grass zones (a few ahead)
    int startZone = static_cast<int>(std::floor(-player->position.z / TEXTURE_ZONE_SIZE));
    std::cout << "Player start Z: " << player->position.z << ", startZone: " << startZone << std::endl;

    // Find the nearest upcoming lake zone so we can bias raft spawns toward the near shore
    int nearestLakeZone = startZone - 1000;
    float nearestLakeDist = 1e12f;
    for (int zz = startZone; zz <= startZone + 8; ++zz) {
        if (mod3(zz) == 1) {
            float zCenter = - (zz * TEXTURE_ZONE_SIZE + TEXTURE_ZONE_SIZE * 0.5f);
            float dist = fabs(zCenter - player->position.z);
            if (dist < nearestLakeDist) {
                nearestLakeDist = dist;
                nearestLakeZone = zz;
            }
        }
    }
    std::cout << "Nearest lake zone: " << nearestLakeZone << std::endl;

    for (int z = startZone; z <= startZone + 8; ++z) {
        int zoneMod = mod3(z);
        std::cout << "Zone " << z << ": mod3=" << zoneMod << std::endl;
        if (zoneMod == 0) { // grass zone
            spawnHeartInZone(z);
        }
    }
    
    // Start playing background music
    std::string musicPath = "assets/Zambolino - Beautiful Day (freetouse.com).mp3";
    if (!audioManager.PlayMusic(musicPath)) {
        std::cerr << "Warning: Could not load music from " << musicPath << std::endl;
        std::cerr << "Note: MP3 files are not supported. Please convert to WAV format." << std::endl;
        std::cerr << "You can convert using: ffmpeg -i input.mp3 -acodec pcm_s16le -ar 44100 output.wav" << std::endl;
    }
    
    // Create multiple cars - กระจายตามเลน (Z axis) พร้อมการเว้นช่องห่าง
    for (int i = 0; i < 8; i++) {
        // Map i to lane indices in range roughly centered around 0. For NUM_LANES=10 this yields -5..4
        int lane = (i % NUM_LANES) - (NUM_LANES / 2);
        bool movingRight = (rand() % 2 == 0);
        Car* car = new Car(lane, LANE_WIDTH, movingRight);
        
        // สเปรดรถให้กระจายตัวตามแกน X เพื่อลดการซ้อนกัน (เพิ่มระยะห่างให้มากขึ้น)
        // Increase X spacing so cars don't pile up in a single lateral line
        if (movingRight) {
            car->position.x = -50.0f + (i * 18.0f); // wider spacing
        } else {
            car->position.x = 50.0f - (i * 18.0f);
        }
        
        // ปรับ Y ให้รถแต่ละคันอยู่ที่ความสูงเดียวกัน เพื่อไม่ให้ซ้อนตามแกน Y
        car->position.y = 0.3f + (lane * 0.1f); // ปรับความสูงตามแนวเล็กน้อยตามแต่ละเลน
        
        // Force the car to spawn in a street zone only (no cars on grass/lake)
        // spread cars across several street zones ahead of the player to avoid overlap
    // Distribute initial cars across the first few street zones so early streets aren't overcrowded
    int extraAheadZones = 1 + (i / 3); // spread every 3 cars into the next zone
    // Determine zone index and place car in that street zone's lane position
    int initZone = getNearestStreetZoneIndex(player->position.z, extraAheadZones);
    car->position.z = - (initZone * TEXTURE_ZONE_SIZE + TEXTURE_ZONE_SIZE * 0.5f) + lane * LANE_WIDTH;

        cars.push_back(car);
    }

    // Create ground
    unsigned int groundVAO = createGroundPlane();
    
    // Load three textures for ground cycling
    unsigned int grassTexture = loadTexture("assets/textures/grass.jpg");
    unsigned int lakeTexture = loadTexture("assets/textures/lake.png");
    unsigned int streetTexture = loadTexture("assets/textures/street.jpg");

    // Store textures in array for easy access
    // We want the world to start with grass, then lake, then street repeating.
    // So index 0 = grass, 1 = lake, 2 = street
    unsigned int groundTextures[3] = { grassTexture, lakeTexture, streetTexture };
    // current texture zone index (0=grass,1=lake,2=street)
    int currentTextureZone = 0;

    // Lighting
    glm::vec3 lightPos(0.0f, 20.0f, 0.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 0.9f);

    std::cout << "=== Turtle Odyssey ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "W/A/S/D - Move" << std::endl;
    std::cout << "SPACE - Jump" << std::endl;
    std::cout << "LEFT SHIFT - Speed Boost (5 sec)" << std::endl;
    std::cout << "[ ] - Decrease/Increase Volume" << std::endl;
    std::cout << "R - Restart Game (when game over)" << std::endl;
    std::cout << "ESC - Exit" << std::endl;
    std::cout << "Goal: Survive as long as possible!" << std::endl;
    std::cout << "======================" << std::endl;

    // Game loop
    while (!glfwWindowShouldClose(window))
    {
        // Per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input
        processInput(window, player, &audioManager);

        // Check for reset key when game is over
        if (gameOver && keys[GLFW_KEY_R]) {
            resetGame(player, cars, heartZonesUsed, hearts, playerHearts, score);
        }

        // Update audio system
        audioManager.Update();

        // Update
        if (!gameOver) {
            player->Update(deltaTime);

            // Update ground texture zone based on player position
            // Calculate which zone the player is in (0=grass, 1=lake, 2=street, cycles)
            int zoneIndex = static_cast<int>(std::floor(-player->position.z / TEXTURE_ZONE_SIZE));
            currentTextureZone = zoneIndex % 3; if (currentTextureZone < 0) currentTextureZone += 3; // Cycle through 0,1,2

            // Spawn hearts ahead on newly discovered grass zones (ensure one per grass zone)
            int playerBaseZone = static_cast<int>(std::floor(-player->position.z / TEXTURE_ZONE_SIZE));
            for (int z = playerBaseZone; z <= playerBaseZone + 12; ++z) {
                if (mod3(z) == 0 && heartZonesUsed.count(z) == 0) {
                    // small chance to spawn (so not every grass has one) - set to always spawn for now
                    spawnHeartInZone(z);
                }
            }

            // Spawn bridges ahead on newly discovered lake zones (ensure one per lake zone)
            std::set<int> existingBridgeZones;
            for (int z = playerBaseZone; z <= playerBaseZone + 12; ++z) {
                if (mod3(z) == 1) {
                    existingBridgeZones.insert(z);
                }
            }

            // Spawn new cars as player moves forward
            if (player->position.z < lastCarSpawnZ - CAR_SPAWN_INTERVAL) {
                lastCarSpawnZ = player->position.z;
                
                // Spawn 2-3 new cars at different lanes (reduced from 3-5)
                int numNewCars = 2 + (rand() % 2); // 2-3 cars
                for (int i = 0; i < numNewCars; ++i) {
                    int lane = (i % NUM_LANES) - (NUM_LANES / 2); // support even-numbered lanes
                    bool movingRight = (rand() % 2 == 0);
                    Car* newCar = new Car(lane, LANE_WIDTH, movingRight);
                    
                    // Spawn further ahead to avoid pop-in
                    if (movingRight) {
                        newCar->position.x = -50.0f + (i * 15.0f);
                    } else {
                        newCar->position.x = 50.0f - (i * 15.0f);
                    }
                    // Place spawn in a street zone ahead of the player to ensure cars only appear on streets
                        int extraZones = 2 + (rand() % 3); // 2..4 zones ahead for variety

                        // Determine the target street zone index and cap cars per zone to avoid overcrowding
                        int targetZone = getNearestStreetZoneIndex(player->position.z, extraZones);
                        // Count existing cars in that zone
                        int existingInZone = 0;
                        for (auto c : cars) {
                            int cz = static_cast<int>(std::floor(-c->position.z / TEXTURE_ZONE_SIZE));
                            if (cz == targetZone) existingInZone++;
                        }

                        const int MAX_CARS_PER_ZONE = NUM_LANES * 2; // allow up to 2 cars per lane
                        if (existingInZone >= MAX_CARS_PER_ZONE) {
                            delete newCar; // skip spawn if zone full
                            continue;
                        }

                        // Place new car in the chosen street zone but offset by lane so each lane gets cars
                        newCar->position.z = - (targetZone * TEXTURE_ZONE_SIZE + TEXTURE_ZONE_SIZE * 0.5f) + lane * LANE_WIDTH;
                        newCar->position.y = 0.3f + (lane * 0.1f);

                        cars.push_back(newCar);
                }
                std::cout << "New cars spawned! Total cars: " << cars.size() << std::endl;
            }

            // Update hearts: check collection first
            for (int h = (int)hearts.size() - 1; h >= 0; --h) {
                GameObject* hg = hearts[h];
                // Simple bobbing animation for visibility
                hg->position.y = 4.0f + sinf(static_cast<float>(glfwGetTime()) * 2.0f) * 0.2f;
                // Check collision with player
                if (player->CheckCollision(hg, 0.0f)) {
                    playerHearts++;
                    std::cout << "Picked up a heart! Hearts=" << playerHearts << std::endl;
                    delete hg;
                    hearts.erase(hearts.begin() + h);
                }
            }

            // Update logs: move and wrap/spawn
            // (No longer needed - bridges are now part of ground texture)

            // Check if player is standing on bridge (bridge is rendered as ground texture in lake zones)
            int playerBaseZone2 = static_cast<int>(std::floor(-player->position.z / TEXTURE_ZONE_SIZE));
            bool playerOnBridge = (mod3(playerBaseZone2) == 1); // In lake zone = on bridge area
            bool playerInWater = false;

            // If player is over water area but not jumping, they die
            if (mod3(playerBaseZone2) == 1) { // lake zone
                // Check if player is far from center (where bridge is) - if so, in water
                // Bridge width is 16 units in shader (±8 units from center), so safe range is ±8 units
                if (fabsf(player->position.x) > 8.5f && !player->isJumping) { // Outside bridge area
                    playerInWater = true;
                }
                
                if (playerInWater) {
                    gameOver = true;
                    std::cout << "\n=== You fell into the water! ===" << std::endl;
                }
            }

            // Update cars and remove ones that are too far behind
            // Use reverse iterator to safely remove elements
            for (int i = (int)cars.size() - 1; i >= 0; --i) {
                cars[i]->Update(deltaTime);

                // Check collision with player
                // Use a small positive margin so the player dies when lightly touching the car
                if (player->CheckCollision(cars[i], 1.f)) {
                    if (playerHearts > 0) {
                        playerHearts--;
                        std::cout << "Hit by car! Hearts left=" << playerHearts << std::endl;
                        // remove car to avoid repeated hits
                        delete cars[i];
                        cars.erase(cars.begin() + i);
                        continue;
                    } else {
                        gameOver = true;
                        std::cout << "\n=== GAME OVER ===" << std::endl;
                        std::cout << "You got hit by a car!" << std::endl;
                        std::cout << "Final Score: " << score << std::endl;
                        std::cout << "Press ESC to exit" << std::endl;
                    }
                }
                
                // Safety: if a car somehow ends up on a non-street zone, remove it
                int carZoneIdx = static_cast<int>(std::floor(-cars[i]->position.z / TEXTURE_ZONE_SIZE));
                int carZoneMod = carZoneIdx % 3; if (carZoneMod < 0) carZoneMod += 3;
                // street zones now use mod == STREET_ZONE_MOD
                if (carZoneMod != STREET_ZONE_MOD) {
                    delete cars[i];
                    cars.erase(cars.begin() + i);
                    continue;
                }

                // Remove cars that are too far behind (cleanup)
                if (cars[i]->position.z > player->position.z + CAR_DESPAWN_DISTANCE) {
                    delete cars[i];
                    cars.erase(cars.begin() + i);
                }
            }

            // Update score based on forward progress
            // คะแนนเพิ่มเมื่อเดินไปข้างหน้า (Z ลดลง) - ไม่มีขีดจำกัด
            int newScore = static_cast<int>(-player->position.z / 2.0f);
            if (newScore > score) {
                score = newScore;
                std::cout << "Distance: " << score * 2 << " meters | Cars active: " << cars.size() << std::endl;
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
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 200.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);

    // Ensure default object texture uniform points to texture unit 0
    shader.setInt("ourTexture", 0);

        // Lighting uniforms
        shader.setVec3("lightPos", lightPos);
        shader.setVec3("lightColor", lightColor);
        shader.setVec3("viewPos", camera.Position);

        // Render multiple ground sections with different textures
        // This creates a continuous visible transition between terrain types
    const float SECTION_SIZE = TEXTURE_ZONE_SIZE; // Size of each ground section (match zone size)
        
        // Bind all three ground textures to texture units 0..2 and inform shader
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, groundTextures[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, groundTextures[1]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, groundTextures[2]);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, bridgeTexture);
    shader.setInt("groundTex[0]", 0);
    shader.setInt("groundTex[1]", 1);
    shader.setInt("groundTex[2]", 2);
    shader.setInt("bridgeTexture", 3);
        shader.setFloat("textureZoneSize", TEXTURE_ZONE_SIZE);
        shader.setBool("useGroundTextures", true);
        shader.setBool("showBridgeInLake", true);  // Show wood bridge texture in lake zones

        // Render multiple sections centered around the player's current zone so the ground follows the player
        // Calculate the base zone index the player is currently in (zones use negative Z forward)
        int baseZone = static_cast<int>(-player->position.z / SECTION_SIZE);
        // Render 9 sections: 4 behind, current, 4 ahead relative to player's zone
        for (int i = -4; i <= 4; ++i) {
            int sectionZone = baseZone + i;
            // compute the world Z coordinate at the center of that section
            float sectionCenterZ = - (sectionZone * SECTION_SIZE + SECTION_SIZE * 0.5f);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, sectionCenterZ));
            shader.setMat4("model", model);
            shader.setVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f)); // White - let texture show

            // Render this section (fragment shader will pick and blend textures based on FragPos.z)
            glBindVertexArray(groundVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        }

        // Turn off ground-specific texturing for other objects
        shader.setBool("useGroundTextures", false);

        // Bind default texture unit for other objects (will use white if no texture loaded)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0); // White texture as default

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

        // Render hearts (life pickups)
        for (auto h : hearts) {
            shader.setMat4("model", h->GetModelMatrix());
            if (heartModel) {
                // Force a solid red color for the heart model (override any model textures)
                shader.setVec3("objectColor", glm::vec3(1.0f, 0.0f, 0.0f));
                shader.setBool("overrideColor", true);
                heartModel->Draw();
                shader.setBool("overrideColor", false);
            } else {
                // Fallback: draw a simple red marker using GameObject's mesh
                shader.setVec3("objectColor", glm::vec3(1.0f, 0.0f, 0.0f));
                h->Draw();
            }
        }

        // Bridges are now rendered as ground texture in lake zones instead of separate objects

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    delete player;
    for (size_t i = 0; i < cars.size(); ++i) {
        delete cars[i];
    }
    cars.clear();
    glDeleteVertexArrays(1, &groundVAO);
    glDeleteTextures(3, groundTextures);

    // Shutdown GDI+
    Gdiplus::GdiplusShutdown(gdiplusToken);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window, Player* player, AudioManager* audioManager)
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

    // Volume control with [ and ]
    if (key == GLFW_KEY_LEFT_BRACKET && action == GLFW_PRESS) {
        if (g_audioManager) {
            ALfloat currentVolume;
            alGetListenerf(AL_GAIN, &currentVolume);
            currentVolume = (currentVolume - 0.1f < 0.0f) ? 0.0f : (currentVolume - 0.1f);
            alListenerf(AL_GAIN, currentVolume);
            std::cout << "Volume: " << static_cast<int>(currentVolume * 100) << "%" << std::endl;
        }
    }
    if (key == GLFW_KEY_RIGHT_BRACKET && action == GLFW_PRESS) {
        if (g_audioManager) {
            ALfloat currentVolume;
            alGetListenerf(AL_GAIN, &currentVolume);
            currentVolume = (currentVolume + 0.1f > 1.0f) ? 1.0f : (currentVolume + 0.1f);
            alListenerf(AL_GAIN, currentVolume);
            std::cout << "Volume: " << static_cast<int>(currentVolume * 100) << "%" << std::endl;
        }
    }

    // Restart game when 'R' is pressed (only when game is over)
    if (key == GLFW_KEY_R && action == GLFW_PRESS && gameOver) {
        std::cout << "Restarting game..." << std::endl;
        gameOver = false;
    }

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
    // Create a ground section (20 units long for terrain transitions)
    // Each section will be rendered multiple times at different Z positions
    float width = 500.0f;   // Wide enough for all lanes
    float depth = 40.0f;    // Length of one section (matches SECTION_SIZE / TEXTURE_ZONE_SIZE) - reduced to 3/4
    
    // Texture coordinate scale - higher values = more tiling = smaller/more detailed textures
    // Scale based on the actual dimensions to avoid stretching
    float texScaleZ = 8.0f;  // Repeat texture 8 times along Z (depth)
    float texScaleX = texScaleZ * (width / depth);  // Scale X proportionally to avoid stretching
    
    float groundVertices[] = {
        // positions                texCoords        
        -width, 0.0f, -depth,      0.0f, 0.0f,
         width, 0.0f, -depth,      texScaleX, 0.0f,
         width, 0.0f,  depth,      texScaleX, texScaleZ,

         width, 0.0f,  depth,      texScaleX, texScaleZ,
        -width, 0.0f,  depth,      0.0f, texScaleZ,
        -width, 0.0f, -depth,      0.0f, 0.0f,
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), groundVertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

void renderGround(unsigned int VAO, Shader* shader, glm::mat4 view, glm::mat4 projection)
{
    glm::mat4 model = glm::mat4(1.0f);
    shader->setMat4("model", model);
    shader->setVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f)); // White - let texture show

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

unsigned int loadTexture(const char* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    // Use GDI+ to load the image
    using namespace Gdiplus;
    
    // Convert char* to wchar_t*
    int len = strlen(path);
    wchar_t* widePath = new wchar_t[len + 1];
    mbstowcs(widePath, path, len + 1);
    
    Image* image = new Image(widePath);
    delete[] widePath;
    
    if (image->GetLastStatus() != Ok || image->GetWidth() == 0 || image->GetHeight() == 0) {
        std::cout << "Warning: Failed to load image: " << path << std::endl;
        delete image;
        
        // Create fallback texture - solid color based on filename
        std::string filename(path);
        unsigned char* data = new unsigned char[128 * 128 * 3];
        
        if (filename.find("street") != std::string::npos) {
            for (int i = 0; i < 128 * 128 * 3; i += 3) {
                data[i] = 60; data[i + 1] = 60; data[i + 2] = 60; // Gray
            }
        } else if (filename.find("grass") != std::string::npos) {
            for (int i = 0; i < 128 * 128 * 3; i += 3) {
                data[i] = 34; data[i + 1] = 139; data[i + 2] = 34; // Green
            }
        } else if (filename.find("lake") != std::string::npos) {
            for (int i = 0; i < 128 * 128 * 3; i += 3) {
                data[i] = 30; data[i + 1] = 144; data[i + 2] = 255; // Blue
            }
        }
        
    glBindTexture(GL_TEXTURE_2D, textureID);
    // Ensure proper alignment for tightly-packed data
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        delete[] data;
        return textureID;
    }
    
    UINT width = image->GetWidth();
    UINT height = image->GetHeight();
    
    // Create an RGBA bitmap to preserve alpha if present
    Bitmap* bitmap = new Bitmap(width, height, PixelFormat32bppARGB);
    Graphics* graphics = Graphics::FromImage(bitmap);
    graphics->DrawImage(image, 0, 0);
    delete graphics;

    // Lock bitmap bits for reading (32bpp ARGB)
    BitmapData bitmapData;
    Rect rect(0, 0, width, height);
    bitmap->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bitmapData);

    unsigned char* pixels = static_cast<unsigned char*>(bitmapData.Scan0);
    unsigned char* textureData = new unsigned char[width * height * 4];

    // Copy and convert BGRA -> RGBA (GDI+ uses BGRA ordering for 32bpp)
    for (UINT y = 0; y < height; ++y) {
        for (UINT x = 0; x < width; ++x) {
            int src_idx = (y * bitmapData.Stride) + (x * 4);
            int dst_idx = (y * width + x) * 4;
            // BGRA -> RGBA
            textureData[dst_idx + 0] = pixels[src_idx + 2]; // R
            textureData[dst_idx + 1] = pixels[src_idx + 1]; // G
            textureData[dst_idx + 2] = pixels[src_idx + 0]; // B
            textureData[dst_idx + 3] = pixels[src_idx + 3]; // A
        }
    }

    bitmap->UnlockBits(&bitmapData);

    // Upload to OpenGL (use RGBA)
    glBindTexture(GL_TEXTURE_2D, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Cleanup
    delete[] textureData;
    delete bitmap;
    delete image;

    std::cout << "Successfully loaded texture: " << path << " (" << width << "x" << height << ")" << std::endl;

    return textureID;
}

void resetGame(Player*& player, std::vector<Car*>& cars, std::set<int>& heartZonesUsed, std::vector<GameObject*>& hearts, int& playerHearts, int& score)
{
    // Clean up old player
    if (player != nullptr) {
        delete player;
    }

    // Clean up old cars
    for (size_t i = 0; i < cars.size(); ++i) {
        delete cars[i];
    }
    cars.clear();

    // Clean up old hearts
    for (size_t i = 0; i < hearts.size(); ++i) {
        delete hearts[i];
    }
    hearts.clear();

    // Reset game state variables
    score = 0;
    playerHearts = 1;
    lastCarSpawnZ = 0.0f;
    heartZonesUsed.clear();

    // Recreate player at starting position
    player = new Player(glm::vec3(0.0f, 0.5f, 15.0f));
    camera.FollowTarget(player->position);

    // Helper lambda functions (same as in main)
    const float TEXTURE_ZONE_SIZE = 40.0f;
    const int NUM_LANES = 10;
    const float LANE_WIDTH = 6.0f;
    const int STREET_ZONE_MOD = 2;

    auto mod3 = [](int v) {
        int m = v % 3;
        if (m < 0) m += 3;
        return m;
    };

    auto getNearestStreetZoneIndex = [&](float referenceZ, int minAheadZones = 1) {
        int baseZone = static_cast<int>(std::floor(-referenceZ / TEXTURE_ZONE_SIZE));
        int targetZone = baseZone + minAheadZones;
        while (mod3(targetZone) != STREET_ZONE_MOD) ++targetZone;
        return targetZone;
    };

    auto spawnHeartInZone = [&](int zoneIndex) {
        if (heartZonesUsed.count(zoneIndex)) return;
        heartZonesUsed.insert(zoneIndex);

        float z = - (zoneIndex * TEXTURE_ZONE_SIZE + TEXTURE_ZONE_SIZE * 0.5f);
        GameObject* h = new GameObject();
        h->position = glm::vec3(0.0f, 4.0f, z);
        h->scale = glm::vec3(1.0f, 1.0f, 1.0f);
        hearts.push_back(h);
    };

    // Recreate hearts in starting zones
    int startZone = static_cast<int>(std::floor(-player->position.z / TEXTURE_ZONE_SIZE));
    for (int z = startZone; z <= startZone + 8; ++z) {
        if (mod3(z) == 0) {
            spawnHeartInZone(z);
        }
    }

    // Recreate initial cars
    for (int i = 0; i < 8; i++) {
        int lane = (i % NUM_LANES) - (NUM_LANES / 2);
        bool movingRight = (rand() % 2 == 0);
        Car* car = new Car(lane, LANE_WIDTH, movingRight);
        
        if (movingRight) {
            car->position.x = -50.0f + (i * 18.0f);
        } else {
            car->position.x = 50.0f - (i * 18.0f);
        }
        
        car->position.y = 0.3f + (lane * 0.1f);
        
        int extraAheadZones = 1 + (i / 3);
        int initZone = getNearestStreetZoneIndex(player->position.z, extraAheadZones);
        car->position.z = - (initZone * TEXTURE_ZONE_SIZE + TEXTURE_ZONE_SIZE * 0.5f) + lane * LANE_WIDTH;

        cars.push_back(car);
    }

    std::cout << "=== GAME RESET ===" << std::endl;
    std::cout << "Lives: " << playerHearts << " | Score: " << score << std::endl;
}
