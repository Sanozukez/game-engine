#include "../src/editor/editor_sky_serialization.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool NearlyEqual(float a, float b, float eps = 1e-4f) {
    return std::fabs(a - b) < eps;
}

int Fail(const std::string& message) {
    std::cerr << "[FAIL] " << message << "\n";
    return 1;
}

} // namespace

int main() {
    using Editor::SkySerialization::ApplySkyField;
    using Editor::SkySerialization::BuildSkyLines;
    using Editor::SkySerialization::IsSkyKey;
    using Editor::SkySerialization::ParseLineKeyValue;
    using Editor::SkySerialization::SkyData;

    {
        SkyData data;
        std::vector<std::string> lines = {
            "sky.asset_profile: project/assets/sky/rohan_soft_day.skyprofile",
            "sky.procedural_enabled: 1",
            "sky.sun_yaw_deg: -15.5",
            "sky.sun_pitch_deg: 12.25",
            "sky.cloud_speed: 0.0042",
            "sky.cloud_direction_deg: 27",
            "sky.cloud_coverage: 0.61",
            "sky.cloud_softness: 0.07",
            "sky.cloud_scale: 1.93",
            "sky.cloud_opacity: 0.73",
            "sky.zenith_color: 0.21,0.42,0.75",
            "sky.horizon_color: 0.66,0.74,0.88",
            "sky.cloud_color: 0.90,0.91,0.92"
        };

        for (const std::string& line : lines) {
            std::string key;
            std::string value;
            if (ParseLineKeyValue(line, key, value)) {
                ApplySkyField(data, key, value);
            }
        }

        if (data.assetProfile != "project/assets/sky/rohan_soft_day.skyprofile") {
            return Fail("asset profile nao foi parseado corretamente");
        }
        if (!data.proceduralEnabled) {
            return Fail("proceduralEnabled deveria ser true");
        }
        if (!NearlyEqual(data.sunYawDeg, -15.5f)) {
            return Fail("sunYawDeg invalido");
        }
        if (!NearlyEqual(data.cloudCoverage, 0.61f)) {
            return Fail("cloudCoverage invalido");
        }
        if (!NearlyEqual(data.cloudColor.b, 0.92f)) {
            return Fail("cloudColor.b invalido");
        }
    }

    {
        SkyData data;
        const float defaultYaw = data.sunYawDeg;
        const Editor::SkySerialization::Color3 defaultCloudColor = data.cloudColor;

        ApplySkyField(data, "sky.sun_yaw_deg", "abc");
        ApplySkyField(data, "sky.cloud_color", "0.1,0.2");

        if (!NearlyEqual(data.sunYawDeg, defaultYaw)) {
            return Fail("valor invalido deveria manter fallback de sunYawDeg");
        }
        if (!NearlyEqual(data.cloudColor.r, defaultCloudColor.r) ||
            !NearlyEqual(data.cloudColor.g, defaultCloudColor.g) ||
            !NearlyEqual(data.cloudColor.b, defaultCloudColor.b)) {
            return Fail("color invalida deveria manter fallback de cloudColor");
        }
    }

    {
        SkyData data;
        data.assetProfile = "project/assets/sky/night.skyprofile";
        std::vector<std::string> lines = BuildSkyLines(data, true);
        if (lines.empty() || lines.front().find("sky.asset_profile:") != 0) {
            return Fail("BuildSkyLines deveria incluir sky.asset_profile");
        }

        std::vector<std::string> linesNoAsset = BuildSkyLines(data, false);
        if (!linesNoAsset.empty() && linesNoAsset.front().find("sky.asset_profile:") == 0) {
            return Fail("BuildSkyLines nao deveria incluir asset_profile quando includeAssetProfile=false");
        }
    }

    {
        if (!IsSkyKey("sky.cloud_speed")) {
            return Fail("IsSkyKey falhou para chave valida");
        }
        if (IsSkyKey("terrain_source")) {
            return Fail("IsSkyKey retornou true para chave invalida");
        }
    }

    std::cout << "[PASS] test_sky_serialization\n";
    return 0;
}
