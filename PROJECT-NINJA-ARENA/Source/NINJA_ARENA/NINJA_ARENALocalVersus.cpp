#include "NINJA_ARENAGameMode.h"

#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameMapsSettings.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "NINJA_ARENACharacter.h"
#include "NinjaCombatStyle.h"

void ANINJA_ARENAGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	const bool bArena = IsTrainingArena();
	bModeMenu = bArena && UGameplayStatics::HasOption(Options, TEXT("ModeMenu"));
	const FString ArenaName = UWorld::RemovePIEPrefix(FPackageName::GetShortName(MapName));
	bCaptureMatch = ArenaName == TEXT("Lvl_Campo") && !UGameplayStatics::HasOption(Options, TEXT("FreeBattle"));
	bTempleArena = ArenaName == TEXT("Lvl_Templo");
	bArenaTrial = bArena && ArenaName == TEXT("L_PNA_Coliseo_Dorado_V4");
	if (bArenaTrial)
	{
		bModeMenu = false;
		return;
	}
	if (bArena && (bCaptureMatch || UGameplayStatics::HasOption(Options, TEXT("TeamBattle"))))
	{
		TeamSize = (bCaptureMatch || UGameplayStatics::ParseOption(Options, TEXT("TeamBattle")) == TEXT("4")) ? 4 : 2;
		TeamBots.SetNum(2 * (TeamSize - 1));
		TeamBotStates.SetNum(TeamBots.Num());
	}
	bLocalVersus = bArena && (IsTeamBattle() || UGameplayStatics::HasOption(Options, TEXT("VersusLocal")));
	bSelectingFighters = bLocalVersus && !UGameplayStatics::HasOption(Options, TEXT("Fighter2"));
}

void ANINJA_ARENAGameMode::EnableModeMenu()
{
	if (UGameViewportClient* View = GetGameInstance()->GetGameViewportClient())
	{
		View->OnOverrideInputKey().BindUObject(this, bModeMenu ? &ANINJA_ARENAGameMode::RouteModeMenuKey : &ANINJA_ARENAGameMode::RouteLocalVersusKey);
	}
}

bool ANINJA_ARENAGameMode::RouteModeMenuKey(FInputKeyEventArgs& Event)
{
		if (Event.Event != IE_Pressed) { return true; }
	const FKey& Key = Event.Key;
	if (Key == EKeys::Escape || Key == EKeys::BackSpace || Key == EKeys::Gamepad_FaceButton_Right)
	{
		MenuStage = FMath::Max(0, MenuStage - 1);
		return true;
	}
	const bool bLeft = Key == EKeys::Left || Key == EKeys::A || Key == EKeys::Gamepad_DPad_Left;
	const bool bRight = Key == EKeys::Right || Key == EKeys::D || Key == EKeys::Gamepad_DPad_Right;
	if (bLeft || bRight)
	{
		int32* Choices[] = {&MenuMode, &MenuArena, &MenuFighters[0], &MenuFighters[1]};
		const int32 Counts[] = {6, MenuMode >= 4 ? 1 : 3, static_cast<int32>(ENinjaFighter::Count), static_cast<int32>(ENinjaFighter::Count)};
		const int32 Count = Counts[MenuStage];
		int32& Current = *Choices[MenuStage];
		Current = (Current + Count + (bRight ? 1 : -1)) % Count;
		return true;
	}
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		if (MenuStage < (IsSoloMenuMode() ? 2 : 3))
		{
			++MenuStage;
			return true;
		}

		const TCHAR* Maps[] = {TEXT("/Game/Arena/Lvl_ThirdPerson"), TEXT("/Game/Arena/Lvl_Templo"), TEXT("/Game/Arena/Lvl_Campo"), TEXT("/Game/NinjaArena/Maps/L_PNA_Coliseo_Dorado_V4")};
		const TCHAR* Modes[] = {TEXT(""), TEXT("VersusLocal=1?"), TEXT("TeamBattle=2?"), TEXT("TeamBattle=4?"), TEXT(""), TEXT("")};
		const auto FighterId = [this](int32 PlayerIndex)
		{
			return UNinjaCombatStyleComponent::GetProfile(static_cast<ENinjaFighter>(MenuFighters[PlayerIndex])).TrainingId;
		};
		FString Options = FString::Printf(TEXT("%sFighter=%s"), Modes[MenuMode], FighterId(0));
		if (!IsSoloMenuMode())
		{
			Options += FString::Printf(TEXT("?Fighter2=%s"), FighterId(1));
		}
		if (MenuMode != 4 && GetMenuArena() == 2)
		{
			Options += TEXT("?FreeBattle=1");
		}
		UGameplayStatics::OpenLevel(this, FName(Maps[GetMenuArena()]), true, Options);
		return true;
	}
	const FKey Choices[] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven};
	const TCHAR* Options[] = {TEXT(""), TEXT("VersusLocal=1"), TEXT("TeamBattle=1"), TEXT("TeamBattle=4"), TEXT(""), TEXT(""), TEXT("VersusLocal=1")};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Choices); ++Index)
	{
		if (Event.Key == Choices[Index])
		{
			const TCHAR* MapPath = Index >= 5 ? TEXT("/Game/Arena/Lvl_Templo")
				: (Index == 4 ? TEXT("/Game/Arena/Lvl_Campo") : TEXT("/Game/Arena/Lvl_ThirdPerson"));
			const FName Map(MapPath);
			UGameplayStatics::OpenLevel(this, Map, true, Options[Index]);
			break;
		}
	}
	return true;
}

