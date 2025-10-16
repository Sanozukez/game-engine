// // engine/math/vec3_json.h
#pragma once

// Inclui as dependências para glm::vec3
#include <glm/glm.hpp> 

// Inclui a biblioteca JSON (nlohmann::json)
#include <nlohmann/json.hpp>

// -------------------------------------------------------------------------
// JSON SERIALIZATION / DESERIALIZATION (Funções de Conversão)
// -------------------------------------------------------------------------

// Nota: Estas funções devem estar no namespace 'glm' para que a biblioteca
// nlohmann/json as encontre automaticamente (Argument-Dependent Lookup - ADL).

namespace glm {
    // 1. Desserialização: JSON -> glm::vec3
    inline void from_json(const nlohmann::json& j, glm::vec3& v) {
        // Assume que o JSON tem o formato array: [x, y, z]
        if (j.is_array() && j.size() == 3) {
            v.x = j[0].get<float>();
            v.y = j[1].get<float>();
            v.z = j[2].get<float>();
        } else {
            // Em caso de formato inválido, é seguro logar um erro, mas o getValue do ConfigManager 
            // já fornece um fallback, então podemos deixar a nlohmann::json falhar (ou usar um valor seguro).
        }
    }

    // 2. Serialização: glm::vec3 -> JSON (Boa prática, mas opcional para este erro)
    inline void to_json(nlohmann::json& j, const glm::vec3& v) {
        j = nlohmann::json::array({v.x, v.y, v.z});
    }
}