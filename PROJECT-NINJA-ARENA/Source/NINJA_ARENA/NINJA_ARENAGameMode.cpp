// Copyright Epic Games, Inc. All Rights Reserved.

#include "NINJA_ARENAGameMode.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "NINJA_ARENACharacter.h"
#include "NinjaCombatStyle.h"
#include "NinjaElementActors.h"
#include "NinjaTrainingHUD.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Misc/App.h"
#include "PNAVitalsComponent.h"
#include "UObject/Package.h"

ANINJA_ARENAGameMode::ANINJA_ARENAGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
}

bool ANINJA_ARENAGameMode::IsTrainingArena() const
{
	const UWorld* World = GetWorld();
	if (!World || World->GetNetMode() != NM_Standalone)
	{
		return false;
	}

	const FString MapPackage = World->GetOutermost()->GetName();
	const FString ShortName = UWorld::RemovePIEPrefix(FPackageName::GetShortName(MapPackage));
	if (FPackageName::GetLongPackagePath(MapPackage) == TEXT("/Game/NinjaArena/Maps") && ShortName == TEXT("L_PNA_Coliseo_Dorado_V4"))
	{
		return true;
	}
	return FPackageName::GetLongPackagePath(MapPackage) == TEXT("/Game/Arena")
		&& (UWorld::RemovePIEPrefix(FPackageName::GetShortName(MapPackage)) == TEXT("Lvl_ThirdPerson")
			|| UWorld::RemovePIEPrefix(FPackageName::GetShortName(MapPackage)) == TEXT("Lvl_Campo")
			|| UWorld::RemovePIEPrefix(FPackageName::GetShortName(MapPackage)) == TEXT("Lvl_Templo"));
}

void ANINJA_ARENAGameMode::BeginPlay()
{
	Super::BeginPlay();
	const bool bTraining = IsTrainingArena();
	SetActorTickEnabled(bTraining && !bModeMenu);
	if (bTraining)
	{
		ApplyArenaPalette();
		if (bTempleArena)
		{
			for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
			{
				if (It->ActorHasTag(TEXT("TempleFragileBridge")))
				{
					TempleBridge = *It;
					break;
				}
			}
		}
		if (bModeMenu)
		{
			EnableModeMenu();
			return;
		}
		if (bLocalVersus)
		{
			EnableLocalVersusInput();
		}
		else
		{
			EnableModeMenu();
		}
	}
}

void ANINJA_ARENAGameMode::ApplyArenaPalette()
{
	const FName AccentName(TEXT("MI_ThirdPersonColWay"));
	const FName StoneName(TEXT("MI_PrototypeGrid_Gray"));
	TMap<UMaterialInterface*, UMaterialInstanceDynamic*> Palette;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		UStaticMeshComponent* Mesh = It->GetStaticMeshComponent();
		for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
		{
			UMaterialInterface* Base = Mesh->GetMaterial(Slot);
			if (!Base || (Base->GetFName() != AccentName && Base->GetFName() != StoneName))
			{
				continue;
			}
			UMaterialInstanceDynamic*& Material = Palette.FindOrAdd(Base);
			if (!Material)
			{
				Material = UMaterialInstanceDynamic::Create(Base, this);
				if (Base->GetFName() == AccentName)
				{
					Material->SetVectorParameterValue(TEXT("Base Color"), FLinearColor(0.085f, 0.145f, 0.215f));
				}
				else
				{
					Material->SetVectorParameterValue(TEXT("SurfaceColor"), FLinearColor(0.24f, 0.20f, 0.16f));
					Material->SetVectorParameterValue(TEXT("TopSurfaceColor"), FLinearColor(0.28f, 0.24f, 0.20f));
					Material->SetVectorParameterValue(TEXT("GridColor"), FLinearColor(0.13f, 0.107f, 0.084f));
					Material->SetVectorParameterValue(TEXT("SubGridColor"), FLinearColor(0.20f, 0.165f, 0.13f));
					Material->SetVectorParameterValue(TEXT("TopGridColor"), FLinearColor(0.15f, 0.126f, 0.10f));
					Material->SetVectorParameterValue(TEXT("TopSubGridGridColor"), FLinearColor(0.23f, 0.195f, 0.16f));
				}
			}
			Mesh->SetMaterial(Slot, Material);
		}
	}
}

