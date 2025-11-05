// src/engine/asset/model.h
// (atualizado — mantém SRP e compat com 3-args)

#pragma once
#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "./../../engine/render/material.h"
#include "./../../engine/math/quat.h"

namespace Engine
{
    namespace Render
    {
        class Shader;
    }
}

namespace Engine
{
    namespace Asset
    {

        struct Vertex
        {
            glm::vec3 Position;
            glm::vec3 Normal;
            glm::vec2 TexCoords;
            glm::vec3 Tangent;
            glm::ivec4 BoneIDs = glm::ivec4(0);
            glm::vec4 Weights = glm::vec4(0.0f);
        };

        struct BoneInfo
        {
            int id;
            glm::mat4 offset; // inverse bind
        };

        template <typename T>
        struct KeyFrame
        {
            float time;
            T value;
        };

        struct BoneChannel
        {
            std::vector<KeyFrame<glm::vec3>> positionKeys;
            std::vector<KeyFrame<Engine::Math::Quat>> rotationKeys;
            std::vector<KeyFrame<glm::vec3>> scaleKeys;
            std::string boneName;
        };

        struct AnimationClip
        {
            std::unordered_map<std::string, BoneChannel> boneChannels;
            float duration = 0.0f;
        };

        struct Node
        {
            std::string name;
            glm::mat4 localTransform;
            std::vector<std::string> childrenNames;
        };

        class Mesh
        {
        public:
            // NOVO: construtor com nodeTransform
            // src/engine/asset/model.h
            Mesh(std::vector<Vertex> &&vertices,
                 std::vector<uint32_t> &&indices,
                 std::unique_ptr<Render::Material> material,
                 const glm::mat4 &nodeTransform);

            // COMPATIBILIDADE: construtor antigo (3 args) → node = Identity
            Mesh(std::vector<Vertex> &&vertices,
                 std::vector<uint32_t> &&indices,
                 std::unique_ptr<Render::Material> material);

            ~Mesh();
            Mesh(const Mesh &other);

            void draw(Engine::Render::Shader &shader);

            size_t getVertexCount() const { return m_vertices.size(); }
            size_t getIndexCount() const { return m_indices.size(); }
            const Render::Material *getMaterial() const { return m_material.get(); }

            const std::vector<Vertex> &getVertices() const { return m_vertices; }
            const std::vector<uint32_t> &getIndices() const { return m_indices; }

            const glm::mat4 &getNodeTransform() const { return m_nodeTransform; }

        private:
            std::vector<Vertex> m_vertices;
            std::vector<uint32_t> m_indices;
            std::unique_ptr<Render::Material> m_material;

            unsigned int m_VAO = 0, m_VBO = 0, m_EBO = 0;

            // NOVO: transform do nó que contém a malha no glTF
            glm::mat4 m_nodeTransform{1.0f};

            void setupMesh();
        };

        class Model
        {
        public:
            Model();
            ~Model();

            void addMesh(std::unique_ptr<Mesh> mesh);
            void draw(Engine::Render::Shader &shader, const std::vector<glm::mat4> *boneTransforms = nullptr);
            const std::vector<std::unique_ptr<Mesh>> &getMeshes() const { return m_meshes; }

            std::unique_ptr<Model> clone() const;

            const std::unordered_map<std::string, BoneInfo> &getBoneInfoMap() const { return m_boneInfoMap; }
            const std::string &getSkeletonRootName() const { return m_skeletonRootNodeName; }
            int getBoneCount() const { return m_boneCounter; }

            void setSkeletonRootName(const std::string &rootName) { m_skeletonRootNodeName = rootName; }
            void addBone(const std::string &name, const glm::mat4 &offset);

            // NOVO: Adiciona osso forçando o ID igual ao índice do joint do GLTF
            void addBone(const std::string &name, const glm::mat4 &offset, int forcedId);

            const std::vector<std::string> getNodeChildren(const std::string &nodeName) const;
            const glm::mat4 getNodeLocalTransform(const std::string &nodeName) const;
            int getBoneIndexByName(const std::string &boneName) const;

            void addAnimationClip(uint32_t nameHash, AnimationClip clip) { m_animationClips[nameHash] = std::move(clip); }
            const AnimationClip *getAnimationClip(uint32_t nameHash) const;

            void addNode(const Node &node);
            void setNodeGlobalTransform(const std::string &nodeName, const glm::mat4 &transform);
            std::vector<glm::vec3> getSkeletonDebugLines(const std::vector<glm::mat4> &finalBoneTransforms) const;

            void setSkeletonBindTransform(const glm::mat4 &m) { m_skeletonBindTransform = m; }
            const glm::mat4 &getSkeletonBindTransform() const { return m_skeletonBindTransform; }

        private:
            std::vector<std::unique_ptr<Mesh>> m_meshes;

            std::unordered_map<std::string, BoneInfo> m_boneInfoMap;
            std::string m_skeletonRootNodeName;
            int m_boneCounter = 0;

            std::unordered_map<uint32_t, AnimationClip> m_animationClips;
            std::unordered_map<std::string, glm::mat4> m_nodeGlobalTransforms;
            std::unordered_map<std::string, Node> m_nodeHierarchy;

            glm::mat4 m_skeletonBindTransform{1.0f};
        };

    }
} // namespaces
