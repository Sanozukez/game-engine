| Pasta | Conteúdo Principal | Propósito Lógico (SRP) | Influência no Build Final |
| :--- | :--- | :--- | :--- |
| **`assets/`** | Modelos (GLB), Texturas, Sons. | **Dados Fonte Imutáveis.** É o *source* de tudo que o jogo vê ou ouve. A Engine **LÊ** daqui. | Não é compilado; é **empacotado** no cliente. |
| **`build/`** | Binários, Arquivos `.vcxproj`, `.o`, `.exe`. | **Saída do Build.** O CMake e o MSBuild geram tudo aqui. | Contém o `.exe` final e bibliotecas. |
| **`cmake/`** | Scripts de *Build* Genéricos. | **Gerenciamento de Dependências.** Define como obter e configurar bibliotecas externas. | Usado apenas em tempo de compilação. |
| **`config/`** | `.json` de Parâmetros. | **Dados de Configuração Variável.** Informações que designers podem alterar sem recompilar. | Lido no *runtime* para ajustar o estado inicial do jogo. |
| **`docs/`** | Arquivos de Documentação. | **Conhecimento do Projeto.** | Nenhuma. |
| **`engine/`** | O Core da Engine (Lógica de Baixo Nível). | **Responsabilidade Fundamental (Core).** Sistemas de Física, Renderização, Áudio, que podem ser reutilizados em outros jogos. | Gerado como uma biblioteca (`.lib` ou `.dll`) para ser usada pelo `src/`. |
| **`deps/`** (dentro de `engine/`) | `cgltf`, `stb_image`. | **Dependências Simples.** Bibliotecas *header-only* ou de arquivo único. | Compilado junto com a `engine/` ou o *Core*. |
| **`glad/`** | Headers e Source de Carregamento OpenGL. | **Dependência de Baixo Nível.** Lida diretamente com a **API Gráfica** (OpenGL). | Compilado junto ao *Render Core*. |
| **`shared/`** | `mmap_format/SceneFileFormat.h`. | **API de Dados Compartilhada.** A ponte de dados entre o *Build Tool* (`tools/`) e a Engine Core (`engine/`). | Usada como *header* (definição de *structs*) nos projetos **`engine/`** e **`tools/`**. |
| **`tools/`** | `mmap_compiler/`, etc. | **Utilitários de Pré-processamento.** Código que roda *antes* do jogo para otimizar *assets* e gerar dados. | Gerado como um executável de **linha de comando** (ex: `mmap_compiler.exe`). |
| **`src/`** | `app.cpp`, `scene.cpp`, `input.cpp`. | **Aplicação/Runtime do Jogo.** É o **ponto de entrada** (`main()`) que une a Engine com o jogo. | Gera o executável final **`game-engine.exe`** (o cliente). |