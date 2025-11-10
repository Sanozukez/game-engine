// Teste rápido para validar tamanhos das structs (Fase 1.1)
// Rodar após compilar: .\build\tests\Debug\test_struct_sizes.exe

#include <iostream>
#include "shared/mmap_format/SceneFileFormat.h"

int main() {
    std::cout << "=== Asset Dictionary Struct Sizes (v101) ===" << std::endl;
    std::cout << std::endl;
    
    std::cout << "AssetDictionaryHeader: " << sizeof(AssetDictionaryHeader) << " bytes" << std::endl;
    std::cout << "  Expected: 28 bytes (ajustado)" << std::endl;
    std::cout << std::endl;
    
    std::cout << "AssetEntry (v101): " << sizeof(AssetEntry) << " bytes" << std::endl;
    std::cout << "  v100 (old): 136 bytes" << std::endl;
    std::cout << "  v101 (new): 152 bytes" << std::endl;
    std::cout << "  Difference: +16 bytes (animation_count + animation_data_offset)" << std::endl;
    std::cout << std::endl;
    
    std::cout << "AnimationMapping: " << sizeof(AnimationMapping) << " bytes" << std::endl;
    std::cout << "  Expected: 88 bytes" << std::endl;
    std::cout << std::endl;
    
    std::cout << "=== Field Offsets (AssetEntry) ===" << std::endl;
    std::cout << "  asset_id offset: " << offsetof(AssetEntry, asset_id) << std::endl;
    std::cout << "  asset_path offset: " << offsetof(AssetEntry, asset_path) << std::endl;
    std::cout << "  asset_type offset: " << offsetof(AssetEntry, asset_type) << std::endl;
    std::cout << "  animation_count offset: " << offsetof(AssetEntry, animation_count) << std::endl;
    std::cout << "  animation_data_offset offset: " << offsetof(AssetEntry, animation_data_offset) << std::endl;
    std::cout << std::endl;
    
    std::cout << "=== Validation ===" << std::endl;
    if (sizeof(AssetDictionaryHeader) == 28) {
        std::cout << "✓ AssetDictionaryHeader size OK" << std::endl;
    } else {
        std::cout << "✗ AssetDictionaryHeader size INCORRECT!" << std::endl;
    }
    
    if (sizeof(AssetEntry) == 152) {
        std::cout << "✓ AssetEntry size OK" << std::endl;
    } else {
        std::cout << "✗ AssetEntry size INCORRECT!" << std::endl;
    }
    
    if (sizeof(AnimationMapping) == 88) {
        std::cout << "✓ AnimationMapping size OK" << std::endl;
    } else {
        std::cout << "✗ AnimationMapping size INCORRECT!" << std::endl;
    }
    
    return 0;
}
