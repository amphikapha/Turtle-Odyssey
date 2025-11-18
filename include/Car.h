#ifndef CAR_H
#define CAR_H

#include "GameObject.h"
#include "Model.h"
#include <glad/glad.h>
#include <cstdlib>
#include <ctime>

class Car : public GameObject {
public:
    int lane;
    float laneWidth;
    Model* model;
    bool useModel;

    Car(int laneNumber, float width, bool movingRight) {
        lane = laneNumber;
        laneWidth = width;
        model = nullptr;
        useModel = false;
        
        // Position based on lane (lanes are in Z axis)
        position.z = lane * laneWidth;  // เปลี่ยนจาก X เป็น Z (แนวตั้ง)
        position.y = 0.3f;
        
        // Start position and velocity based on direction
        // รถวิ่งแนวนอน (X axis)
        if (movingRight) {
            position.x = -30.0f;  // เริ่มทางซ้าย
            velocity.x = 10.0f + (rand() % 4); // วิ่งไปขวา (10-13 units/sec)
            rotation.y = 90.0f;  // หมุนหันไปขวา
        } else {
            position.x = 30.0f;   // เริ่มทางขวา
            velocity.x = -(10.0f + (rand() % 4)); // วิ่งไปซ้าย (10-13 units/sec)
            rotation.y = -90.0f; // หมุนหันไปซ้าย
        }

        scale = glm::vec3(2.5f, 2.5f, 2.5f); // ขนาดรถ (ใหญ่ขึ้น)
        
        // Random car color
        float r = 0.3f + static_cast<float>(rand()) / RAND_MAX * 0.7f;
        float g = 0.3f + static_cast<float>(rand()) / RAND_MAX * 0.7f;
        float b = 0.3f + static_cast<float>(rand()) / RAND_MAX * 0.7f;
        color = glm::vec3(r, g, b);

    // Try to load model (new assets path under assets/models)
    LoadModel("assets/models/free-retro-american-car-cartoon-low-poly/source/RetroCar/RetroCar.obj");
        
        // Fallback to cube if model not found
        if (!useModel) {
            scale = glm::vec3(4.0f, 1.2f, 2.0f); // Box car size (ยาวแนวนอน - ใหญ่ขึ้น)
            CreateCarMesh();
        }
    }

    ~Car() {
        if (model != nullptr) {
            delete model;
        }
    }

    void LoadModel(const std::string& path) {
        model = new Model();
        useModel = model->loadModel(path);
        if (useModel) {
            // std::cout << "Car model loaded from: " << path << std::endl;
        } else {
            std::cout << "Could not load car model, using fallback cube" << std::endl;
            delete model;
            model = nullptr;
        }
    }

    void Update(float deltaTime) override {
        GameObject::Update(deltaTime);

        // Reset car if it goes off screen with more spacing
        // รถวิ่งแนวนอน (X axis)
        if (velocity.x > 0 && position.x > 40.0f) {
            position.x = -50.0f; // เริ่มไกลออกไปเพื่อหลีกเลี่ยงการชน
        } else if (velocity.x < 0 && position.x < -40.0f) {
            position.x = 50.0f;
        }
    }

    void Draw() override {
        if (useModel && model != nullptr) {
            model->Draw();
        } else {
            GameObject::Draw();
        }
    }

private:
    void CreateCarMesh() {
        // Simple box car mesh
        vertices = {
            // Front
            -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
             0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
            // Back
            -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
             0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
            // Left
            -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
            // Right
             0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
            // Bottom
            -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
             0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
            // Top
            -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
             0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f,
        };

        SetupMesh();
    }
};

#endif
