#include "NinjaElementActors.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NINJA_ARENAGameMode.h"
#include "NinjaElementComponent.h"
#include "UObject/ConstructorHelpers.h"

ANinjaElementProp::ANinjaElementProp()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shape"));
	SetRootComponent(Mesh);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCastShadow(false);
	Elements = CreateDefaultSubobject<UNinjaElementComponent>(TEXT("Elements"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ColorMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	Mesh->SetStaticMesh(Cube.Object);
	Mesh->SetMaterial(0, ColorMaterial.Object);
	CrystalShape = Cone.Object;
}

void ANinjaElementProp::Configure(bool bAsCrystal)
{
	bCrystal = bAsCrystal;
	if (bCrystal)
	{
		Mesh->SetStaticMesh(CrystalShape);
	}
	SetActorScale3D(FVector(bCrystal ? 0.53 : 0.62, bCrystal ? 0.53 : 0.62, 0.78));
	Start = GetActorTransform();
	Material = Mesh->CreateDynamicMaterialInstance(0);
	RefreshVisual();
}

FVector ANinjaElementProp::GetStructureScale(bool bAsStone)
{
	return bAsStone ? FVector(2.60, 0.92, 2.12) : FVector(1.15, 1.15, 2.35);
}

void ANinjaElementProp::ConfigureStructure(bool bAsStone)
{
	bStructure = true;
	bStone = bAsStone;
	MaxHealth = bStone ? 145.f : 115.f;
	Health = MaxHealth;
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetActorScale3D(GetStructureScale(bStone));
	Start = GetActorTransform();
	Material = Mesh->CreateDynamicMaterialInstance(0);
	RefreshVisual();
}

void ANinjaElementProp::Restore()
{
	Health = MaxHealth;
	SetActorTransform(Start, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	Elements->ResetElementState();
}

void ANinjaElementProp::Push(const FVector& Direction, float Strength)
{
	if (bCrystal || bStructure || Health <= 0.f || Elements->IsFrozen())
	{
		return;
	}

	const FVector Destination = GetActorLocation() + Direction.GetSafeNormal2D() * FMath::Clamp(Strength * 0.24f, 0.f, 185.f);
	const FCollisionQueryParams Query(SCENE_QUERY_STAT(ElementPropFloor), false, this);
	FHitResult Floor;
	if (GetWorld()->LineTraceSingleByChannel(Floor, Destination + FVector(0.0, 0.0, 45.0),
		Destination - FVector(0.0, 0.0, 135.0), ECC_Visibility, Query) && Floor.ImpactNormal.Z > 0.65)
	{
		const FVector RestingPoint = Floor.ImpactPoint + FVector(0.0, 0.0, 51.0);
		SetActorLocation(RestingPoint, true);
	}
}

const TCHAR* ANinjaElementProp::GetDisplayName() const
{
	if (bCrystal)
	{
		return TEXT("CRISTAL");
	}
	return bStructure ? (bStone ? TEXT("MURO") : TEXT("PILAR")) : TEXT("CAJA");
}

FLinearColor ANinjaElementProp::GetBaseColor() const
{
	if (bCrystal)
	{
		return FLinearColor(0.42f, 0.17f, 0.96f);
	}
	if (bStructure)
	{
		return bStone ? FLinearColor(0.33f, 0.47f, 0.59f) : FLinearColor(0.59f, 0.36f, 0.20f);
	}
	return FLinearColor(0.76f, 0.35f, 0.10f);
}

void ANinjaElementProp::RefreshVisual()
{
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), Elements->GetStatusColor(GetBaseColor()));
	}
}

float ANinjaElementProp::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Health <= 0.f || Damage <= 0.f)
	{
		return 0.f;
	}
	const float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	Health = FMath::Max(0.f, Health - ActualDamage);
	if (Health <= 0.f)
	{
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
		Elements->ResetElementState();
		if (ANINJA_ARENAGameMode* Mode = GetWorld()->GetAuthGameMode<ANINJA_ARENAGameMode>())
		{
			Mode->HandlePropDestroyed(*this);
		}
	}
	return ActualDamage;
}

ANinjaElementPulse::ANinjaElementPulse()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pulse"));
	SetRootComponent(Mesh);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ColorMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	Mesh->SetStaticMesh(Sphere.Object);
	Mesh->SetMaterial(0, ColorMaterial.Object);
}

void ANinjaElementPulse::Show(const FLinearColor& Color, double Length, float Radius)
{
	SetActorScale3D(FVector(FMath::Max(0.15, Length / 100.0), Radius / 320.f, Radius / 320.f));
	if (UMaterialInstanceDynamic* Tint = Mesh->CreateDynamicMaterialInstance(0))
	{
		Tint->SetVectorParameterValue(TEXT("Color"), Color);
	}
	SetLifeSpan(0.27f);
}
