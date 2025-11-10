#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <fstream>
#include <filesystem>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<unsigned int> textures;
    unsigned int VAO, VBO, EBO;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<unsigned int> textures = {}) {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;
        setupMesh();
    }

    void Draw() {
        // Bind textures
        for (unsigned int i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, textures[i]);
        }
        
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        
        glActiveTexture(GL_TEXTURE0);
    }

private:
    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // Vertex positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        // Vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

        // Vertex texture coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0);
    }
};

class Model {
public:
    std::vector<Mesh> meshes;
    std::string directory;
    std::string modelPath;
    bool loaded;

    Model() : loaded(false), modelPath("") {}

    Model(const std::string& path) {
        modelPath = path;
        loaded = loadModel(path);
    }

    bool loadModel(const std::string& path) {
        modelPath = path;
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, 
            aiProcess_Triangulate | 
            aiProcess_FlipUVs | 
            aiProcess_GenNormals |
            aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
            return false;
        }

        directory = path.substr(0, path.find_last_of('/'));
        processNode(scene->mRootNode, scene);
        
        std::cout << "Model loaded successfully: " << path << std::endl;
        std::cout << "  Meshes: " << meshes.size() << std::endl;
        
        return true;
    }

    void Draw() {
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw();
    }

private:
    unsigned int getTextureForMesh(const std::string& meshName) {
        std::vector<unsigned int> textureIDs;
        
        // Load textures for turtle models
        if (modelPath.find("toonturtle") != std::string::npos) {
            // Map mesh names to texture files for turtle
            std::string textureName;
            if (meshName.find("Shell") != std::string::npos) {
                textureName = "Shell_fix.png";
            } else if (meshName.find("Head") != std::string::npos) {
                textureName = "Turtle_Head.png";
            } else if (meshName.find("Leg") != std::string::npos || meshName.find("Feet") != std::string::npos) {
                textureName = "Turtle_Legs.png";
            } else if (meshName.find("Arm") != std::string::npos || meshName.find("Hand") != std::string::npos) {
                textureName = "Turte_Arms.png";
            } else {
                // Default to shell texture
                textureName = "Shell_fix.png";
            }

            std::string texturePath = directory + "/../textures/" + textureName;
            std::ifstream f(texturePath);
            if (f.good()) {
                f.close();
                unsigned int textureID = loadTexture(texturePath.c_str());
                if (textureID != 0) {
                    return textureID;
                }
            }
            f.close();
        }
        // Load textures for car models
        else if (modelPath.find("Retro") != std::string::npos || modelPath.find("retro") != std::string::npos) {
            // Try different possible texture paths
            std::vector<std::string> possiblePaths = {
                directory + "/../textures/Retro Car.jpeg",
                directory + "/Retro Car.jpeg",
                "assets/free-retro-american-car-cartoon-low-poly/textures/Retro Car.jpeg"
            };
            
            for (const auto& texturePath : possiblePaths) {
                std::cout << "Attempting to load car texture from: " << texturePath << std::endl;
                std::ifstream f(texturePath);
                if (f.good()) {
                    f.close();
                    unsigned int textureID = loadTexture(texturePath.c_str());
                    if (textureID != 0) {
                        std::cout << "Successfully loaded car texture: " << texturePath << std::endl;
                        return textureID;
                    }
                } else {
                    std::cout << "Car texture file not found at: " << texturePath << std::endl;
                }
                f.close();
            }
        }
        
        return 0;
    }

    void processNode(aiNode* node, const aiScene* scene) {
        // Process all meshes in the node
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene, node->mName.C_Str()));
        }

        // Process children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene, const std::string& nodeName) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        // Process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            
            // Position
            vertex.Position = glm::vec3(
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z
            );

            // Normals
            if (mesh->HasNormals()) {
                vertex.Normal = glm::vec3(
                    mesh->mNormals[i].x,
                    mesh->mNormals[i].y,
                    mesh->mNormals[i].z
                );
            } else {
                vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            // Texture coordinates
            if (mesh->mTextureCoords[0]) {
                vertex.TexCoords = glm::vec2(
                    mesh->mTextureCoords[0][i].x,
                    mesh->mTextureCoords[0][i].y
                );
            } else {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }

            vertices.push_back(vertex);
        }

        // Process indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // Try to load appropriate texture for this mesh
        std::vector<unsigned int> meshTextures;
        unsigned int texID = getTextureForMesh(nodeName);
        if (texID != 0) {
            meshTextures.push_back(texID);
        }
        
        return Mesh(vertices, indices, meshTextures);
    }

    unsigned int loadTexture(const char* path) {
        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
        if (data) {
            GLenum format = GL_RGB;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
            std::cout << "Texture loaded: " << path << std::endl;
        } else {
            std::cout << "Failed to load texture: " << path << std::endl;
            stbi_image_free(data);
        }

        return textureID;
    }
};

#endif
