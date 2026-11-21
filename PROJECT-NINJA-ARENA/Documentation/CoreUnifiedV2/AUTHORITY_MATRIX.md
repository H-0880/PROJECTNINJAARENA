# NINJA_ARENA CORE UNIFIED V2 — Authority Matrix

This package is a conservative refactor of CombatFusion V1.

## Active authorities

| Responsibility | Authority now | Notes |
|---|---|---|
| Health / Energy / KO | `UPNAVitalsComponent` | Single source of truth. |
| Legacy attack execution / combo timers | `ANINJA_ARENACharacter` | Preserved for compatibility; migrate later. |
| I-Frames / Hyper Armor / Parry / Weapon Trace | `UPNAAdvancedCombatComponent` + notifies | Additive layer. |
| Element states | `UNinjaElementComponent` | Preserved. |
| Hit feedback | `UNinjaCombatFeedbackComponent` | Preserved. |
| Fighter style/profile logic | `UNinjaCombatStyleComponent` | Preserved. |
| Stage mode / ringout rules | `APNAStageDirector` | New stage authority. |
| Duel plane constraint | `UPNADuelConstraintComponent` | Active only in Duel25D. |
| Damage policy helper | `UPNADamageResolver` | Stateless helper; owns no health. |

## Explicitly NOT activated

The Vertical Slice versions of:
- `UPNAResourceComponent`
- `UPNADamageComponent`
- `UPNACombatComponent`

are intentionally NOT added to the Character in V2.

Reason: Combat Core 3.2 already owns those responsibilities. Activating the new
components now would create duplicate health/combat state. They should be migrated
one responsibility at a time later.

## Variants

`Variant_Combat`, `Variant_Platforming`, and `Variant_SideScrolling` remain physically
untouched because existing code/assets may reference their C++ classes.

Treat them as **Legacy / Reference**. Do not build new NINJA_ARENA gameplay on top of
them. A later module split can remove them from the primary compile path after asset
reference auditing.
