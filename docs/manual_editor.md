# Manual do Editor do Game Engine

## Visão geral

O editor é a ferramenta principal para construir, ajustar e validar o conteúdo do jogo. Ele não é apenas um palco bonito: ele concentra o fluxo de desenvolvimento em torno de quatro funções centrais:

- criar e organizar a cena
- selecionar e ajustar objetos
- testar o jogo em runtime
- acompanhar logs, assets e status do projeto

Para um MMORPG, o editor precisa facilitar o trabalho de cenografia, entidades, gameplay, itens, NPCs e mundo. A estrutura da interface deve ser orientada para essas tarefas, não para estética de estúdio de áudio.

---

## 1. O que é um editor de cena

O editor de cena é a área do engine onde você monta e ajusta um "estado do mundo" que o jogo vai carregar e executar. Em termos práticos, ele não é apenas um visor bonito: é o ambiente em que você cria a experiência do jogo antes de testar o runtime.

### O que é uma cena
Uma cena é a unidade de conteúdo do jogo que representa um momento ou ambiente específico do mundo. Ela pode conter:

- terreno ou piso
- iluminação
- player start / spawn points
- NPCs e mobs
- props e estruturas
- colisores e limites de mapa
- prefabs e objetos reutilizáveis
- dados de gameplay relacionados ao mapa

Em outras palavras, uma cena é um "mapa + objetos + regras do ambiente". Em um MMORPG, ela pode ser uma vila, uma cidade, uma dungeon, uma região de mundo aberto, um campo de batalha etc.

### O que significa editar uma cena
Editar uma cena significa preparar esse conjunto de elementos e ajustar seu comportamento no mundo do jogo. Isso inclui:

- posicionar objetos no mundo
- alterar escala, rotação e transformações
- selecionar um NPC, um prop ou um ponto de spawn
- ajustar propriedades do ambiente
- validar se o chão, colisão e iluminação fazem sentido
- ajustar configuração do mapa antes de rodar o jogo

Na prática, você não "edita o jogo inteiro"; você edita uma instância específica do mundo, que pode depois ser carregada pelo runtime. É como montar um ambiente de produção antes de testar o fluxo de gameplay.

### Diferença entre cena, mundo e gameplay
- Cena: o ambiente / mapa atual em edição.
- Mundo: conjunto de cenas, regiões ou dados globais do jogo.
- Gameplay: a lógica que acontece dentro da cena, como movimentação, combate, quests, eventos e interações.

Uma cena pode ser parte de um mundo maior, mas ela é a unidade principal de edição visual e estrutural.

---

## 2. Menu principal

O menu superior organiza comandos globais do projeto.

### File
- Novo projeto
- Abrir projeto
- Salvar
- Salvar como
- Sair

Função: gerencia o ciclo de vida do projeto e do arquivo de cena.

### Edit
- Undo / Redo
- Duplication
- Delete

Função: ações rápidas de edição e manipulação de objetos.

### View
- mostrar/ocultar toolbar
- mostrar/ocultar hierarchy
- mostrar/ocultar inspector
- mostrar/ocultar console
- resetar layout

Função: personalizar a tela para cada momento de trabalho.

### World
- Terrain
- Lighting
- Prefabs

Função: acessar blocos estruturais do mundo.

### Tools
- Asset Browser
- Profiler
- Console

Função: ferramentas auxiliares de produção.

---

## 2. Toolbar

A toolbar é a área de ação rápida para executar o fluxo principal de desenvolvimento.

### Botões básicos
- Play: abre o runtime do jogo usando o executável do game-engine
- Pause: interrompe a simulação
- Stop: encerra a execução atual
- Build: compila ou reconstrói o projeto
- Bake: processa assets ou dados de bake

### Propósito de um editor de MMORPG
Nesta fase, o botão principal é o Play. Ele permite testar rapidamente:
- personagem controlado
- sistema de câmera
- física e colisão
- terreno e mundo
- lógica de gameplay básica

A interface de runtime dentro do editor existe para reduzir o tempo entre edição e validação.

---

## 3. Viewport do editor de cena

A viewport é a janela principal de visualização da cena. Ela é o "palco" onde você observa a estrutura do mapa e o estado atual do mundo em construção.

### O que ela representa na prática
Quando você trabalha na viewport, você está olhando para a versão editável do ambiente do jogo. É ali que você vê:

- o posicionamento dos objetos
- o alinhamento do terreno
- se o spawn está correto
- se a estrutura está no lugar certo
- se os elementos do mapa parecem coerentes juntos

É a área onde a lógica do mundo começa a virar uma experiência visualmente legível.

### Fluxo mental correto
No editor de cena, a sua atenção deve estar em:

1. o que está no mapa
2. onde cada elemento está
3. como eles se relacionam entre si
4. se o runtime vai interpretar corretamente aquele ambiente

---

## 4. Hierarquia da cena / World Outliner

A janela de hierarquia lista todos os objetos da cena em estrutura de árvore.

### O que ela mostra
- mundo 3D/2.5D
- objetos em cena
- iluminação
- entidades e prefabs
- acesso visual direta ao mundo

