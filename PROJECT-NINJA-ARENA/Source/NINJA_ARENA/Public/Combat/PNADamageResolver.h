#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PNADamageResolver.generated.h"

USTRUCT(BlueprintType)
struct FPNAResolvedDamage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float HealthDamage = 0.f;

    UPROPERTY(BlueprintReadOnly)
    float KnockbackScale = 1.f;

    UPROPERTY(BlueprintReadOnly)
    bool bBlocked = false;

    UPROPERTY(BlueprintReadOnly)
    bool bPerfectGuard = false;

    UPROPERTY(BlueprintReadOnly)
    bool bIgnoreHitStun = false;
};

/**
 * Stateless damage-policy helper.
 * It deliberately does NOT own Health or Energy. PNAVitalsComponent remains
 * the authoritative resource store during the Core Unified migration.
 */
UCLASS()
class NINJA_ARENA_API UPNADamageResolver : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="PNA|Combat|Damage")
    static FPNAResolvedDamage ResolveBasicDamage(
        float RawDamage,
        bool bBlocking,
        bool bPerfectGuard,
        bool bHyperArmor,
        float GuardDamageMultiplier = 0.2f);
};
