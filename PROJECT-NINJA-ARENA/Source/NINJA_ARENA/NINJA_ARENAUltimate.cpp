#include "NINJA_ARENACharacter.h"

#include "Engine/OverlapResult.h"
#include "Components/CapsuleComponent.h"
#include "Misc/App.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Scalability.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "NinjaCombatFeedback.h"
#include "NinjaCombatStyle.h"
#include "NinjaElementActors.h"
#include "PNAVitalsComponent.h"

namespace
{
	void StepThroughShadow(ANINJA_ARENACharacter& Fighter, const FVector& Direction)
	{
		if (!Fighter.GetCharacterMovement()->IsMovingOnGround())
		{
			return;
		}
		UWorld* World = Fighter.GetWorld();
		const UCapsuleComponent* Capsule = Fighter.GetCapsuleComponent();
		const FVector Origin = Fighter.GetActorLocation();
		const FCollisionQueryParams Query(SCENE_QUERY_STAT(GhostbladeStep), false, &Fighter);
		const FCollisionShape Shape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight());
		FHitResult Block;
		const bool bBlocked = World->SweepSingleByProfile(Block, Origin, Origin + Direction * 225.0, FQuat::Identity,
			Capsule->GetCollisionProfileName(), Shape, Query);
		const FVector Target = Origin + Direction * (bBlocked ? FMath::Max(0.f, Block.Distance - 20.f) : 225.f);
		FHitResult Floor;
		if (World->LineTraceSingleByChannel(Floor, Target + FVector(0, 0, 45), Target - FVector(0, 0, 165), ECC_Visibility, Query)
			&& Fighter.GetCharacterMovement()->IsWalkable(Floor) && FVector::DistSquared2D(Target, Origin) > FMath::Square(25.0))
		{
			const FVector Landing = Floor.ImpactPoint + FVector(0, 0, Capsule->GetScaledCapsuleHalfHeight() + 3.f);
			if (!World->OverlapBlockingTestByProfile(Landing, FQuat::Identity, Capsule->GetCollisionProfileName(), Shape, Query))
			{
				Fighter.TeleportTo(Landing, Direction.Rotation());
			}
		}
	}

	void ShowGhostbladeCut(UWorld& World, const FVector& Origin, const FVector& Direction)
	{
		if (ANinjaElementPulse* Pulse = World.SpawnActor<ANinjaElementPulse>(Origin + Direction * 110.0, Direction.Rotation()))
		{
			Pulse->Show(FLinearColor(0.22f, 0.58f, 1.f), 220.f, 75.f);
		}
		if (!FApp::CanEverRender() || Scalability::GetEffectsQualityDirect(true) == 0)
		{
			return;
		}
		const TSoftObjectPtr<UNiagaraSystem> Slash(FSoftObjectPath(
			TEXT("/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Slash.NS_Free_Magic_Slash")));
		if (UNiagaraSystem* System = Slash.LoadSynchronous())
		{
			if (UNiagaraComponent* Fx = UNiagaraFunctionLibrary::SpawnSystemAtLocation(&World, System, Origin + Direction * 90.0,
				Direction.Rotation(), FVector(0.66), true, false))
			{
				const FLinearColor Blue(0.18f, 0.65f, 2.f);
				Fx->SetVariableLinearColor(TEXT("User.Color_Spiral1"), Blue);
				Fx->SetVariableLinearColor(TEXT("User.Color_Sparks1"), Blue);
				Fx->SetVariableLinearColor(TEXT("User.Color_Sparks2"), FLinearColor(0.48f, 0.8f, 1.f));
				Fx->SetVariableLinearColor(TEXT("User.Color_Smoke"), FLinearColor(0.11f, 0.22f, 0.52f));
				Fx->Activate(true);
				FTimerHandle Lifetime;
				World.GetTimerManager().SetTimer(Lifetime, FTimerDelegate::CreateWeakLambda(Fx, [Fx]() { Fx->DestroyComponent(); }), 0.8f, false);
			}
		}
	}
}

float ANINJA_ARENACharacter::GetUltimateCooldownRemaining() const
{
	const UWorld* World = GetWorld();
	return World ? static_cast<float>(FMath::Max(0.0, UltimateReadyAt - World->GetTimeSeconds())) : 0.f;
}

