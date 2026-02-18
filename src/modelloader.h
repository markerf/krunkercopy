#pragma once
#include <GL/glew.h>
#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <iostream>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

struct Mesh {
    unsigned int VAO, VBO, EBO;
    int indexCount;
    int indexComponentType;
    int textureIndex; // -1 = no texture
    glm::vec3 AABBmin;
    glm::vec3 AABBmax;
    glm::mat4 worldtransform;
    glm::vec4 baseColor;

};

class ModelLoader {
public:
    std::vector<Mesh> meshes;
    std::vector<unsigned int> glTextures; // OpenGL texture IDs
    tinygltf::Model model;

    bool load(const std::string& path) {
        tinygltf::TinyGLTF loader;
        std::string err, warn;

        bool ret;
        if (path.substr(path.find_last_of('.')) == ".glb")
            ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);
        else
            ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);

        if (!warn.empty()) std::cerr << "GLTF Warning: " << warn << std::endl;
        if (!err.empty())  std::cerr << "GLTF Error: "   << err  << std::endl;
        if (!ret) return false;

        uploadTextures();

        for (auto& mesh : model.meshes)
            for (auto& primitive : mesh.primitives)
                uploadPrimitive(primitive);
            
        buildWorldTransforms();

        return true;
    }

    void draw(unsigned int shaderProgram, unsigned int mvploc, glm::mat4 vpMatrix,unsigned int colorloc) {
        unsigned int hasTexLoc = glGetUniformLocation(shaderProgram, "hasTexture");
        unsigned int tex0loc   = glGetUniformLocation(shaderProgram, "texture0");

        auto& scene = model.scenes[model.defaultScene];
        for (int nodeIdx : scene.nodes)
            drawNode(nodeIdx, glm::mat4(1.0f), mvploc, hasTexLoc, tex0loc, vpMatrix,colorloc);
    }

    void buildWorldTransforms() {
        auto& scene = model.scenes[model.defaultScene];
        for (int nodeIdx : scene.nodes)
            applyTransformToNode(nodeIdx, glm::mat4(1.0f));
    }

    void cleanup() {
        for (auto& m : meshes) {
            glDeleteVertexArrays(1, &m.VAO);
            glDeleteBuffers(1, &m.VBO);
            glDeleteBuffers(1, &m.EBO);
        }
        if (!glTextures.empty())
            glDeleteTextures((GLsizei)glTextures.size(), glTextures.data());
    }

