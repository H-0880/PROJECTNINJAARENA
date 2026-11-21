#pragma once

#include "CoreMinimal.h"
#include "PNATypes.generated.h"

UENUM(BlueprintType)
enum class EPNAStageMode : uint8
{
    Arena3D UMETA(DisplayName="Arena 3D"),
    Duel25D UMETA(DisplayName="Duel 2.5D"),
    Smash UMETA(DisplayName="Smash"),
    Kart UMETA(DisplayName="Kart")
};

UENUM(BlueprintType)
enum class EPNAHitStrength : uint8
{
    Light,
    Medium,
    Heavy,
    Special
};

/**
 * Readable high-level combat state.
 *
 * Combat Core 3.2 still owns the legacy booleans/timers during this migration.
 * NINJA_ARENACharacter::GetCombatState derives one canonical state from them.
 * This avoids adding a second competing state machine while giving Blueprint/UI
 * a single source for presentation and later refactoring.
 */
UENUM(BlueprintType)
enum class EPNACombatState : uint8
{
    Neutral,
    Attacking,
    Recovery,
    Blocking,
    Parrying,
    Dodging,
    HitStun,
    Launched,
    Airborne,
    KO,
    Ultimate,
    RoundLocked
};

USTRUCT(BlueprintType)
struct FPNAHitData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Damage = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float HitStun = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float KnockbackForce = 650.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float UpwardBias = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EPNAHitStrength Strength = EPNAHitStrength::Light;
};
