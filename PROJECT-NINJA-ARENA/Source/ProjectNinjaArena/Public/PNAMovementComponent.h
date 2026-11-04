#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PNATypes.h"
#include "PNAMovementComponent.generated.h"

/**
 * PNA movement: sprint, dash/dodge (ground + air), wall run and wall jump.
 * Sprint and dash are sent through CMC compressed flags, so they are
 * client-predicted and server-validated. Double jump uses ACharacter::JumpMaxCount.
 */
UCLASS()
class PROJECTNINJAARENA_API UPNAMovementComponent : public UCharacterMovementComponent
{
    GENERATED_BODY()

    friend class FSavedMove_PNA;

public:
    UPNAMovementComponent();

    // ---- Input-facing API ----
    UFUNCTION(BlueprintCallable, Category="PNA|Movement")
    void SetWantsSprint(bool bEnabled) { bWantsToSprint = bEnabled; }

    /** Requests a dash/dodge in the current input direction (or facing if no input). */
    UFUNCTION(BlueprintCallable, Category="PNA|Movement")
    void RequestDash();

    // ---- State queries (Animation Blueprint / UI / combat) ----
    UFUNCTION(BlueprintPure, Category="PNA|Movement")
    bool IsDashing() const { return bIsDashing; }

    UFUNCTION(BlueprintPure, Category="PNA|Movement")
    bool IsWallRunning() const { return MovementMode == MOVE_Custom && CustomMovementMode == PNAMovementModes::WallRun; }

    UFUNCTION(BlueprintPure, Category="PNA|Movement")
    bool IsWallOnRight() const { return bWallOnRight; }

    /** True during the first DashInvulnerableTime seconds of a dash. Combat should read this later. */
    UFUNCTION(BlueprintPure, Category="PNA|Movement")
    bool IsDashInvulnerable() const { return bIsDashing && (DashDuration - DashTimeRemaining) <= DashInvulnerableTime; }

    UFUNCTION(BlueprintPure, Category="PNA|Movement")
    float GetDashCooldownRemaining() const { return DashCooldownRemaining; }

    UFUNCTION(BlueprintPure, Category="PNA|Movement")
    EPNAMovementState GetPNAMovementState() const;

    // ---- Tuning ----
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|Ground")
    float WalkSpeed = 500.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|Ground")
    float SprintSpeed = 800.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|Dash")
    float DashSpeed = 1500.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|Dash")
    float DashDuration = 0.18f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|Dash")
    float DashCooldown = 0.7f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|Dash")
    float DashInvulnerableTime = 0.10f;

    /** Fraction of dash speed kept when the dash ends (0..1). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|Dash", meta=(ClampMin="0.0", ClampMax="1.0"))
    float DashExitSpeedRetain = 0.35f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|Dash")
    int32 MaxAirDashes = 1;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|WallRun")
    bool bWallRunEnabled = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|WallRun")
    float WallRunSpeed = 850.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|WallRun")
    float WallRunMinSpeed = 350.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|WallRun")
    float WallRunMaxDuration = 1.6f;

    /** Upward velocity applied when the wall run starts. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|WallRun")
    float WallRunEntryUpVelocity = 120.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|WallRun")
    float WallRunGravityScale = 0.15f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|WallRun")
    float WallRunMaxFallSpeed = 250.f;

    /** Extra reach (beyond capsule radius) of the side wall traces. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|WallRun")
    float WallRunTraceDistance = 45.f;

    /** Walls must be near-vertical: |normal.Z| must be <= this. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|WallRun")
    float WallRunMaxNormalZ = 0.25f;

    /** Minimum alignment (dot) between travel direction and wall direction to start. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|WallRun")
    float WallRunMinAlignment = 0.5f;

    /** Minimum height above the ground required to start a wall run. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|WallRun")
    float WallRunMinHeight = 60.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|WallRun")
    float WallRunReentryCooldown = 0.35f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|WallRun")
    float WallJumpAwaySpeed = 550.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|WallRun")
    float WallJumpForwardSpeed = 450.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Movement|WallRun")
    float WallJumpUpSpeed = 700.f;

    // ---- CMC overrides ----
    virtual float GetMaxSpeed() const override;
    virtual float GetGravityZ() const override;
    virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;
    virtual void UpdateFromCompressedFlags(uint8 Flags) override;
    virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;

protected:
    virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
    virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
    virtual void PhysCustom(float DeltaTime, int32 Iterations) override;
    virtual void PhysicsRotation(float DeltaTime) override;

private:
    // Dash
    bool CanDash() const;
    void StartDash();
    void EndDash();

    // Wall run
    void TryStartWallRun();
    void StartWallRun(const FVector& Normal, const FVector& Direction, bool bOnRight);
    void ExitWallRun();
    void WallJump();
    void PhysWallRun(float DeltaTime, int32 Iterations);
    bool TraceWall(const FVector& Direction, FHitResult& OutHit) const;
    bool IsNearGround(float ExtraDistance) const;
    ECollisionChannel GetTraceChannel() const;

    // Flags sent through saved moves
    bool bWantsToSprint = false;
    bool bWantsToDash = false;

    // Dash state
    bool bIsDashing = false;
    FVector DashDirection = FVector::ForwardVector;
    float DashTimeRemaining = 0.f;
    float DashCooldownRemaining = 0.f;
    int32 AirDashesUsed = 0;

    // Wall run state
    bool bWallOnRight = false;
    FVector WallRunNormal = FVector::ZeroVector;
    FVector WallRunDirection = FVector::ForwardVector;
    float WallRunTimeRemaining = 0.f;
    float WallRunCooldownRemaining = 0.f;
};
