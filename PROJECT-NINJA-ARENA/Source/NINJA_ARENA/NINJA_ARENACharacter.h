
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Core/PNATypes.h"
#include "NINJA_ARENACharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UNinjaCombatFeedbackComponent;
class UNinjaCombatStyleComponent;
class UNinjaElementComponent;
class UPNAVitalsComponent;
class UPNAAdvancedCombatComponent;
class UPNADuelConstraintComponent;
class APNAStageDirector;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(abstract)
class ANINJA_ARENACharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components",
		meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components",
		meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

protected:

	// Existing input actions
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MouseLookAction;

	// New Ninja Arena input actions
	UPROPERTY(EditAnywhere, Category = "Input|Ninja")
	UInputAction* SprintAction;

	UPROPERTY(EditAnywhere, Category = "Input|Ninja")
	UInputAction* DashAction;

	// Movement configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ninja|Movement")
	float NormalWalkSpeed = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ninja|Movement")
	float SprintSpeed = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ninja|Movement")
	float DashSpeed = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ninja|Movement",
		meta = (ClampMin = "0.05"))
	float DashDuration = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ninja|Movement",
		meta = (ClampMin = "0.0"))
	float DashCooldown = 0.7f;

	UPROPERTY(EditDefaultsOnly, Category="PNA|Aerial", meta=(ClampMin="0.05", ClampMax="0.3"))
	float AirDashDuration = 0.16f;

	UPROPERTY(EditDefaultsOnly, Category="PNA|Aerial", meta=(ClampMin="0.1", ClampMax="1.0"))
	float WallRunDuration = 0.65f;

	UPROPERTY(EditDefaultsOnly, Category="PNA|Aerial", meta=(ClampMin="100.0"))
	float WallRunSpeed = 620.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PNA|Combat")
	UPNAVitalsComponent* Vitals;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PNA|Elements")
	TObjectPtr<UNinjaElementComponent> Elements;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PNA|Feedback")
	TObjectPtr<UNinjaCombatFeedbackComponent> CombatFeedback;

	UPROPERTY(VisibleAnywhere, Category="PNA|Style")
	TObjectPtr<UNinjaCombatStyleComponent> CombatStyle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PNA|Combat|Advanced", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPNAAdvancedCombatComponent> AdvancedCombat;

	/** Enabled only while the active StageDirector is in Duel25D mode. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PNA|Modes", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPNADuelConstraintComponent> DuelConstraint;

	/** Cached stage authority. Not owned by the character. */
	UPROPERTY(Transient, BlueprintReadOnly, Category="PNA|Stage", meta=(AllowPrivateAccess="true"))
	TObjectPtr<APNAStageDirector> StageDirector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Combat", meta=(ClampMin="0.0"))
	float LightDamage = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Combat", meta=(ClampMin="0.0"))
	float HeavyDamage = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Combat", meta=(ClampMin="0.0"))
	float LightEnergyCost = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Combat", meta=(ClampMin="0.0"))
	float HeavyEnergyCost = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Combat", meta=(ClampMin="0.0"))
	float AttackRange = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Combat", meta=(ClampMin="1.0"))
	float AttackRadius = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Combat", meta=(ClampMin="0.0"))
	float AttackCooldown = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Combat", meta=(ClampMin="0.0"))
	float ComboWindow = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Combat", meta=(ClampMin="0.0"))
	float ComboDamageBonusPerStep = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Defense", meta=(ClampMin="0.0"))
	float GuardEnergyCost = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Defense", meta=(ClampMin="0.0", ClampMax="1.0"))
	float GuardDamageMultiplier = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Combat", meta=(ClampMin="0.0"))
	float LightHitStun = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Combat", meta=(ClampMin="0.0"))
	float HeavyHitStun = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Combat")
	FVector2D HeavyLaunchVelocity = FVector2D(290.f, 540.f);

	UPROPERTY(EditDefaultsOnly, Category="PNA|Aerial", meta=(ClampMin="0.5", ClampMax="2.0"))
	float AirComboWindow = 1.25f;

	UPROPERTY(EditDefaultsOnly, Category="PNA|Aerial", meta=(ClampMin="0.12", ClampMax="0.35"))
	float AirAttackCooldown = 0.18f;

	UPROPERTY(EditDefaultsOnly, Category="PNA|Aerial")
	bool bEnableAirPursuit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Arena")
	float FallZ = -1000.f;

	UPROPERTY(EditDefaultsOnly, Category="PNA|Ultimate", meta=(ClampMin="0.0"))
	float UltimateEnergyCost = 45.f;

	UPROPERTY(EditDefaultsOnly, Category="PNA|Ultimate", meta=(ClampMin="0.0"))
	float UltimateDamage = 44.f;

	UPROPERTY(EditDefaultsOnly, Category="PNA|Ultimate", meta=(ClampMin="0.5"))
	float UltimateCooldown = 9.f;

	UPROPERTY(EditDefaultsOnly, Category="PNA|Ultimate", meta=(ClampMin="0.1", ClampMax="1.0"))
	float UltimateWindup = 0.24f;

	FTransform SpawnTransform;
	float NextAttackTime = 0.f;
	float BufferedAttackUntil = 0.f;
	bool bBufferedHeavyAttack = false;
	int32 ComboIndex = 0;
	float ComboExpiresAt = 0.f;
	float StunEndsAt = 0.f;
	bool bGuarding = false;
	float PerfectGuardUntil = 0.f;
	float LastDirectionalInputAt = -1.f;
	bool bLaunched = false;
	bool bRoundLocked = false;
	int32 CombatTeam = INDEX_NONE;
	struct FPendingStrike
	{
		float ImpactAt = 0.f;
		float Damage = 0.f;
		float RangeBonus = 0.f;
		FVector Facing = FVector::ForwardVector;
		bool bHeavy = false;
	};
	FPendingStrike PendingStrike;
	void PerformAttack(bool bHeavyAttack);
	void ToggleRaizenForm();
	float CounterReadyUntil = 0.f;
	float CameraClearanceLift = 0.f;
	TWeakObjectPtr<ANINJA_ARENACharacter> LockedOpponent;
	TWeakObjectPtr<ANINJA_ARENACharacter> LastMeleeTarget;
	float LastMeleeDamage = 0.f;
	float MeleeChainDamage = 0.f;
	float MeleeChainUntil = 0.f;
	int32 MeleeChainHits = 0;
	void ToggleOpponentLock();
	void RecordMeleeContact(ANINJA_ARENACharacter* Target, float Damage, bool bBlocked);
	bool bRaizenPowerForm = false;
	void ResolveStrike(const FPendingStrike& Strike);
	FVector GetMeleeFacing() const;
	FVector GetMeleeCenter(const FVector& Facing) const;
	void ReceiveMeleeHit(ANINJA_ARENACharacter* Attacker, float Damage, bool bHeavyAttack);
	void ResetTrainingRound();
	bool HandleRoundLoss();
	virtual void BeginPlay() override;
	virtual void FellOutOfWorld(const UDamageType& DamageType) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual bool CanJumpInternal_Implementation() const override;
	bool bWantsToSprint = false;
	bool bIsDashing = false;

	FVector2D LastMovementInput = FVector2D::ZeroVector;
	FVector DashDirection = FVector::ForwardVector;

	float DashEndTime = 0.f;
	float NextDashAvailableTime = 0.f;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	void StartSprint();
	void StopSprint();
	void StartDash();
	void EndDash();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void HandleStageModeChanged(EPNAStageMode OldMode, EPNAStageMode NewMode);

	void ApplyStageMode(EPNAStageMode NewMode);

