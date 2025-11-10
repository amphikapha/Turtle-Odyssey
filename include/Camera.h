#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Target;
    glm::vec3 Up;
    glm::vec3 Offset; // Offset from target for third-person view

    float Distance;
    float Height;
    float Angle;

    Camera(glm::vec3 position = glm::vec3(0.0f, 5.0f, 10.0f)) {
        Position = position;
        Target = glm::vec3(0.0f, 0.0f, 0.0f);
        Up = glm::vec3(0.0f, 1.0f, 0.0f);
        Distance = 12.0f;  // ระยะห่างจากเต่า
        Height = 6.0f;     // ความสูงของกล้อง
        Angle = 45.0f;
        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix() {
        return glm::lookAt(Position, Target, Up);
    }

    void FollowTarget(glm::vec3 targetPos) {
        Target = targetPos;
        updateCameraVectors();
    }

private:
    void updateCameraVectors() {
        // Camera positioned behind and above the target
        // กล้องอยู่ด้านหลังเต่า (Z เพิ่มขึ้น = ด้านหลัง)
        Position.x = Target.x;
        Position.y = Target.y + Height;
        Position.z = Target.z + Distance;  // บวกเพื่อให้อยู่ด้านหลัง
    }
};

#endif
