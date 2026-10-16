#pragma once

#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "editor_viewport_interaction_utils.h"
#include "render/opengl_types.h"

namespace Editor::ViewportSceneRenderer {

inline GLuint CreateSceneShader() {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aColor;
        uniform mat4 uProjection;
        uniform mat4 uView;
        uniform mat4 uModel;
        out vec3 vColor;
        void main() {
            gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
            vColor = aColor;
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 vColor;
        uniform vec3 uColor;
        out vec4 FragColor;
        void main() {
            FragColor = vec4(vColor * uColor, 1.0);
        }
    )";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

inline void RenderSimpleScene3D(
    int width,
    int height,
    float yaw,
    float pitch,
    float distance,
    const glm::vec3& target,
    const std::string& selectedObject,
    const std::unordered_map<std::string, glm::vec3>& objectPositions,
    const std::unordered_map<std::string, glm::vec3>& objectScales) {
    static GLuint program = 0;
    static GLuint gridVao = 0;
    static GLuint gridVbo = 0;
    static GLuint boxVao = 0;
    static GLuint boxVbo = 0;
    static GLuint outlineVao = 0;
    static GLuint outlineVbo = 0;
    static bool initialized = false;

    if (!initialized) {
        program = CreateSceneShader();

        struct Vertex {
            float x, y, z;
            float r, g, b;
        };

        std::vector<Vertex> gridVertices;
        constexpr int kGridExtent = 120;
        const glm::vec3 bgColor = glm::vec3(0.247f, 0.247f, 0.247f); // #3F3F3F
        const glm::vec3 minorNear = glm::vec3(0.35f, 0.35f, 0.35f);
        const glm::vec3 majorNear = glm::vec3(0.46f, 0.46f, 0.46f);
        const glm::vec3 xAxisNear = glm::vec3(0.57f, 0.30f, 0.30f);
        const glm::vec3 zAxisNear = glm::vec3(0.33f, 0.40f, 0.58f);

        for (int i = -kGridExtent; i <= kGridExtent; ++i) {
            const float d = static_cast<float>(std::abs(i)) / static_cast<float>(kGridExtent);
            const float fade = std::exp(-d * 2.8f);
            const bool isMajor = (i % 10 == 0);

            const glm::vec3 nearXLine = (i == 0) ? zAxisNear : (isMajor ? majorNear : minorNear);
            const glm::vec3 nearZLine = (i == 0) ? xAxisNear : (isMajor ? majorNear : minorNear);
            const glm::vec3 cx = bgColor + (nearXLine - bgColor) * fade;
            const glm::vec3 cz = bgColor + (nearZLine - bgColor) * fade;

            // x=i line runs along Z direction (visual Z axis when i==0)
            gridVertices.push_back({float(i), 0.0f, -float(kGridExtent), cx.r, cx.g, cx.b});
            gridVertices.push_back({float(i), 0.0f, float(kGridExtent), cx.r, cx.g, cx.b});
            // z=i line runs along X direction (visual X axis when i==0)
            gridVertices.push_back({-float(kGridExtent), 0.0f, float(i), cz.r, cz.g, cz.b});
            gridVertices.push_back({float(kGridExtent), 0.0f, float(i), cz.r, cz.g, cz.b});
        }

        glGenVertexArrays(1, &gridVao);
        glGenBuffers(1, &gridVbo);
        glBindVertexArray(gridVao);
        glBindBuffer(GL_ARRAY_BUFFER, gridVbo);
        glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(Vertex), gridVertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
        glBindVertexArray(0);

        std::vector<Vertex> boxVertices = {
            {-1.0f, -0.5f,  1.0f,  0.26f, 0.64f, 0.94f}, { 1.0f, -0.5f,  1.0f,  0.26f, 0.64f, 0.94f}, { 1.0f,  0.5f,  1.0f,  0.26f, 0.64f, 0.94f}, {-1.0f,  0.5f,  1.0f,  0.26f, 0.64f, 0.94f},
            {-1.0f, -0.5f, -1.0f,  0.20f, 0.80f, 0.74f}, { 1.0f, -0.5f, -1.0f,  0.20f, 0.80f, 0.74f}, { 1.0f,  0.5f, -1.0f,  0.20f, 0.80f, 0.74f}, {-1.0f,  0.5f, -1.0f,  0.20f, 0.80f, 0.74f},
            {-1.0f,  0.5f,  1.0f,  0.72f, 0.47f, 0.27f}, { 1.0f,  0.5f,  1.0f,  0.72f, 0.47f, 0.27f}, { 1.0f,  0.5f, -1.0f,  0.72f, 0.47f, 0.27f}, {-1.0f,  0.5f, -1.0f,  0.72f, 0.47f, 0.27f},
            {-1.0f, -0.5f,  1.0f,  0.31f, 0.76f, 0.41f}, { 1.0f, -0.5f,  1.0f,  0.31f, 0.76f, 0.41f}, { 1.0f, -0.5f, -1.0f,  0.31f, 0.76f, 0.41f}, {-1.0f, -0.5f, -1.0f,  0.31f, 0.76f, 0.41f},
            { 1.0f, -0.5f,  1.0f,  0.89f, 0.81f, 0.59f}, { 1.0f,  0.5f,  1.0f,  0.89f, 0.81f, 0.59f}, { 1.0f,  0.5f, -1.0f,  0.89f, 0.81f, 0.59f}, { 1.0f, -0.5f, -1.0f,  0.89f, 0.81f, 0.59f},
            {-1.0f, -0.5f,  1.0f,  0.85f, 0.66f, 0.74f}, {-1.0f,  0.5f,  1.0f,  0.85f, 0.66f, 0.74f}, {-1.0f,  0.5f, -1.0f,  0.85f, 0.66f, 0.74f}, {-1.0f, -0.5f, -1.0f,  0.85f, 0.66f, 0.74f},
            {-1.0f, -0.5f,  1.0f,  0.87f, 0.55f, 0.71f}, { 1.0f, -0.5f,  1.0f,  0.87f, 0.55f, 0.71f}, { 1.0f,  0.5f,  1.0f,  0.87f, 0.55f, 0.71f}, {-1.0f,  0.5f,  1.0f,  0.87f, 0.55f, 0.71f}
        };

        glGenVertexArrays(1, &boxVao);
        glGenBuffers(1, &boxVbo);
        glBindVertexArray(boxVao);
        glBindBuffer(GL_ARRAY_BUFFER, boxVbo);
        glBufferData(GL_ARRAY_BUFFER, boxVertices.size() * sizeof(Vertex), boxVertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
        glBindVertexArray(0);

        const std::vector<glm::vec3> outlineVertices = {
            { -1.0f, -1.0f, -1.0f }, { 1.0f, -1.0f, -1.0f },
            { 1.0f, -1.0f, -1.0f }, { 1.0f,  1.0f, -1.0f },
            { 1.0f,  1.0f, -1.0f }, { -1.0f,  1.0f, -1.0f },
            { -1.0f,  1.0f, -1.0f }, { -1.0f, -1.0f, -1.0f },
            { -1.0f, -1.0f,  1.0f }, { 1.0f, -1.0f,  1.0f },
            { 1.0f, -1.0f,  1.0f }, { 1.0f,  1.0f,  1.0f },
            { 1.0f,  1.0f,  1.0f }, { -1.0f,  1.0f,  1.0f },
            { -1.0f,  1.0f,  1.0f }, { -1.0f, -1.0f,  1.0f },
            { -1.0f, -1.0f, -1.0f }, { -1.0f, -1.0f,  1.0f },
            {  1.0f, -1.0f, -1.0f }, {  1.0f, -1.0f,  1.0f },
            {  1.0f,  1.0f, -1.0f }, {  1.0f,  1.0f,  1.0f },
            { -1.0f,  1.0f, -1.0f }, { -1.0f,  1.0f,  1.0f }
        };

        glGenVertexArrays(1, &outlineVao);
        glGenBuffers(1, &outlineVbo);
        glBindVertexArray(outlineVao);
        glBindBuffer(GL_ARRAY_BUFFER, outlineVbo);
        glBufferData(GL_ARRAY_BUFFER, outlineVertices.size() * sizeof(glm::vec3), outlineVertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glDisableVertexAttribArray(1);
        glVertexAttrib3f(1, 1.0f, 1.0f, 1.0f);
        glBindVertexArray(0);

        initialized = true;
    }

    glm::mat4 projection = glm::perspective(glm::radians(60.0f), float(width) / float(height > 0 ? height : 1), 0.1f, 100.0f);
    glm::mat4 view = ViewportInteractionUtils::BuildSceneView(yaw, pitch, distance, target);
    glm::mat4 model = glm::mat4(1.0f);

    glClearColor(0.247f, 0.247f, 0.247f, 1.0f); // #3F3F3F (Blender-like)
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_CULL_FACE);

    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "uProjection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(program, "uView"), 1, GL_FALSE, &view[0][0]);
    glUniform3f(glGetUniformLocation(program, "uColor"), 1.0f, 1.0f, 1.0f);

    glBindVertexArray(gridVao);
    glUniformMatrix4fv(glGetUniformLocation(program, "uModel"), 1, GL_FALSE, &model[0][0]);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(4 * (2 * 120 + 1)));

    const glm::vec3 wallPos = objectPositions.count("Wall") ? objectPositions.at("Wall") : glm::vec3(0.0f, 1.5f, -2.0f);
    const glm::vec3 wallScale = objectScales.count("Wall") ? objectScales.at("Wall") : glm::vec3(3.0f, 1.5f, 0.6f);
    const glm::vec3 playerPos = objectPositions.count("Player Spawn") ? objectPositions.at("Player Spawn") : glm::vec3(-4.0f, 1.3f, 0.0f);
    const glm::vec3 playerScale = objectScales.count("Player Spawn") ? objectScales.at("Player Spawn") : glm::vec3(1.0f, 1.5f, 1.0f);
    const glm::vec3 npcPos = objectPositions.count("NPC Spawn") ? objectPositions.at("NPC Spawn") : glm::vec3(4.0f, 1.0f, 1.5f);
    const glm::vec3 npcScale = objectScales.count("NPC Spawn") ? objectScales.at("NPC Spawn") : glm::vec3(0.7f, 1.0f, 0.7f);
    glm::mat4 wallModel = glm::translate(glm::mat4(1.0f), wallPos) * glm::scale(glm::mat4(1.0f), wallScale);
    glm::mat4 playerModel = glm::translate(glm::mat4(1.0f), playerPos) * glm::scale(glm::mat4(1.0f), playerScale);
    glm::mat4 npcModel = glm::translate(glm::mat4(1.0f), npcPos) * glm::scale(glm::mat4(1.0f), npcScale);

    glBindVertexArray(boxVao);
    glUniform3f(glGetUniformLocation(program, "uColor"), 1.0f, 1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(program, "uModel"), 1, GL_FALSE, &wallModel[0][0]);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 16, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 20, 4);

    glUniformMatrix4fv(glGetUniformLocation(program, "uModel"), 1, GL_FALSE, &playerModel[0][0]);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 16, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 20, 4);

    glUniformMatrix4fv(glGetUniformLocation(program, "uModel"), 1, GL_FALSE, &npcModel[0][0]);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 16, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 20, 4);

    glm::mat4 selectedModel = glm::mat4(1.0f);
    if (selectedObject == "Wall") {
        selectedModel = wallModel;
    } else if (selectedObject == "Player Spawn") {
        selectedModel = playerModel;
    } else if (selectedObject == "NPC Spawn") {
        selectedModel = npcModel;
    } else if (selectedObject == "Ground") {
        selectedModel = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.05f, 0.0f)), glm::vec3(12.0f, 0.1f, 12.0f));
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

} // namespace Editor::ViewportSceneRenderer