void ANINJA_ARENAGameMode::ApplyMatchFighter(ANINJA_ARENACharacter& Fighter, const TCHAR* Option) const
{
	const FString Requested = UGameplayStatics::ParseOption(OptionsString, Option);
	if (Requested.IsEmpty()) { return; }
	for (int32 Index = 0; Index < static_cast<int32>(ENinjaFighter::Count); ++Index)
	{
		const ENinjaFighter Id = static_cast<ENinjaFighter>(Index);
		if (Requested.Equals(UNinjaCombatStyleComponent::GetProfile(Id).TrainingId, ESearchCase::IgnoreCase))
		{
			Fighter.GetCombatStyle()->SetFighter(Id);
			return;
		}
	}
}

ANINJA_ARENACharacter* ANINJA_ARENAGameMode::GetDuelOpponent(const ANINJA_ARENACharacter* Fighter) const
{
	if (Fighter == TrainingPlayer.Get())
	{
		return Opponent.Get();
	}
	return Fighter == Opponent.Get() ? TrainingPlayer.Get() : nullptr;
}

bool ANINJA_ARENAGameMode::IsSecondLocalPlayer(const AController* Controller) const
{
	const APlayerController* PlayerController = Cast<APlayerController>(Controller);
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	return bLocalVersus && LocalPlayer && GetGameInstance()->GetLocalPlayerByIndex(1) == LocalPlayer;
}

ANINJA_ARENACharacter* ANINJA_ARENAGameMode::GetLocalVersusFighter()
{
	APlayerController* Second = UGameplayStatics::GetPlayerController(this, 1);
	if (!Second && GetGameInstance()->GetNumLocalPlayers() < 2)
	{
		Second = UGameplayStatics::CreatePlayer(this, 1);
	}
	if (Second && !Second->GetPawn())
	{
		HandleStartingNewPlayer(Second);
	}
	return Second ? Cast<ANINJA_ARENACharacter>(Second->GetPawn()) : nullptr;
}

void ANINJA_ARENAGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	if (IsSecondLocalPlayer(NewPlayer) && TrainingPlayer.IsValid() && !NewPlayer->GetPawn())
	{
		FTransform Start;
		if (FindRivalStart(*TrainingPlayer.Get(), Start))
		{
			RestartPlayerAtTransform(NewPlayer, Start);
			if (NewPlayer->GetPawn())
			{
				return;
			}
		}
	}
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

APawn* ANINJA_ARENAGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& Start)
{
	UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer);
	if (!IsSecondLocalPlayer(NewPlayer) || !PawnClass || !PawnClass->IsChildOf(ANINJA_ARENACharacter::StaticClass()))
	{
		return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, Start);
	}
	FActorSpawnParameters Spawn;
	Spawn.Instigator = GetInstigator();
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.bDeferConstruction = true;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	ANINJA_ARENACharacter* Fighter = GetWorld()->SpawnActor<ANINJA_ARENACharacter>(PawnClass, Start, Spawn);
	if (Fighter)
	{
		Fighter->AutoPossessAI = EAutoPossessAI::Disabled;
		Fighter->GetCombatStyle()->SetFighter(ENinjaFighter::Ascua);
		Fighter->FinishSpawning(Start);
	}
	return IsValid(Fighter) ? Fighter : nullptr;
}

