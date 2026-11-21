#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PNADuelConstraintComponent.generated.h"

UCLASS(ClassGroup=(NINJA_ARENA), meta=(BlueprintSpawnableComponent))
class NINJA_ARENA_API UPNADuelConstraintComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPNADuelConstraintComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PNA|Duel")
    bool bEnabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PNA|Duel")
    float PlaneY = 0.f;

    UFUNCTION(BlueprintCallable, Category="PNA|Duel")
    void Configure(bool bNewEnabled, float NewPlaneY);

    UFUNCTION(BlueprintCallable, Category="PNA|Duel")
    void SetEnabled(bool bNewEnabled);

    UFUNCTION(BlueprintCallable, Category="PNA|Duel")
    void SetPlaneY(float NewPlaneY);

    UFUNCTION(BlueprintPure, Category="PNA|Duel")
    FVector ProjectToPlane(const FVector& Location) const;

protected:
    virtual void BeginPlay() override;

private:
    void ApplyConstraint();
};
