# Changelog
## PNA 0.2 — Advanced movement
- Movement: sprint y dash enviados como flags de CMC (predicción cliente + validación servidor); antes cambiaban MaxWalkSpeed solo en cliente y provocaban correcciones en red.
- Dash: ahora domina la velocidad horizontal vía CalcVelocity (antes la fricción lo frenaba), cooldown por delta de movimiento (determinista), ventana de invulnerabilidad `IsDashInvulnerable()` para el combate futuro.
- Air dash (MaxAirDashes), doble salto (JumpMaxCount = 2).
- Wall run como modo de movimiento custom + wall jump.
- Estado `EPNAMovementState` para el Animation Blueprint (`GetPNAMovementState`).
- Lock-on opcional (`UPNALockOnComponent`, actores con tag `PNATarget`).
- Mapping context se añade en `NotifyControllerChanged` (más fiable que BeginPlay).
### Limitaciones conocidas
- Dash/wall-run no se replican como estado a proxies simulados: sus animaciones se derivan de velocidad/modo (el modo custom sí se replica).
- Timers de cooldown no se restauran en re-simulación tras una corrección de red (impacto menor).
- Lock-on es solo cámara local; no hay cambio de objetivo todavía.
