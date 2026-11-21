
#include "NINJA_ARENACharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "NINJA_ARENA.h"
#include "NINJA_ARENAGameMode.h"
#include "NinjaCombatFeedback.h"
#include "NinjaCombatStyle.h"
#include "NinjaElementActors.h"
#include "NinjaElementComponent.h"
#include "PNAVitalsComponent.h"
#include "PNAAdvancedCombatComponent.h"
#include "Modes/PNADuelConstraintComponent.h"
#include "Stage/PNAStageDirector.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"

ANINJA_ARENACharacter::ANINJA_ARENACharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	Vitals = CreateDefaultSubobject<UPNAVitalsComponent>(TEXT("Vitals"));
	Elements = CreateDefaultSubobject<UNinjaElementComponent>(TEXT("Elements"));
	CombatFeedback = CreateDefaultSubobject<UNinjaCombatFeedbackComponent>(TEXT("CombatFeedback"));
	CombatStyle = CreateDefaultSubobject<UNinjaCombatStyleComponent>(TEXT("CombatStyle"));
	AdvancedCombat = CreateDefaultSubobject<UPNAAdvancedCombatComponent>(TEXT("AdvancedCombat"));
	DuelConstraint = CreateDefaultSubobject<UPNADuelConstraintComponent>(TEXT("DuelConstraint"));

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);

	GetCharacterMovement()->JumpZVelocity = 500.f;
	JumpMaxCount = 2;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(
		TEXT("CameraBoom"));

	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(
		TEXT("FollowCamera"));

	FollowCamera->SetupAttachment(
		CameraBoom,
		USpringArmComponent::SocketName);

	FollowCamera->bUsePawnControlRotation = false;
}

void ANINJA_ARENACharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (IsLocallyControlled() && CameraBoom)
	{
		const float DesiredLift = CameraBoom->IsCollisionFixApplied() ? 40.f : 0.f;
		const float Lift = FMath::FInterpTo(CameraClearanceLift, DesiredLift, DeltaTime, 7.f);
		CameraBoom->TargetOffset.Z += Lift - CameraClearanceLift;
		CameraClearanceLift = Lift;
	}
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		const ANINJA_ARENAGameMode* Mode = GetWorld()->GetAuthGameMode<ANINJA_ARENAGameMode>();
		if (Mode && Mode->IsSecondLocalPlayer(PlayerController))
		{
			const float Yaw = static_cast<float>(PlayerController->IsInputKeyDown(EKeys::L)) - PlayerController->IsInputKeyDown(EKeys::J);
			const float Pitch = static_cast<float>(PlayerController->IsInputKeyDown(EKeys::K)) - PlayerController->IsInputKeyDown(EKeys::I);
			if (Yaw != 0.f || Pitch != 0.f)
			{
				DoLook(Yaw * 105.f * DeltaTime, Pitch * 75.f * DeltaTime);
			}
		}
	}
	if (bRoundLocked)
	{
		return;
	}

	if (HasAuthority() && (GetActorLocation().Z < FallZ || Vitals->IsDefeated()))
	{
		if (!HandleRoundLoss())
		{
			RespawnAtStart();
		}
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (ANINJA_ARENACharacter* Target = LockedOpponent.Get())
	{
		const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
		if (Target->GetVitals()->IsDefeated() || ToTarget.SizeSquared() > FMath::Square(1800.0))
		{
			LockedOpponent.Reset();
		}
		else if (IsLocallyControlled() && !bWallRunning && GetController() && HasClearAttackLine(*Target))
		{
			FRotator Look = ToTarget.Rotation();
			Look.Pitch = FMath::Clamp(Look.Pitch - 9.0, -28.0, 21.0);
			GetController()->SetControlRotation(FMath::RInterpTo(GetControlRotation(), Look, DeltaTime, 8.f));
		}
	}
	if (PendingStrike.ImpactAt > 0.f)
	{
		if (IsCombatLocked() || IsGuarding() || bIsDashing || bWallRunning)
		{
			PendingStrike = {};
		}
		else if (Now >= PendingStrike.ImpactAt)
		{
			const FPendingStrike Strike = PendingStrike;
			PendingStrike = {};
			ResolveStrike(Strike);
		}
	}
	if (BufferedAttackUntil > 0.f)
	{
		if (Now > BufferedAttackUntil || IsGuarding() || (IsCombatLocked() && !CombatFeedback->IsInHitStop()))
		{
			BufferedAttackUntil = 0.f;
		}
		else if (Now >= NextAttackTime && !IsCombatLocked())
		{
			BufferedAttackUntil = 0.f;
			PerformAttack(bBufferedHeavyAttack);
		}
	}
	if (bUltimateCharging && Now >= UltimateCommitAt)
	{
		CommitUltimate();
	}
	if (bLaunched && Now >= AirComboEndsAt && !GetCharacterMovement()->IsFalling())
	{
		ClearLaunchState();
	}
	if (bWallRunning)
	{
		UpdateWallRun(Now);
	}
	if (bIsDashing)
	{
		if (Now >= DashEndTime)
		{
			EndDash();
		}
		else
		{
			FVector CurrentVelocity =
				GetCharacterMovement()->Velocity;

			const float Speed = DashSpeed * Elements->GetSpeedScale();
			CurrentVelocity.X = DashDirection.X * Speed;
			CurrentVelocity.Y = DashDirection.Y * Speed;

			GetCharacterMovement()->Velocity = CurrentVelocity;
		}
	}
}

