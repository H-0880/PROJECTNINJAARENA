#include "PNAMovementComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"

// ---------------------------------------------------------------------------
// Network prediction: saved move carrying sprint + dash flags
// ---------------------------------------------------------------------------
class FSavedMove_PNA : public FSavedMove_Character
{
    typedef FSavedMove_Character Super;

public:
    uint8 bSavedWantsSprint : 1;
    uint8 bSavedWantsDash : 1;

    FSavedMove_PNA() : bSavedWantsSprint(false), bSavedWantsDash(false) {}

    virtual void Clear() override
    {
        Super::Clear();
        bSavedWantsSprint = false;
        bSavedWantsDash = false;
    }

    virtual uint8 GetCompressedFlags() const override
    {
        uint8 Result = Super::GetCompressedFlags();
        if (bSavedWantsSprint) Result |= FLAG_Custom_0;
        if (bSavedWantsDash)   Result |= FLAG_Custom_1;
        return Result;
    }

    virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override
    {
        const FSavedMove_PNA* Other = static_cast<const FSavedMove_PNA*>(NewMove.Get());
        if (bSavedWantsSprint != Other->bSavedWantsSprint || bSavedWantsDash != Other->bSavedWantsDash)
        {
            return false;
        }
        return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
    }

    virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel,
                            FNetworkPredictionData_Client_Character& ClientData) override
    {
        Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
        if (const UPNAMovementComponent* Move = Cast<UPNAMovementComponent>(C->GetCharacterMovement()))
        {
            bSavedWantsSprint = Move->bWantsToSprint;
            bSavedWantsDash = Move->bWantsToDash;
        }
    }

    virtual void PrepMoveFor(ACharacter* C) override
    {
        Super::PrepMoveFor(C);
        if (UPNAMovementComponent* Move = Cast<UPNAMovementComponent>(C->GetCharacterMovement()))
        {
            Move->bWantsToSprint = bSavedWantsSprint;
            Move->bWantsToDash = bSavedWantsDash;
        }
    }
};

class FNetworkPredictionData_Client_PNA : public FNetworkPredictionData_Client_Character
{
    typedef FNetworkPredictionData_Client_Character Super;

public:
    FNetworkPredictionData_Client_PNA(const UCharacterMovementComponent& ClientMovement)
        : Super(ClientMovement) {}

    virtual FSavedMovePtr AllocateNewMove() override
    {
        return FSavedMovePtr(new FSavedMove_PNA());
    }
};

// ---------------------------------------------------------------------------
// Component
// ---------------------------------------------------------------------------
UPNAMovementComponent::UPNAMovementComponent()
{
    MaxWalkSpeed = WalkSpeed;
    JumpZVelocity = 640.f;
    AirControl = 0.65f;
    MaxAcceleration = 2400.f;
    GroundFriction = 10.f;
    BrakingDecelerationWalking = 2200.f;
    BrakingDecelerationFalling = 300.f;
    FallingLateralFriction = 0.4f;
    GravityScale = 1.6f;
    RotationRate = FRotator(0.f, 720.f, 0.f);
    bOrientRotationToMovement = true;
}

FNetworkPredictionData_Client* UPNAMovementComponent::GetPredictionData_Client() const
{
    check(PawnOwner != nullptr);
    if (ClientPredictionData == nullptr)
    {
        UPNAMovementComponent* MutableThis = const_cast<UPNAMovementComponent*>(this);
        MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_PNA(*this);
        MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
        MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
    }
    return ClientPredictionData;
}

void UPNAMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
    Super::UpdateFromCompressedFlags(Flags);
    bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
    bWantsToDash = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
}

EPNAMovementState UPNAMovementComponent::GetPNAMovementState() const
{
    if (bIsDashing) return EPNAMovementState::Dashing;
    if (IsWallRunning()) return EPNAMovementState::WallRunning;
    if (IsFalling()) return EPNAMovementState::Falling;
    if (bWantsToSprint && Velocity.SizeSquared2D() > FMath::Square(WalkSpeed * 1.05f))
    {
        return EPNAMovementState::Sprinting;
    }
    return EPNAMovementState::Grounded;
}

float UPNAMovementComponent::GetMaxSpeed() const
{
    if (bIsDashing) return DashSpeed;
    if (IsWallRunning()) return WallRunSpeed;
    return Super::GetMaxSpeed();
}

float UPNAMovementComponent::GetGravityZ() const
{
    // Air dash hovers: no gravity while dashing in the air.
    if (bIsDashing && IsFalling()) return 0.f;
    return Super::GetGravityZ();
}

void UPNAMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
    if (bIsDashing)
    {
        // Dash fully owns horizontal velocity: no friction/braking can eat it.
        Velocity.X = DashDirection.X * DashSpeed;
        Velocity.Y = DashDirection.Y * DashSpeed;
        return;
    }
    Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
}