private:
	static constexpr int32 MaxAirComboHits = 3;

	TWeakObjectPtr<ANINJA_ARENACharacter> LaunchSource;
	TWeakObjectPtr<ANINJA_ARENACharacter> AirComboTarget;
	int32 ReceivedAirHits = 0;
	float AirComboEndsAt = 0.f;
	bool bUsedPursuit = false;
	bool bUsedAirRecovery = false;
	bool bUsedAirDash = false;
	bool bUsedWallRun = false;
	bool bWallRunning = false;
	float WallRunEndsAt = 0.f;
	FVector WallNormal = FVector::ZeroVector;
	FVector WallRunDirection = FVector::ZeroVector;

	void ResetAerialState(bool bRespawning);
	void ClearLaunchState();
	bool HasClearAttackLine(const AActor& Target) const;
	ANINJA_ARENACharacter* GetActiveAirTarget() const;
	bool TryAirComboHit(float Now);
	bool TryAirRecovery();
	bool TraceWall(const FVector& Direction, FHitResult& Hit) const;
	void StartWallRun();
	void EndWallRun();
	void UpdateWallRun(float Now);
	void JumpOffWall();

	bool bUltimateCharging = false;
	double UltimateCommitAt = 0.0;
	double UltimateReadyAt = 0.0;
	FVector UltimateDirection = FVector::ForwardVector;
	bool TryBlockHit(const FVector& ToAttacker, bool& bPerfectGuard);
	void CommitUltimate();
	void CancelUltimate();
	void ReceiveUltimateHit(ANINJA_ARENACharacter& Attacker, float Damage);

