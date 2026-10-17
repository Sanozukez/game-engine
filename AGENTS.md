# AGENTS.md

## Objetivo
Este arquivo define regras operacionais para evitar erros de execucao em ambientes diferentes (casa e servico), onde drive, shell e pasta inicial podem variar.

## Regra Principal: Sempre descobrir a raiz do repo antes de executar comandos
Nunca assumir caminho fixo como M:\\ ou C:\\.

Use sempre:
- PowerShell: $repoRoot = (git rev-parse --show-toplevel).Trim(); Set-Location $repoRoot

Se o comando acima falhar, pare e confirme que voce esta dentro de um repositorio Git.

## Politica de caminhos
- Nao hardcode caminhos absolutos para build e execucao.
- Sempre usar caminhos relativos a raiz do repo.
- Considerar que o mesmo projeto pode estar em maquinas com paths diferentes.

## VS Dev Shell (Visual Studio)
Abrir o VsDevShell pode trocar o diretorio atual para outro local (exemplo: C:\\Users\\...\\source\\repos).

Quando usar VsDevShell, SEMPRE fazer novamente:
1) Descobrir raiz: git rev-parse --show-toplevel
2) Voltar para raiz: Set-Location <raiz-do-repo>

## Fluxo padrao de build
Na raiz do repositorio:
1) cmake -S . -B build -G "Visual Studio 17 2022"
2) cmake --build build --config Debug --target game-engine
3) cmake --build build --config Debug --target game-engine-editor

## Fluxo padrao de execucao
Na raiz do repositorio:
- Jogo: .\\build\\src\\client\\Debug\\game-engine.exe
- Editor: .\\build\\src\\client\\Debug\\game-engine-editor.exe

## Pre-flight check obrigatorio (antes de build, run, commit, push)
Executar e validar:
1) git rev-parse --show-toplevel
2) Get-Location
3) git status -sb

Se houver divergencia entre Get-Location e a raiz do repo, corrigir antes de continuar.

## Regras para comandos automatizados
- Preferir comandos que comecem com "cd <raiz-do-repo>;" quando houver risco de cwd incorreto.
- Evitar comandos destrutivos sem confirmacao explicita.
- Em duvida sobre ambiente, validar estado com git status e Get-Location antes de executar.

## Observacao de contexto deste projeto
- Em casa, normalmente a execucao ocorre a partir de M:\\Dev\\game-engine.
- Em outras maquinas, o mesmo repo pode estar em C:\\... ou outro drive.
- Portanto, a automacao deve ser orientada por "raiz detectada" e nunca por path fixo.

## Regras de arquitetura (obrigatorio para novos modulos)
- Priorizar SRP: cada modulo deve ter responsabilidade unica e fronteiras claras.
- Priorizar desacoplamento: evitar dependencia de UI, renderer ou plataforma em regras de negocio.
- Tudo que for reutilizavel entre editor, client runtime e server deve nascer em modulo compartilhado da engine.
- Evitar criar logica "somente do editor" quando a feature pertence ao jogo final.
- Editor deve atuar como adaptador/orquestrador de UI para servicos de dominio compartilhados.

## Politica de posicionamento de codigo
- Dominio reutilizavel: colocar em `engine/` (ex.: streaming, world partition, regras de coordenadas, contratos de cena/chunk).
- Adaptadores de ferramenta: colocar em `src/editor/` (ex.: paineis ImGui, bindings de UI, debug overlays de autoria).
- Runtime client: usar os servicos compartilhados de `engine/` e adicionar somente integracao de jogo/render.
- Server: usar os mesmos servicos compartilhados de `engine/` sem depender de UI/editor.

## Checklist antes de implementar
1) Esta feature sera usada no jogo final (runtime ou server)?
2) A regra principal esta em `engine/` e nao presa ao editor?
3) O editor apenas consome a API compartilhada via adaptador?
4) O design evita arquivo "monstro" (SRP, composicao, modulos pequenos)?

## Regra de auditoria previa (obrigatoria)
Antes de iniciar qualquer feature nova (mapa, camera, streaming, inventario, UI, etc):
1) Verificar se ja existe implementacao parcial/experimental no projeto (pastas ativas e backup).
2) Mapear pontos de integracao para nao quebrar runtime existente.
3) Reaproveitar e refatorar o que ja existe, evitando duplicacao e conflito de logica.
4) So criar modulo novo quando nao houver base reaproveitavel.

Objetivo: evitar regressao por sobrescrever fluxo existente sem consciencia do estado atual do codigo.
