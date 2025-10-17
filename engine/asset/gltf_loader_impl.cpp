// engine/asset/gltf_loader_impl.cpp
//
// Este arquivo tem a responsabilidade única (SRP) de ser o ponto de
// implementação para bibliotecas C de arquivo único (single-file libraries),
// como cgltf e stb_image.
//
// O uso das diretivas #define *IMPLEMENTATION* aqui garante:
// 1. Conformidade com o Princípio da Uma Definição (ODR): O código real das funções
//    é compilado APENAS UMA VEZ neste arquivo .cpp.
// 2. Estabilidade do Linker: Evita erros de ligação (LNK2005/LNK1169) no projeto principal.
// 3. SRP: Mantém o código lógico dos loaders (gltf_loader.cpp) limpo e separado da
//    implementação de terceiros.

#define CGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <cgltf.h>
#include <stb_image.h>