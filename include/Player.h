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
        scale = glm::vec3(2.0f, 2.0f, 2.0f); // ลองเพิ่มขนาดมากขึ้น
        rotation = glm::vec3(0.0f, 0.0f, 0.0f); // ไม่หมุนก่อน ดูรูปแบบเดิม
        color = glm::vec3(0.2f, 0.6f, 0.3f); // Green turtle
        moveSpeed = 5.0f;
        jumpHeight = 2.0f;
        isJumping = false;
        jumpVelocity = 0.0f;
        gravity = -15.0f;
        hasSpeedBoost = false;
        speedBoostTimer = 0.0f;
        model = nullptr;
        useModel = false;

        // Try to load model
        LoadModel("assets/turtle/source/turtle/scene.gltf");
        
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

            if (position.y <= 1.0f) { // Ground level
                position.y = 1.0f;
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
