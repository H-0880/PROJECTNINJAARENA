#include "NINJA_ARENAGameMode.h"

#include "Engine/World.h"
#include "NINJA_ARENACharacter.h"
#include "NinjaArenaHazards.h"
#include "NinjaElementActors.h"
#include "Scalability.h"

bool ANINJA_ARENAGameMode::FindFeatureGround(const ANINJA_ARENACharacter& Player, double Heading,
	double Padding, double HalfHeight, FVector& OutGround) const
{
	const FVector Origin = Player.GetActorLocation();
	const FVector RivalLocation = Opponent->GetActorLocation();
	FCollisionQueryParams Query(SCENE_QUERY_STAT(NinjaFeatureGround), false, &Player);
	Query.AddIgnoredActor(Opponent.Get());
	const auto FindFloor = [this, &Query, &Origin](const FVector& Point, FHitResult& Floor)
	{
		return GetWorld()->LineTraceSingleByChannel(Floor, FVector(Point.X, Point.Y, Origin.Z + 235.0),
			FVector(Point.X, Point.Y, Origin.Z - 365.0), ECC_Visibility, Query) && Floor.ImpactNormal.Z >= 0.85;
	};

	for (const double Distance : { 860.0, 730.0, 610.0, 510.0 })
	{
		for (const double Angle : { 0.0, 20.0, -20.0, 45.0, -45.0, 75.0, -75.0, 110.0, -110.0, 180.0 })
		{
			const FVector Candidate = Origin + FRotator(0.0, Heading + Angle, 0.0).Vector() * Distance;
			if (FVector::DistSquared2D(Candidate, RivalLocation) < FMath::Square(410.0)
				|| ElementProps.ContainsByPredicate([&Candidate, Padding](const TObjectPtr<ANinjaElementProp>& Prop)
				{
					return IsValid(Prop) && FVector::DistSquared2D(Candidate, Prop->GetActorLocation()) < FMath::Square(Padding + 125.0);
				})
				|| ArenaStructures.ContainsByPredicate([&Candidate, Padding](const TObjectPtr<ANinjaElementProp>& Structure)
				{
					return IsValid(Structure) && FVector::DistSquared2D(Candidate, Structure->GetActorLocation()) < FMath::Square(Padding + 245.0);
				}))
			{
				continue;
			}

			FHitResult Ground;
			if (!FindFloor(Candidate, Ground))
			{
				continue;
			}
			bool bSupported = true;
			for (const FVector& Direction : { FVector::ForwardVector, -FVector::ForwardVector, FVector::RightVector, -FVector::RightVector })
			{
				FHitResult Edge;
				if (!FindFloor(Candidate + Direction * Padding, Edge) || FMath::Abs(Edge.ImpactPoint.Z - Ground.ImpactPoint.Z) > 28.0)
				{
					bSupported = false;
					break;
				}
			}
			if (!bSupported || GetWorld()->OverlapBlockingTestByChannel(Ground.ImpactPoint + FVector(0.0, 0.0, HalfHeight + 18.0),
				FQuat::Identity, ECC_Pawn, FCollisionShape::MakeBox(FVector(Padding, Padding, HalfHeight)), Query))
			{
				continue;
			}

			OutGround = Ground.ImpactPoint;
			return true;
		}
	}
	return false;
}

void ANINJA_ARENAGameMode::SpawnArenaFeatures(const ANINJA_ARENACharacter& Player)
{
	const double Heading = (Opponent->GetActorLocation() - Player.GetActorLocation()).Rotation().Yaw;
	for (const bool bStone : { false, true })
	{
		const FVector Extents = ANinjaElementProp::GetStructureScale(bStone) * 50.0;
		FVector Ground;
		if (FindFeatureGround(Player, Heading + (bStone ? 95.0 : -95.0), FMath::Max(Extents.X, Extents.Y) + 13.0, Extents.Z, Ground))
		{
			const FVector Center = Ground + FVector(0.0, 0.0, Extents.Z + 2.0);
			if (ANinjaElementProp* Structure = GetWorld()->SpawnActor<ANinjaElementProp>(Center, FRotator::ZeroRotator))
			{
				Structure->ConfigureStructure(bStone);
				ArenaStructures.Add(Structure);
			}
		}
	}

	constexpr float HazardRadius = 170.f;
	FVector Ground;
	if (FindFeatureGround(Player, Heading + 180.0, HazardRadius + 15.0, 48.0, Ground))
	{
		ArenaHazard = GetWorld()->SpawnActor<ANinjaArenaDangerZone>(Ground + FVector(0.0, 0.0, 3.0), FRotator::ZeroRotator);
		if (IsValid(ArenaHazard))
		{
			ArenaHazard->Configure(HazardRadius, 12.f, false);
		}
	}
}

