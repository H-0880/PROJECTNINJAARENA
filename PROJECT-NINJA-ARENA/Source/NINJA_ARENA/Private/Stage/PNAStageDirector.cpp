#include "Stage/PNAStageDirector.h"

APNAStageDirector::APNAStageDirector()
{
    PrimaryActorTick.bCanEverTick = false;
}

void APNAStageDirector::SetStageMode(EPNAStageMode NewMode)
{
    if (CurrentMode == NewMode)
    {
        return;
    }

    const EPNAStageMode OldMode = CurrentMode;
    CurrentMode = NewMode;
    OnStageModeChanged.Broadcast(OldMode, NewMode);
}

bool APNAStageDirector::IsRingout(const FVector& Location) const
{
    return Location.Z <= RingoutZ;
}

FVector APNAStageDirector::ConstrainToDuelPlane(const FVector& Location) const
{
    FVector Out = Location;
    Out.Y = DuelPlaneY;
    return Out;
}

FVector APNAStageDirector::ComputeKnockbackDirection(const FVector& Attacker, const FVector& Victim, float UpwardBias) const
{
    FVector Direction = Victim - Attacker;
    Direction.Z = 0.f;

    if (Direction.IsNearlyZero())
    {
        Direction = FVector::ForwardVector;
    }
    else
    {
        Direction.Normalize();
    }

    Direction.Z = FMath::Max(0.f, UpwardBias);
    return Direction.GetSafeNormal();
}
