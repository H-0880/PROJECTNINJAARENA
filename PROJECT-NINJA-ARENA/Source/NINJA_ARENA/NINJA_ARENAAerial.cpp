#include "NINJA_ARENACharacter.h"
#include "NINJA_ARENAGameMode.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "NinjaCombatFeedback.h"
#include "NinjaCombatStyle.h"
#include "NinjaElementComponent.h"
#include "PNAVitalsComponent.h"

void ANINJA_ARENACharacter::ClearLaunchState()
{
	bLaunched = false;
	LaunchSource.Reset();
	ReceivedAirHits = 0;
	AirComboEndsAt = 0.f;
}

void ANINJA_ARENACharacter::ResetAerialState(bool bRespawning)
{
	EndWallRun();
	ClearLaunchState();
	if (bRespawning || bUsedPursuit || GetAirComboStep() > 0)
	{
		AirComboTarget.Reset();
		bUsedPursuit = false;
	}
	bUsedAirRecovery = false;
	bUsedAirDash = false;
	bUsedWallRun = false;
}

bool ANINJA_ARENACharacter::HasClearAttackLine(const AActor& Target) const
{
	FCollisionQueryParams Query(SCENE_QUERY_STAT(NinjaAttackSight), false, this);
	Query.AddIgnoredActor(&Target);
	if (const ANINJA_ARENAGameMode* Mode = GetWorld()->GetAuthGameMode<ANINJA_ARENAGameMode>())
	{
		Mode->IgnoreAlliesForTrace(*this, Query);
	}
	return !GetWorld()->LineTraceTestByChannel(GetActorLocation(), Target.GetActorLocation(), ECC_Visibility, Query);
}

ANINJA_ARENACharacter* ANINJA_ARENACharacter::GetActiveAirTarget() const
{
	ANINJA_ARENACharacter* Target = AirComboTarget.Get();
	const UWorld* World = GetWorld();
	if (World && Target && Target->IsLaunched() && Target->LaunchSource.Get() == this
		&& !Target->GetVitals()->IsDefeated() && World->GetTimeSeconds() < Target->AirComboEndsAt)
	{
		return Target;
	}
	return nullptr;
}

int32 ANINJA_ARENACharacter::GetAirComboStep() const
{
	const ANINJA_ARENACharacter* Target = GetActiveAirTarget();
	return Target ? Target->ReceivedAirHits : 0;
}

bool ANINJA_ARENACharacter::CanAirPursue() const
{
	const ANINJA_ARENACharacter* Target = GetActiveAirTarget();
	return bEnableAirPursuit && !bUsedPursuit && !IsCombatLocked() && !IsGuarding() && Target
		&& Target->ReceivedAirHits < MaxAirComboHits
		&& FVector::DistSquared(GetActorLocation(), Target->GetActorLocation()) < FMath::Square(620.0);
}

void ANINJA_ARENACharacter::StartAirPursuit()
{
	if (!HasAuthority() || !CanAirPursue())
	{
		return;
	}

	const ANINJA_ARENACharacter* Target = GetActiveAirTarget();
	if (!HasClearAttackLine(*Target))
	{
		return;
	}

	bUsedPursuit = true;
	EndDash();
	EndWallRun();
	ConsumeMovementInputVector();
	const FVector Offset = Target->GetActorLocation() - GetActorLocation();
	const FVector Direction = Offset.GetSafeNormal2D();
	if (!Direction.IsNearlyZero())
	{
		SetActorRotation(Direction.Rotation());
	}

	const double ArrivalTime = 0.24;
	const FVector RivalVelocity = Target->GetVelocity();
	const double Advance = FMath::Max(0.0, Offset.Size2D() - 126.0);
	FVector Impulse = (Direction * (Advance / ArrivalTime) + FVector(RivalVelocity.X, RivalVelocity.Y, 0.0) * 0.65)
		.GetClampedToMaxSize(1160.0);
	Impulse.Z = FMath::Clamp(Offset.Z / ArrivalTime + RivalVelocity.Z * 0.66
		- GetCharacterMovement()->GetGravityZ() * ArrivalTime * 0.5, 440.0, 750.0);
	LaunchCharacter(Impulse, true, true);
	CombatFeedback->CuePursuit();
	CombatStyle->PlayDash();
}

bool ANINJA_ARENACharacter::TryAirComboHit(float Now)
{
	ANINJA_ARENACharacter* Target = GetActiveAirTarget();
	if (!Target)
	{
		return false;
	}
	const FVector Offset = Target->GetActorLocation() - GetActorLocation();
	if (Offset.SizeSquared() > FMath::Square(320.0) ||
		FVector::DotProduct(Offset.GetSafeNormal2D(), GetActorForwardVector()) < -0.1 ||
		!HasClearAttackLine(*Target))
	{
		return false;
	}
	if (Target->ReceivedAirHits >= MaxAirComboHits || !Vitals->SpendEnergy(LightEnergyCost))
	{
		return true;
	}

	EndWallRun();
	NextAttackTime = Now + AirAttackCooldown;
	ComboIndex = 0;
	ComboExpiresAt = 0.f;
	const float Damage = LightDamage * (1.f + Target->ReceivedAirHits * ComboDamageBonusPerStep);
	CombatStyle->PlayStrike(Target->ReceivedAirHits + 1, false);
	const float AppliedDamage = UGameplayStatics::ApplyDamage(Target, Damage, GetController(), this, nullptr);
	if (AppliedDamage <= 0.f)
	{
		return true;
	}
	UNinjaCombatFeedbackComponent::Hit(*this, *Target, ENinjaImpactKind::Aerial);

	const int32 HitCount = ++Target->ReceivedAirHits;
	if (!Target->Vitals->IsDefeated())
	{
		Target->StunEndsAt = FMath::Max(Target->StunEndsAt, Now + LightHitStun);
		const FVector Direction = Offset.GetSafeNormal2D();
		if (HitCount == MaxAirComboHits)
		{
			Target->LaunchCharacter(Direction * 260.0 - FVector(0.0, 0.0, 480.0), true, true);
		}
		else
		{
			Target->LaunchCharacter(Direction * 120.0 + FVector(0.0, 0.0, 235.0), true, true);
			UCharacterMovementComponent* Movement = GetCharacterMovement();
			Movement->Velocity.Z = FMath::Max(Movement->Velocity.Z, 180.0);
		}
	}
	return true;
}