void ANINJA_ARENACharacter::StartUltimate()
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || !HasActorBegunPlay() || !World || IsCombatLocked() || IsGuarding()
		|| World->GetTimeSeconds() < FMath::Max(UltimateReadyAt, static_cast<double>(NextAttackTime))
		|| !Vitals->SpendEnergy(UltimateEnergyCost))
	{
		return;
	}

	const double Now = World->GetTimeSeconds();
	const AController* AimController = GetController();
	const double Yaw = AimController ? AimController->GetControlRotation().Yaw : GetActorRotation().Yaw;
	UltimateDirection = FRotator(0.0, Yaw, 0.0).Vector();
	SetActorRotation(UltimateDirection.Rotation());
	bUltimateCharging = true;
	UltimateCommitAt = Now + UltimateWindup;
	UltimateReadyAt = Now + UltimateCooldown;
	NextAttackTime = static_cast<float>(UltimateCommitAt + 0.36);
	ComboIndex = 0;
	ComboExpiresAt = 0.f;
	EndDash();
	EndWallRun();
	StopSprint();
	StopJumping();
	ConsumeMovementInputVector();
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->Velocity.X = 0.0;
	Movement->Velocity.Y = 0.0;
	CombatFeedback->CueUltimateCharge();
	CombatStyle->PlayCharge();
}

void ANINJA_ARENACharacter::CancelUltimate()
{
	if (bUltimateCharging)
	{
		bUltimateCharging = false;
		UltimateCommitAt = 0.0;
		CombatFeedback->ReleaseCameraCue();
		CombatStyle->StopActionPose();
	}
}

void ANINJA_ARENACharacter::CommitUltimate()
{
	bUltimateCharging = false;
	UltimateCommitAt = 0.0;
	if (IsCombatLocked() || IsGuarding())
	{
		CombatFeedback->ReleaseCameraCue();
		CombatStyle->StopActionPose();
		return;
	}

	UWorld* World = GetWorld();
	const bool bSpectral = CombatStyle->GetFighter() == ENinjaFighter::Kuroboshi;
	if (bSpectral)
	{
		StepThroughShadow(*this, UltimateDirection);
		ShowGhostbladeCut(*World, GetActorLocation(), UltimateDirection);
	}
	const FVector Origin = GetActorLocation();
	const FVector Center = Origin + UltimateDirection * (bSpectral ? 128.0 : 210.0);
	const float Damage = bSpectral ? UltimateDamage * 0.82f : UltimateDamage;
	CombatFeedback->CueUltimateRelease(Origin + UltimateDirection * 110.0);
	CombatStyle->PlaySound(ENinjaStyleSound::Ultimate, Origin);
	FCollisionObjectQueryParams Objects(ECC_Pawn);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	const FCollisionQueryParams Query(SCENE_QUERY_STAT(PulsoNinja), false, this);
	TArray<FOverlapResult> Hits;
	World->OverlapMultiByObjectType(Hits, Center, FQuat::Identity, Objects, FCollisionShape::MakeSphere(bSpectral ? 105.f : 185.f), Query);

	TSet<AActor*> Checked;
	for (const FOverlapResult& Hit : Hits)
	{
		AActor* Target = Hit.GetActor();
		if (!IsValid(Target) || IsCombatAlly(Target) || !Target->CanBeDamaged() || Checked.Contains(Target)
			|| FVector::DotProduct(Target->GetActorLocation() - Origin, UltimateDirection) <= 0.0)
		{
			continue;
		}
		Checked.Add(Target);
		if (!HasClearAttackLine(*Target))
		{
			continue;
		}
		if (ANINJA_ARENACharacter* Fighter = Cast<ANINJA_ARENACharacter>(Target))
		{
			Fighter->ReceiveUltimateHit(*this, Damage);
		}
		else if (Target->IsA<ANinjaElementProp>())
		{
			UGameplayStatics::ApplyDamage(Target, Damage, GetController(), this, nullptr);
		}
	}
}

void ANINJA_ARENACharacter::ReceiveUltimateHit(ANINJA_ARENACharacter& Attacker, float Damage)
{
	if (bRoundLocked || Vitals->IsDefeated())
	{
		return;
	}

	const FVector ToAttacker = (Attacker.GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	bool bPerfectGuard;
	const bool bBlocked = TryBlockHit(ToAttacker, bPerfectGuard);
	const float FinalDamage = bPerfectGuard ? 0.f : (bBlocked ? Damage * FMath::Clamp(GuardDamageMultiplier, 0.f, 1.f) : Damage);
	const float AppliedDamage = UGameplayStatics::ApplyDamage(this, FinalDamage, Attacker.GetController(), &Attacker, nullptr);
	if (AppliedDamage > 0.f || bBlocked)
	{
		UNinjaCombatFeedbackComponent::Hit(Attacker, *this, bPerfectGuard ? ENinjaImpactKind::PerfectGuard : bBlocked ? ENinjaImpactKind::Guard : ENinjaImpactKind::Ultimate, false);
	}
	if (AppliedDamage <= 0.f || bBlocked || Vitals->IsDefeated())
	{
		return;
	}

	StunEndsAt = GetWorld()->GetTimeSeconds() + 0.65f;
	ClearLaunchState();
	bLaunched = true;
	AirComboEndsAt = GetWorld()->GetTimeSeconds() + 1.25f;
	InterruptForElement(-ToAttacker * 615.0 + FVector(0.0, 0.0, 465.0));
}