private:
    void uploadTextures() {
        glTextures.resize(model.textures.size());
        glGenTextures((GLsizei)model.textures.size(), glTextures.data());

        for (size_t i = 0; i < model.textures.size(); i++) {
            auto& tex   = model.textures[i];
            auto& image = model.images[tex.source];

            glBindTexture(GL_TEXTURE_2D, glTextures[i]);

            // wrapping & filtering
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            GLenum format = GL_RGBA;
            if (image.component == 1) format = GL_RED;
            else if (image.component == 3) format = GL_RGB;
            else if (image.component == 4) format = GL_RGBA;

            glTexImage2D(GL_TEXTURE_2D, 0, format,
                image.width, image.height, 0,
                format, GL_UNSIGNED_BYTE,
                image.image.data());

            glGenerateMipmap(GL_TEXTURE_2D);
        }
    }

    int getTextureIndexForPrimitive(const tinygltf::Primitive& primitive) {
        if (primitive.material < 0) return -1;

        auto& mat = model.materials[primitive.material];
        int texIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
        if (texIndex < 0) return -1;

        return model.textures[texIndex].source >= 0 ? texIndex : -1;
    }

    void uploadPrimitive(tinygltf::Primitive& primitive) {
        // --- POSITIONS ---
        if (primitive.attributes.find("POSITION") == primitive.attributes.end()) return;
        auto& posAccessor = model.accessors[primitive.attributes["POSITION"]];
        auto& posView     = model.bufferViews[posAccessor.bufferView];
        auto& posBuffer   = model.buffers[posView.buffer];
        const float* positions = reinterpret_cast<const float*>(
            posBuffer.data.data() + posView.byteOffset + posAccessor.byteOffset);

        // --- UVs (TEXCOORD_0) ---
        const float* uvs = nullptr;
        if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
            auto& uvAccessor = model.accessors[primitive.attributes["TEXCOORD_0"]];
            auto& uvView     = model.bufferViews[uvAccessor.bufferView];
            auto& uvBuffer   = model.buffers[uvView.buffer];
            uvs = reinterpret_cast<const float*>(
                uvBuffer.data.data() + uvView.byteOffset + uvAccessor.byteOffset);
        }

        // --- Interleave pos + uv into one buffer ---
        size_t vertCount = posAccessor.count;
        std::vector<float> interleaved;
        interleaved.reserve(vertCount * 5); // xyz + uv
        
        glm::vec3 minBounds(FLT_MAX);
        glm::vec3 maxBounds(-FLT_MAX);
        
        
        for (size_t i = 0; i < vertCount; i++) {
            interleaved.push_back(positions[i * 3 + 0]);
            interleaved.push_back(positions[i * 3 + 1]);
            interleaved.push_back(positions[i * 3 + 2]);
           
            glm::vec3 pos = { 
                positions[i * 3 + 0],
                positions[i * 3 + 1],
                positions[i * 3 + 2]
            };
            
            minBounds = glm::min(minBounds,pos); 
            maxBounds = glm::max(maxBounds,pos);

            if (uvs) {
                interleaved.push_back(uvs[i * 2 + 0]);
                interleaved.push_back(uvs[i * 2 + 1]);
            } else {
                interleaved.push_back(0.0f);
                interleaved.push_back(0.0f);
            }
        }

        auto& idxAccessor = model.accessors[primitive.indices];
        auto& idxView     = model.bufferViews[idxAccessor.bufferView];
        auto& idxBuffer   = model.buffers[idxView.buffer];
        const void* indexData = idxBuffer.data.data() + idxView.byteOffset + idxAccessor.byteOffset;

        Mesh md;
        glGenVertexArrays(1, &md.VAO);
        glBindVertexArray(md.VAO);

        glGenBuffers(1, &md.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, md.VBO);
        glBufferData(GL_ARRAY_BUFFER,
            interleaved.size() * sizeof(float),
            interleaved.data(), GL_STATIC_DRAW);

        // location 0: position (xyz)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // location 1: uv
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glGenBuffers(1, &md.EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, md.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            idxAccessor.count * tinygltf::GetComponentSizeInBytes(idxAccessor.componentType),
            indexData, GL_STATIC_DRAW);


        if (primitive.material >= 0) {
            auto& mat = model.materials[primitive.material];
            auto& c = mat.pbrMetallicRoughness.baseColorFactor;
            md.baseColor = glm::vec4(c[0], c[1], c[2], c[3]);
        } else {
            md.baseColor = glm::vec4(1.0f);  // default white
        }

        md.indexCount         = idxAccessor.count;
        md.indexComponentType = idxAccessor.componentType;
        md.textureIndex       = getTextureIndexForPrimitive(primitive);
        md.AABBmin = minBounds;
        md.AABBmax = maxBounds;
        md.worldtransform = glm::mat4(1.0f);
        meshes.push_back(md);
    }

    void drawNode(int nodeIndex, glm::mat4 parentTransform, unsigned int mvploc,
                  unsigned int hasTexLoc, unsigned int tex0loc, glm::mat4 vpMatrix,unsigned int colorloc)
    {
        auto& node = model.nodes[nodeIndex];
        glm::mat4 localTransform = glm::mat4(1.0f);


       // glm::mat4 worldTransform = parentTransform * localTransform;

        glm::mat4 worldTransform; 
        if (node.mesh >= 0) {
            //glm::mat4 mvp = vpMatrix * worldTransform;
            //glUniformMatrix4fv(mvploc, 1, GL_FALSE, &mvp[0][0]);

            int meshOffset = 0;
            for (int i = 0; i < node.mesh; i++)
                meshOffset += model.meshes[i].primitives.size();


            auto& md = meshes[meshOffset ];
            glm::mat4 worldTransform = md.worldtransform;
            glm::mat4 mvp = vpMatrix * worldTransform;
            glUniformMatrix4fv(mvploc, 1, GL_FALSE, &mvp[0][0]);

            for (int i = 0; i < (int)model.meshes[node.mesh].primitives.size(); i++) {
                auto& md = meshes[meshOffset + i];
                if (md.textureIndex >= 0 && md.textureIndex < (int)glTextures.size()) {
                    glUniform1i(hasTexLoc, 1);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, glTextures[md.textureIndex]);
                    glUniform1i(tex0loc, 0);
                } else {
                    glUniform1i(hasTexLoc, 0);
                }

                glUniform4f(colorloc, md.baseColor.r, md.baseColor.g, md.baseColor.b, md.baseColor.a);
                glBindVertexArray(md.VAO);
                glDrawElements(GL_TRIANGLES, md.indexCount, md.indexComponentType, 0);
            }
        }

        for (int child : node.children)
            drawNode(child, worldTransform, mvploc, hasTexLoc, tex0loc, vpMatrix,colorloc);
    }


    void applyTransformToNode(int nodeIndex, glm::mat4 parentTransform) {
    auto& node = model.nodes[nodeIndex];
    glm::mat4 localTransform = glm::mat4(1.0f);

    if (node.matrix.size() == 16) {
        localTransform = glm::make_mat4(node.matrix.data());
    } else {
        if (node.translation.size() == 3)
            localTransform = glm::translate(localTransform,
                glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
        if (node.rotation.size() == 4)
            localTransform *= glm::mat4_cast(
                glm::quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]));
        if (node.scale.size() == 3)
            localTransform = glm::scale(localTransform,
                glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
    }

    glm::mat4 worldTransform = parentTransform * localTransform;

    if (node.mesh >= 0) {
        int meshOffset = 0;
        for (int i = 0; i < node.mesh; i++)
            meshOffset += model.meshes[i].primitives.size();

        for (int i = 0; i < (int)model.meshes[node.mesh].primitives.size(); i++)
            meshes[meshOffset + i].worldtransform = worldTransform;
    }

    for (int child : node.children)
        applyTransformToNode(child, worldTransform);
    }
};