void ANINJA_ARENAGameMode::InitializeHUDForPlayer_Implementation(APlayerController* NewPlayer)
{
	if (IsTrainingArena())
	{
		NewPlayer->ClientSetHUD(ANinjaTrainingHUD::StaticClass());
		return;
	}
	Super::InitializeHUDForPlayer_Implementation(NewPlayer);
}

void ANINJA_ARENAGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bArenaTrial)
	{
		if (!bTrialFighterApplied)
		{
			if (ANINJA_ARENACharacter* Player = Cast<ANINJA_ARENACharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
			{
				ApplyMatchFighter(*Player, TEXT("Fighter"));
				bTrialFighterApplied = true;
			}
		}
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (!TrainingPlayer.IsValid() || !IsValid(Opponent))
	{
		if (Now < NextSpawnAttempt)
		{
			return;
		}
		NextSpawnAttempt = Now + 1.f;
		APlayerController* Controller = GetWorld()->GetFirstPlayerController();
		ANINJA_ARENACharacter* Player = Controller ? Cast<ANINJA_ARENACharacter>(Controller->GetPawn()) : nullptr;
		if (Player && Player->HasActorBegunPlay())
		{
			if (!bLocalVersus && IsValid(Opponent))
			{
				Opponent->Destroy();
			}
			TrainingPlayer = Player;
			ApplyMatchFighter(*Player, TEXT("Fighter"));
			Opponent = bLocalVersus ? GetLocalVersusFighter() : SpawnTrainingOpponent(*Player);
			if (IsValid(Opponent) && bLocalVersus)
			{
				ApplyMatchFighter(*Opponent, TEXT("Fighter2"));
			}
			if (Opponent && IsTeamBattle())
			{
				Player->SetCombatTeam(0);
				Opponent->SetCombatTeam(1);
			}
			if (Opponent && !bLocalVersus)
			{
				const FRotator Facing = (Opponent->GetActorLocation() - Player->GetActorLocation()).Rotation();
				Player->SetActorRotation(FRotator(0.0, Facing.Yaw, 0.0));
				Controller->SetControlRotation(FRotator(-8.0, Facing.Yaw, 0.0));
				Player->GetCameraBoom()->SocketOffset = FVector(0.0, 48.0, 30.0);
			}
			if (Opponent && bLocalVersus)
			{
				if (bSelectingFighters || (IsTeamBattle() && !SpawnTeamBots()))
				{
					OpenFighterSelection();
				}
				else
				{
					if (ElementProps.IsEmpty())
					{
						SpawnElementProps(*Player);
						SpawnArenaFeatures(*Player);
					}
					ResetRound(false);
					FrameLocalFighters();
				}
			}
			else if (Opponent && ElementProps.IsEmpty())
			{
				SpawnElementProps(*Player);
				SpawnArenaFeatures(*Player);
			}
			RoundResult = ENinjaRoundResult::Fighting;
			ResetBotStates(Now);
		}
		return;
	}

	if (RoundResult == ENinjaRoundResult::Fighting)
	{
		if (bTempleArena && !bSelectingFighters)
		{
			UpdateTempleLaunchPads();
			UpdateTempleBridge(Now);
		}
		if (!bSelectingFighters && IsTeamBattle())
		{
			UpdateTeamBattle(Now);
			if (bCaptureMatch && RoundResult == ENinjaRoundResult::Fighting)
			{
				UpdateCapture(DeltaSeconds);
			}
		}
		else if (!bLocalVersus)
		{
			UpdateBot(*Opponent, TrainingPlayer.Get(), TrainingBot, Now);
		}
	}
	else if (Now >= RestartRoundAt)
	{
		ResetRound();
	}
}

bool ANINJA_ARENAGameMode::FindRivalStart(const ANINJA_ARENACharacter& Player, FTransform& OutStart, float Distance,
	double HeadingOffset, bool bLeaveSpaceForCamera) const
{
	const FVector Origin = Player.GetActorLocation();
	const float HalfHeight = Player.GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FCollisionQueryParams Query(SCENE_QUERY_STAT(TrainingSpawnFloor), false, &Player);
	for (const float AngleOffset : { 0.f, 45.f, -45.f, 90.f, -90.f, 135.f, -135.f, 180.f })
	{
		const double Heading = Player.GetActorRotation().Yaw + HeadingOffset + AngleOffset;
		if (bLeaveSpaceForCamera && FMath::Abs(FMath::FindDeltaAngleDegrees(Player.GetActorRotation().Yaw + 180.0, Heading)) <= 38.0)
		{
			continue;
		}
		const float Radius = bCaptureMatch && Distance == 370.f && !bLeaveSpaceForCamera ? 3000.f : Distance;
		const FVector Candidate = Origin + FRotator(0.0, Heading, 0.0).Vector() * Radius;
		FHitResult Floor;
		if (!GetWorld()->LineTraceSingleByChannel(Floor, Candidate + FVector(0.f, 0.f, 80.f),
			Candidate - FVector(0.f, 0.f, HalfHeight + 140.f), ECC_Visibility, Query)
			|| Floor.ImpactNormal.Z < Player.GetCharacterMovement()->GetWalkableFloorZ())
		{
			continue;
		}

		const FVector Location = Floor.ImpactPoint + FVector(0.f, 0.f, HalfHeight + 3.f);
		if (GetWorld()->LineTraceTestByChannel(Origin, Location, ECC_Visibility, Query))
		{
			continue;
		}

		const UCapsuleComponent* Capsule = Player.GetCapsuleComponent();
		if (!GetWorld()->OverlapBlockingTestByProfile(Location, FQuat::Identity, Capsule->GetCollisionProfileName(),
			FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), HalfHeight), Query))
		{
			OutStart = FTransform(FRotator(0.f, (Origin - Location).Rotation().Yaw, 0.f), Location);
			return true;
		}
	}
	return false;
}

