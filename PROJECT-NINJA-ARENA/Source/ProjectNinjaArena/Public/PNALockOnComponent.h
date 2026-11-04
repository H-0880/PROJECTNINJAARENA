#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PNALockOnComponent.generated.h"

/**
 * Optional lock-on. Targets are actors tagged with TargetTag (default "PNATarget").
 * While locked, the controller rotation tracks the target and the character strafes.
 * Local-only (camera) logic; it does not need replication.
 */
UCLASS(ClassGroup=(PNA), meta=(BlueprintSpawnableComponent))
class PROJECTNINJAARENA_API UPNALockOnComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPNALockOnComponent();

    UFUNCTION(BlueprintCallable, Category="PNA|LockOn")
    void ToggleLockOn();

    UFUNCTION(BlueprintCallable, Category="PNA|LockOn")
    void ClearTarget();

    UFUNCTION(BlueprintPure, Category="PNA|LockOn")
    bool HasTarget() const { return Target.IsValid(); }

    UFUNCTION(BlueprintPure, Category="PNA|LockOn")
    AActor* GetTarget() const { return Target.Get(); }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|LockOn")
    FName TargetTag = TEXT("PNATarget");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|LockOn")
    float AcquireDistance = 1800.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|LockOn")
    float BreakDistance = 2400.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|LockOn")
    float MaxAcquireAngleDegrees = 60.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|LockOn")
    float AimHeightOffset = 60.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|LockOn")
    float RotationInterpSpeed = 12.f;

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    AActor* FindBestTarget() const;
    void SetStrafeMode(bool bEnabled);

    TWeakObjectPtr<AActor> Target;
    bool bSavedOrientToMovement = true;
    bool bSavedUseDesiredRotation = false;
};
