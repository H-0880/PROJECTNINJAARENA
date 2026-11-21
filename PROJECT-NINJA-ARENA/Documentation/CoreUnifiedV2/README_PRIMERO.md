# NINJA_ARENA — Core Unified V2

Base: Combat Core 3.2 + CombatFusion V1.

## Cambios activos
- `PNAVitalsComponent` sigue siendo autoridad única de Health/Energy/KO.
- `PNAAdvancedCombatComponent` sigue controlando I-Frames/HyperArmor/Parry/WeaponTrace.
- Se añade `APNAStageDirector`.
- Se añade `UPNADuelConstraintComponent` funcional.
- Se añade `EPNACombatState` y `Character::GetCombatState()`.
- Se añade `UPNADamageResolver` sin estado.
- El Character se conecta automáticamente al primer StageDirector del mapa.
- DuelConstraint sólo se activa en `Duel25D`.

## No activado
- `PNAResourceComponent`
- `PNADamageComponent` del Vertical Slice
- `PNACombatComponent` del Vertical Slice

Esto es intencional para evitar autoridades duplicadas.

## Variants
Se conservan físicamente intactos para no romper referencias. Desde ahora se consideran `Legacy/Reference`.

## Prueba segura
1. Cierra Unreal.
2. Ejecuta `Tools/APPLY_CORE_V2.ps1`.
3. Compila `NINJA_ARENAEditor`.
4. Prueba primero Arena3D.
5. Añade `APNAStageDirector` a un mapa de laboratorio.
6. Cambia a Duel25D y verifica la restricción.
7. Si falla, usa `Tools/RESTORE_PREVIOUS_SOURCE.ps1`.

Lee `Docs/PENDIENTE_DE_SOPESAR.md` antes de la próxima gran integración.
