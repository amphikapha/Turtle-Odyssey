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
    glm::vec3 diffuseColor;
    unsigned int VAO, VBO, EBO;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<unsigned int> textures = {}) {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;
        this->diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
        setupMesh();
    }

    void Draw() {
        // Bind textures
        for (unsigned int i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, textures[i]);
        }
        // If shader has an "objectColor" uniform, set it to the mesh diffuse color when appropriate.
        // However, if the shader requests an overrideColor, respect that and do not overwrite the uniform
        GLint currentProg = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currentProg);
        if (currentProg != 0) {
            GLint loc = glGetUniformLocation(currentProg, "objectColor");
            GLint overrideLoc = glGetUniformLocation(currentProg, "overrideColor");
            bool overrideActive = false;
            if (overrideLoc != -1) {
                GLint ov = 0;
                // Query the current value of overrideColor from the active program
                glGetUniformiv((GLuint)currentProg, overrideLoc, &ov);
                overrideActive = (ov != 0);
            }
            if (loc != -1 && !overrideActive) {
                glUniform3f(loc, diffuseColor.r, diffuseColor.g, diffuseColor.b);
            }
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
        
        // Load textures for Goblin model (GoblinMutantSPDONEFINAL.fbx)
        if (modelPath.find("goblin-3d-model-free") != std::string::npos || 
            modelPath.find("GoblinMutant") != std::string::npos) {
            std::cout << "DEBUG: Getting texture for goblin mesh: " << meshName << std::endl;
            
            std::string textureName;
            
            // Map mesh names to texture files based on goblin model parts
            if (meshName.find("Body") != std::string::npos) {
                textureName = "GoblinZBDone_Body_BaseColor.png";
            } else if (meshName.find("Shell") != std::string::npos || meshName.find("shell") != std::string::npos) {
                textureName = "GoblinZBDone_Shell_BaseColor.png";
            } else if (meshName.find("WaistBand") != std::string::npos || meshName.find("waist") != std::string::npos) {
                textureName = "GoblinZBDone_WaistBandShell_BaseColor.png";
            } else {
                // Default to body texture
                textureName = "GoblinZBDone_Body_BaseColor.png";
            }
            
            // Try different possible texture paths
            std::vector<std::string> possiblePaths = {
                directory + "/../textures/" + textureName,
                directory + "/" + textureName,
                "assets/goblin-3d-model-free/textures/" + textureName
            };
            
            for (const auto& texturePath : possiblePaths) {
                std::ifstream f(texturePath);
                if (f.good()) {
                    f.close();
                    unsigned int textureID = loadTexture(texturePath.c_str());
                    if (textureID != 0) {
                        std::cout << "Successfully loaded goblin texture: " << texturePath << std::endl;
                        return textureID;
                    }
                }
                f.close();
            }
            
            // Fallback: try to load any available goblin texture
            std::vector<std::string> goblinTextures = {
                "GoblinZBDone_Body_BaseColor.png",
                "GoblinZBDone_Shell_BaseColor.png",
                "GoblinZBDone_WaistBandShell_BaseColor.png"
            };
            
            for (const auto& textureName : goblinTextures) {
                std::vector<std::string> possiblePaths = {
                    directory + "/../textures/" + textureName,
                    directory + "/" + textureName,
                    "assets/goblin-3d-model-free/textures/" + textureName
                };
                
                for (const auto& texturePath : possiblePaths) {
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
            }
        }
        // Load textures for new detailed turtle model (model.dae)
        else if (modelPath.find("turtle/source/model") != std::string::npos) {
            std::cout << "DEBUG: Getting texture for mesh: " << meshName << " (modelPath: " << modelPath << ")" << std::endl;
            // Map mesh names to texture files based on material names from DAE
            std::string textureName;
            
            if (meshName.find("tete") != std::string::npos || meshName.find("Object008") != std::string::npos) {
                textureName = "tete_albedo.jpg";
            } else if (meshName.find("carapace") != std::string::npos || meshName.find("Object009") != std::string::npos) {
                textureName = "carapace_albedo.jpg";
            } else if (meshName.find("yeux_langue") != std::string::npos || meshName.find("Object010") != std::string::npos) {
                textureName = "yeux_langue_albedo.jpg";
            } else if (meshName.find("pattes") != std::string::npos || meshName.find("Sphere") != std::string::npos) {
                textureName = "pattes_albedo.jpg";
            } else if (meshName.find("queue") != std::string::npos || meshName.find("Object011") != std::string::npos || 
                       meshName.find("Object012") != std::string::npos || meshName.find("Object013") != std::string::npos ||
                       meshName.find("Object014") != std::string::npos || meshName.find("Object015") != std::string::npos) {
                textureName = "queue_albedo.jpg";
            } else if (meshName.find("dessous") != std::string::npos || meshName.find("Object016") != std::string::npos) {
                textureName = "dessous_albedo.jpg";
            } else {
                // Default texture
                textureName = "tete_albedo.jpg";
            }
            
            // Try different possible texture paths
            std::vector<std::string> possiblePaths = {
                directory + "/../textures/" + textureName,
                "assets/turtle/textures/" + textureName,
                "assets/turtle/source/model/model/textures/" + textureName
            };
            
            for (const auto& texturePath : possiblePaths) {
                std::ifstream f(texturePath);
                if (f.good()) {
                    f.close();
                    unsigned int textureID = loadTexture(texturePath.c_str());
                    if (textureID != 0) {
                        std::cout << "Successfully loaded turtle texture: " << texturePath << std::endl;
                        return textureID;
                    }
                }
                f.close();
            }
        }
        // Load textures for new 3D turtle model (GLB)
        else if (modelPath.find("123b415d79b4") != std::string::npos) {
            // Load textures from the new turtle model
            std::vector<std::string> turtleTextures = {
                "Image_0_0.jpeg",
                "Image_1_1@channels=B.jpeg",
                "Image_1_1@channels=G.jpeg"
            };
            
            // Try to load first available texture
            for (const auto& textureName : turtleTextures) {
                std::vector<std::string> possiblePaths = {
                    directory + "/../textures/" + textureName,
                    directory + "/" + textureName,
                    "assets/123b415d79b4-74f3574a4468-turtle-cartoon--3d/textures/" + textureName
                };
                
                for (const auto& texturePath : possiblePaths) {
                    std::ifstream f(texturePath);
                    if (f.good()) {
                        f.close();
                        unsigned int textureID = loadTexture(texturePath.c_str());
                        if (textureID != 0) {
                            std::cout << "Successfully loaded turtle texture: " << texturePath << std::endl;
                            return textureID;
                        }
                    }
                    f.close();
                }
            }
        }
        // Load textures for old toon turtle models
        else if (modelPath.find("toonturtle") != std::string::npos) {
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
            // Multiple car textures to choose from
            std::vector<std::string> carTextures = {
                "Retro Car.jpeg",
                "Retro Car Purple.jpg"
            };
            
            // Randomly select one texture
            int randomIndex = rand() % carTextures.size();
            std::string selectedTexture = carTextures[randomIndex];
            
            // Try different possible texture paths
            std::vector<std::string> possiblePaths = {
                directory + "/../textures/" + selectedTexture,
                directory + "/" + selectedTexture,
                "assets/free-retro-american-car-cartoon-low-poly/textures/" + selectedTexture
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

        // Debug: print mesh info for turtle model
        if (modelPath.find("turtle/source/model") != std::string::npos) {
            std::cout << "Processing mesh: " << nodeName << " (mesh " << mesh << ")" << std::endl;
        }

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
        
        Mesh m(vertices, indices, meshTextures);
        // Attempt to read material diffuse color from Assimp material; use it as mesh diffuseColor
        if (mesh->mMaterialIndex >= 0 && scene && scene->mMaterials) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            aiColor4D color;
            if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
                m.diffuseColor = glm::vec3(color.r, color.g, color.b);
            }
        }

        return m;
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
