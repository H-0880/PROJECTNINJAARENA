#include "NINJA_ARENAGameMode.h"

#include "CollisionQueryParams.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "NINJA_ARENACharacter.h"
#include "NinjaCombatStyle.h"
#include "PNAVitalsComponent.h"

namespace
{
	bool IsLivingFighter(const ANINJA_ARENACharacter* Fighter)
	{
		return IsValid(Fighter) && !Fighter->GetVitals()->IsDefeated();
	}
}

ANINJA_ARENACharacter* ANINJA_ARENAGameMode::GetTeamCaptain(int32 Team) const
{
	return Team == 0 ? TrainingPlayer.Get() : (Team == 1 ? Opponent.Get() : nullptr);
}

TConstArrayView<TObjectPtr<ANINJA_ARENACharacter>> ANINJA_ARENAGameMode::GetTeamBots(int32 Team) const
{
	if (!IsTeamBattle() || (Team != 0 && Team != 1))
	{
		return {};
	}

	const int32 BotsPerTeam = TeamSize - 1;
	return MakeArrayView(TeamBots).Slice(Team * BotsPerTeam, BotsPerTeam);
}

TArray<ANINJA_ARENACharacter*, TInlineAllocator<8>> ANINJA_ARENAGameMode::GetRoundFighters() const
{
	TArray<ANINJA_ARENACharacter*, TInlineAllocator<8>> Fighters;
	for (ANINJA_ARENACharacter* Captain : {TrainingPlayer.Get(), Opponent.Get()})
	{
		if (IsValid(Captain))
		{
			Fighters.Add(Captain);
		}
	}
	for (ANINJA_ARENACharacter* Bot : TeamBots)
	{
		if (IsValid(Bot))
		{
			Fighters.Add(Bot);
		}
	}
	return Fighters;
}

ANINJA_ARENACharacter* ANINJA_ARENAGameMode::GetTeamCompanion(const ANINJA_ARENACharacter* Fighter) const
{
	if (!IsTeamBattle() || !IsValid(Fighter))
	{
		return nullptr;
	}

	const int32 Team = Fighter->GetCombatTeam();
	ANINJA_ARENACharacter* Captain = GetTeamCaptain(Team);
	if (Captain != Fighter && IsLivingFighter(Captain))
	{
		return Captain;
	}
	for (ANINJA_ARENACharacter* Bot : GetTeamBots(Team))
	{
		if (Bot != Fighter && IsLivingFighter(Bot))
		{
			return Bot;
		}
	}
	return nullptr;
}

void ANINJA_ARENAGameMode::IgnoreAlliesForTrace(const ANINJA_ARENACharacter& Fighter, FCollisionQueryParams& Query) const
{
	if (!IsTeamBattle())
	{
		return;
	}

	const int32 Team = Fighter.GetCombatTeam();
	if (const ANINJA_ARENACharacter* Captain = GetTeamCaptain(Team); IsValid(Captain) && Captain != &Fighter)
	{
		Query.AddIgnoredActor(Captain);
	}
	for (const ANINJA_ARENACharacter* Bot : GetTeamBots(Team))
	{
		if (IsValid(Bot) && Bot != &Fighter)
		{
			Query.AddIgnoredActor(Bot);
		}
	}
}

void ANINJA_ARENAGameMode::UpdateTeamSpectator(int32 Team)
{
	ANINJA_ARENACharacter* Captain = GetTeamCaptain(Team);
	if (!IsValid(Captain) || IsLivingFighter(Captain))
	{
		return;
	}
	APlayerController* Controller = Cast<APlayerController>(Captain->GetController());
	if (!Controller)
	{
		return;
	}

	const ANINJA_ARENACharacter* Viewed = Cast<ANINJA_ARENACharacter>(Controller->GetViewTarget());
	if (IsLivingFighter(Viewed) && Captain->IsCombatAlly(Viewed))
	{
		return;
	}
	if (ANINJA_ARENACharacter* Ally = GetTeamCompanion(Captain))
	{
		Controller->SetViewTargetWithBlend(Ally, 0.22f);
	}
}

