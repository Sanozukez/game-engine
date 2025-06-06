// M:/Dev/game-engine/temp_test.cpp
#include "cgltf.h" // Apenas tente incluir cgltf.h
int main() {
    // Apenas para que compile como um executável mínimo
    cgltf_options options = {0};
    cgltf_data* data = nullptr;
    cgltf_parse_file(&options, "temp.gltf", &data);
    if (data) cgltf_free(data);
    return 0;
}