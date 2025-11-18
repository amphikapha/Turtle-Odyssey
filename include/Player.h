#ifndef PLAYER_H
#define PLAYER_H

#include "GameObject.h"
#include "Model.h"
#include <glad/glad.h>

class Player : public GameObject {
public:
    float moveSpeed;
    float jumpHeight;
    bool isJumping;
    float jumpVelocity;
    float gravity;
    bool hasSpeedBoost;
    float speedBoostTimer;
    Model* model;
    bool useModel;

    Player(glm::vec3 startPos) {
        position = startPos;
        position.y = 3.5f; // Adjust height so feet are on ground (not sinking)
        scale = glm::vec3(1.0f, 1.0f, 1.0f);
        rotation = glm::vec3(0.0f, 0.0f, 0.0f); // Face forward
        color = glm::vec3(1.0f, 1.0f, 1.0f);
        moveSpeed = 8.0f;
        jumpHeight = 2.0f;
        isJumping = false;
        jumpVelocity = 0.0f;
        gravity = -15.0f;
        hasSpeedBoost = false;
        speedBoostTimer = 0.0f;
        model = nullptr;
        useModel = false;

        // Try to load goblin model
        LoadModel("assets/goblin-3d-model-free/source/GoblinMutantSPDONEFINAL.fbx");
        
        // Fallback to cube if model not found
        if (!useModel) {
            CreateTurtleMesh();
        }
    }

    ~Player() {
        if (model != nullptr) {
            delete model;
        }
    }

    void LoadModel(const std::string& path) {
        model = new Model();
        useModel = model->loadModel(path);
        if (useModel) {
            std::cout << "Player model loaded from: " << path << std::endl;
        } else {
            std::cout << "Could not load player model, using fallback cube" << std::endl;
            delete model;
            model = nullptr;
        }
    }

    void Update(float deltaTime) override {
        // Apply gravity for jumping
        if (isJumping) {
            jumpVelocity += gravity * deltaTime;
            position.y += jumpVelocity * deltaTime;

            if (position.y <= 3.5f) { // Ground level (adjusted)
                position.y = 3.5f;
                isJumping = false;
                jumpVelocity = 0.0f;
            }
        }

        // Speed boost timer
        if (hasSpeedBoost) {
            speedBoostTimer -= deltaTime;
            if (speedBoostTimer <= 0.0f) {
                hasSpeedBoost = false;
            }
        }

        GameObject::Update(deltaTime);
    }

    void Move(glm::vec3 direction, float deltaTime) {
        float speed = moveSpeed;
        if (hasSpeedBoost) {
            speed *= 2.0f; // Double speed with boost
        }

        position += direction * speed * deltaTime;

        // Rotate turtle to face movement direction (ปิดไว้ก่อน)
        // if (glm::length(direction) > 0.01f) {
        //     rotation.y = glm::degrees(atan2(direction.x, direction.z));
        // }
    }

    void Jump() {
        if (!isJumping) {
            isJumping = true;
            jumpVelocity = sqrtf(2.0f * -gravity * jumpHeight);
        }
    }

    void ActivateSpeedBoost() {
        hasSpeedBoost = true;
        speedBoostTimer = 5.0f; // 5 seconds boost
    }

    void Draw() override {
        if (useModel && model != nullptr) {
            model->Draw();
        } else {
            GameObject::Draw();
        }
    }

private:
    void CreateTurtleMesh() {
        // Simple turtle shape (body + head)
        vertices = {
            // Body (cube-like)
            -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
             0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
             0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
            -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
             0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
             0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f,
        };

        SetupMesh();
    }
};

#endif
