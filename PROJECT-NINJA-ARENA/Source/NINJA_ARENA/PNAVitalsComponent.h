#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PNAVitalsComponent.generated.h"

UCLASS(ClassGroup=(PNA), meta=(BlueprintSpawnableComponent))
class UPNAVitalsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPNAVitalsComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PNA|Vitals", meta=(ClampMin="1.0"))
	float MaxHealth = 100.f;

	UPROPERTY(BlueprintReadOnly, Category="PNA|Vitals")
	float Health = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PNA|Vitals", meta=(ClampMin="0.0"))
	float MaxEnergy = 100.f;

	UPROPERTY(BlueprintReadOnly, Category="PNA|Vitals")
	float Energy = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Vitals", meta=(ClampMin="0.0"))
	float EnergyRegenPerSecond = 10.f;

	UFUNCTION(BlueprintCallable, Category="PNA|Vitals")
	void ResetVitals();

	UFUNCTION(BlueprintCallable, Category="PNA|Vitals")
	bool SpendEnergy(float Amount);

	UFUNCTION(BlueprintPure, Category="PNA|Vitals")
	bool IsDefeated() const { return Health <= 0.f; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
	void HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);
};