// ---- Dash -----------------------------------------------------------------
void UPNAMovementComponent::RequestDash()
{
    if (CanDash())
    {
        bWantsToDash = true;
    }
}

bool UPNAMovementComponent::CanDash() const
{
    if (bIsDashing || DashCooldownRemaining > 0.f || IsWallRunning()) return false;
    if (IsMovingOnGround()) return true;
    return IsFalling() && AirDashesUsed < MaxAirDashes;
}

void UPNAMovementComponent::StartDash()
{
    FVector Dir = Acceleration.GetSafeNormal2D();
    if (Dir.IsNearlyZero())
    {
        Dir = UpdatedComponent->GetForwardVector().GetSafeNormal2D();
    }
    DashDirection = Dir;
    bIsDashing = true;
    DashTimeRemaining = DashDuration;
    DashCooldownRemaining = DashCooldown;

    if (IsFalling())
    {
        ++AirDashesUsed;
        Velocity.Z = 0.f;
    }
}

void UPNAMovementComponent::EndDash()
{
    bIsDashing = false;
    Velocity.X *= DashExitSpeedRetain;
    Velocity.Y *= DashExitSpeedRetain;
}

// ---- Per-move state -------------------------------------------------------
void UPNAMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
    Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

    MaxWalkSpeed = bWantsToSprint ? SprintSpeed : WalkSpeed;

    DashCooldownRemaining = FMath::Max(0.f, DashCooldownRemaining - DeltaSeconds);
    WallRunCooldownRemaining = FMath::Max(0.f, WallRunCooldownRemaining - DeltaSeconds);

    // Wall jump: ACharacter::CanJump refuses jumps in custom modes, so we consume the jump press here.
    if (IsWallRunning() && CharacterOwner && CharacterOwner->bPressedJump)
    {
        WallJump();
        CharacterOwner->StopJumping();
    }

    if (bWantsToDash)
    {
        bWantsToDash = false;
        if (CanDash())
        {
            StartDash();
        }
    }

    if (bIsDashing)
    {
        DashTimeRemaining -= DeltaSeconds;
        if (DashTimeRemaining <= 0.f)
        {
            EndDash();
        }
    }

    TryStartWallRun();
}

void UPNAMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
    Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

    if (MovementMode == MOVE_Walking)
    {
        AirDashesUsed = 0;
    }
}

// ---- Wall run -------------------------------------------------------------
ECollisionChannel UPNAMovementComponent::GetTraceChannel() const
{
    return UpdatedPrimitive ? UpdatedPrimitive->GetCollisionObjectType().GetValue() : ECC_Pawn;
}

bool UPNAMovementComponent::TraceWall(const FVector& Direction, FHitResult& OutHit) const
{
    if (!CharacterOwner || !GetWorld() || !UpdatedComponent) return false;

    const float Radius = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();
    const FVector Start = UpdatedComponent->GetComponentLocation();
    const FVector End = Start + Direction.GetSafeNormal() * (Radius + WallRunTraceDistance);

    FCollisionQueryParams Params(SCENE_QUERY_STAT(PNAWallTrace), false, CharacterOwner);
    const bool bHit = GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, GetTraceChannel(), Params);
    return bHit && OutHit.bBlockingHit && FMath::Abs(OutHit.ImpactNormal.Z) <= WallRunMaxNormalZ;
}

bool UPNAMovementComponent::IsNearGround(float ExtraDistance) const
{
    if (!CharacterOwner || !GetWorld() || !UpdatedComponent) return false;

    const float HalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const FVector Start = UpdatedComponent->GetComponentLocation();
    const FVector End = Start - FVector::UpVector * (HalfHeight + ExtraDistance);

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(PNAGroundTrace), false, CharacterOwner);
    return GetWorld()->LineTraceSingleByChannel(Hit, Start, End, GetTraceChannel(), Params);
}

void UPNAMovementComponent::TryStartWallRun()
{
    if (!bWallRunEnabled || !IsFalling() || bIsDashing || WallRunCooldownRemaining > 0.f) return;
    if (Velocity.Size2D() < WallRunMinSpeed) return;

    const FVector Velocity2D = Velocity.GetSafeNormal2D();
    const FVector Input2D = Acceleration.GetSafeNormal2D();
    if (Input2D.IsNearlyZero() || FVector::DotProduct(Input2D, Velocity2D) < 0.3f) return;
    if (IsNearGround(WallRunMinHeight)) return;

    const FVector Right = UpdatedComponent->GetRightVector();
    for (int32 Side = 0; Side < 2; ++Side)
    {
        const bool bRight = (Side == 0);
        FHitResult Hit;
        if (!TraceWall(bRight ? Right : -Right, Hit)) continue;

        const FVector Normal = Hit.ImpactNormal.GetSafeNormal2D();
        FVector Tangent = FVector::CrossProduct(Normal, FVector::UpVector).GetSafeNormal();
        if (FVector::DotProduct(Tangent, Velocity2D) < 0.f) Tangent = -Tangent;
        if (FVector::DotProduct(Tangent, Velocity2D) < WallRunMinAlignment) continue;

        StartWallRun(Normal, Tangent, bRight);
        return;
    }
}

