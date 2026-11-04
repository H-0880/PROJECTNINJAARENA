#pragma once

#include "CoreMinimal.h"
#include "PNATypes.generated.h"

/** High-level locomotion state, intended to drive the Animation Blueprint. */
UENUM(BlueprintType)
enum class EPNAMovementState : uint8
{
    Grounded    UMETA(DisplayName = "Grounded"),
    Sprinting   UMETA(DisplayName = "Sprinting"),
    Falling     UMETA(DisplayName = "Falling"),
    Dashing     UMETA(DisplayName = "Dashing"),
    WallRunning UMETA(DisplayName = "WallRunning")
};

/** Custom movement mode ids used with MOVE_Custom. */
namespace PNAMovementModes
{
    constexpr uint8 WallRun = 1;
}