int32 ANINJA_ARENAGameMode::GetActiveFragmentCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<ANinjaArenaDebris>& Debris : LiveDebris)
	{
		Count += Debris.IsValid() ? 1 : 0;
	}
	return Count;
}

void ANINJA_ARENAGameMode::HandlePropDestroyed(const ANinjaElementProp& Prop)
{
	if (RoundResult != ENinjaRoundResult::Fighting || !IsTrainingArena())
	{
		return;
	}

	const FVector Center = Prop.GetActorLocation();
	const FVector Extents = Prop.GetActorScale3D() * 50.0;
	const int32 Effects = Scalability::GetEffectsQualityDirect(true);
	const int32 Budget = FMath::Clamp(FragmentBudget, 0, Effects == 0 ? 12 : (Effects == 1 ? 24 : 42));
	const int32 PerStructure = FMath::Clamp(MaxFragmentsPerStructure, 0, Effects == 0 ? 4 : (Effects == 1 ? 8 : 12));
	const int32 PerProp = Prop.IsStructure() ? PerStructure : FMath::Min(PerStructure, Effects == 0 ? 2 : 4);
	LiveDebris.RemoveAll([](const TWeakObjectPtr<ANinjaArenaDebris>& Debris) { return !Debris.IsValid(); });
	const int32 SpawnCount = FMath::Clamp(Budget - LiveDebris.Num(), 0, PerProp);
	for (int32 Index = 0; Index < SpawnCount; ++Index)
	{
		const double Angle = FMath::DegreesToRadians(360.0 * Index / SpawnCount + FMath::FRandRange(-16.f, 16.f));
		const FVector Outward(FMath::Cos(Angle), FMath::Sin(Angle), 0.0);
		const double Height = FMath::FRandRange(-0.4, 0.7) * Extents.Z;
		const FVector Offset = Outward * FVector(Extents.X * 0.6, Extents.Y * 0.6, 0.0) + FVector(0.0, 0.0, Height);
		if (ANinjaArenaDebris* Debris = GetWorld()->SpawnActor<ANinjaArenaDebris>(Center + Offset, Outward.Rotation()))
		{
			const FVector Velocity = Outward * FMath::FRandRange(145.0, 285.0) + FVector(0.0, 0.0, FMath::FRandRange(125.0, 255.0));
			Debris->Activate(Prop.IsStructure() ? 0.27 : 0.15, Velocity, Prop.GetBaseColor(), Effects == 0 ? 1.6f : 2.3f);
			LiveDebris.Add(Debris);
		}
	}

	if (Prop.IsStructure())
	{
		CollapseZones.RemoveAll([](const TWeakObjectPtr<ANinjaArenaDangerZone>& Zone) { return !Zone.IsValid(); });
		if (CollapseZones.Num() < 2)
		{
			const FVector Ground = Center - FVector(0.0, 0.0, Extents.Z - 2.0);
			if (ANinjaArenaDangerZone* Collapse = GetWorld()->SpawnActor<ANinjaArenaDangerZone>(Ground, FRotator::ZeroRotator))
			{
				Collapse->Configure(190.f, 18.f, true);
				CollapseZones.Add(Collapse);
			}
		}
	}
}

void ANINJA_ARENAGameMode::ResetArenaFeatures()
{
	ResetTempleBridge();
	for (const TWeakObjectPtr<ANinjaArenaDebris>& Debris : LiveDebris)
	{
		if (Debris.IsValid())
		{
			Debris->Destroy();
		}
	}
	LiveDebris.Reset();
	for (const TWeakObjectPtr<ANinjaArenaDangerZone>& Zone : CollapseZones)
	{
		if (Zone.IsValid())
		{
			Zone->Destroy();
		}
	}
	CollapseZones.Reset();
	for (ANinjaElementProp* Structure : ArenaStructures)
	{
		if (IsValid(Structure))
		{
			Structure->Restore();
		}
	}
	if (IsValid(ArenaHazard))
	{
		ArenaHazard->RestartCycle();
	}
}
