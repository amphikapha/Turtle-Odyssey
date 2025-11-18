#ifndef HUD_H
#define HUD_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>

class HUD {
public:
    unsigned int VAO, VBO;
    unsigned int shaderProgram;
    
    HUD() : VAO(0), VBO(0), shaderProgram(0) {
        setupHUDShader();
        setupQuadMesh();
    }
    
    ~HUD() {
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
        if (VBO != 0) glDeleteBuffers(1, &VBO);
        if (shaderProgram != 0) glDeleteProgram(shaderProgram);
    }
    
    void Draw(int distance, int lives, int screenWidth, int screenHeight) {
        GLint oldProgram;
        glGetIntegerv(GL_CURRENT_PROGRAM, &oldProgram);
        GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
        
        glUseProgram(shaderProgram);
        
        // Set orthographic projection for 2D rendering
        glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);
        GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        // Disable depth test for 2D rendering
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Draw distance text (top-left)
        std::ostringstream oss;
        oss << "Distance: " << distance << "m";
        DrawString(oss.str(), 20.0f, 30.0f, 1.5f, glm::vec3(1.0f, 1.0f, 1.0f));
        
        // Draw lives text (top-right)
        oss.str("");
        oss.clear();
        oss << "Lives: " << lives;
        float livesWidth = oss.str().length() * 16.0f * 1.5f;
        DrawString(oss.str(), (float)(screenWidth - livesWidth - 20), 30.0f, 1.5f, 
                   glm::vec3(1.0f, 0.3f, 0.3f));
        
        glDisable(GL_BLEND);
        if (depthEnabled) glEnable(GL_DEPTH_TEST);
        glUseProgram(oldProgram);
    }

private:
    void setupHUDShader() {
        const char* vertexShader = R"(
            #version 330 core
            layout(location = 0) in vec2 position;
            
            uniform mat4 projection;
            
            void main() {
                gl_Position = projection * vec4(position, 0.0, 1.0);
            }
        )";
        
        const char* fragmentShader = R"(
            #version 330 core
            uniform vec3 color;
            out vec4 FragColor;
            
            void main() {
                FragColor = vec4(color, 1.0);
            }
        )";
        
        // Compile vertex shader
        unsigned int vShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vShader, 1, &vertexShader, NULL);
        glCompileShader(vShader);
        checkCompileErrors(vShader, "VERTEX");
        
        // Compile fragment shader
        unsigned int fShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fShader, 1, &fragmentShader, NULL);
        glCompileShader(fShader);
        checkCompileErrors(fShader, "FRAGMENT");
        
        // Link shaders
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vShader);
        glAttachShader(shaderProgram, fShader);
        glLinkProgram(shaderProgram);
        checkCompileErrors(shaderProgram, "PROGRAM");
        
        glDeleteShader(vShader);
        glDeleteShader(fShader);
    }
    
    void setupQuadMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
    }
    
    // Draw a single character made of line segments
    void DrawCharacter(char ch, float x, float y, float scale, glm::vec3 color, glm::mat4 projection) {
        std::vector<glm::vec2> vertices;
        
        // Define character shapes as line segments
        // Each character is roughly 8x8 units
        float w = 8.0f * scale;
        float h = 8.0f * scale;
        
        if (ch >= '0' && ch <= '9') {
            // Draw rectangular border with some detail
            float dx = 1.0f * scale;
            float dy = 1.0f * scale;
            
            // Top horizontal line
            vertices.push_back({x + dx, y + dy});
            vertices.push_back({x + w - dx, y + dy});
            
            // Right vertical line
            vertices.push_back({x + w - dx, y + dy});
            vertices.push_back({x + w - dx, y + h - dy});
            
            // Bottom horizontal line
            vertices.push_back({x + w - dx, y + h - dy});
            vertices.push_back({x + dx, y + h - dy});
            
            // Left vertical line
            vertices.push_back({x + dx, y + h - dy});
            vertices.push_back({x + dx, y + dy});
            
            // Middle horizontal (for 8)
            vertices.push_back({x + dx, y + h / 2});
            vertices.push_back({x + w - dx, y + h / 2});
        }
        else if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
            // Simple letter shapes
            float dx = 1.0f * scale;
            float dy = 1.0f * scale;
            
            // Left vertical
            vertices.push_back({x + dx, y + dy});
            vertices.push_back({x + dx, y + h - dy});
            
            // Right vertical
            vertices.push_back({x + w - dx, y + dy});
            vertices.push_back({x + w - dx, y + h - dy});
            
            // Top horizontal
            vertices.push_back({x + dx, y + dy});
            vertices.push_back({x + w - dx, y + dy});
            
            // Bottom horizontal
            vertices.push_back({x + dx, y + h - dy});
            vertices.push_back({x + w - dx, y + h - dy});
            
            // Middle horizontal (for 'E', 'F', etc.)
            vertices.push_back({x + dx, y + h / 2});
            vertices.push_back({x + w - dx, y + h / 2});
        }
        else if (ch == ':') {
            float dot1_x = x + w / 2 - 1.5f * scale;
            float dot1_y = y + 2.0f * scale;
            float dot2_x = x + w / 2 - 1.5f * scale;
            float dot2_y = y + h - 3.0f * scale;
            float dotSize = 1.5f * scale;
            
            // Top dot
            vertices.push_back({dot1_x, dot1_y});
            vertices.push_back({dot1_x + dotSize, dot1_y});
            vertices.push_back({dot1_x + dotSize, dot1_y + dotSize});
            vertices.push_back({dot1_x, dot1_y + dotSize});
            
            // Bottom dot
            vertices.push_back({dot2_x, dot2_y});
            vertices.push_back({dot2_x + dotSize, dot2_y});
            vertices.push_back({dot2_x + dotSize, dot2_y + dotSize});
            vertices.push_back({dot2_x, dot2_y + dotSize});
        }
        else if (ch == ' ') {
            // Space - no vertices
            return;
        }
        else {
            // Default character - simple square
            vertices.push_back({x, y});
            vertices.push_back({x + w, y});
            vertices.push_back({x + w, y + h});
            vertices.push_back({x, y + h});
        }
        
        if (vertices.empty()) return;
        
        glUseProgram(shaderProgram);
        GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(colorLoc, color.x, color.y, color.z);
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec2), vertices.data(), GL_DYNAMIC_DRAW);
        
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glDrawArrays(GL_LINES, 0, (GLsizei)vertices.size());
    }
    
    void DrawString(const std::string& text, float x, float y, float scale, glm::vec3 color) {
        glm::mat4 projection = glm::ortho(0.0f, 1280.0f, 720.0f, 0.0f, -1.0f, 1.0f);
        
        float charWidth = 10.0f * scale;
        for (size_t i = 0; i < text.length(); ++i) {
            DrawCharacter(text[i], x + (i * charWidth), y, scale, color, projection);
        }
    }
    
    void checkCompileErrors(unsigned int shader, const std::string& type) {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << std::endl;
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << std::endl;
            }
        }
    }
};

#endif
