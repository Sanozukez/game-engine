# Próximos Passos - Skybox, UI e Pipeline de Assets/Cenas

Este documento resume o estado atual e define o próximo ciclo de implementação para continuar amanhã.

## 1. Status atual (já implementado)

### Sky procedural no editor
- Céu procedural renderizado no viewport.
- Nuvens estilizadas com ruído + limiar (look recortado, estilo MMORPG clássico).
- Movimento lento de nuvens com direção configurável.
- Controle de sol (yaw/pitch), cobertura/opacidade/escala/softness de nuvens e cores de céu.

### UI de edição de céu
- Sliders e color pickers no painel `Sky / Clouds`.
- Presets rápidos: Noon, Sunset, Night.
- Botão de reset dos parâmetros.

### Persistência de sky na cena
- Carrega `sky.*` do arquivo de cena.
- Salva `sky.*` no arquivo de cena.
- Botões: `Save Sky To Scene` e `Reload Sky From Scene`.

### Sky Asset Browser (primeira versão)
- Pasta interna do projeto para assets de céu: `project/assets/sky`.
- Lista de arquivos `.skyprofile`.
- Ações implementadas:
  - salvar perfil atual como asset;
  - aplicar asset selecionado no céu atual;
  - sobrescrever asset;
  - deletar asset.

---

## 2. O que falta do skybox/sky system

### 2.1 Curto prazo (prioridade alta)
- Persistir na cena a referência do asset de sky selecionado:
  - exemplo de chave: `sky.asset_profile: project/assets/sky/rohan_soft_day.skyprofile`
- Na carga da cena:
  - se existir `sky.asset_profile`, carregar o `.skyprofile` automaticamente;
  - fallback para `sky.*` embutido na cena, se não houver asset.
- Melhorar UX do browser:
  - indicador de asset ativo;
  - botão `Apply + Save to Scene` (aplica e grava referência de uma vez).

### 2.2 Médio prazo
- Refatorar parser customizado de `.scene`/`.skyprofile` para um módulo de serialização dedicado.
- Adicionar validação e mensagens de erro amigáveis para perfil inválido/corrompido.
- Adicionar import de sky profile externo para pasta interna do projeto (com cópia e normalização de nome).

### 2.3 Evolução visual
- Estrelas simples para preset noturno (baixo custo).
- Camada opcional de nuvem secundária para maior variação artística.
- Leve ajuste de curva de cor por horário (sem volumetria pesada).

---

## 3. UI: separação entre cena e asset (modelo correto)

### Conceito
- **Asset Browser**: gerencia arquivos reutilizáveis do projeto (importar, listar, versionar, deletar).
- **Scene Editor/Inspector**: instancia e referencia assets dentro da cena.

### Regra
- Cena não deve depender de caminhos absolutos externos.
- Asset importado deve ser copiado para pasta interna do projeto.
- Cena salva referências relativas a assets internos (não dados duplicados quando não necessário).

### Fluxo esperado
1. Importar/criar asset no Asset Browser.
2. Asset ficar em pasta do projeto (`project/assets/...`).
3. Usuário selecionar asset no Inspector/Scene.
4. Cena salvar referência para esse asset.
5. Runtime carregar asset por referência da cena.

---

## 4. Contrato sugerido para cena e sky asset

### Exemplo de cena (`.scene`)
- `sky.asset_profile: project/assets/sky/rohan_soft_day.skyprofile`
- `sky.procedural_enabled: 1` (opcional como fallback)

### Exemplo de sky profile (`.skyprofile`)
- `sky.sun_yaw_deg: ...`
- `sky.sun_pitch_deg: ...`
- `sky.cloud_speed: ...`
- `sky.cloud_direction_deg: ...`
- `sky.cloud_coverage: ...`
- `sky.cloud_softness: ...`
- `sky.cloud_scale: ...`
- `sky.cloud_opacity: ...`
- `sky.zenith_color: r,g,b`
- `sky.horizon_color: r,g,b`
- `sky.cloud_color: r,g,b`

---

## 5. Backlog de amanhã (ordem recomendada)