ANINJA_ARENACharacter* ANINJA_ARENAGameMode::SpawnTrainingOpponent(ANINJA_ARENACharacter& Player)
{
	FTransform Start;
	return FindRivalStart(Player, Start) ? SpawnBot(Player.GetClass(), Start, true) : nullptr;
}

ANINJA_ARENACharacter* ANINJA_ARENAGameMode::SpawnBot(TSubclassOf<ANINJA_ARENACharacter> FighterClass,
	const FTransform& Start, bool bAscua)
{
	ANINJA_ARENACharacter* Rival = GetWorld()->SpawnActorDeferred<ANINJA_ARENACharacter>(
		FighterClass, Start, this, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
	if (IsValid(Rival))
	{
		Rival->AutoPossessPlayer = EAutoReceiveInput::Disabled;
		Rival->AutoPossessAI = EAutoPossessAI::Disabled;
		Rival->GetCharacterMovement()->bRunPhysicsWithNoController = true;
		Rival->GetCombatStyle()->SetFighter(bAscua ? ENinjaFighter::Ascua : ENinjaFighter::Junco);
		Rival->FinishSpawning(Start);
	}
	return IsValid(Rival) ? Rival : nullptr;
}

void ANINJA_ARENAGameMode::SpawnElementProps(ANINJA_ARENACharacter& Player)
{
	const FVector Origin = Player.GetActorLocation();
	const FVector Lane = (Opponent->GetActorLocation() - Origin).GetSafeNormal2D();
	const float Heading = Lane.Rotation().Yaw;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ElementTrainingFloor), false, &Player);
	Query.AddIgnoredActor(Opponent.Get());
	for (const bool bCrystal : { false, true })
	{
		const float Side = bCrystal ? 1.f : -1.f;
		for (const float Offset : { 57.f, 78.f, 100.f, 130.f, -60.f, -95.f, -130.f, 170.f })
		{
			const FVector Candidate = Origin + FRotator(0.f, Heading + Offset * Side, 0.f).Vector() * 365.0;
			FHitResult Floor;
			if (!GetWorld()->LineTraceSingleByChannel(Floor, Candidate + FVector(0.0, 0.0, 80.0),
				Candidate - FVector(0.0, 0.0, 260.0), ECC_Visibility, Query)
				|| Floor.ImpactNormal.Z < Player.GetCharacterMovement()->GetWalkableFloorZ())
			{
				continue;
			}

			const FVector Spot = Floor.ImpactPoint + FVector(0.0, 0.0, 51.0);
			if (FMath::Abs(Spot.Z - Origin.Z) > 100.0
				|| GetWorld()->LineTraceTestByChannel(Origin, Spot, ECC_Visibility, Query)
				|| FVector::DistSquared2D(Spot, Opponent->GetActorLocation()) < FMath::Square(130.0)
				|| ElementProps.ContainsByPredicate([&Spot](const TObjectPtr<ANinjaElementProp>& Existing)
				{
					return IsValid(Existing) && FVector::DistSquared2D(Existing->GetActorLocation(), Spot) < FMath::Square(160.0);
				}))
			{
				continue;
			}
			if (ANinjaElementProp* Prop = GetWorld()->SpawnActor<ANinjaElementProp>(Spot, FRotator::ZeroRotator))
			{
				Prop->Configure(bCrystal);
				ElementProps.Add(Prop);
				break;
			}
		}
	}
}

