// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NINJA_ARENAGameMode.generated.h"

class ANINJA_ARENACharacter;
class ANinjaArenaDangerZone;
class ANinjaArenaDebris;
class ANinjaElementProp;
struct FInputKeyEventArgs;
struct FCollisionQueryParams;

enum class ENinjaRoundResult : uint8
{
	Fighting,
	Victory,
	Defeat,
	Draw
};

struct FNinjaBotState
{
	TWeakObjectPtr<ANINJA_ARENACharacter> Target;
	FVector Move = FVector::ZeroVector;
	float NextAttack = 0.f;
	float NextTurn = 0.f;
	float NextTargetPick = 0.f;
	int32 AttackCount = 0;
	bool bPreparing = false;
};

UCLASS(abstract)
class ANINJA_ARENAGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ANINJA_ARENAGameMode();

	bool NotifyFighterDown(ANINJA_ARENACharacter* Fighter);
	void RequestRoundReset(const ANINJA_ARENACharacter* Requester);
	ANINJA_ARENACharacter* GetTrainingOpponent() const { return Opponent.Get(); }
	ANINJA_ARENACharacter* GetDuelOpponent(const ANINJA_ARENACharacter* Fighter) const;
	ANINJA_ARENACharacter* GetTeamCompanion(const ANINJA_ARENACharacter* Fighter) const;
	TConstArrayView<TObjectPtr<ANINJA_ARENACharacter>> GetTeamBots(int32 Team) const;
	void IgnoreAlliesForTrace(const ANINJA_ARENACharacter& Fighter, FCollisionQueryParams& Query) const;
	int32 GetTeamSize() const { return TeamSize; }
	bool IsTeamBattle() const { return TeamSize > 1; }
	bool IsModeMenu() const { return bModeMenu; }
	int32 GetMenuStage() const { return MenuStage; }
	int32 GetMenuMode() const { return MenuMode; }
	int32 GetMenuArena() const { return MenuMode >= 4 ? MenuMode - 2 : MenuArena; }
	bool IsSoloMenuMode() const { return MenuMode == 0 || MenuMode == 5; }
	bool IsArenaTrial() const { return bArenaTrial; }
	int32 GetMenuFighter(bool bSecond) const { return MenuFighters[bSecond ? 1 : 0]; }
	bool IsCaptureMatch() const { return bCaptureMatch; }
	float GetCaptureScore(int32 Team) const { return CaptureScore[Team]; }
	int32 GetCaptureOwner(int32 Point) const { return CaptureOwner[Point]; }
	float GetTempleBridgeTime() const;
	bool IsTempleBridgeDown() const { return bTempleBridgeDown; }
	static FVector GetCaptureCenter(int32 Point) { return FVector(0.0, (Point - 1) * 1250.0, 0.0); }
	bool IsLocalVersus() const { return bLocalVersus; }
	bool IsSecondLocalPlayer(const AController* Controller) const;
	bool IsSelectingFighters() const { return bSelectingFighters; }
	bool IsFighterConfirmed(const AController* Controller) const;
	ENinjaRoundResult GetRoundResult() const { return RoundResult; }
	int32 GetRoundNumber() const { return RoundNumber; }
	bool IsOpponentPreparingAttack() const { return TrainingBot.bPreparing; }
	const TArray<TObjectPtr<ANinjaElementProp>>& GetElementProps() const { return ElementProps; }
	const TArray<TObjectPtr<ANinjaElementProp>>& GetArenaStructures() const { return ArenaStructures; }
	const TArray<TWeakObjectPtr<ANinjaArenaDangerZone>>& GetCollapseZones() const { return CollapseZones; }
	ANinjaArenaDangerZone* GetArenaHazard() const { return ArenaHazard.Get(); }
	int32 GetActiveFragmentCount() const;
	void HandlePropDestroyed(const ANinjaElementProp& Prop);

protected:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void InitializeHUDForPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& Start) override;

	UPROPERTY(EditDefaultsOnly, Category="PNA|Training", meta=(ClampMin="0.1"))
	float RoundPause = 2.f;

	UPROPERTY(EditDefaultsOnly, Category="PNA|Training", meta=(ClampMin="0.1"))
	float OpponentAttackWindup = 0.55f;

	UPROPERTY(EditDefaultsOnly, Category="PNA|Training", meta=(ClampMin="0.35"))
	float OpponentAttackInterval = 1.4f;

	UPROPERTY(EditDefaultsOnly, Category="PNA|Arena", meta=(ClampMin="0", ClampMax="12"))
	int32 MaxFragmentsPerStructure = 12;

	UPROPERTY(EditDefaultsOnly, Category="PNA|Arena", meta=(ClampMin="0", ClampMax="42"))
	int32 FragmentBudget = 42;