void ANINJA_ARENACharacter::BeginPlay()
{
	Super::BeginPlay();
	SpawnTransform = GetActorTransform();

	// StageDirector is optional. Arena3D remains the safe default when none exists.
	for (TActorIterator<APNAStageDirector> It(GetWorld()); It; ++It)
	{
		StageDirector = *It;
		break;
	}

	if (StageDirector)
	{
		StageDirector->OnStageModeChanged.AddDynamic(this, &ANINJA_ARENACharacter::HandleStageModeChanged);
		ApplyStageMode(StageDirector->GetStageMode());
	}
	else if (DuelConstraint)
	{
		DuelConstraint->Configure(false, GetActorLocation().Y);
	}
}

void ANINJA_ARENACharacter::HandleStageModeChanged(EPNAStageMode OldMode, EPNAStageMode NewMode)
{
    ApplyStageMode(NewMode);
}

void ANINJA_ARENACharacter::ApplyStageMode(EPNAStageMode NewMode)
{
    if (!DuelConstraint)
    {
        return;
    }

    if (NewMode == EPNAStageMode::Duel25D)
    {
        const float PlaneY = StageDirector ? StageDirector->DuelPlaneY : GetActorLocation().Y;
        DuelConstraint->Configure(true, PlaneY);
    }
    else
    {
        DuelConstraint->SetEnabled(false);
    }
}

EPNACombatState ANINJA_ARENACharacter::GetCombatState() const
{
    if (bRoundLocked)
    {
        return EPNACombatState::RoundLocked;
    }

    if (Vitals && Vitals->IsDefeated())
    {
        return EPNACombatState::KO;
    }

    if (bUltimateCharging)
    {
        return EPNACombatState::Ultimate;
    }

    if (AdvancedCombat && AdvancedCombat->IsParryWindowOpen())
    {
        return EPNACombatState::Parrying;
    }

    if (bLaunched)
    {
        return EPNACombatState::Launched;
    }

    if (IsStunned())
    {
        return EPNACombatState::HitStun;
    }

    if (bGuarding)
    {
        return EPNACombatState::Blocking;
    }

    if (bIsDashing)
    {
        return EPNACombatState::Dodging;
    }

    if (PendingStrike.ImpactAt > 0.f)
    {
        return EPNACombatState::Attacking;
    }

    if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())
    {
        return EPNACombatState::Airborne;
    }

    return EPNACombatState::Neutral;
}

void ANINJA_ARENACharacter::ForceRespawn()
{
	RespawnAtStart();
}

void ANINJA_ARENACharacter::FellOutOfWorld(const UDamageType& DamageType)
{
	if (!bRoundLocked && !HandleRoundLoss() && !RespawnAtStart())
	{
		Super::FellOutOfWorld(DamageType);
	}
}

bool ANINJA_ARENACharacter::HandleRoundLoss()
{
	ANINJA_ARENAGameMode* TrainingMode = GetWorld()->GetAuthGameMode<ANINJA_ARENAGameMode>();
	return TrainingMode && TrainingMode->NotifyFighterDown(this);
}

void ANINJA_ARENACharacter::ResetTrainingRound()
{
	if (ANINJA_ARENAGameMode* TrainingMode = GetWorld()->GetAuthGameMode<ANINJA_ARENAGameMode>())
	{
		TrainingMode->RequestRoundReset(this);
	}
}

bool ANINJA_ARENACharacter::RespawnAtStart()
{
	UWorld* World = GetWorld();
	if (!World || !HasAuthority() || !HasActorBegunPlay())
	{
		return false;
	}

	if (!TeleportTo(SpawnTransform.GetLocation(), SpawnTransform.Rotator()))
	{
		AGameModeBase* GameMode = World->GetAuthGameMode();
		AActor* Start = GameMode && GetController() ? GameMode->FindPlayerStart(GetController()) : nullptr;
		if (!Start || !TeleportTo(Start->GetActorLocation(), Start->GetActorRotation()))
		{
			return false;
		}
	}

	bRaizenPowerForm = false;
	LockedOpponent.Reset();
	LastMeleeTarget.Reset();
	LastMeleeDamage = MeleeChainDamage = MeleeChainUntil = 0.f;
	MeleeChainHits = 0;
	CounterReadyUntil = 0.f;
	EndDash();
	StopSprint();
	CancelUltimate();
	UltimateReadyAt = 0.0;
	CombatFeedback->ResetFeedback();
	AdvancedCombat->EndTraceWindow();
	AdvancedCombat->SetInvulnerable(false);
	AdvancedCombat->SetHyperArmor(false);
	AdvancedCombat->SetParryWindow(false);
	CombatStyle->StopActionPose(0.f);
	LastMovementInput = FVector2D::ZeroVector;
	NextDashAvailableTime = 0.f;
	NextAttackTime = 0.f;
	PendingStrike = {};
	BufferedAttackUntil = 0.f;
	ComboIndex = 0;
	ComboExpiresAt = 0.f;
	StunEndsAt = 0.f;
	ResetAerialState(true);
	EndGuard();

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->StopMovementImmediately();
	Movement->ClearAccumulatedForces();
	ConsumeMovementInputVector();
	StopJumping();
	Movement->SetDefaultMovementMode();
	ResetJumpState();
	Vitals->ResetVitals();
	Elements->ResetElementState();
	return true;
}