void UPNAMovementComponent::StartWallRun(const FVector& Normal, const FVector& Direction, bool bOnRight)
{
    WallRunNormal = Normal;
    WallRunDirection = Direction;
    bWallOnRight = bOnRight;
    WallRunTimeRemaining = WallRunMaxDuration;
    AirDashesUsed = 0;
    if (CharacterOwner) CharacterOwner->JumpCurrentCount = 0; // full jump budget again

    Velocity = Direction * WallRunSpeed;
    Velocity.Z = WallRunEntryUpVelocity;
    SetMovementMode(MOVE_Custom, PNAMovementModes::WallRun);
}

void UPNAMovementComponent::ExitWallRun()
{
    WallRunCooldownRemaining = WallRunReentryCooldown;
    SetMovementMode(MOVE_Falling);
}

void UPNAMovementComponent::WallJump()
{
    Velocity = WallRunNormal * WallJumpAwaySpeed
             + WallRunDirection * WallJumpForwardSpeed
             + FVector::UpVector * WallJumpUpSpeed;
    WallRunCooldownRemaining = WallRunReentryCooldown;
    if (CharacterOwner) CharacterOwner->JumpCurrentCount = 1; // one air jump left
    SetMovementMode(MOVE_Falling);
}

void UPNAMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
    if (CustomMovementMode == PNAMovementModes::WallRun)
    {
        PhysWallRun(DeltaTime, Iterations);
        return;
    }
    Super::PhysCustom(DeltaTime, Iterations);
}

void UPNAMovementComponent::PhysWallRun(float DeltaTime, int32 Iterations)
{
    if (DeltaTime < MIN_TICK_TIME || !CharacterOwner || !UpdatedComponent) return;

    WallRunTimeRemaining -= DeltaTime;

    FHitResult WallHit;
    const bool bWallStillThere = TraceWall(-WallRunNormal, WallHit);
    if (!bWallStillThere || WallRunTimeRemaining <= 0.f || Acceleration.IsNearlyZero() || IsNearGround(5.f))
    {
        ExitWallRun();
        StartNewPhysics(DeltaTime, Iterations);
        return;
    }

    // Follow the wall surface (handles gentle curves).
    WallRunNormal = WallHit.ImpactNormal.GetSafeNormal2D();
    FVector Tangent = FVector::CrossProduct(WallRunNormal, FVector::UpVector).GetSafeNormal();
    if (FVector::DotProduct(Tangent, WallRunDirection) < 0.f) Tangent = -Tangent;
    WallRunDirection = Tangent;

    Velocity.Z += GetGravityZ() * WallRunGravityScale * DeltaTime;
    Velocity.Z = FMath::Max(Velocity.Z, -WallRunMaxFallSpeed);
    Velocity.X = Tangent.X * WallRunSpeed;
    Velocity.Y = Tangent.Y * WallRunSpeed;

    const FVector OldLocation = UpdatedComponent->GetComponentLocation();
    const FVector Delta = Velocity * DeltaTime;
    FHitResult Hit(1.f);
    SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);

    if (Hit.Time < 1.f)
    {
        // Obstacle straight ahead (corner / end of wall): leave the wall.
        if (FVector::DotProduct(Hit.ImpactNormal.GetSafeNormal2D(), WallRunDirection) < -0.5f)
        {
            ExitWallRun();
            StartNewPhysics(DeltaTime * (1.f - Hit.Time), Iterations);
            return;
        }
        HandleImpact(Hit, DeltaTime, Delta);
        SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
    }

    if (!bJustTeleported)
    {
        Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / DeltaTime;
    }
}

void UPNAMovementComponent::PhysicsRotation(float DeltaTime)
{
    if (IsWallRunning() && UpdatedComponent)
    {
        FRotator Current = UpdatedComponent->GetComponentRotation();
        FRotator Target = WallRunDirection.Rotation();
        Target.Pitch = 0.f;
        Target.Roll = 0.f;
        Current.Pitch = 0.f;
        Current.Roll = 0.f;
        const FRotator NewRotation = FMath::RInterpConstantTo(Current, Target, DeltaTime, 720.f);
        MoveUpdatedComponent(FVector::ZeroVector, NewRotation, false);
        return;
    }
    Super::PhysicsRotation(DeltaTime);
}
