#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "NinjaCombatStyle.generated.h"

class UAnimMontage;
class UAnimSequenceBase;
class UMaterialInterface;
class USkeletalMeshComponent;
class USoundBase;
class UStaticMesh;
class UStaticMeshComponent;

enum class ENinjaFighter : uint8
{
	Junco,
	Ascua,
	Kuroboshi,
	Raizen,
	AkiraVioleta,
	Byakuro,
	TripoA,
	TripoB,
	Count
};

struct FNinjaFighterProfile
{
	const TCHAR* Name;
	FLinearColor Armor;
	FLinearColor Accent;
	FLinearColor Mask;
	const TCHAR* TrainingId;
	float LightDamageScale[3];
	float LightPlayRate[3];
	float RecoveryScale;
	float HeavyDamageScale;
	const TCHAR* ModelFolder = nullptr;
};

enum class ENinjaStyleSound : uint8
{
	Light,
	Heavy,
	Dash,
	Energy,
	Ultimate,
	Step,
	Swing
};

UCLASS()
class UNinjaCombatStyleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNinjaCombatStyleComponent();
	void SetFighter(ENinjaFighter NewFighter);
	ENinjaFighter GetFighter() const { return FighterId; }
	static const FNinjaFighterProfile& GetProfile(ENinjaFighter Fighter);
	const FNinjaFighterProfile& GetProfile() const { return GetProfile(FighterId); }
	const TCHAR* GetFighterName() const { return GetProfile().Name; }
	void PlayStrike(int32 Step, bool bHeavy, float GroundWindup = 0.f);
	void PlayBladeTrace();
	void PlayDash();
	void PlayCharge();
	void PlayWindup(bool bHeavy);
	void StopActionPose(float BlendTime = 0.08f);
	void PlaySound(ENinjaStyleSound Sound, const FVector& Location);
	const USoundBase* GetSound(ENinjaStyleSound Sound) const;
	UAnimMontage* GetActionMontage() const { return ActionMontage.Get(); }
	const UAnimSequenceBase* GetLastSequence() const { return LastSequence.Get(); }
	int32 GetCostumePartCount() const { return CostumeParts.Num(); }
	int32 GetTintedSlotCount() const { return TintedSlotCount; }
	USkeletalMeshComponent* GetFighterModel() const { return FighterModel.Get(); }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void ApplyCostume();
	bool ApplyFighterModel(USkeletalMeshComponent& Body);
	void RemoveFighterModel(USkeletalMeshComponent& Body);
	void AddCostumePart(UStaticMesh* Shape, FName Bone, const FVector& Offset, const FVector& Scale,
		const FLinearColor& Color);
	void PlayPose(UAnimSequenceBase* Sequence, float Rate, float StartTime = 0.f);

	UPROPERTY()
	TArray<TObjectPtr<UAnimSequenceBase>> LightStrikes;

	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> HeavyStrike;

	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> DashPose;

	UPROPERTY()
	TArray<TObjectPtr<USoundBase>> Sounds;

	UPROPERTY()
	TObjectPtr<UStaticMesh> SmoothPart;

	UPROPERTY()
	TObjectPtr<UStaticMesh> FlatPart;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> PartMaterial;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> CostumeParts;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> FighterModel;

	EVisibilityBasedAnimTickOption OriginalTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	bool bOriginalVisible = true;

	TWeakObjectPtr<UAnimMontage> ActionMontage;
	TWeakObjectPtr<UAnimSequenceBase> LastSequence;
	ENinjaFighter FighterId = ENinjaFighter::Junco;
	int32 TintedSlotCount = 0;
	double NextFootstep = 0.0;
	double NextSound = 0.0;
};