void ANINJA_ARENACharacter::SetRoundLocked(bool bLocked)
{
	bRoundLocked = bLocked;
	if (bLocked)
	{
		PendingStrike = {};
		BufferedAttackUntil = 0.f;
		CancelUltimate();
		CombatFeedback->ResetFeedback();
		CombatStyle->StopActionPose(0.f);
		EndDash();
		EndWallRun();
		StopSprint();
		EndGuard();
		StopJumping();
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->ClearAccumulatedForces();
		GetCharacterMovement()->DisableMovement();
	}
	else
	{
		GetCharacterMovement()->SetDefaultMovementMode();
	}
}

bool ANINJA_ARENACharacter::IsStunned() const
{
	const UWorld* World = GetWorld();
	return World && World->GetTimeSeconds() < StunEndsAt;
}

bool ANINJA_ARENACharacter::IsCombatLocked() const
{
	return bRoundLocked || bLaunched || bUltimateCharging || IsStunned() || Vitals->IsDefeated()
		|| Elements->IsCharged() || CombatFeedback->IsInHitStop();
}

void ANINJA_ARENACharacter::BeginGuard()
{
	if (!bGuarding && !IsCombatLocked() && !bIsDashing && !bWallRunning)
	{
		StopSprint();
		BufferedAttackUntil = 0.f;
		if (PendingStrike.ImpactAt > 0.f)
		{
			PendingStrike = {};
			CombatStyle->StopActionPose();
		}
		bGuarding = true;
		PerfectGuardUntil = GetWorld()->GetTimeSeconds() + 0.10f;
	}
}

void ANINJA_ARENACharacter::EndGuard()
{
	bGuarding = false;
	PerfectGuardUntil = 0.f;
}

void ANINJA_ARENACharacter::Landed(const FHitResult& Hit)
{
	ResetAerialState(false);
	EndDash();
	Super::Landed(Hit);
}

bool ANINJA_ARENACharacter::CanJumpInternal_Implementation() const
{
	return !IsCombatLocked() && !IsGuarding() && Super::CanJumpInternal_Implementation();
}

bool ANINJA_ARENACharacter::IsCombatAlly(const AActor* Other) const
{
	const ANINJA_ARENACharacter* Fighter = Cast<ANINJA_ARENACharacter>(Other);
	return Fighter && Fighter != this && CombatTeam != INDEX_NONE && Fighter->CombatTeam == CombatTeam;
}

float ANINJA_ARENACharacter::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bRoundLocked || Vitals->IsDefeated() || IsCombatAlly(DamageCauser))
	{
		return 0.f;
	}
	const float AppliedDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	if (AppliedDamage > 0.f && bUltimateCharging)
	{
		CancelUltimate();
	}
	return AppliedDamage;
}

bool ANINJA_ARENACharacter::TryBlockHit(const FVector& ToAttacker, bool& bPerfectGuard)
{
	bPerfectGuard = false;
	if (!IsGuarding() || FVector::DotProduct(GetActorForwardVector(), ToAttacker) <= 0.25)
	{
		return false;
	}

	const bool bTimed = GetWorld()->GetTimeSeconds() < PerfectGuardUntil;
	PerfectGuardUntil = 0.f;
	const bool bBlocked = Vitals->SpendEnergy(bTimed ? GuardEnergyCost * 0.5f : GuardEnergyCost);
	bPerfectGuard = bBlocked && bTimed;
	return bBlocked;
}

