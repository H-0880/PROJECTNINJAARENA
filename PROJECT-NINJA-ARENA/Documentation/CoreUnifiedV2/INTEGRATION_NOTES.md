# CORE UNIFIED V2 — Integration Notes

## What changed
- Added `PNATypes.h` with `EPNACombatState`.
- Added `APNAStageDirector`.
- Added functional `UPNADuelConstraintComponent` using CharacterMovement plane constraints.
- Added stateless `UPNADamageResolver`.
- Connected Character to StageDirector/DuelConstraint.
- Added `GetCombatState()` as a canonical read-only projection of legacy Core 3.2 state.
- Kept `PNAVitalsComponent` as the only Health/Energy authority.
- Did NOT activate Vertical Slice Resource/Damage/Combat components.
- Kept Variant_* intact for compatibility.

## Safe test order
1. Back up Source.
2. Compile Editor.
3. Arena3D PIE: verify movement/combat unchanged.
4. Place `APNAStageDirector`; keep mode Arena3D and re-test.
5. Switch StageDirector to Duel25D and verify Y-plane constraint.
6. Switch back to Arena3D and verify free 3D movement returns.
7. Test respawn.
8. Test guard/parry/i-frames/hyper armor.
9. Inspect `GetCombatState()` in Blueprint/debug HUD.

## Important
This package has static validation only in this environment. It still requires
UnrealBuildTool/UHT + PIE validation in the real UE 5.8 project.
