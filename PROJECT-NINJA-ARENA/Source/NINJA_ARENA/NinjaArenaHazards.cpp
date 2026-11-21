#include "NinjaArenaHazards.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NINJA_ARENACharacter.h"
#include "UObject/ConstructorHelpers.h"

ANinjaArenaDebris::ANinjaArenaDebris()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Fragment"));
	SetRootComponent(Mesh);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCollisionObjectType(ECC_PhysicsBody);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCastShadow(false);
	SetCanBeDamaged(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ColorMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	Mesh->SetStaticMesh(Cube.Object);
	Mesh->SetMaterial(0, ColorMaterial.Object);
}

void ANinjaArenaDebris::Activate(double Scale, const FVector& Velocity, const FLinearColor& Color, float Lifetime)
{
	SetActorScale3D(FVector(Scale, Scale * 0.78, Scale * 0.66));
	if (UMaterialInstanceDynamic* Tint = Mesh->CreateDynamicMaterialInstance(0))
	{
		Tint->SetVectorParameterValue(TEXT("Color"), Color);
	}
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetSimulatePhysics(true);
	Mesh->SetPhysicsLinearVelocity(Velocity);
	Mesh->AddAngularImpulseInDegrees(FVector(170.0, -110.0, 140.0), NAME_None, true);
	SetLifeSpan(Lifetime);
}

ANinjaArenaDangerZone::ANinjaArenaDangerZone()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneMarker"));
	SetRootComponent(Mesh);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCastShadow(false);
	SetCanBeDamaged(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ColorMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	Mesh->SetStaticMesh(Cylinder.Object);
	Mesh->SetMaterial(0, ColorMaterial.Object);
}

void ANinjaArenaDangerZone::Configure(float InRadius, float InDamage, bool bInCollapse)
{
	Radius = InRadius;
	Damage = InDamage;
	bCollapse = bInCollapse;
	SetActorScale3D(FVector(Radius / 50.f, Radius / 50.f, 0.035f));
	Material = Mesh->CreateDynamicMaterialInstance(0);
	RestartCycle();
}

void ANinjaArenaDangerZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(PhaseTimer);
	GetWorldTimerManager().ClearTimer(DamageTimer);
	Super::EndPlay(EndPlayReason);
}

void ANinjaArenaDangerZone::RestartCycle()
{
	GetWorldTimerManager().ClearTimer(PhaseTimer);
	GetWorldTimerManager().ClearTimer(DamageTimer);
	Phase = ENinjaArenaZonePhase::Rest;
	if (bCollapse)
	{
		AdvanceCycle();
	}
	else
	{
		PaintPhase();
		GetWorldTimerManager().SetTimer(PhaseTimer, this, &ANinjaArenaDangerZone::AdvanceCycle, RestSeconds, false);
	}
}

void ANinjaArenaDangerZone::AdvanceCycle()
{
	float NextDelay = RestSeconds;
	switch (Phase)
	{
	case ENinjaArenaZonePhase::Rest:
		Phase = ENinjaArenaZonePhase::Warning;
		NextDelay = bCollapse ? 0.42f : WarningSeconds;
		break;
	case ENinjaArenaZonePhase::Warning:
		Phase = ENinjaArenaZonePhase::Active;
		NextDelay = bCollapse ? 0.5f : ActiveSeconds;
		DamageOccupants();
		if (!bCollapse)
		{
			GetWorldTimerManager().SetTimer(DamageTimer, this, &ANinjaArenaDangerZone::DamageOccupants, 0.59f, true);
		}
		break;
	case ENinjaArenaZonePhase::Active:
		GetWorldTimerManager().ClearTimer(DamageTimer);
		if (bCollapse)
		{
			Destroy();
			return;
		}
		Phase = ENinjaArenaZonePhase::Rest;
		break;
	}
	PaintPhase();
	GetWorldTimerManager().SetTimer(PhaseTimer, this, &ANinjaArenaDangerZone::AdvanceCycle, NextDelay, false);
}

void ANinjaArenaDangerZone::DamageOccupants()
{
	const FVector Center = GetActorLocation();
	const FCollisionObjectQueryParams Pawns(ECC_Pawn);
	const FCollisionQueryParams Query(SCENE_QUERY_STAT(NinjaArenaHazard), false, this);
	TArray<FOverlapResult> Hits;
	GetWorld()->OverlapMultiByObjectType(Hits, Center + FVector(0.0, 0.0, 85.0), FQuat::Identity, Pawns,
		FCollisionShape::MakeSphere(Radius + 45.f), Query);
	TSet<ANINJA_ARENACharacter*> Damaged;
	for (const FOverlapResult& Hit : Hits)
	{
		ANINJA_ARENACharacter* Fighter = Cast<ANINJA_ARENACharacter>(Hit.GetActor());
		if (!IsValid(Fighter) || Damaged.Contains(Fighter) ||
			FVector::DistSquared2D(Center, Fighter->GetActorLocation()) > FMath::Square(static_cast<double>(Radius)))
		{
			continue;
		}
		Damaged.Add(Fighter);
		const double FootHeight = Fighter->GetActorLocation().Z - Fighter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		const double VerticalTolerance = bCollapse ? 185.0 : 74.0;
		if (FMath::Abs(FootHeight - Center.Z) > VerticalTolerance)
		{
			continue;
		}
		FCollisionQueryParams Sight = Query;
		Sight.AddIgnoredActor(Fighter);
		if (!GetWorld()->LineTraceTestByChannel(Center + FVector(0.0, 0.0, 27.0), Fighter->GetActorLocation(), ECC_Visibility, Sight))
		{
			UGameplayStatics::ApplyDamage(Fighter, Damage, nullptr, this, nullptr);
		}
	}
}

void ANinjaArenaDangerZone::PaintPhase()
{
	if (!Material)
	{
		return;
	}
	const FLinearColor Color = Phase == ENinjaArenaZonePhase::Active ? FLinearColor(1.f, 0.11f, 0.04f)
		: (Phase == ENinjaArenaZonePhase::Warning ? FLinearColor(1.f, 0.62f, 0.09f) : FLinearColor(0.22f, 0.26f, 0.32f));
	Material->SetVectorParameterValue(TEXT("Color"), Color);
}