public:

	ANINJA_ARENACharacter();
	UFUNCTION(BlueprintCallable, Category="PNA|Arena") void ForceRespawn();
	UFUNCTION(BlueprintPure, Category="PNA|Combat") UPNAVitalsComponent* GetVitals() const { return Vitals; }

	UFUNCTION(BlueprintPure, Category="PNA|Elements")
	UNinjaElementComponent* GetElements() const { return Elements; }

	UNinjaCombatFeedbackComponent* GetCombatFeedback() const { return CombatFeedback; }
	UNinjaCombatStyleComponent* GetCombatStyle() const { return CombatStyle; }
	UPNAAdvancedCombatComponent* GetAdvancedCombat() const { return AdvancedCombat; }

	UFUNCTION(BlueprintPure, Category="PNA|Modes")
	UPNADuelConstraintComponent* GetDuelConstraint() const { return DuelConstraint; }

	UFUNCTION(BlueprintPure, Category="PNA|Stage")
	APNAStageDirector* GetStageDirector() const { return StageDirector; }

	/** Canonical read-only view of the legacy Core 3.2 combat state. */
	UFUNCTION(BlueprintPure, Category="PNA|Combat")
	EPNACombatState GetCombatState() const;

	UFUNCTION(BlueprintCallable, Category="PNA|Ultimate")
	void StartUltimate();

	UFUNCTION(BlueprintPure, Category="PNA|Ultimate")
	float GetUltimateCooldownRemaining() const;

	bool IsUltimateCharging() const { return bUltimateCharging; }
	float GetUltimateEnergyCost() const { return UltimateEnergyCost; }

	void RefreshMovementSpeed();
	void InterruptForElement(const FVector& Impulse = FVector::ZeroVector);
	bool IsRoundLocked() const { return bRoundLocked; }

	UFUNCTION(BlueprintCallable, Category="PNA|Combat")
	void LightAttack();

	UFUNCTION(BlueprintCallable, Category="PNA|Combat")
	void HeavyAttack();

	UFUNCTION(BlueprintPure, Category="PNA|Combat")
	int32 GetComboStep() const;
	const ANINJA_ARENACharacter* GetLockedOpponent() const { return LockedOpponent.Get(); }
	float GetLastMeleeDamage() const { return LastMeleeDamage; }
	float GetMeleeChainDamage() const;
	int32 GetMeleeChainHits() const;

	UFUNCTION(BlueprintCallable, Category="PNA|Defense")
	void BeginGuard();

	UFUNCTION(BlueprintCallable, Category="PNA|Defense")
	void EndGuard();

	UFUNCTION(BlueprintPure, Category="PNA|Defense")
	bool IsGuarding() const { return bGuarding; }

	UFUNCTION(BlueprintPure, Category="PNA|Combat")
	bool IsStunned() const;
	void SetCombatTeam(int32 Team) { CombatTeam = Team; }
	int32 GetCombatTeam() const { return CombatTeam; }
	bool IsCombatAlly(const AActor* Other) const;
	bool CanMeleeReach(const ANINJA_ARENACharacter& Target, const FVector& Facing) const;

	UFUNCTION(BlueprintPure, Category="PNA|Combat")
	bool IsLaunched() const { return bLaunched; }
	bool IsCounterReady() const;
	bool IsRaizenPowerForm() const { return bRaizenPowerForm; }
	bool CanAirRecover() const;

	UFUNCTION(BlueprintCallable, Category="PNA|Aerial")
	void StartAirPursuit();

	UFUNCTION(BlueprintPure, Category="PNA|Aerial")
	bool CanAirPursue() const;

	UFUNCTION(BlueprintPure, Category="PNA|Aerial")
	int32 GetAirComboStep() const;

	bool IsWallRunning() const { return bWallRunning; }
	FVector GetWallRunSurfaceNormal() const { return bWallRunning ? WallNormal : FVector::ZeroVector; }
	bool HasAirDash() const { return !bUsedAirDash; }
	bool IsCombatLocked() const;
	void SetRoundLocked(bool bLocked);
	bool RespawnAtStart();
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	virtual void SetupPlayerInputComponent(
		class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMove(float Right, float Forward);

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoLook(float Yaw, float Pitch);

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpStart();

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpEnd();

	FORCEINLINE USpringArmComponent* GetCameraBoom() const
	{
		return CameraBoom;
	}

	FORCEINLINE UCameraComponent* GetFollowCamera() const
	{
		return FollowCamera;
	}
};
