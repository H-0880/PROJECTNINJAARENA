#include "Combat/PNADamageResolver.h"

FPNAResolvedDamage UPNADamageResolver::ResolveBasicDamage(
    float RawDamage,
    bool bBlocking,
    bool bPerfectGuard,
    bool bHyperArmor,
    float GuardDamageMultiplier)
{
    FPNAResolvedDamage Result;
    const float SafeDamage = FMath::Max(0.f, RawDamage);

    Result.bBlocked = bBlocking || bPerfectGuard;
    Result.bPerfectGuard = bPerfectGuard;
    Result.bIgnoreHitStun = bHyperArmor;

    if (bPerfectGuard)
    {
        Result.HealthDamage = 0.f;
        Result.KnockbackScale = 0.f;
    }
    else if (bBlocking)
    {
        Result.HealthDamage = SafeDamage * FMath::Clamp(GuardDamageMultiplier, 0.f, 1.f);
        Result.KnockbackScale = 0.35f;
    }
    else
    {
        Result.HealthDamage = SafeDamage;
        Result.KnockbackScale = 1.f;
    }

    return Result;
}
