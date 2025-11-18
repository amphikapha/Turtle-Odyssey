#ifndef CUBEMAP_H
#define CUBEMAP_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <iostream>
#include <vector>
#include <windows.h>
#include <gdiplus.h>

class Cubemap {
public:
    unsigned int textureID;
    unsigned int VAO, VBO;

    Cubemap() : textureID(0), VAO(0), VBO(0) {}

    ~Cubemap() {
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
        if (VBO != 0) glDeleteBuffers(1, &VBO);
        if (textureID != 0) glDeleteTextures(1, &textureID);
    }

    bool LoadCubemap(const std::string& posX, const std::string& negX,
                     const std::string& posY, const std::string& negY,
                     const std::string& posZ, const std::string& negZ) {
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

        std::vector<std::string> faces = { posX, negX, posY, negY, posZ, negZ };
        GLenum targets[6] = {
            GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
            GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
            GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
        };

        for (unsigned int i = 0; i < faces.size(); ++i) {
            if (!LoadCubemapFace(targets[i], faces[i])) {
                std::cerr << "Failed to load cubemap face: " << faces[i] << std::endl;
                return false;
            }
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        std::cout << "Cubemap loaded successfully!" << std::endl;
        return true;
    }

    void SetupMesh() {
        // Create a cube mesh for the skybox
        float skyboxVertices[] = {
            // Positions          
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void Draw() {
        glBindVertexArray(VAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

private:
    bool LoadCubemapFace(GLenum target, const std::string& path) {
        using namespace Gdiplus;

        // Convert char* to wchar_t*
        int len = path.length();
        wchar_t* widePath = new wchar_t[len + 1];
        mbstowcs(widePath, path.c_str(), len + 1);

        Image* image = new Image(widePath);
        delete[] widePath;

        if (image->GetLastStatus() != Ok || image->GetWidth() == 0 || image->GetHeight() == 0) {
            std::cerr << "Warning: Failed to load image: " << path << std::endl;
            delete image;
            return false;
        }

        UINT width = image->GetWidth();
        UINT height = image->GetHeight();

        // Create an RGBA bitmap
        Bitmap* bitmap = new Bitmap(width, height, PixelFormat32bppARGB);
        Graphics* graphics = Graphics::FromImage(bitmap);
        graphics->DrawImage(image, 0, 0);
        delete graphics;

        // Lock bitmap bits for reading
        BitmapData bitmapData;
        Rect rect(0, 0, width, height);
        bitmap->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bitmapData);

        unsigned char* pixels = static_cast<unsigned char*>(bitmapData.Scan0);
        unsigned char* textureData = new unsigned char[width * height * 4];

        // Copy and convert BGRA -> RGBA
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

        // Upload to OpenGL
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
        glTexImage2D(target, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);

        delete[] textureData;
        delete bitmap;
        delete image;

        std::cout << "Loaded cubemap face: " << path << " (" << width << "x" << height << ")" << std::endl;

        return true;
    }
};

#endif