FVector ANINJA_ARENAGameMode::FindBotMove(const ANINJA_ARENACharacter& Bot, const ANINJA_ARENACharacter& Target,
	const FVector& Forward) const
{
	const FVector Origin = Bot.GetActorLocation();
	const UWorld* World = GetWorld();
	FCollisionQueryParams Query(SCENE_QUERY_STAT(TrainingOpponent), false, &Bot);
	Query.AddIgnoredActor(&Target);
	for (const float Angle : {0.f, 37.f, -37.f, 70.f, -70.f, 105.f, -105.f})
	{
		const FVector Direction = Forward.RotateAngleAxis(Angle, FVector::UpVector);
		const FVector Ahead = Origin + Direction * 100.f;
		FHitResult Floor;
		if (World->LineTraceSingleByChannel(Floor, Ahead + FVector(0, 0, 35), Ahead - FVector(0, 0, 175), ECC_Visibility, Query)
			&& Floor.ImpactNormal.Z >= Bot.GetCharacterMovement()->GetWalkableFloorZ()
			&& !World->SweepTestByChannel(Origin, Ahead, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(31.f), Query))
		{
			return Direction;
		}
	}
	return FVector::ZeroVector;
}

void ANINJA_ARENAGameMode::UpdateBot(ANINJA_ARENACharacter& Bot, ANINJA_ARENACharacter* Target,
	FNinjaBotState& State, float Now)
{
	const auto CancelWindup = [&Bot, &State]()
	{
		if (State.bPreparing)
		{
			Bot.GetCombatStyle()->StopActionPose();
			State.bPreparing = false;
		}
	};
	if (State.Target.Get() != Target)
	{
		CancelWindup();
		State.Target = Target;
		State.NextTurn = 0.f;
	}
	if (!IsValid(Target) || Target->GetVitals()->IsDefeated() || Bot.IsCombatLocked()
		|| !Bot.GetCharacterMovement()->IsMovingOnGround())
	{
		CancelWindup();
		State.NextAttack = FMath::Max(State.NextAttack, Now + OpponentAttackWindup);
		State.NextTurn = 0.f;
		return;
	}

	const FVector Delta = Target->GetActorLocation() - Bot.GetActorLocation();
	const FVector Direction = Delta.GetSafeNormal2D();
	const bool bNear = Delta.SizeSquared2D() <= FMath::Square(195.f) && FMath::Abs(Delta.Z) <= 110.f;
	if (!bNear || !Bot.CanMeleeReach(*Target, Direction))
	{
		CancelWindup();
		if (Now >= State.NextTurn)
		{
			State.Move = FindBotMove(Bot, *Target, Direction);
			State.NextTurn = Now + 0.12f;
		}
		if (!State.Move.IsNearlyZero())
		{
			Bot.AddMovementInput(State.Move, bNear ? 0.42f : 0.7f, true);
		}
		return;
	}

	State.NextTurn = 0.f;
	Bot.SetActorRotation(Direction.Rotation());
	Bot.GetCharacterMovement()->StopMovementImmediately();
	Bot.ConsumeMovementInputVector();
	if (Now < State.NextAttack)
	{
		return;
	}
	if (!State.bPreparing)
	{
		State.bPreparing = true;
		State.NextAttack = Now + OpponentAttackWindup;
		Bot.GetCombatStyle()->PlayWindup((State.AttackCount + 1) % 3 == 0);
		return;
	}

	State.bPreparing = false;
	State.NextAttack = Now + OpponentAttackInterval;
	if (++State.AttackCount % 3 == 0)
	{
		Bot.HeavyAttack();
	}
	else
	{
		Bot.LightAttack();
	}
}

void ANINJA_ARENAGameMode::ResetBotStates(float Now)
{
	TrainingBot = {};
	TrainingBot.NextAttack = Now + OpponentAttackInterval;
	for (int32 Index = 0; Index < TeamBotStates.Num(); ++Index)
	{
		FNinjaBotState& State = TeamBotStates[Index];
		State = {};
		State.NextAttack = TrainingBot.NextAttack + (TeamSize == 4 ? 0.18f * (Index % (TeamSize - 1)) : 0.f);
	}
}