private:
	bool IsTrainingArena() const;
	bool bTempleArena = false;
	bool bArenaTrial = false;
	bool bTrialFighterApplied = false;
	void UpdateTempleLaunchPads();
	void UpdateTempleBridge(float Now);
	void ResetTempleBridge();
	TWeakObjectPtr<AActor> TempleBridge;
	float TempleBridgeChangeAt = 0.f;
	bool bTempleBridgeDown = false;
	bool FindRivalStart(const ANINJA_ARENACharacter& Player, FTransform& OutStart, float Distance = 370.f,
		double HeadingOffset = 0.0, bool bLeaveSpaceForCamera = false) const;
	ANINJA_ARENACharacter* SpawnBot(TSubclassOf<ANINJA_ARENACharacter> FighterClass, const FTransform& Start, bool bAscua);
	ANINJA_ARENACharacter* SpawnTrainingOpponent(ANINJA_ARENACharacter& Player);
	ANINJA_ARENACharacter* GetLocalVersusFighter();
	void EnableLocalVersusInput();
	int32 MenuStage = 0;
	int32 MenuMode = 0;
	int32 MenuArena = 0;
	int32 MenuFighters[2] = {0, 1};
	void ApplyMatchFighter(ANINJA_ARENACharacter& Fighter, const TCHAR* Option) const;
	void EnableModeMenu();
	bool RouteModeMenuKey(FInputKeyEventArgs& Event);
	void UpdateCapture(float DeltaSeconds);
	bool RouteLocalVersusKey(FInputKeyEventArgs& Event);
	void OpenFighterSelection();
	void HandleFighterSelectionKey(const FInputKeyEventArgs& Event);
	void FrameLocalFighters();
	void SpawnElementProps(ANINJA_ARENACharacter& Player);
	void ApplyArenaPalette();
	void SpawnArenaFeatures(const ANINJA_ARENACharacter& Player);
	bool FindFeatureGround(const ANINJA_ARENACharacter& Player, double Heading, double Padding,
		double HalfHeight, FVector& OutGround) const;
	void ResetArenaFeatures();
	bool SpawnTeamBots();
	void UpdateTeamBattle(float Now);
	void UpdateTeamSpectator(int32 Team);
	ANINJA_ARENACharacter* GetTeamCaptain(int32 Team) const;
	TArray<ANINJA_ARENACharacter*, TInlineAllocator<8>> GetRoundFighters() const;
	void UpdateBot(ANINJA_ARENACharacter& Bot, ANINJA_ARENACharacter* Target, FNinjaBotState& State, float Now);
	FVector FindBotMove(const ANINJA_ARENACharacter& Bot, const ANINJA_ARENACharacter& Target, const FVector& Forward) const;
	void FinishRound(ENinjaRoundResult Result);
	void ResetBotStates(float Now);
	void ResetRound(bool bAdvanceRound = true);

	TWeakObjectPtr<ANINJA_ARENACharacter> TrainingPlayer;

	UPROPERTY(Transient)
	TObjectPtr<ANINJA_ARENACharacter> Opponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ANINJA_ARENACharacter>> TeamBots;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ANinjaElementProp>> ElementProps;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ANinjaElementProp>> ArenaStructures;

	UPROPERTY(Transient)
	TObjectPtr<ANinjaArenaDangerZone> ArenaHazard;

	TArray<TWeakObjectPtr<ANinjaArenaDebris>> LiveDebris;
	TArray<TWeakObjectPtr<ANinjaArenaDangerZone>> CollapseZones;

	ENinjaRoundResult RoundResult = ENinjaRoundResult::Fighting;
	int32 RoundNumber = 1;
	float NextSpawnAttempt = 0.f;
	float RestartRoundAt = 0.f;
	FNinjaBotState TrainingBot;
	TArray<FNinjaBotState> TeamBotStates;
	int32 TeamSize = 1;
	bool bLocalVersus = false;
	bool bModeMenu = false;
	bool bCaptureMatch = false;
	float CaptureScore[2] = {};
	int32 CaptureOwner[3] = {INDEX_NONE, INDEX_NONE, INDEX_NONE};
	bool bSelectingFighters = false;
	bool bFighterConfirmed[2] = {false, false};
	bool bPreviousGamepadRouting = false;
	bool bConfiguredGamepadRouting = false;
};



