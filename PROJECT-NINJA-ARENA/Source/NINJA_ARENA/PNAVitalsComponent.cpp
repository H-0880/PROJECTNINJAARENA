#include "PNAVitalsComponent.h"
#include "GameFramework/Actor.h"

UPNAVitalsComponent::UPNAVitalsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPNAVitalsComponent::BeginPlay()
{
	Super::BeginPlay();
	ResetVitals();
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddUniqueDynamic(this, &UPNAVitalsComponent::HandleAnyDamage);
	}
}

void UPNAVitalsComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.RemoveDynamic(this, &UPNAVitalsComponent::HandleAnyDamage);
	}
	Super::EndPlay(EndPlayReason);
}

void UPNAVitalsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsDefeated())
	{
		Energy = FMath::Clamp(Energy + FMath::Max(0.f, EnergyRegenPerSecond) * DeltaTime, 0.f, FMath::Max(0.f, MaxEnergy));
	}
}

void UPNAVitalsComponent::ResetVitals()
{
	Health = FMath::Max(1.f, MaxHealth);
	Energy = FMath::Max(0.f, MaxEnergy);
}

bool UPNAVitalsComponent::SpendEnergy(float Amount)
{
	if (IsDefeated() || !FMath::IsFinite(Amount) || Amount < 0.f || Energy < Amount)
	{
		return false;
	}
	Energy -= Amount;
	return true;
}

void UPNAVitalsComponent::HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage > 0.f)
	{
		Health = FMath::Max(0.f, Health - Damage);
	}
}
