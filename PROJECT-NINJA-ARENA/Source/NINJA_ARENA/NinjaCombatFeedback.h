#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "NinjaCombatFeedback.generated.h"

class ANINJA_ARENACharacter;
class UCameraComponent;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;

enum class ENinjaImpactKind : uint8
{
	Guard,
	PerfectGuard,
	Light,
	Heavy,
	Aerial,
	Ultimate
};

UCLASS()
class UNinjaCombatFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNinjaCombatFeedbackComponent();
	static void Hit(ANINJA_ARENACharacter& Attacker, AActor& Target, ENinjaImpactKind Kind, bool bSpawnBurst = true);
	void CuePursuit();
	void CueUltimateCharge();
	void CueUltimateRelease(const FVector& Location);
	void ReleaseCameraCue();
	void ResetFeedback();
	bool IsInHitStop() const { return bHitStopped; }
	bool HasCameraEffect() const { return bCameraCaptured; }
	bool IsReacting() const { return bReacting; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditDefaultsOnly, Category="PNA|Camera")
	bool bEnableActionCamera = true;

	UPROPERTY(EditDefaultsOnly, Category="PNA|Camera")
	bool bEnableCameraShake = true;

private:
	void ApplyImpact(ENinjaImpactKind Kind, bool bHitTarget);
	void BeginHitStop(float Duration);
	void EndHitStop();
	UCameraComponent* CaptureLocalCamera();
	void BeginCameraCue(float FovOffset, const FVector& Offset, float Duration);
	void RestoreCamera();
	void RestoreReaction();

	FTimerHandle HitStopTimer;
	bool bHitStopped = false;
	float SavedTimeDilation = 1.f;
	float SavedAnimRate = 1.f;
	double HitStopUntil = 0.0;

	bool bReacting = false;
	FRotator MeshHome = FRotator::ZeroRotator;
	double ReactionStart = 0.0;
	float ReactionDuration = 0.f;
	float ReactionAngle = 0.f;

	bool bCameraCaptured = false;
	FVector CameraHome = FVector::ZeroVector;
	FRotator CameraHomeRotation = FRotator::ZeroRotator;
	float CameraHomeFov = 90.f;
	FVector CueOffset = FVector::ZeroVector;
	FVector CurrentOffset = FVector::ZeroVector;
	float CueFov = 0.f;
	float CurrentFov = 0.f;
	double CueUntil = 0.0;
	double ShakeStart = 0.0;
	float ShakeDuration = 0.f;
	float ShakePower = 0.f;
};

UCLASS()
class ANinjaImpactBurst : public AActor
{
	GENERATED_BODY()

public:
	ANinjaImpactBurst();
	void Configure(ENinjaImpactKind Kind);
	float GetWaveRadius() const { return WaveRadius; }

protected:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere, Category="PNA|Effects")
	TObjectPtr<UStaticMeshComponent> Flash;

	UPROPERTY(VisibleAnywhere, Category="PNA|Effects")
	TObjectPtr<UInstancedStaticMeshComponent> Wave;

	double StartTime = 0.0;
	float FlashSize = 0.f;
	float WaveRadius = 0.f;
	float EffectDuration = 0.20f;
};