1. Salvar/ler `sky.asset_profile` no arquivo da cena.
2. Aplicar auto-load do `.skyprofile` na abertura da cena.
3. Botão `Apply + Save to Scene` no Sky Asset Browser.
4. Destacar visualmente perfil ativo na lista.
5. Build + smoke test do editor.
6. Se der tempo: stars noturnas simples.

---

## 6. Critérios de pronto (DoD)

- Selecionar um sky profile e fechar/abrir o editor mantém o mesmo céu automaticamente.
- Cena carrega o perfil referenciado sem intervenção manual.
- Não há dependência de arquivo externo fora de `project/`.
- Build do `game-engine-editor` verde.
- Fluxo claro para artista/design: criar asset -> aplicar na cena -> salvar cena.

---

## 7. Observações de direção de arte

- Manter visual no estilo Rohan/MU com upgrade moderno.
- Priorizar nuvens estilizadas e movimento sutil.
- Evitar volumetria pesada nesta fase para preservar performance e escopo.

---

## 8. Fluxo geral de assets/cenas (além de skybox)

Esta seção consolida a arquitetura recomendada para o editor como um todo.

### 8.1 Separação de responsabilidade na UI

- Asset Browser:
  - importar, versionar, visualizar e organizar assets do projeto;
  - ao importar, copiar para pasta interna (`project/assets/...`);
  - gerar metadados estáveis (`assetId`, tipo, caminho relativo, hash).

- Scene Hierarchy:
  - mostrar instâncias da cena atual;
  - não importar arquivo nesse painel;
  - operar no nível de entidades/componentes e referências.

- Inspector:
  - escolher qual asset cada componente referencia;
  - trocar referência da cena (sem mover/copiar arquivo aqui);
  - exemplo: `SkyComponent.skyProfileAssetId`.

### 8.2 Fluxo ideal de uso

1. Importar asset no Asset Browser.
2. Asset virar entrada no catálogo do projeto.
3. Criar entidade/componente na cena (ex.: Sky, Mesh, Material).
4. Selecionar no Inspector qual asset será referenciado.
5. Salvar cena com referência relativa (ou `assetId`).
6. Runtime resolver referência e carregar asset internamente.

### 8.3 Por que esse modelo escala

- Evita dependência de caminho externo de máquina.
- Facilita build, empacotamento e distribuição.
- Permite reuso de assets entre múltiplas cenas.
- Mantém SRP: conteúdo (asset) separado de instanciamento (cena).

---

## 9. Próximos passos do pipeline geral (não-sky)

### 9.1 Catálogo de assets do projeto

- Criar arquivo de catálogo único (ex.: `project/assets/catalog.assetdb`).
- Campos mínimos por item:
  - `assetId`;
  - `type` (sky_profile, mesh, texture, material, terrain, etc);
  - `relativePath`;
  - `displayName`;
  - `sourcePath` (opcional, para rastreabilidade);
  - `hash` (detecção de mudança).

### 9.2 Importador padrão do editor

- Botão `Import Asset` no Asset Browser.
- Copiar arquivo externo para subpasta interna por tipo:
  - `project/assets/sky`
  - `project/assets/models`
  - `project/assets/textures`
  - `project/assets/materials`
- Normalizar nome de arquivo.
- Registrar no catálogo.

### 9.3 Integração com cena

- Na cena, salvar referência por `assetId` (preferencial) ou caminho relativo.
- No load da cena, validar assets ausentes e reportar no console.
- No Inspector, exibir dropdown/picker por tipo de asset compatível.

### 9.4 UX mínima para produção

- Filtro por tipo no Asset Browser.
- Busca por nome.
- Renomear asset (com atualização de referência no catálogo).
- Remoção com confirmação e checagem de uso na cena.

---

## 10. Backlog consolidado para continuar amanhã

1. Finalizar `sky.asset_profile` com auto-load e referência ativa.
2. Introduzir catálogo de assets (`assetdb`) com leitura/escrita básica.
3. Implementar importador genérico com cópia para `project/assets/...`.
4. Expor picker de asset no Inspector (começando por sky profile).
5. Registrar mensagens de erro amigáveis para referência quebrada.
6. Build + smoke test do editor ao final de cada etapa.
