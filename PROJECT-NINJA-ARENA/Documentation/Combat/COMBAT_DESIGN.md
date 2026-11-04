# Combat Design — Initial Specification

## Combat loop
Neutral → approach/evade → attack startup → active hit window → recovery → neutral or combo follow-up.

## Initial actions
- Light attack: fast startup, low commitment.
- Heavy attack: slower startup, stronger impact.
- Guard: reduces or prevents defined damage; guard-break rules later.
- Dodge: invulnerability frames only after frame/timing design is tested.
- Dash: movement tool; not automatically invulnerable.

## Required attack data
AttackId, damage, startup duration, active duration, recovery duration, hit reaction, knockback, stamina/energy cost, cancel rules and allowed follow-ups.

## Validation
Use debug hitboxes, combat logs and a training dummy. Do not tune damage before hit registration and animation timing are reliable.