void ANINJA_ARENAGameMode::FinishRound(ENinjaRoundResult Result)
{
	RoundResult = Result;
	RestartRoundAt = GetWorld()->GetTimeSeconds() + RoundPause;
	ResetBotStates(GetWorld()->GetTimeSeconds());
	for (ANINJA_ARENACharacter* Fighter : GetRoundFighters())
	{
		Fighter->SetRoundLocked(true);
	}
}

bool ANINJA_ARENAGameMode::NotifyFighterDown(ANINJA_ARENACharacter* Fighter)
{
	if (!TrainingPlayer.IsValid() || !IsValid(Opponent) || !IsValid(Fighter))
	{
		return false;
	}
	if (IsTeamBattle() && Fighter->GetCombatTeam() != INDEX_NONE)
	{
		Fighter->GetVitals()->Health = 0.f;
		Fighter->SetRoundLocked(true);
		Fighter->SetActorEnableCollision(false);
		Fighter->SetActorHiddenInGame(true);
		UpdateTeamSpectator(Fighter->GetCombatTeam());
		return true;
	}
	if (Fighter != TrainingPlayer.Get() && Fighter != Opponent.Get())
	{
		return false;
	}
	if (RoundResult == ENinjaRoundResult::Fighting)
	{
		FinishRound(Fighter == Opponent.Get() ? ENinjaRoundResult::Victory : ENinjaRoundResult::Defeat);
	}
	return true;
}

void ANINJA_ARENAGameMode::RequestRoundReset(const ANINJA_ARENACharacter* Requester)
{
	if (!bSelectingFighters && TrainingPlayer.IsValid() && IsValid(Opponent)
		&& (Requester == TrainingPlayer.Get() || (bLocalVersus && Requester == Opponent.Get())))
	{
		ResetRound();
	}
}

void ANINJA_ARENAGameMode::ResetRound(bool bAdvanceRound)
{
	CaptureScore[0] = CaptureScore[1] = 0.f;
	for (int32& PointTeam : CaptureOwner) { PointTeam = INDEX_NONE; }
	ResetArenaFeatures();
	const auto Fighters = GetRoundFighters();
	TArray<bool, TInlineAllocator<8>> RestoreCollision;
	for (ANINJA_ARENACharacter* Fighter : Fighters)
	{
		RestoreCollision.Add(IsTeamBattle() || Fighter->GetActorEnableCollision());
		Fighter->SetActorEnableCollision(false);
	}
	for (ANINJA_ARENACharacter* Fighter : Fighters)
	{
		Fighter->RespawnAtStart();
	}
	for (int32 Index = 0; Index < Fighters.Num(); ++Index)
	{
		ANINJA_ARENACharacter* Fighter = Fighters[Index];
		APlayerController* Controller = Cast<APlayerController>(Fighter->GetController());
		const bool bHideBot = bSelectingFighters && !Controller;
		Fighter->SetActorHiddenInGame(bHideBot);
		Fighter->SetActorEnableCollision(!bHideBot && RestoreCollision[Index]);
		Fighter->SetRoundLocked(bSelectingFighters);
		if (Controller)
		{
			Controller->SetViewTarget(Fighter);
		}
	}
	for (ANinjaElementProp* Prop : ElementProps)
	{
		if (IsValid(Prop))
		{
			Prop->Restore();
		}
	}

	if (bAdvanceRound)
	{
		++RoundNumber;
	}
	RoundResult = ENinjaRoundResult::Fighting;
	ResetBotStates(GetWorld()->GetTimeSeconds());
}

float ANINJA_ARENAGameMode::GetTempleBridgeTime() const
{
	return TempleBridge.IsValid() && TempleBridgeChangeAt > 0.f
		? FMath::Max(0.f, TempleBridgeChangeAt - GetWorld()->GetTimeSeconds())
		: 0.f;
}

void ANINJA_ARENAGameMode::ResetTempleBridge()
{
	TempleBridgeChangeAt = 0.f;
	bTempleBridgeDown = false;
	if (AActor* Bridge = TempleBridge.Get())
	{
		Bridge->SetActorHiddenInGame(false);
		Bridge->SetActorEnableCollision(true);
	}
}

