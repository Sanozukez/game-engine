#pragma once

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace Editor::SkySerialization {

struct Color3 {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
};

struct SkyData {
    bool proceduralEnabled = true;
    float sunYawDeg = 35.0f;
    float sunPitchDeg = 22.0f;
    float cloudSpeed = 0.0035f;
    float cloudDirectionDeg = 18.0f;
    float cloudCoverage = 0.54f;
    float cloudSoftness = 0.08f;
    float cloudScale = 1.75f;
    float cloudOpacity = 0.72f;
    Color3 zenithColor = {0.20f, 0.39f, 0.70f};
    Color3 horizonColor = {0.70f, 0.78f, 0.90f};
    Color3 cloudColor = {0.92f, 0.93f, 0.95f};
    std::string assetProfile;
};

inline std::string TrimCopy(const std::string& value) {
    auto begin = value.begin();
    while (begin != value.end() && std::isspace(static_cast<unsigned char>(*begin))) {
        ++begin;
    }

    auto end = value.end();
    while (end != begin && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }

    return std::string(begin, end);
}

inline bool StartsWith(const std::string& value, const std::string& prefix) {
    return value.rfind(prefix, 0) == 0;
}

inline bool IsSkyKey(const std::string& key) {
    return StartsWith(key, "sky.");
}

inline bool ParseLineKeyValue(const std::string& line, std::string& keyOut, std::string& valueOut) {
    const std::string trimmed = TrimCopy(line);
    if (trimmed.empty() || trimmed[0] == '#') {
        return false;
    }

    const std::size_t separatorPos = trimmed.find(':');
    if (separatorPos == std::string::npos) {
        return false;
    }

    keyOut = TrimCopy(trimmed.substr(0, separatorPos));
    valueOut = TrimCopy(trimmed.substr(separatorPos + 1));
    return !keyOut.empty();
}

inline float ParseFloatOrDefault(const std::string& value, float fallback) {
    try {
        return std::stof(TrimCopy(value));
    } catch (...) {
        return fallback;
    }
}

inline bool ParseBoolOrDefault(const std::string& value, bool fallback) {
    const std::string normalized = TrimCopy(value);
    if (normalized == "1" || normalized == "true" || normalized == "TRUE") {
        return true;
    }
    if (normalized == "0" || normalized == "false" || normalized == "FALSE") {
        return false;
    }
    return fallback;
}

inline Color3 ParseColor3OrDefault(const std::string& value, const Color3& fallback) {
    std::stringstream ss(value);
    std::string chunk;
    std::vector<float> channels;

    while (std::getline(ss, chunk, ',')) {
        channels.push_back(ParseFloatOrDefault(chunk, 0.0f));
    }

    if (channels.size() != 3) {
        return fallback;
    }

    return Color3{channels[0], channels[1], channels[2]};
}

inline void ApplySkyField(SkyData& skyData, const std::string& key, const std::string& value) {
    if (key == "sky.asset_profile") {
        skyData.assetProfile = value;
    } else if (key == "sky.procedural_enabled") {
        skyData.proceduralEnabled = ParseBoolOrDefault(value, skyData.proceduralEnabled);
    } else if (key == "sky.sun_yaw_deg") {
        skyData.sunYawDeg = ParseFloatOrDefault(value, skyData.sunYawDeg);
    } else if (key == "sky.sun_pitch_deg") {
        skyData.sunPitchDeg = ParseFloatOrDefault(value, skyData.sunPitchDeg);
    } else if (key == "sky.cloud_speed") {
        skyData.cloudSpeed = ParseFloatOrDefault(value, skyData.cloudSpeed);
    } else if (key == "sky.cloud_direction_deg") {
        skyData.cloudDirectionDeg = ParseFloatOrDefault(value, skyData.cloudDirectionDeg);
    } else if (key == "sky.cloud_coverage") {
        skyData.cloudCoverage = ParseFloatOrDefault(value, skyData.cloudCoverage);
    } else if (key == "sky.cloud_softness") {
        skyData.cloudSoftness = ParseFloatOrDefault(value, skyData.cloudSoftness);
    } else if (key == "sky.cloud_scale") {
        skyData.cloudScale = ParseFloatOrDefault(value, skyData.cloudScale);
    } else if (key == "sky.cloud_opacity") {
        skyData.cloudOpacity = ParseFloatOrDefault(value, skyData.cloudOpacity);
    } else if (key == "sky.zenith_color") {
        skyData.zenithColor = ParseColor3OrDefault(value, skyData.zenithColor);
    } else if (key == "sky.horizon_color") {
        skyData.horizonColor = ParseColor3OrDefault(value, skyData.horizonColor);
    } else if (key == "sky.cloud_color") {
        skyData.cloudColor = ParseColor3OrDefault(value, skyData.cloudColor);
    }
}

inline std::vector<std::string> BuildSkyLines(const SkyData& skyData, bool includeAssetProfile) {
    std::vector<std::string> lines;

    if (includeAssetProfile && !skyData.assetProfile.empty()) {
        lines.push_back("sky.asset_profile: " + skyData.assetProfile);
    }

    lines.push_back("sky.procedural_enabled: " + std::to_string(skyData.proceduralEnabled ? 1 : 0));
    lines.push_back("sky.sun_yaw_deg: " + std::to_string(skyData.sunYawDeg));
    lines.push_back("sky.sun_pitch_deg: " + std::to_string(skyData.sunPitchDeg));
    lines.push_back("sky.cloud_speed: " + std::to_string(skyData.cloudSpeed));
    lines.push_back("sky.cloud_direction_deg: " + std::to_string(skyData.cloudDirectionDeg));
    lines.push_back("sky.cloud_coverage: " + std::to_string(skyData.cloudCoverage));
    lines.push_back("sky.cloud_softness: " + std::to_string(skyData.cloudSoftness));
    lines.push_back("sky.cloud_scale: " + std::to_string(skyData.cloudScale));
    lines.push_back("sky.cloud_opacity: " + std::to_string(skyData.cloudOpacity));
    lines.push_back("sky.zenith_color: " + std::to_string(skyData.zenithColor.r) + "," + std::to_string(skyData.zenithColor.g) + "," + std::to_string(skyData.zenithColor.b));
    lines.push_back("sky.horizon_color: " + std::to_string(skyData.horizonColor.r) + "," + std::to_string(skyData.horizonColor.g) + "," + std::to_string(skyData.horizonColor.b));
    lines.push_back("sky.cloud_color: " + std::to_string(skyData.cloudColor.r) + "," + std::to_string(skyData.cloudColor.g) + "," + std::to_string(skyData.cloudColor.b));

    return lines;
}

} // namespace Editor::SkySerialization