void ANINJA_ARENAGameMode::EnableLocalVersusInput()
{
	UGameMapsSettings* Maps = GetMutableDefault<UGameMapsSettings>();
	bPreviousGamepadRouting = Maps->GetSkipAssigningGamepadToPlayer1();
	Maps->SetSkipAssigningGamepadToPlayer1(true);
	bConfiguredGamepadRouting = true;
	if (UGameViewportClient* Viewport = GetGameInstance()->GetGameViewportClient())
	{
		Viewport->SetForceDisableSplitscreen(false);
		Viewport->OnOverrideInputKey().BindUObject(this, &ANINJA_ARENAGameMode::RouteLocalVersusKey);
	}
}

void ANINJA_ARENAGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bConfiguredGamepadRouting)
	{
		GetMutableDefault<UGameMapsSettings>()->SetSkipAssigningGamepadToPlayer1(bPreviousGamepadRouting);
	}
	if (UGameInstance* Instance = GetGameInstance())
	{
		UGameViewportClient* Viewport = Instance->GetGameViewportClient();
		if (Viewport && Viewport->OnOverrideInputKey().IsBoundToObject(this))
		{
			Viewport->OnOverrideInputKey().Unbind();
		}
	}
	Super::EndPlay(EndPlayReason);
}

bool ANINJA_ARENAGameMode::RouteLocalVersusKey(FInputKeyEventArgs& Event)
{
	if (Event.Key == EKeys::F10)
	{
		if (Event.Event == IE_Pressed)
		{
			if (APlayerController* Second = UGameplayStatics::GetPlayerController(this, 1))
			{
				UGameplayStatics::RemovePlayer(Second, true);
			}
			UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Arena/Lvl_ThirdPerson")), true, TEXT("ModeMenu=1"));
		}
		return true;
	}
	if (!bLocalVersus)
	{
		if (Event.Key != EKeys::F3) { return false; }
		if (Event.Event == IE_Pressed && TrainingPlayer.IsValid() && IsValid(Opponent))
		{
			UNinjaCombatStyleComponent* Style = TrainingPlayer->GetCombatStyle();
			const int32 Count = static_cast<int32>(ENinjaFighter::Count);
			const int32 Choice = (static_cast<int32>(Style->GetFighter()) + 1) % Count;
			Style->SetFighter(static_cast<ENinjaFighter>(Choice));
			RequestRoundReset(TrainingPlayer.Get());
		}
		return true;
	}
	if (Event.Key == EKeys::F2)
	{
		if (Event.Event == IE_Pressed && !bSelectingFighters && TrainingPlayer.IsValid() && IsValid(Opponent))
		{
			OpenFighterSelection();
		}
		return true;
	}
	if (bSelectingFighters)
	{
		HandleFighterSelectionKey(Event);
		return true;
	}

	// Both players use the same character bindings, with separate input routed by the viewport.
	static const TMap<FKey, FKey> SecondPlayerKeys = {
		{EKeys::Up, EKeys::W}, {EKeys::Down, EKeys::S}, {EKeys::Left, EKeys::A}, {EKeys::Right, EKeys::D},
		{EKeys::I, EKeys::I}, {EKeys::J, EKeys::J}, {EKeys::K, EKeys::K}, {EKeys::L, EKeys::L},
		{EKeys::U, EKeys::LeftMouseButton}, {EKeys::O, EKeys::RightMouseButton}, {EKeys::P, EKeys::Q},
		{EKeys::RightControl, EKeys::SpaceBar}, {EKeys::RightShift, EKeys::F},
		{EKeys::Y, EKeys::C}, {EKeys::H, EKeys::G}, {EKeys::B, EKeys::V}, {EKeys::M, EKeys::Tab}, {EKeys::N, EKeys::E}, {EKeys::BackSpace, EKeys::R},
		{EKeys::Six, EKeys::One}, {EKeys::Seven, EKeys::Two}, {EKeys::Eight, EKeys::Three},
		{EKeys::Nine, EKeys::Four}, {EKeys::Zero, EKeys::Five},
		{EKeys::Gamepad_FaceButton_Right, EKeys::LeftMouseButton}, {EKeys::Gamepad_FaceButton_Top, EKeys::RightMouseButton},
		{EKeys::Gamepad_FaceButton_Left, EKeys::G}, {EKeys::Gamepad_LeftShoulder, EKeys::Q}, {EKeys::Gamepad_RightShoulder, EKeys::F},
		{EKeys::Gamepad_LeftThumbstick, EKeys::C}, {EKeys::Gamepad_RightThumbstick, EKeys::Five},
		{EKeys::Gamepad_DPad_Up, EKeys::One}, {EKeys::Gamepad_DPad_Right, EKeys::Two},
		{EKeys::Gamepad_DPad_Down, EKeys::Three}, {EKeys::Gamepad_DPad_Left, EKeys::Four},
		{EKeys::Gamepad_Special_Left, EKeys::E}, {EKeys::Gamepad_Special_Right, EKeys::R}
	};
	const FKey* Mapped = SecondPlayerKeys.Find(Event.Key);
	APlayerController* Controller = Mapped ? UGameplayStatics::GetPlayerController(this, 1) : nullptr;
	if (!Controller)
	{
		return false;
	}
	FInputKeyEventArgs Routed = Event;
	Routed.Key = *Mapped;
	Controller->InputKey(Routed);
	return true;
}