void ANINJA_ARENAGameMode::UpdateTempleBridge(float Now)
{
	AActor* Bridge = TempleBridge.Get();
	if (!Bridge) { return; }
	if (TempleBridgeChangeAt > 0.f)
	{
		if (Now < TempleBridgeChangeAt) { return; }
		if (bTempleBridgeDown)
		{
			// Wait for clear space before restoring a solid surface.
			FCollisionQueryParams Query(SCENE_QUERY_STAT(TempleBridgeRestore), false, Bridge);
			const FCollisionObjectQueryParams Pawns(ECC_Pawn);
			if (GetWorld()->OverlapAnyTestByObjectType(Bridge->GetActorLocation(), FQuat::Identity, Pawns,
				FCollisionShape::MakeBox(FVector(320, 210, 32)), Query))
			{
				TempleBridgeChangeAt = Now + 0.20f;
				return;
			}
			ResetTempleBridge();
		}
		else
		{
			bTempleBridgeDown = true;
			Bridge->SetActorEnableCollision(false);
			Bridge->SetActorHiddenInGame(true);
			TempleBridgeChangeAt = Now + 5.f;
			if (FApp::CanEverRender())
			{
				static const TSoftObjectPtr<UNiagaraSystem> CollapseFx(FSoftObjectPath(TEXT(
					"/Game/FreeStylizedLaserBeamVFX/VFX/Hit/NS_Hit_02.NS_Hit_02")));
				if (UNiagaraSystem* System = CollapseFx.LoadSynchronous())
				{
					UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), System, Bridge->GetActorLocation(),
						FRotator::ZeroRotator, FVector(1.6));
				}
			}
		}
		return;
	}

	const FVector Center = Bridge->GetActorLocation();
	for (const ANINJA_ARENACharacter* Fighter : GetRoundFighters())
	{
		const FVector Offset = Fighter->GetActorLocation() - Center;
		const double Feet = Offset.Z - Fighter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		if (!Fighter->GetVitals()->IsDefeated() && Fighter->GetCharacterMovement()->IsMovingOnGround()
			&& FMath::Abs(Offset.X) < 320 && FMath::Abs(Offset.Y) < 210 && Feet >= 20 && Feet < 42)
		{
			TempleBridgeChangeAt = Now + 1.4f;
			break;
		}
	}
}

void ANINJA_ARENAGameMode::UpdateTempleLaunchPads()
{
	for (ANINJA_ARENACharacter* Fighter : GetRoundFighters())
	{
		if (Fighter->IsCombatLocked() || Fighter->IsGuarding() || !Fighter->GetCharacterMovement()->IsMovingOnGround())
		{
			continue;
		}
		const FVector Location = Fighter->GetActorLocation();
		const double Side = Location.Y < 0.0 ? -1.0 : 1.0;
		const FVector Center(0.0, Side * 725.0, 0.0);
		if (Location.Z < 180.0 && FVector::DistSquared2D(Location, Center) <= FMath::Square(140.0))
		{
			Fighter->LaunchCharacter(FVector(0, Side * 390.0, 880.0), true, true);
		}
	}
}

void ANINJA_ARENAGameMode::UpdateCapture(float DeltaSeconds)
{
	const auto Fighters = GetRoundFighters();
	for (int32 Point = 0; Point < 3; ++Point)
	{
		bool Present[2] = {};
		const FVector Center = GetCaptureCenter(Point);
		for (const ANINJA_ARENACharacter* Fighter : Fighters)
		{
			const int32 Team = Fighter->GetCombatTeam();
			if ((Team == 0 || Team == 1) && !Fighter->GetVitals()->IsDefeated()
				&& FMath::Abs(Fighter->GetActorLocation().Z) < 260.0
				&& FVector::DistSquared2D(Fighter->GetActorLocation(), Center) <= FMath::Square(320.0))
			{
				Present[Team] = true;
			}
		}
		CaptureOwner[Point] = Present[0] == Present[1] ? (Present[0] ? 2 : INDEX_NONE) : (Present[1] ? 1 : 0);
		if (CaptureOwner[Point] == 0 || CaptureOwner[Point] == 1)
		{
			CaptureScore[CaptureOwner[Point]] += 4.f * DeltaSeconds;
		}
	}
	if (CaptureScore[0] >= 100.f || CaptureScore[1] >= 100.f)
	{
		FinishRound(CaptureScore[0] == CaptureScore[1] ? ENinjaRoundResult::Draw
			: (CaptureScore[0] > CaptureScore[1] ? ENinjaRoundResult::Victory : ENinjaRoundResult::Defeat));
	}
}
