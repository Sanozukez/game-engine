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

struct SkyRenderParams {
    bool proceduralEnabled = true;
    float timeSeconds = 0.0f;
    float sunYawDeg = 35.0f;
    float sunPitchDeg = 22.0f;
    float cloudSpeed = 0.0035f;
    float cloudDirectionDeg = 18.0f;
    float cloudCoverage = 0.54f;
    float cloudSoftness = 0.08f;
    float cloudScale = 1.75f;
    float cloudOpacity = 0.72f;
    glm::vec3 zenithColor = glm::vec3(0.20f, 0.39f, 0.70f);
    glm::vec3 horizonColor = glm::vec3(0.70f, 0.78f, 0.90f);
    glm::vec3 cloudColor = glm::vec3(0.92f, 0.93f, 0.95f);
};

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

inline GLuint CreateSkyShader() {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        out vec2 vUv;
        void main() {
            vUv = aPos * 0.5 + 0.5;
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 vUv;
        out vec4 FragColor;

        uniform mat4 uInverseView;
        uniform mat4 uInverseProjection;
        uniform vec3 uSunDir;
        uniform vec3 uZenithColor;
        uniform vec3 uHorizonColor;
        uniform vec3 uCloudColor;
        uniform float uTime;
        uniform float uCloudSpeed;
        uniform float uCloudDirectionRad;
        uniform float uCloudCoverage;
        uniform float uCloudSoftness;
        uniform float uCloudScale;
        uniform float uCloudOpacity;

        float hash12(vec2 p) {
            vec3 p3 = fract(vec3(p.xyx) * 0.1031);
            p3 += dot(p3, p3.yzx + 33.33);
            return fract((p3.x + p3.y) * p3.z);
        }

        float valueNoise(vec2 p) {
            vec2 i = floor(p);
            vec2 f = fract(p);
            f = f * f * (3.0 - 2.0 * f);

            float a = hash12(i + vec2(0.0, 0.0));
            float b = hash12(i + vec2(1.0, 0.0));
            float c = hash12(i + vec2(0.0, 1.0));
            float d = hash12(i + vec2(1.0, 1.0));

            return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
        }

        float fbm(vec2 p) {
            float sum = 0.0;
            float amp = 0.5;
            float freq = 1.0;
            for (int i = 0; i < 5; ++i) {
                sum += valueNoise(p * freq) * amp;
                freq *= 2.02;
                amp *= 0.52;
            }
            return sum;
        }

        void main() {
            vec2 ndc = vUv * 2.0 - 1.0;
            vec4 clip = vec4(ndc, 1.0, 1.0);
            vec4 viewPos = uInverseProjection * clip;
            viewPos /= max(viewPos.w, 0.0001);
            vec3 dir = normalize((uInverseView * vec4(viewPos.xyz, 0.0)).xyz);

            float horizon = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);
            float t = pow(horizon, 1.25);
            vec3 sky = mix(uHorizonColor, uZenithColor, t);

            float downTint = smoothstep(0.06, -0.18, dir.y);
            sky = mix(sky, uHorizonColor * 0.48, downTint);

            float sunAmount = max(dot(normalize(uSunDir), dir), 0.0);
            vec3 sunGlow = vec3(1.0, 0.78, 0.52) * pow(sunAmount, 84.0) * 0.9;
            vec3 sunHalo = vec3(1.0, 0.63, 0.34) * pow(sunAmount, 18.0) * 0.35;

            vec2 windDir = vec2(cos(uCloudDirectionRad), sin(uCloudDirectionRad));
            vec2 cloudUV = dir.xz / max(dir.y + 0.34, 0.17);
            cloudUV *= uCloudScale;
            cloudUV += windDir * (uTime * uCloudSpeed);

            float largeShape = fbm(cloudUV * 0.72);
            float detailShape = fbm(cloudUV * 1.90 + vec2(9.2, -4.3));
            float n = mix(largeShape, detailShape, 0.38);

            float threshold = mix(0.66, 0.38, clamp(uCloudCoverage, 0.0, 1.0));
            float softness = max(uCloudSoftness, 0.01);
            float cloudMask = smoothstep(threshold - softness, threshold + softness, n);
            cloudMask *= smoothstep(-0.08, 0.20, dir.y);

            // Stylized cutout cloud look for classic MMORPG art direction.
            float stylizedMask = smoothstep(0.42, 0.58, cloudMask);
            vec3 color = sky + sunGlow + sunHalo;
            color = mix(color, uCloudColor, stylizedMask * clamp(uCloudOpacity, 0.0, 1.0));

            FragColor = vec4(color, 1.0);
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

inline void RenderProceduralSky(
    const glm::mat4& projection,
    const glm::mat4& view,
    const SkyRenderParams& skyParams) {
    static GLuint skyProgram = 0;
    static GLuint skyVao = 0;
    static GLuint skyVbo = 0;

    if (skyProgram == 0) {
        skyProgram = CreateSkyShader();

        const float fullscreenTri[6] = {
            -1.0f, -1.0f,
             3.0f, -1.0f,
            -1.0f,  3.0f
        };

        glGenVertexArrays(1, &skyVao);
        glGenBuffers(1, &skyVbo);
        glBindVertexArray(skyVao);
        glBindBuffer(GL_ARRAY_BUFFER, skyVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(fullscreenTri), fullscreenTri, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }

    const float yawRad = glm::radians(skyParams.sunYawDeg);
    const float pitchRad = glm::radians(skyParams.sunPitchDeg);
    const glm::vec3 sunDir = glm::normalize(glm::vec3(
        std::cos(pitchRad) * std::sin(yawRad),
        std::sin(pitchRad),
        -std::cos(pitchRad) * std::cos(yawRad)));

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glUseProgram(skyProgram);
    const glm::mat4 inverseView = glm::inverse(view);
    const glm::mat4 inverseProjection = glm::inverse(projection);

    glUniformMatrix4fv(glGetUniformLocation(skyProgram, "uInverseView"), 1, GL_FALSE, &inverseView[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(skyProgram, "uInverseProjection"), 1, GL_FALSE, &inverseProjection[0][0]);
    glUniform3fv(glGetUniformLocation(skyProgram, "uSunDir"), 1, &sunDir[0]);
    glUniform3fv(glGetUniformLocation(skyProgram, "uZenithColor"), 1, &skyParams.zenithColor[0]);
    glUniform3fv(glGetUniformLocation(skyProgram, "uHorizonColor"), 1, &skyParams.horizonColor[0]);
    glUniform3fv(glGetUniformLocation(skyProgram, "uCloudColor"), 1, &skyParams.cloudColor[0]);
    glUniform1f(glGetUniformLocation(skyProgram, "uTime"), skyParams.timeSeconds);
    glUniform1f(glGetUniformLocation(skyProgram, "uCloudSpeed"), skyParams.cloudSpeed);
    glUniform1f(glGetUniformLocation(skyProgram, "uCloudDirectionRad"), glm::radians(skyParams.cloudDirectionDeg));
    glUniform1f(glGetUniformLocation(skyProgram, "uCloudCoverage"), skyParams.cloudCoverage);
    glUniform1f(glGetUniformLocation(skyProgram, "uCloudSoftness"), skyParams.cloudSoftness);
    glUniform1f(glGetUniformLocation(skyProgram, "uCloudScale"), skyParams.cloudScale);
    glUniform1f(glGetUniformLocation(skyProgram, "uCloudOpacity"), skyParams.cloudOpacity);

    glBindVertexArray(skyVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glUseProgram(0);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

inline void RenderSimpleScene3D(
    int width,
    int height,
    float yaw,
    float pitch,
    float distance,
    const glm::vec3& target,
    const SkyRenderParams& skyParams,
    const std::string& selectedObject,
    const std::unordered_map<std::string, glm::vec3>& objectPositions,
    const std::unordered_map<std::string, glm::vec3>& objectScales,
    const std::unordered_map<std::string, glm::vec3>& objectRotationsDeg) {
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

    if (skyParams.proceduralEnabled) {
        glClear(GL_COLOR_BUFFER_BIT);
        RenderProceduralSky(projection, view, skyParams);
    } else {
        glClearColor(0.247f, 0.247f, 0.247f, 1.0f); // #3F3F3F (Blender-like)
        glClear(GL_COLOR_BUFFER_BIT);
    }

    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    glDisable(GL_CULL_FACE);

    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "uProjection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(program, "uView"), 1, GL_FALSE, &view[0][0]);
    glUniform3f(glGetUniformLocation(program, "uColor"), 1.0f, 1.0f, 1.0f);

    glBindVertexArray(gridVao);
    glUniformMatrix4fv(glGetUniformLocation(program, "uModel"), 1, GL_FALSE, &model[0][0]);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(4 * (2 * 120 + 1)));

    auto buildModel = [](const glm::vec3& position, const glm::vec3& rotationDeg, const glm::vec3& scaleValue) {
        glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(rotationDeg.y), glm::vec3(0.0f, 1.0f, 0.0f));
        rot = glm::rotate(rot, glm::radians(rotationDeg.x), glm::vec3(1.0f, 0.0f, 0.0f));
        rot = glm::rotate(rot, glm::radians(rotationDeg.z), glm::vec3(0.0f, 0.0f, 1.0f));
        return glm::translate(glm::mat4(1.0f), position) * rot * glm::scale(glm::mat4(1.0f), scaleValue);
    };

    std::vector<std::string> objectIds;
    objectIds.reserve(objectPositions.size());
    for (const auto& kv : objectPositions) {
        if (kv.first != "Ground") {
            objectIds.push_back(kv.first);
        }
    }
    std::sort(objectIds.begin(), objectIds.end());

    glBindVertexArray(boxVao);
    glUniform3f(glGetUniformLocation(program, "uColor"), 1.0f, 1.0f, 1.0f);

    for (const std::string& objectId : objectIds) {
        const glm::vec3 pos = objectPositions.count(objectId) ? objectPositions.at(objectId) : glm::vec3(0.0f);
        const glm::vec3 scl = objectScales.count(objectId) ? objectScales.at(objectId) : glm::vec3(1.0f);
        const glm::vec3 rot = objectRotationsDeg.count(objectId) ? objectRotationsDeg.at(objectId) : glm::vec3(0.0f);
        const glm::mat4 modelForObject = buildModel(pos, rot, scl);

        glUniformMatrix4fv(glGetUniformLocation(program, "uModel"), 1, GL_FALSE, &modelForObject[0][0]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
        glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
        glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);
        glDrawArrays(GL_TRIANGLE_STRIP, 16, 4);
        glDrawArrays(GL_TRIANGLE_STRIP, 20, 4);
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

} // namespace Editor::ViewportSceneRenderer
