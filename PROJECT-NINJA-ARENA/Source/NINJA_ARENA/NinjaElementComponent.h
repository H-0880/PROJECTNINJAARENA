#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NinjaElementComponent.generated.h"

class ANINJA_ARENACharacter;
class UNiagaraSystem;

UENUM(BlueprintType)
enum class ENinjaElement : uint8
{
	Fire,
	Lightning,
	Wind,
	Ice,
	Water
};

USTRUCT(BlueprintType)
struct FNinjaElementAbility
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Element")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Element", meta=(ClampMin="0.0"))
	float EnergyCost = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Element", meta=(ClampMin="0.1"))
	float Cooldown = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Element", meta=(ClampMin="50.0"))
	float Range = 650.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Element", meta=(ClampMin="10.0"))
	float HitRadius = 118.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Element", meta=(ClampMin="0.0"))
	float Damage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Element", meta=(ClampMin="0.0"))
	float EffectDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Element", meta=(ClampMin="0.0"))
	float Force = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Element")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Element")
	bool bAffectsCharacters = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Element")
	bool bAffectsObjects = true;
};

UCLASS(ClassGroup=(PNA), meta=(BlueprintSpawnableComponent))
class UNinjaElementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNinjaElementComponent();

	UFUNCTION(BlueprintCallable, Category="PNA|Elements")
	bool CastElement(ENinjaElement Element);

	UFUNCTION(BlueprintPure, Category="PNA|Elements")
	float GetCooldownRemaining(ENinjaElement Element) const;

	UFUNCTION(BlueprintPure, Category="PNA|Elements")
	bool IsBurning() const;

	UFUNCTION(BlueprintPure, Category="PNA|Elements")
	bool IsWet() const;

	UFUNCTION(BlueprintPure, Category="PNA|Elements")
	bool IsFrozen() const;

	UFUNCTION(BlueprintPure, Category="PNA|Elements")
	bool IsCharged() const;

	const FNinjaElementAbility* GetAbility(ENinjaElement Element) const { return Abilities.Find(Element); }
	float GetSpeedScale() const { return IsFrozen() ? 0.55f : 1.f; }
	const TCHAR* GetStatusLabel() const;
	FLinearColor GetStatusColor(const FLinearColor& DefaultColor) const;
	void ResetElementState();
	const UNiagaraSystem* GetLightningEffect() const;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Elements")
	TMap<ENinjaElement, FNinjaElementAbility> Abilities;

private:
	void ReceiveElement(ANINJA_ARENACharacter& Source, ENinjaElement Element, const FNinjaElementAbility& Ability, const FVector& Direction);
	void RefreshStatus();
	uint8 GetStatusMask() const;

	UPROPERTY(EditDefaultsOnly, Category="PNA|Elements")
	TObjectPtr<UNiagaraSystem> LightningEffect;

	TMap<ENinjaElement, float> ReadyAt;
	float NextCastAt = 0.f;
	float BurnUntil = 0.f;
	float NextBurnAt = 0.f;
	float BurnDamage = 0.f;
	float WetUntil = 0.f;
	float FrozenUntil = 0.f;
	float ChargedUntil = 0.f;
	uint8 LastStatusMask = 0;
	TWeakObjectPtr<ANINJA_ARENACharacter> BurnSource;
};
