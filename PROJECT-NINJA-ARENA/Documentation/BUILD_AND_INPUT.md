# Build and Input Setup (UE 5.8) — PNA 0.2

## 1. Crear el proyecto
Nombra el proyecto **ProjectNinjaArena** (sin guiones bajos) para que el módulo y la macro `PROJECTNINJAARENA_API` coincidan con el código.
Third Person → C++ → Desktop → Scalable → sin Starter Content. Cierra el editor y copia encima `Source/`, `Config/` y `Documentation/`.
Si ya creaste `PROJECT_NINJA_ARENA`, reemplaza `PROJECTNINJAARENA_API` por `PROJECT_NINJA_ARENA_API` en los 3 headers públicos, o mueve los archivos a la carpeta del módulo existente.

## 2. Build.cs
En `ProjectNinjaArena.Build.cs`:
```csharp
PublicDependencyModuleNames.AddRange(new string[] {
    "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
```
Activa el plugin Enhanced Input (ya viene por defecto). Si algún include falla en 5.8, dímelo con el error exacto.

## 3. Input Actions (Content/Input)
| Asset | Tipo | Teclado/Ratón | Mando |
|---|---|---|---|
| IA_Move | Axis2D | WASD (Swizzle + Negate) | Stick izq. |
| IA_Look | Axis2D | Ratón XY | Stick der. |
| IA_Jump | Bool | Espacio | A / Cruz |
| IA_Sprint | Bool | Shift izq. | Stick izq. pulsado |
| IA_Dash | Bool | Ctrl izq. | B / Círculo |
| IA_LockOn | Bool | Tab | Stick der. pulsado |

Crea `IMC_Player` con estos mapeos. Verifica con `showdebug enhancedinput`.

## 4. Blueprint
1. `BP_PNA_Player` (hijo de PNACharacter): asigna `MappingContext` y las IA_*, y el Skeletal Mesh (con rig).
2. GameMode → Default Pawn Class = `BP_PNA_Player`.
3. Mapa de pruebas: una pared larga vertical (para wall run) y cubos/maniquíes con el tag `PNATarget` (para lock-on).

## 5. Pruebas de aceptación PNA 0.2
- [ ] Compila sin errores (Development Editor).
- [ ] Doble salto (2 saltos en el aire, se recupera al aterrizar).
- [ ] Dash en 8 direcciones en suelo; cooldown ~0.7 s.
- [ ] Air dash (1 por salto), flota sin caer durante el dash.
- [ ] Wall run: salto lateral hacia una pared corriendo a >350 uc/s con input hacia delante; sale al soltar input, al acabar el tiempo o en una esquina.
- [ ] Wall jump con Espacio durante el wall run; queda 1 salto aéreo.
- [ ] Lock-on: Tab fija el objetivo más centrado, la cámara lo sigue, se suelta a >2400 uc.
- [ ] Mando funciona igual que teclado.
- [ ] Multijugador (PIE: 2 jugadores, Net Mode Listen Server) sin tirones de corrección en sprint/dash.

## 6. Valores a ajustar primero
Todo está en `UPNAMovementComponent` (categorías `PNA|Movement|*`), editable en el Blueprint: velocidades, DashDuration, MaxAirDashes, WallRun*, WallJump*.

## Estado
Código no compilado contra UE 5.8 (sin motor disponible aquí). Es probable que haya 1–3 errores menores de firma/include; se corrigen con el log de compilación.
