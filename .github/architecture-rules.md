# Architecture Rules - Reusable First

## Objective
Ensure every new feature is designed for reuse in final game runtime (client) and server, not only for editor tooling.

## Core Principles
- SRP: one responsibility per module/class.
- Decoupling: domain logic must not depend on ImGui, OpenGL panel code, or platform-specific UI.
- Reuse-first: if runtime/server can use it, implement in shared engine modules first.
- Adapter pattern: editor consumes shared services through thin adapters.

## Where Code Belongs
- Shared reusable logic: engine/
- Editor tooling and UI: src/editor/
- Runtime integration: src/client/
- Server integration: src/server/

## Mandatory Implementation Flow
1. Define domain contracts and data structures in engine/.
2. Implement service logic in engine/ with clean APIs.
3. Add editor adapter in src/editor/ to consume the service.
4. Integrate runtime/client and server with the same service API.

## Mandatory Pre-Development Audit
Before creating any new feature/module:
1. Audit existing code paths in active and backup directories.
2. Confirm what already works in runtime/editor to avoid breaking flows.
3. Reuse/refactor existing logic when possible.
4. Only introduce new module names when no compatible base exists.

## Anti-Patterns to Avoid
- Implementing streaming/chunk logic only inside editor panel files.
- Coupling business rules to window/input UI events.
- Large all-in-one files that mix rendering, IO, state management, and domain rules.

## Done Criteria for New Features
- Can be consumed by editor, client runtime, and server without rewrite.
- Editor layer contains only tool UX and orchestration.
- Unit boundaries are clear and testable.
