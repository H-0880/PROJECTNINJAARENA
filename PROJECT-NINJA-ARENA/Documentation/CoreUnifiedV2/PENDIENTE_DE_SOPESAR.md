# PENDIENTE DE SOPESAR — NO OLVIDAR

Before the next major merge, explicitly weigh these items instead of adding them automatically:

1. **PNACombatComponent migration**
   - Extract attack execution and combo timers from Character only after V2 compiles/PIE passes.
   - Do not run a second combat state machine in parallel.

2. **Resource system**
   - Decide whether `PNAVitalsComponent` remains final or is later renamed/evolved.
   - Do not activate `PNAResourceComponent` while Vitals is authoritative.

3. **Damage architecture**
   - Move block/parry/armor/element calculations gradually into `PNADamageResolver`.
   - Health changes still end in `PNAVitalsComponent`.

4. **Variants**
   - Audit Blueprint/C++ references before moving Variant_* into a separate Legacy module.
   - Goal: faster compile and fewer dependencies, but only after reference validation.

5. **Movement / Lock-on**
   - Integrate donor ideas only after locating the actual current PNA movement/lock-on ownership.
   - Do not replace working movement blindly.

6. **GAS / GameplayTags**
   - Reconsider once 2–3 real characters need cooldowns/status effects/network prediction.
   - Not required for the current vertical slice.

7. **Multiplayer / rollback**
   - Do not harden rollback or prediction before local 1v1 and PIE two-client tests are stable.

8. **Kart**
   - Keep architecturally separate until Arena3D, Duel25D and Smash are cohesive.

9. **Donor library**
   - Migrate blocks with dependencies, never isolated `.uasset` files.
   - Retarget skeletons/animations in staging before main project migration.

10. **Presentation**
    - After core stability, prioritize animation quality, combat feedback, camera, UI and audio over adding more systems.
