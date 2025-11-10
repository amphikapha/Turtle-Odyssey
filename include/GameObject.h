#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

class GameObject {
public:
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation;
    glm::vec3 color;
    glm::vec3 velocity;
    
    unsigned int VAO, VBO;
    std::vector<float> vertices;
    bool isActive;

    GameObject() {
        position = glm::vec3(0.0f);
        scale = glm::vec3(1.0f);
        rotation = glm::vec3(0.0f);
        color = glm::vec3(1.0f);
        velocity = glm::vec3(0.0f);
        isActive = true;
        VAO = 0;
        VBO = 0;
    }

    virtual ~GameObject() {
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
        if (VBO != 0) glDeleteBuffers(1, &VBO);
    }

    virtual void Update(float deltaTime) {
        position += velocity * deltaTime;
    }

    glm::mat4 GetModelMatrix() {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);
        return model;
    }

    virtual void Draw() {
        if (VAO != 0) {
            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 3);
            glBindVertexArray(0);
        }
    }

    // Simple AABB collision detection
    bool CheckCollision(GameObject* other) {
        float thisHalfX = scale.x / 2.0f;
        float thisHalfZ = scale.z / 2.0f;
        float otherHalfX = other->scale.x / 2.0f;
        float otherHalfZ = other->scale.z / 2.0f;

        bool collisionX = position.x + thisHalfX >= other->position.x - otherHalfX &&
                         other->position.x + otherHalfX >= position.x - thisHalfX;

        bool collisionZ = position.z + thisHalfZ >= other->position.z - otherHalfZ &&
                         other->position.z + otherHalfZ >= position.z - thisHalfZ;

        return collisionX && collisionZ;
    }

protected:
    void SetupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }
};

#endif
