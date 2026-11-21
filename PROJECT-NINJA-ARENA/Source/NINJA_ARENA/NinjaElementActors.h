#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NinjaElementActors.generated.h"

class UMaterialInstanceDynamic;
class UNinjaElementComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class ANinjaElementProp : public AActor
{
	GENERATED_BODY()

public:
	ANinjaElementProp();
	void Configure(bool bAsCrystal);
	void ConfigureStructure(bool bAsStone);
	static FVector GetStructureScale(bool bAsStone);
	void Restore();
	void Push(const FVector& Direction, float Strength);
	void RefreshVisual();
	bool IsCrystal() const { return bCrystal; }
	bool IsStructure() const { return bStructure; }
	bool IsFlammable() const { return !bCrystal && !bStone; }
	const TCHAR* GetDisplayName() const;
	FLinearColor GetBaseColor() const;
	float GetHealth() const { return Health; }
	UNinjaElementComponent* GetElements() const { return Elements; }
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

private:
	UPROPERTY(VisibleAnywhere, Category="Training")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category="Training")
	TObjectPtr<UNinjaElementComponent> Elements;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> Material;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CrystalShape;

	FTransform Start;
	float MaxHealth = 80.f;
	float Health = 80.f;
	bool bCrystal = false;
	bool bStructure = false;
	bool bStone = false;
};

UCLASS()
class ANinjaElementPulse : public AActor
{
	GENERATED_BODY()

public:
	ANinjaElementPulse();
	void Show(const FLinearColor& Color, double Length, float Radius);

private:
	UPROPERTY(VisibleAnywhere, Category="Element")
	TObjectPtr<UStaticMeshComponent> Mesh;
};
