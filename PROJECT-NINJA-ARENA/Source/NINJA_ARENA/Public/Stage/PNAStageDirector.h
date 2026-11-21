#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/PNATypes.h"
#include "PNAStageDirector.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPNAStageModeChanged, EPNAStageMode, OldMode, EPNAStageMode, NewMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPNARingout, AActor*, Victim);

UCLASS(Blueprintable)
class NINJA_ARENA_API APNAStageDirector : public AActor
{
    GENERATED_BODY()

public:
    APNAStageDirector();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PNA|Stage")
    EPNAStageMode CurrentMode = EPNAStageMode::Arena3D;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Stage")
    float RingoutZ = -900.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Stage")
    float DuelPlaneY = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Stage")
    float HazardDamage = 15.f;

    UPROPERTY(BlueprintAssignable, Category="PNA|Stage")
    FPNAStageModeChanged OnStageModeChanged;

    UPROPERTY(BlueprintAssignable, Category="PNA|Stage")
    FPNARingout OnRingout;

    UFUNCTION(BlueprintCallable, Category="PNA|Stage")
    void SetStageMode(EPNAStageMode NewMode);

    UFUNCTION(BlueprintPure, Category="PNA|Stage")
    EPNAStageMode GetStageMode() const { return CurrentMode; }

    UFUNCTION(BlueprintPure, Category="PNA|Stage")
    bool IsRingout(const FVector& Location) const;

    UFUNCTION(BlueprintPure, Category="PNA|Stage")
    FVector ConstrainToDuelPlane(const FVector& Location) const;

    UFUNCTION(BlueprintPure, Category="PNA|Stage")
    FVector ComputeKnockbackDirection(const FVector& Attacker, const FVector& Victim, float UpwardBias=0.25f) const;
};
