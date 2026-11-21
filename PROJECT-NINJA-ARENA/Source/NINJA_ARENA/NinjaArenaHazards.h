#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "NinjaArenaHazards.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

enum class ENinjaArenaZonePhase : uint8
{
	Rest,
	Warning,
	Active
};

UCLASS()
class ANinjaArenaDebris : public AActor
{
	GENERATED_BODY()

public:
	ANinjaArenaDebris();
	void Activate(double Scale, const FVector& Velocity, const FLinearColor& Color, float Lifetime);

private:
	UPROPERTY(VisibleAnywhere, Category="Arena")
	TObjectPtr<UStaticMeshComponent> Mesh;
};

UCLASS()
class ANinjaArenaDangerZone : public AActor
{
	GENERATED_BODY()

public:
	ANinjaArenaDangerZone();
	void Configure(float InRadius, float InDamage, bool bInCollapse);
	void RestartCycle();
	ENinjaArenaZonePhase GetPhase() const { return Phase; }
	bool IsCollapse() const { return bCollapse; }
	float GetRadius() const { return Radius; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void AdvanceCycle();
	void DamageOccupants();
	void PaintPhase();

	UPROPERTY(VisibleAnywhere, Category="Arena")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

	UPROPERTY(EditDefaultsOnly, Category="Arena", meta=(ClampMin="0.1"))
	float RestSeconds = 5.f;

	UPROPERTY(EditDefaultsOnly, Category="Arena", meta=(ClampMin="0.1"))
	float WarningSeconds = 0.9f;

	UPROPERTY(EditDefaultsOnly, Category="Arena", meta=(ClampMin="0.1"))
	float ActiveSeconds = 1.35f;

	FTimerHandle PhaseTimer;
	FTimerHandle DamageTimer;
	ENinjaArenaZonePhase Phase = ENinjaArenaZonePhase::Rest;
	float Radius = 170.f;
	float Damage = 12.f;
	bool bCollapse = false;
};