bool ANINJA_ARENACharacter::CanAirRecover() const
{
	return bLaunched && !bUsedAirRecovery && !bRoundLocked && !IsStunned() && !Vitals->IsDefeated()
		&& !CombatFeedback->IsInHitStop() && GetCharacterMovement()->IsFalling();
}

bool ANINJA_ARENACharacter::TryAirRecovery()
{
	if (!HasAuthority() || !CanAirRecover())
	{
		return false;
	}

	bUsedAirRecovery = true;
	ClearLaunchState();
	StopJumping();
	JumpCurrentCount = JumpMaxCount;
	const FVector Velocity = GetVelocity();
	FVector Recovery(Velocity.X * 0.55, Velocity.Y * 0.55, 245.0);
	if (const AController* RecoveryController = GetController(); RecoveryController && GetWorld()->GetTimeSeconds() - LastDirectionalInputAt <= 0.14f)
	{
		const FRotator CameraYaw(0.0, RecoveryController->GetControlRotation().Yaw, 0.0);
		Recovery += CameraYaw.RotateVector(FVector(LastMovementInput.Y, LastMovementInput.X, 0.0)) * 485.0;
	}
	LaunchCharacter(Recovery, true, true);
	CombatStyle->PlayDash();
	return true;
}

bool ANINJA_ARENACharacter::TraceWall(const FVector& Direction, FHitResult& Hit) const
{
	const FVector Origin = GetActorLocation();
	const double WallReach = GetCapsuleComponent()->GetScaledCapsuleRadius() + 36.0;
	const FCollisionQueryParams Query(SCENE_QUERY_STAT(NinjaWallRun), false, this);
	if (!GetWorld()->SweepSingleByChannel(Hit, Origin, Origin + Direction * WallReach, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(12.f), Query)
		|| FMath::Abs(Hit.ImpactNormal.Z) > 0.22 || !Hit.GetComponent())
	{
		return false;
	}
	const ECollisionChannel HitType = Hit.GetComponent()->GetCollisionObjectType();
	return HitType == ECC_WorldStatic || HitType == ECC_WorldDynamic;
}

void ANINJA_ARENACharacter::StartWallRun()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (bUsedWallRun || bIsDashing || IsCombatLocked() || IsGuarding() || !Movement->IsFalling())
	{
		return;
	}

	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = GetActorRightVector().GetSafeNormal2D();
	FHitResult Nearest;
	for (const FVector& Probe : { Forward, Right, -Right, -Forward })
	{
		FHitResult Hit;
		if (TraceWall(Probe, Hit) && (!Nearest.bBlockingHit || Hit.Distance < Nearest.Distance))
		{
			Nearest = Hit;
		}
	}
	if (!Nearest.bBlockingHit)
	{
		return;
	}

	WallNormal = Nearest.ImpactNormal.GetSafeNormal2D();
	WallRunDirection = FVector::CrossProduct(FVector::UpVector, WallNormal);
	const FVector Travel = Movement->Velocity.SizeSquared2D() > FMath::Square(60.0)
		? Movement->Velocity.GetSafeNormal2D() : Forward;
	if (FVector::DotProduct(WallRunDirection, Travel) < 0.0)
	{
		WallRunDirection *= -1.0;
	}
	bUsedWallRun = true;
	bWallRunning = true;
	WallRunEndsAt = GetWorld()->GetTimeSeconds() + WallRunDuration;
	SetActorRotation(WallRunDirection.Rotation());
	UpdateWallRun(GetWorld()->GetTimeSeconds());
}

void ANINJA_ARENACharacter::UpdateWallRun(float Now)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	FHitResult Hit;
	if (Now >= WallRunEndsAt || !Movement->IsFalling() || IsCombatLocked() || !TraceWall(-WallNormal, Hit))
	{
		EndWallRun();
		return;
	}
	Movement->Velocity = WallRunDirection * (WallRunSpeed * Elements->GetSpeedScale()) - WallNormal * 64.0 + FVector(0.0, 0.0, 40.0);
}

void ANINJA_ARENACharacter::EndWallRun()
{
	bWallRunning = false;
}

void ANINJA_ARENACharacter::JumpOffWall()
{
	EndWallRun();
	JumpCurrentCount = FMath::Min(JumpCurrentCount + 1, JumpMaxCount);
	LaunchCharacter(WallNormal * 340.0 + WallRunDirection * 220.0 + FVector(0.0, 0.0, 430.0), true, true);
}
