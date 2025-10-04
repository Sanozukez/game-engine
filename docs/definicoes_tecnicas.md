### Build - Comandos


| Comando             | Ferramenta Principal                  | Função Exata                                                                                                                                                                                 |
| ------------------- | ------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| cmake -B build      | CMake (Sistema de Configuração)       | Configura e Gera. Analisa seus arquivos CMakeLists.txt, verifica dependências (FetchContent), e cria a pasta build/ contendo os arquivos de projeto nativos (Ex: .vcxproj do Visual Studio). |
| cmake --build build | MSBuild/Ninja (Sistema de Compilação) | Compila e Linka. Lê os arquivos de projeto gerados (.vcxproj) e executa o compilador (MSVC) para transformar seu código-fonte (.cpp) em binários (.obj, .lib, .exe).                         |

cmake -B build (Geração): Você só precisa executar este comando quando:

Adiciona um novo arquivo .cpp (ou move um arquivo).

Adiciona uma nova biblioteca de terceiros (dependencies.cmake).

Altera as flags de compilação no CMakeLists.txt.

Não limpa a build, mas recria o sistema de projeto.

cmake --build build (Compilação): Você o executa sempre que faz uma alteração no código (.cpp ou .h). Ele é eficiente porque compila apenas os arquivos que foram modificados.

### O que é uma cena (scene) no contexto da engine

O que é uma cena no contexto de um jogo.
Cena (Scene) é o Container de Estado e Comportamento do Mundo para um Dado Ponto no Tempo.