bool ANINJA_ARENAGameMode::IsFighterConfirmed(const AController* Controller) const
{
	return bFighterConfirmed[IsSecondLocalPlayer(Controller) ? 1 : 0];
}

void ANINJA_ARENAGameMode::OpenFighterSelection()
{
	bSelectingFighters = true;
	bFighterConfirmed[0] = bFighterConfirmed[1] = false;
	RoundNumber = 1;
	ResetRound(false);
	FrameLocalFighters();
}

void ANINJA_ARENAGameMode::FrameLocalFighters()
{
	const double Heading = (Opponent->GetActorLocation() - TrainingPlayer->GetActorLocation()).Rotation().Yaw;
	ANINJA_ARENACharacter* Fighters[] = {TrainingPlayer.Get(), Opponent.Get()};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Fighters); ++Index)
	{
		ANINJA_ARENACharacter* Fighter = Fighters[Index];
		const double Facing = Heading + (Index == 0 ? 0.0 : 180.0);
		Fighter->SetActorRotation(FRotator(0.0, Facing, 0.0));
		Fighter->GetCameraBoom()->SocketOffset = bSelectingFighters ? FVector(0, 20, 38) : FVector(0, 48, 30);
		if (APlayerController* Controller = Cast<APlayerController>(Fighter->GetController()))
		{
			Controller->FlushPressedKeys();
			Controller->SetControlRotation(FRotator(-8.0, Facing + (bSelectingFighters ? 145.0 : 0.0), 0.0));
		}
	}
}

void ANINJA_ARENAGameMode::HandleFighterSelectionKey(const FInputKeyEventArgs& Event)
{
	if (Event.Event != IE_Pressed || !TrainingPlayer.IsValid() || !IsValid(Opponent))
	{
		return;
	}
	const FKey& Key = Event.Key;
	const bool bSecond = Key.IsGamepadKey() || Key == EKeys::Left || Key == EKeys::Right || Key == EKeys::Enter;
	const bool bPrevious = Key == EKeys::A || Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left;
	const bool bNext = Key == EKeys::D || Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right;
	const bool bConfirm = Key == EKeys::SpaceBar || Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom;
	bool& bConfirmed = bFighterConfirmed[bSecond ? 1 : 0];
	if (bPrevious || bNext)
	{
		ANINJA_ARENACharacter* Fighter = bSecond ? Opponent.Get() : TrainingPlayer.Get();
		UNinjaCombatStyleComponent* Style = Fighter->GetCombatStyle();
		const int32 Count = static_cast<int32>(ENinjaFighter::Count);
		const int32 Choice = (static_cast<int32>(Style->GetFighter()) + Count + (bNext ? 1 : -1)) % Count;
		Style->SetFighter(static_cast<ENinjaFighter>(Choice));
		bConfirmed = false;
	}
	else if (bConfirm)
	{
		bConfirmed = !bConfirmed;
		if (bFighterConfirmed[0] && bFighterConfirmed[1])
		{
			if (IsTeamBattle() && !SpawnTeamBots())
			{
				bConfirmed = false;
				return;
			}
			bSelectingFighters = false;
			if (ElementProps.IsEmpty())
			{
				SpawnElementProps(*TrainingPlayer.Get());
				SpawnArenaFeatures(*TrainingPlayer.Get());
			}
			ResetRound(false);
			FrameLocalFighters();
		}
	}
}