void ANINJA_ARENACharacter::ReceiveMeleeHit(ANINJA_ARENACharacter* Attacker, float Damage, bool bHeavyAttack)
{
	if (!IsValid(Attacker) || bRoundLocked || bLaunched || Vitals->IsDefeated() || Damage <= 0.f)
	{
		return;
	}

	const EPNAAdvancedDefenseResult AdvancedDefense = AdvancedCombat
		? AdvancedCombat->EvaluateIncomingMelee(Attacker)
		: EPNAAdvancedDefenseResult::None;
	if (AdvancedDefense == EPNAAdvancedDefenseResult::Invulnerable)
	{
		return;
	}
	if (AdvancedDefense == EPNAAdvancedDefenseResult::Parried)
	{
		const float Now = GetWorld()->GetTimeSeconds();
		Attacker->StunEndsAt = FMath::Max(Attacker->StunEndsAt, Now + 0.38f);
		Attacker->BufferedAttackUntil = 0.f;
		Attacker->PendingStrike = {};
		UNinjaCombatFeedbackComponent::Hit(*Attacker, *this, ENinjaImpactKind::PerfectGuard);
		return;
	}

	const FVector ToAttacker = (Attacker->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	bool bPerfectGuard;
	const bool bBlocked = TryBlockHit(ToAttacker, bPerfectGuard);
	if (bPerfectGuard && CombatStyle->GetFighter() == ENinjaFighter::Kuroboshi)
	{
		const float Now = GetWorld()->GetTimeSeconds();
		CounterReadyUntil = Now + 0.60f;
		NextAttackTime = FMath::Min(NextAttackTime, Now);
	}
	const float FinalDamage = bPerfectGuard ? 0.f : (bBlocked ? Damage * FMath::Clamp(GuardDamageMultiplier, 0.f, 1.f) : Damage);
	const float PreviousHealth = Vitals->Health;
	const float AppliedDamage = UGameplayStatics::ApplyDamage(this, FinalDamage, Attacker->GetController(), Attacker, nullptr);
	Attacker->RecordMeleeContact(this, FMath::Max(0.f, PreviousHealth - Vitals->Health), bBlocked);
	if (AppliedDamage > 0.f || bBlocked)
	{
		UNinjaCombatFeedbackComponent::Hit(*Attacker, *this, bPerfectGuard ? ENinjaImpactKind::PerfectGuard : bBlocked ? ENinjaImpactKind::Guard
			: (bHeavyAttack ? ENinjaImpactKind::Heavy : ENinjaImpactKind::Light));
	}
	if (AppliedDamage <= 0.f || bBlocked || Vitals->IsDefeated())
	{
		return;
	}
	if (AdvancedDefense == EPNAAdvancedDefenseResult::Armored)
	{
		// Hyper armor keeps damage but suppresses hit-stun, movement interruption and launch.
		return;
	}

	BufferedAttackUntil = 0.f;
	PendingStrike = {};
	StunEndsAt = GetWorld()->GetTimeSeconds() + FMath::Max(0.f, bHeavyAttack ? HeavyHitStun : LightHitStun);
	EndGuard();
	EndDash();
	EndWallRun();
	StopSprint();
	StopJumping();
	ConsumeMovementInputVector();
	GetCharacterMovement()->StopMovementImmediately();
	if (bHeavyAttack && HeavyLaunchVelocity.Y > 0.f)
	{
		bLaunched = true;
		LaunchSource = Attacker;
		AirComboEndsAt = GetWorld()->GetTimeSeconds() + AirComboWindow;
		ReceivedAirHits = 0;
		Attacker->AirComboTarget = this;
		Attacker->bUsedPursuit = false;
		GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		LaunchCharacter(-ToAttacker * FMath::Max(0.f, HeavyLaunchVelocity.X) + FVector(0.f, 0.f, HeavyLaunchVelocity.Y), true, true);
	}
}

void ANINJA_ARENACharacter::LightAttack()
{
	PerformAttack(false);
}

void ANINJA_ARENACharacter::HeavyAttack()
{
	PerformAttack(true);
}

int32 ANINJA_ARENACharacter::GetComboStep() const
{
	const UWorld* World = GetWorld();
	return World && World->GetTimeSeconds() < ComboExpiresAt ? ComboIndex : 0;
}

FVector ANINJA_ARENACharacter::GetMeleeCenter(const FVector& Facing) const
{
	return GetActorLocation() + Facing * FMath::Max(0.f, AttackRange);
}

bool ANINJA_ARENACharacter::CanMeleeReach(const ANINJA_ARENACharacter& Target, const FVector& Facing) const
{
	float Radius, HalfHeight;
	Target.GetCapsuleComponent()->GetScaledCapsuleSize(Radius, HalfHeight);
	const double SegmentHalfLength = FMath::Max(0.f, HalfHeight - Radius);
	FVector Offset = Target.GetActorLocation() - GetMeleeCenter(Facing);
	Offset.Z -= FMath::Clamp(Offset.Z, -SegmentHalfLength, SegmentHalfLength);
	return Offset.SizeSquared() <= FMath::Square(FMath::Max(1.f, AttackRadius) + Radius) && HasClearAttackLine(Target);
}

void ANINJA_ARENACharacter::ToggleOpponentLock()
{
	if (LockedOpponent.IsValid())
	{
		LockedOpponent.Reset();
		return;
	}
	const ANINJA_ARENAGameMode* Mode = GetWorld()->GetAuthGameMode<ANINJA_ARENAGameMode>();
	ANINJA_ARENACharacter* Rival = Mode ? Mode->GetDuelOpponent(this) : nullptr;
	if (!Rival || IsCombatLocked())
	{
		return;
	}

	const FVector Forward = GetControlRotation().Vector().GetSafeNormal2D();
	double BestScore = 0.0;
	const auto Consider = [this, &BestScore, &Forward](ANINJA_ARENACharacter* Target)
	{
		if (!IsValid(Target) || Target->GetVitals()->IsDefeated()) { return; }
		const FVector Offset = Target->GetActorLocation() - GetActorLocation();
		const double Distance = Offset.Size();
		const double Alignment = FVector::DotProduct(Forward, Offset.GetSafeNormal2D());
		const double Score = (Alignment + 1.0) / FMath::Max(100.0, Distance);
		if (Distance <= 1500.0 && Alignment > 0.20 && Score > BestScore && HasClearAttackLine(*Target))
		{
			LockedOpponent = Target;
			BestScore = Score;
		}
	};
	Consider(Rival);
	if (Mode->IsTeamBattle())
	{
		for (const TObjectPtr<ANINJA_ARENACharacter>& Bot : Mode->GetTeamBots(Rival->GetCombatTeam()))
		{
			Consider(Bot.Get());
		}
	}
}

void ANINJA_ARENACharacter::RecordMeleeContact(ANINJA_ARENACharacter* Target, float Damage, bool bBlocked)
{
	const float Now = GetWorld()->GetTimeSeconds();
	LastMeleeDamage = Damage;
	if (bBlocked || Damage <= 0.f)
	{
		MeleeChainUntil = 0.f;
		return;
	}
	if (Now >= MeleeChainUntil || LastMeleeTarget.Get() != Target)
	{
		MeleeChainDamage = 0.f;
		MeleeChainHits = 0;
	}
	LastMeleeTarget = Target;
	MeleeChainDamage += Damage;
	++MeleeChainHits;
	MeleeChainUntil = Now + 1.0f;
}

int32 ANINJA_ARENACharacter::GetMeleeChainHits() const
{
	return GetWorld()->GetTimeSeconds() < MeleeChainUntil ? MeleeChainHits : 0;
}

float ANINJA_ARENACharacter::GetMeleeChainDamage() const
{
	return GetMeleeChainHits() > 0 ? MeleeChainDamage : 0.f;
}

FVector ANINJA_ARENACharacter::GetMeleeFacing() const
{
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	if (const ANINJA_ARENACharacter* Locked = LockedOpponent.Get(); Locked && !Locked->GetVitals()->IsDefeated())
	{
		const FVector Direction = (Locked->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		if (CanMeleeReach(*Locked, Direction)) { return Direction; }
	}
	const AController* AimController = GetController();
	if (!AimController || !IsLocallyControlled())
	{
		return Forward;
	}

	const ANINJA_ARENAGameMode* Mode = GetWorld()->GetAuthGameMode<ANINJA_ARENAGameMode>();
	const ANINJA_ARENACharacter* Rival = Mode ? Mode->GetDuelOpponent(this) : nullptr;
	const FVector CameraFacing = FRotator(0.0, AimController->GetControlRotation().Yaw, 0.0).Vector();
	FVector Facing = Forward;
	double BestAlignment = 0.67;
	const auto Consider = [this, &CameraFacing, &BestAlignment, &Facing](const ANINJA_ARENACharacter* Candidate)
	{
		if (IsValid(Candidate) && Candidate != this && !Candidate->GetVitals()->IsDefeated())
		{
			const FVector ToCandidate = (Candidate->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
			const double Alignment = FVector::DotProduct(CameraFacing, ToCandidate);
			if (Alignment >= BestAlignment && CanMeleeReach(*Candidate, ToCandidate))
			{
				BestAlignment = Alignment;
				Facing = ToCandidate;
			}
		}
	};
	Consider(Rival);
	if (IsValid(Rival))
	{
		for (const ANINJA_ARENACharacter* Bot : Mode->GetTeamBots(Rival->GetCombatTeam()))
		{
			Consider(Bot);
		}
	}
	return Facing;
}

bool ANINJA_ARENACharacter::IsCounterReady() const
{
	return CombatStyle->GetFighter() == ENinjaFighter::Kuroboshi && GetWorld()->GetTimeSeconds() < CounterReadyUntil;
}

void ANINJA_ARENACharacter::ToggleRaizenForm()
{
	if (!HasAuthority() || CombatStyle->GetFighter() != ENinjaFighter::Raizen || IsCombatLocked() || IsGuarding()
		|| PendingStrike.ImpactAt > 0.f || GetWorld()->GetTimeSeconds() < NextAttackTime)
	{
		return;
	}
	if (!bRaizenPowerForm && !Vitals->SpendEnergy(14.f))
	{
		return;
	}
	bRaizenPowerForm = !bRaizenPowerForm;
	RefreshMovementSpeed();
	CombatStyle->PlaySound(ENinjaStyleSound::Energy, GetActorLocation());
}

void ANINJA_ARENACharacter::PerformAttack(bool bHeavyAttack)
{
	UWorld* World = GetWorld();
	if (!World || !HasActorBegunPlay() || !HasAuthority() || IsGuarding())
	{
		return;
	}

	const float Now = World->GetTimeSeconds();
	const bool bHitStop = CombatFeedback->IsInHitStop();
	if (IsCombatLocked() || Now < NextAttackTime)
	{
		constexpr float InputBufferWindow = 0.16f;
		if (IsPlayerControlled() && (bHitStop || !IsCombatLocked()) && !IsStunned()
			&& !bRoundLocked && !bLaunched && !bUltimateCharging && !Vitals->IsDefeated() && !Elements->IsCharged()
			&& NextAttackTime <= Now + InputBufferWindow)
		{
			BufferedAttackUntil = Now + InputBufferWindow;
			bBufferedHeavyAttack = bHeavyAttack;
		}
		return;
	}
	BufferedAttackUntil = 0.f;
	const bool bInAir = GetCharacterMovement()->IsFalling();
	if (!bHeavyAttack && bInAir && TryAirComboHit(Now))
	{
		return;
	}
	const ENinjaFighter Fighter = CombatStyle->GetFighter();
	const bool bCounter = bHeavyAttack && !bInAir && IsCounterReady();
	const bool bPower = Fighter == ENinjaFighter::Raizen && bRaizenPowerForm;
	const bool bAkiraEntry = Fighter == ENinjaFighter::AkiraVioleta && bHeavyAttack && !bInAir;
	const float Cost = bCounter ? 12.f : (bHeavyAttack ? HeavyEnergyCost : LightEnergyCost);
	if (!Vitals->SpendEnergy(Cost))
	{
		return;
	}

	EndWallRun();
	const bool bGroundCombo = !bHeavyAttack && !bInAir;
	ComboIndex = bGroundCombo ? (GetComboStep() % 3) + 1 : 0;
	ComboExpiresAt = bGroundCombo ? Now + FMath::Max(0.f, ComboWindow) : 0.f;
	const FNinjaFighterProfile& Profile = CombatStyle->GetProfile();
	const float CustomLightScale = Profile.LightDamageScale[FMath::Max(0, ComboIndex - 1)];
	const float LightScale = CustomLightScale > 0.f
		? CustomLightScale : 1.f + FMath::Max(0, ComboIndex - 1) * ComboDamageBonusPerStep;
	const float Damage = (bHeavyAttack ? HeavyDamage * Profile.HeavyDamageScale : LightDamage * LightScale) * (bCounter ? 1.18f : (bPower ? 1.20f : 1.f));
	const FVector Facing = GetMeleeFacing();
	SetActorRotation(Facing.Rotation());
	const bool bSpectralFinisher = Fighter == ENinjaFighter::Kuroboshi && ComboIndex == 3;
	const float Windup = bCounter ? 0.11f : (bHeavyAttack ? 0.19f * Profile.RecoveryScale
		: (bSpectralFinisher ? 0.16f : 0.125f * 1.28f / Profile.LightPlayRate[FMath::Max(0, ComboIndex - 1)]));
	CombatStyle->PlayStrike(ComboIndex, bHeavyAttack, bInAir ? 0.f : Windup);
	const float RecoveryFloor = bHeavyAttack ? 0.27f : 0.14f;
	NextAttackTime = Now + FMath::Max(AttackCooldown * Profile.RecoveryScale, Windup + RecoveryFloor) * (bPower ? 1.12f : 1.f);
	PendingStrike = {Now + Windup, Damage, bCounter ? 58.f : (bSpectralFinisher ? 42.f : 0.f), Facing, bHeavyAttack};
	if (bCounter)
	{
		CounterReadyUntil = 0.f;
	}
	if (bAkiraEntry)
	{
		// Character movement sweeps this step against walls and other fighters.
		GetCharacterMovement()->Velocity = Facing * 265.0;
	}
}

void ANINJA_ARENACharacter::ResolveStrike(const FPendingStrike& Strike)
{
	UWorld* World = GetWorld();
	const float Damage = Strike.Damage;
	const bool bHeavyAttack = Strike.bHeavy;
	if (bHeavyAttack || Strike.RangeBonus > 0.f)
	{
		CombatStyle->PlayBladeTrace();
	}
	const FVector Origin = GetActorLocation();
	const FVector Center = GetMeleeCenter(Strike.Facing) + Strike.Facing * Strike.RangeBonus;
	TArray<FOverlapResult> Hits;
	FCollisionObjectQueryParams ObjectQuery(ECC_Pawn);
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldDynamic);
	const FCollisionQueryParams Query(SCENE_QUERY_STAT(PNAAttack), false, this);
	World->OverlapMultiByObjectType(Hits, Center, FQuat::Identity, ObjectQuery,
		FCollisionShape::MakeSphere(FMath::Max(1.f, AttackRadius)), Query);

	TSet<AActor*> CheckedTargets;
	bool bConnected = false;
	for (const FOverlapResult& Hit : Hits)
	{
		AActor* Target = Hit.GetActor();
		const UPrimitiveComponent* HitComponent = Hit.GetComponent();
		if (!IsValid(Target) || IsCombatAlly(Target) || !HitComponent || !Target->CanBeDamaged() || CheckedTargets.Contains(Target)
			|| (HitComponent->GetCollisionObjectType() != ECC_Pawn && !Target->IsA<ANinjaElementProp>()))
		{
			continue;
		}
		CheckedTargets.Add(Target);

		if (HasClearAttackLine(*Target))
		{
			bConnected = true;
			if (ANINJA_ARENACharacter* Fighter = Cast<ANINJA_ARENACharacter>(Target))
			{
				Fighter->ReceiveMeleeHit(this, Damage, bHeavyAttack);
			}
			else
			{
				if (UGameplayStatics::ApplyDamage(Target, Damage, GetController(), this, nullptr) > 0.f)
				{
					UNinjaCombatFeedbackComponent::Hit(*this, *Target, bHeavyAttack ? ENinjaImpactKind::Heavy : ENinjaImpactKind::Light);
				}
			}
		}
	}
	if (!bConnected)
	{
		if (bHeavyAttack)
		{
			NextAttackTime += 0.12f;
		}
		CombatStyle->PlaySound(ENinjaStyleSound::Swing, Origin);
	}
}

void ANINJA_ARENACharacter::SetupPlayerInputComponent(
	UInputComponent* PlayerInputComponent)
{
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ANINJA_ARENACharacter::LightAttack).bConsumeInput = false;
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ANINJA_ARENACharacter::HeavyAttack).bConsumeInput = false;
	PlayerInputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &ANINJA_ARENACharacter::ToggleOpponentLock).bConsumeInput = false;
	PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &ANINJA_ARENACharacter::BeginGuard).bConsumeInput = false;
	PlayerInputComponent->BindKey(EKeys::Q, IE_Released, this, &ANINJA_ARENACharacter::EndGuard).bConsumeInput = false;
	PlayerInputComponent->BindKey(EKeys::R, IE_Pressed, this, &ANINJA_ARENACharacter::ResetTrainingRound).bConsumeInput = false;
	PlayerInputComponent->BindKey(EKeys::C, IE_Pressed, this, &ANINJA_ARENACharacter::StartAirPursuit).bConsumeInput = false;
	PlayerInputComponent->BindKey(EKeys::V, IE_Pressed, this, &ANINJA_ARENACharacter::ToggleRaizenForm).bConsumeInput = false;
	PlayerInputComponent->BindKey(EKeys::G, IE_Pressed, this, &ANINJA_ARENACharacter::StartUltimate).bConsumeInput = false;
	PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &ANINJA_ARENACharacter::StartWallRun).bConsumeInput = false;
	PlayerInputComponent->BindKey(EKeys::E, IE_Released, this, &ANINJA_ARENACharacter::EndWallRun).bConsumeInput = false;
	const auto BindElement = [this, PlayerInputComponent](const FKey& Key, ENinjaElement Element)
	{
		FInputKeyBinding Binding(FInputChord(Key), IE_Pressed);
		Binding.bConsumeInput = false;
		Binding.KeyDelegate.GetDelegateForManualSet().BindWeakLambda(this, [this, Element]()
		{
			Elements->CastElement(Element);
		});
		PlayerInputComponent->KeyBindings.Add(MoveTemp(Binding));
	};
	BindElement(EKeys::One, ENinjaElement::Fire);
	BindElement(EKeys::Two, ENinjaElement::Lightning);
	BindElement(EKeys::Three, ENinjaElement::Wind);
	BindElement(EKeys::Four, ENinjaElement::Ice);
	BindElement(EKeys::Five, ENinjaElement::Water);
	if (!DashAction)
	{
		PlayerInputComponent->BindKey(EKeys::F, IE_Pressed, this, &ANINJA_ARENACharacter::StartDash).bConsumeInput = false;
	}
	if (UEnhancedInputComponent* EnhancedInputComponent =
		Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jump
		EnhancedInputComponent->BindAction(
			JumpAction,
			ETriggerEvent::Started,
			this,
			&ANINJA_ARENACharacter::DoJumpStart);

		EnhancedInputComponent->BindAction(
			JumpAction,
			ETriggerEvent::Completed,
			this,
			&ANINJA_ARENACharacter::DoJumpEnd);

		// Movement and camera
		EnhancedInputComponent->BindAction(
			MoveAction,
			ETriggerEvent::Triggered,
			this,
			&ANINJA_ARENACharacter::Move);

		EnhancedInputComponent->BindAction(
			MouseLookAction,
			ETriggerEvent::Triggered,
			this,
			&ANINJA_ARENACharacter::Look);

		EnhancedInputComponent->BindAction(
			LookAction,
			ETriggerEvent::Triggered,
			this,
			&ANINJA_ARENACharacter::Look);

		// Ninja sprint
		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(
				SprintAction,
				ETriggerEvent::Started,
				this,
				&ANINJA_ARENACharacter::StartSprint);

			EnhancedInputComponent->BindAction(
				SprintAction,
				ETriggerEvent::Completed,
				this,
				&ANINJA_ARENACharacter::StopSprint);

			EnhancedInputComponent->BindAction(
				SprintAction,
				ETriggerEvent::Canceled,
				this,
				&ANINJA_ARENACharacter::StopSprint);
		}

		// Ninja dash
		if (DashAction)
		{
			EnhancedInputComponent->BindAction(
				DashAction,
				ETriggerEvent::Started,
				this,
				&ANINJA_ARENACharacter::StartDash);
		}
	}
	else
	{
		UE_LOG(
			LogNINJA_ARENA,
			Error,
			TEXT("Enhanced Input component not found on %s"),
			*GetNameSafe(this));
	}
}