### Abas sugeridas
- Scene: visão principal do mundo
- Camera: visão de câmera e overlay de debug
- Runtime: painel dedicado ao estado do fluxo de execução do jogo

### Importância para produção de MMORPG
A viewport é onde você verá se a cena está correta, se o chão está alinhado, se o personagem está no lugar certo, se a iluminação funciona e se o mundo parece consistente.

---

## 5. Hierarquia da cena / World Outliner

A janela de hierarquia lista todos os objetos da cena em estrutura de árvore.

### Exemplos de elementos
- RootScene
- Player
- Terrain
- Environment
- NPCs
- Props
- Spawn Points

### Para que serve
- organizar objetos da cena
- entender a estrutura do mundo
- selecionar rapidamente entidades
- esconder ou ativar elementos
- montar hierarquias de objetos, NPCs e prefabs

### Em MMORPG
Isso é fundamental para lidar com:
- casas e mapas
- mobs e NPCs
- estruturas do mundo
- pontos de spawn
- prefabs reutilizáveis

---

## 6. Inspector

A janela de inspector mostra as propriedades do objeto selecionado.

### Seções típicas
- Transform
  - Position
  - Rotation
  - Scale
- Renderer
  - visible
  - material
  - scale
- Physics
  - gravity
  - kinematic
  - collision

### Propósito
Quando você seleciona um objeto, o inspector mostra tudo que pode ser ajustado. Ele é a ferramenta central para edição precisa.

### Em MMORPG
Isso será usado para:
- posicionar NPCs, itens e portas
- ajustar colisão de objetos
- configurar prefabs e logic components
- detalhar objetos do cenário

---

## 7. Console

A console exibe logs, avisos e mensagens do editor e do engine.

### Tipos de mensagem
- Info
- Warning
- Error
- Log

### Função prática
- diagnosticar falhas
- acompanhar carga de assets
- verificar problemas de cena
- detectar erros de compilação, runtime ou dados

### Em MMORPG
No desenvolvimento real, isso será usado para rastrear:
- asset faltando
- script com erro
- problema de colisão
- cena sem carregar corretamente

---

## 8. Runtime window / Play flow

A janela de runtime representa o fluxo principal de validação do jogo dentro do editor.

### Objetivo
Permitir que o desenvolvedor clique em Play dentro do editor e teste diretamente:
- gameplay
- personagens
- mundo
- logica de movimentação
- implantação da cena

### Fluxo recomendado
1. abrir editor
2. montar ou carregar mapa
3. selecionar cena
4. clicar em Play
5. observar runtime
6. ajustar propriedades
7. repetir

### Importância para MMORPG
Esse fluxo é o coração do ciclo de desenvolvimento de produção. Sem ele, você trava toda a validação em “edita e reabre o jogo manualmente”.

---

## 9. Como esse editor deve evoluir no seu projeto

Para um MMORPG, a interface deve crescer em camadas:

### Fase 1: editor base
- menu
- toolbar
- viewport
- hierarchy
- inspector
- console

### Fase 2: mundo e assets
- browser de assets
- material library
- terrain editor
- prefab library

### Fase 3: gameplay e dados
- NPC editor
- quest editor
- item editor
- spawn editor

### Fase 4: multiplayer / server tools
- connection debug
- region data
- world state view
- network inspector

---

## 10. Regra prática para não se perder

Quando você abrir o editor, pense assim:

- Hierarchy = lista dos elementos da cena
- Inspector = detalhes do elemento selecionado
- Viewport = visualização do mundo em construção
- Scene Editor = montagem e organização da cena
- Console = diagnóstico
- Toolbar = ações rápidas do fluxo de trabalho

Essa é a mental model mais importante para entender qualquer editor profissional. Em um engine de MMORPG, a maior parte do trabalho criativo e técnico começa no editor de cena, porque é nele que o ambiente ganha forma antes de virar gameplay real.

---

## 11. Resumo prático do que é "editar cena"

Editar cena significa trabalhar no conteúdo do mapa e dos objetos do jogo como um conjunto coeso. Você não está só movendo uma caixa no 3D; você está:

- definindo a estrutura do ambiente
- definindo local de spawn
- organizando NPCs, props e estruturas
- ajustando transformações para que o mundo fique coerente
- preparando o estado do jogo para ser validado em runtime

Quando o projeto evolui, isso vira a base para:

- quests e áreas de missão
- cidades e regiões
- caminhos de mobs
- sistemas de interação
- criação de mapas maiores e mais complexos

Em resumo: editar cena é preparar o mundo para que o jogo funcione e pareça correto antes de rodar a experiência completa.

---

## 10. Recomendação para o seu MMORPG

O foco principal deve ser tornar a criação do mundo e dos personagens rápida e visual. Para isso, as janelas mais importantes são:

1. Viewport
2. Hierarchy
3. Inspector
4. Console
5. Runtime/Play

Essas cinco áreas resolvem a maior parte do fluxo de produção.

---

## Resumo rápido

O editor funciona como um centro de comando do jogo. Ele permite:

- criar mundo
- organizar objetos
- ajustar propriedades
- testar gameplay
- corrigir erros em tempo real

O objetivo final não é fazer o editor bonito por si só, e sim fazer ele eficiente para construir seu MMORPG.
