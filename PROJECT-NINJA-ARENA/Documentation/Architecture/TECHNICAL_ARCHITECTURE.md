# PNA Technical Architecture

## Runtime layers
1. Core: game instance, game modes, player state, shared tags and data.
2. Character: pawn, movement, camera, input and animation-facing interfaces.
3. Combat: attack definitions, hit detection, damage, guard, evade and combo state.
4. Abilities: ability activation, costs, cooldowns and effects. Adopt GAS incrementally after the base project compiles.
5. AI: perception, behavior and combat decision-making.
6. Multiplayer: server-authoritative match state, replicated player state and validated combat requests.
7. Modes: Arena first; Racing and Adventure remain isolated modules.

## Rules
- Keep input intent separate from combat outcomes.
- Treat the server as authority for damage, cooldowns and match results.
- Use data assets/data tables for tunable character values.
- Avoid per-frame Blueprint tick where event-driven logic is sufficient.
- Prefer animation montages and notify windows for authored attacks; validate hit windows in playtests.
- Keep platform-specific input mappings separate from gameplay actions.

## First vertical slice
One player, one small test arena, one target dummy, movement/camera/dash, then one light attack with a visible hit-confirm debug trace.