bool ANINJA_ARENAGameMode::SpawnTeamBots()
{
	const int32 BotsPerTeam = TeamSize - 1;
	const double FormationAngles[] = {100.0, -100.0, 50.0};
	for (int32 Index = 0; Index < TeamBots.Num(); ++Index)
	{
		const int32 Team = Index / BotsPerTeam;
		const int32 Position = Index % BotsPerTeam;
		ANINJA_ARENACharacter* Captain = GetTeamCaptain(Team);
		const ANINJA_ARENACharacter* Enemy = GetTeamCaptain(1 - Team);
		const bool bCaptainIsAscua = Captain->GetCombatStyle()->GetFighter() == ENinjaFighter::Ascua;
		const bool bAscua = Position == 1 ? bCaptainIsAscua : !bCaptainIsAscua;

		if (!IsValid(TeamBots[Index]))
		{
			const float StartDistance = Position == 0 ? 190.f : 230.f;
			for (const float Distance : {StartDistance, 275.f, 345.f, 420.f})
			{
				FTransform Start;
				if (!FindRivalStart(*Captain, Start, Distance, FormationAngles[Position], TeamSize == 4))
				{
					continue;
				}
				const FVector ToEnemy = Enemy->GetActorLocation() - Start.GetLocation();
				Start.SetRotation(ToEnemy.GetSafeNormal2D().Rotation().Quaternion());
				TeamBots[Index] = SpawnBot(Captain->GetClass(), Start, bAscua);
				if (IsValid(TeamBots[Index]))
				{
					break;
				}
			}
			if (!IsValid(TeamBots[Index]))
			{
				UE_LOG(LogTemp, Warning, TEXT("Team Battle: no se encontro salida para el aliado %d del equipo %d"), Position + 1, Team + 1);
				return false;
			}
		}

		TeamBots[Index]->SetCombatTeam(Team);
		TeamBots[Index]->GetCombatStyle()->SetFighter(bAscua ? ENinjaFighter::Ascua : ENinjaFighter::Junco);
		TeamBots[Index]->SetActorEnableCollision(true);
		TeamBots[Index]->SetActorHiddenInGame(true);
		TeamBots[Index]->SetRoundLocked(true);
	}
	return true;
}

void ANINJA_ARENAGameMode::UpdateTeamBattle(float Now)
{
	const auto Fighters = GetRoundFighters();
	bool bTeamAlive[2] = {};
	for (ANINJA_ARENACharacter* Fighter : Fighters)
	{
		const int32 Team = Fighter->GetCombatTeam();
		if (Team != 0 && Team != 1)
		{
			continue;
		}
		if (IsLivingFighter(Fighter))
		{
			bTeamAlive[Team] = true;
		}
		else if (!Fighter->IsRoundLocked())
		{
			NotifyFighterDown(Fighter);
		}
	}
	if (!bTeamAlive[0] || !bTeamAlive[1])
	{
		const ENinjaRoundResult Result = (bTeamAlive[0] == bTeamAlive[1]) ? ENinjaRoundResult::Draw
			: (bTeamAlive[0] ? ENinjaRoundResult::Victory : ENinjaRoundResult::Defeat);
		FinishRound(Result);
		return;
	}

	const int32 BotsPerTeam = TeamSize - 1;
	for (int32 Index = 0; Index < TeamBots.Num(); ++Index)
	{
		ANINJA_ARENACharacter* Bot = TeamBots[Index].Get();
		if (!IsLivingFighter(Bot))
		{
			continue;
		}

		const int32 Team = Index / BotsPerTeam;
		FNinjaBotState& State = TeamBotStates[Index];
		ANINJA_ARENACharacter* Target = State.Target.Get();
		if (!IsLivingFighter(Target) || (!State.bPreparing && Now >= State.NextTargetPick))
		{
			Target = nullptr;
			double BestScore = TNumericLimits<double>::Max();
			for (ANINJA_ARENACharacter* Enemy : Fighters)
			{
				if (!IsLivingFighter(Enemy) || Enemy->GetCombatTeam() == Team)
				{
					continue;
				}
				double Score = FVector::DistSquared(Bot->GetActorLocation(), Enemy->GetActorLocation());
				for (int32 AllyIndex = Team * BotsPerTeam; AllyIndex < (Team + 1) * BotsPerTeam; ++AllyIndex)
				{
					if (AllyIndex != Index && TeamBotStates[AllyIndex].Target.Get() == Enemy && IsLivingFighter(TeamBots[AllyIndex]))
					{
						Score += FMath::Square(150.0);
					}
				}
				if (Score < BestScore)
				{
					Target = Enemy;
					BestScore = Score;
				}
			}
			State.NextTargetPick = Now + (TeamSize == 4 ? 0.33f + 0.04f * (Index % BotsPerTeam) : 0.f);
		}
		if (bCaptureMatch && IsValid(Target) && !State.bPreparing && !Bot->IsCombatLocked()
			&& Bot->GetCharacterMovement()->IsMovingOnGround()
			&& FVector::DistSquared2D(Bot->GetActorLocation(), Target->GetActorLocation()) > FMath::Square(460.0))
		{
			const FVector Point(0.0, (Index % BotsPerTeam - 1) * 1250.0, 0.0);
			const FVector Offset = Point - Bot->GetActorLocation();
			State.Target = Target;
			if (Offset.SizeSquared2D() > FMath::Square(190.0))
			{
				if (Now >= State.NextTurn)
				{
					State.Move = FindBotMove(*Bot, *Target, Offset.GetSafeNormal2D());
					State.NextTurn = Now + 0.16f;
				}
				Bot->AddMovementInput(State.Move, 0.7f, true);
			}
			continue;
		}
		UpdateBot(*Bot, Target, State, Now);
	}
}