void ANINJA_ARENACharacter::Move(
	const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	DoMove(MovementVector.X, MovementVector.Y);
}

void ANINJA_ARENACharacter::Look(
	const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ANINJA_ARENACharacter::DoMove(
	float Right,
	float Forward)
{
	LastMovementInput = FVector2D(Right, Forward).GetClampedToMaxSize(1.f);
	LastDirectionalInputAt = GetWorld()->GetTimeSeconds();
	if (bIsDashing || bWallRunning || IsCombatLocked())
	{
		return;
	}

	if (GetController() != nullptr)
	{
		const FRotator Rotation =
			GetController()->GetControlRotation();

		const FRotator YawRotation(
			0.f,
			Rotation.Yaw,
			0.f);

		const FVector ForwardDirection =
			FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		const FVector RightDirection =
			FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ANINJA_ARENACharacter::DoLook(
	float Yaw,
	float Pitch)
{
	if (GetController() != nullptr)
	{
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ANINJA_ARENACharacter::StartSprint()
{
	if (bIsDashing || IsCombatLocked() || IsGuarding())
	{
		return;
	}

	bWantsToSprint = true;

	RefreshMovementSpeed();
}

void ANINJA_ARENACharacter::StopSprint()
{
	bWantsToSprint = false;

	RefreshMovementSpeed();
}

void ANINJA_ARENACharacter::RefreshMovementSpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = (bWantsToSprint ? SprintSpeed : NormalWalkSpeed) * Elements->GetSpeedScale() * (bRaizenPowerForm ? 0.79f : 1.f);
}

void ANINJA_ARENACharacter::InterruptForElement(const FVector& Impulse)
{
	CancelUltimate();
	EndDash();
	EndWallRun();
	EndGuard();
	StopSprint();
	StopJumping();
	ConsumeMovementInputVector();
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->Velocity.X = 0.0;
	Movement->Velocity.Y = 0.0;
	if (!Impulse.IsNearlyZero())
	{
		Movement->SetMovementMode(MOVE_Falling);
		LaunchCharacter(Impulse, true, true);
	}
}

void ANINJA_ARENACharacter::StartDash()
{
	if (!GetWorld() || bIsDashing || IsCombatLocked() || IsGuarding())
	{
		return;
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	const bool bInAir = Movement->IsFalling();
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime < NextDashAvailableTime || (bInAir && !HasAirDash()) ||
		(!bInAir && !Movement->IsMovingOnGround()))
	{
		return;
	}

	FVector Direction = FVector::ZeroVector;

	if (GetController())
	{
		const FRotator ControlRotation =
			GetController()->GetControlRotation();

		const FRotator YawRotation(
			0.f,
			ControlRotation.Yaw,
			0.f);

		const FVector Forward =
			FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		const FVector Right =
			FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		Direction =
			Forward * LastMovementInput.Y +
			Right * LastMovementInput.X;
	}

	if (Direction.IsNearlyZero())
	{
		Direction = GetActorForwardVector();
	}

	Direction.Z = 0.f;
	Direction.Normalize();

	DashDirection = Direction;

	EndWallRun();
	bIsDashing = true;
	bUsedAirDash |= bInAir;

	DashEndTime = CurrentTime + (bInAir ? AirDashDuration : DashDuration);
	NextDashAvailableTime = CurrentTime + DashCooldown;

	FVector CurrentVelocity = Movement->Velocity;

	const float Speed = DashSpeed * Elements->GetSpeedScale();
	CurrentVelocity.X = DashDirection.X * Speed;
	CurrentVelocity.Y = DashDirection.Y * Speed;

	if (bInAir)
	{
		CurrentVelocity.Z = FMath::Clamp(CurrentVelocity.Z, -120.0, 120.0);
	}
	Movement->Velocity = CurrentVelocity;
	CombatStyle->PlayDash();
}

void ANINJA_ARENACharacter::EndDash()
{
	bIsDashing = false;
}

void ANINJA_ARENACharacter::DoJumpStart()
{
	if (bWallRunning)
	{
		JumpOffWall();
	}
	else if (!TryAirRecovery())
	{
		Jump();
	}
}

void ANINJA_ARENACharacter::DoJumpEnd()
{
	StopJumping();
}
