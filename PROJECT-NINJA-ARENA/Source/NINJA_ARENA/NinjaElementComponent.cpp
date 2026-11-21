#include "NinjaElementComponent.h"

#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NINJA_ARENACharacter.h"
#include "NINJA_ARENAGameMode.h"
#include "NinjaCombatStyle.h"
#include "NinjaElementActors.h"
#include "PNAVitalsComponent.h"
#include "Scalability.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	FNinjaElementAbility MakeAbility(const TCHAR* Name, float Cost, float Cooldown, float Damage,
		float Duration, const FLinearColor& Color, float Force = 0.f)
	{
		FNinjaElementAbility Ability;
		Ability.Name = Name;
		Ability.EnergyCost = Cost;
		Ability.Cooldown = Cooldown;
		Ability.Damage = Damage;
		Ability.EffectDuration = Duration;
		Ability.Color = Color;
		Ability.Force = Force;
		return Ability;
	}

	bool IsElementTimerActive(const UWorld* World, float Until)
	{
		return World && World->GetTimeSeconds() < Until;
	}
}

UNinjaElementComponent::UNinjaElementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickInterval = 0.05f;

	Abilities.Add(ENinjaElement::Fire, MakeAbility(TEXT("FUEGO"), 18.f, 1.6f, 14.f, 3.f, FLinearColor(1.f, 0.3f, 0.07f)));
	Abilities.Add(ENinjaElement::Lightning, MakeAbility(TEXT("RAYO"), 22.f, 2.f, 20.f, 0.45f, FLinearColor(0.9f, 0.66f, 1.f)));
	Abilities.Add(ENinjaElement::Wind, MakeAbility(TEXT("VIENTO"), 12.f, 1.25f, 5.f, 0.f, FLinearColor(0.37f, 1.f, 0.68f), 620.f));
	Abilities.Add(ENinjaElement::Ice, MakeAbility(TEXT("HIELO"), 16.f, 1.8f, 9.f, 2.4f, FLinearColor(0.55f, 0.9f, 1.f)));
	Abilities.Add(ENinjaElement::Water, MakeAbility(TEXT("AGUA"), 8.f, 1.f, 0.f, 3.5f, FLinearColor(0.16f, 0.51f, 1.f)));

	static ConstructorHelpers::FObjectFinderOptional<UNiagaraSystem> Lightning(
		TEXT("/Game/GrimzaFX_simple_lightning/Particles/NS_Lightning_01b.NS_Lightning_01b"));
	LightningEffect = Lightning.Get();
}

const UNiagaraSystem* UNinjaElementComponent::GetLightningEffect() const { return LightningEffect.Get(); }

bool UNinjaElementComponent::IsBurning() const { return IsElementTimerActive(GetWorld(), BurnUntil); }
bool UNinjaElementComponent::IsWet() const { return IsElementTimerActive(GetWorld(), WetUntil); }
bool UNinjaElementComponent::IsFrozen() const { return IsElementTimerActive(GetWorld(), FrozenUntil); }
bool UNinjaElementComponent::IsCharged() const { return IsElementTimerActive(GetWorld(), ChargedUntil); }

float UNinjaElementComponent::GetCooldownRemaining(ENinjaElement Element) const
{
	const UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.f;
	return World ? FMath::Max(0.f, FMath::Max(NextCastAt, ReadyAt.FindRef(Element)) - Now) : 0.f;
}

bool UNinjaElementComponent::CastElement(ENinjaElement Element)
{
	ANINJA_ARENACharacter* Source = Cast<ANINJA_ARENACharacter>(GetOwner());
	const FNinjaElementAbility* Ability = GetAbility(Element);
	UWorld* World = GetWorld();
	if (!Source || !World || !Source->HasAuthority() || !Ability || !Source->HasActorBegunPlay()
		|| Source->IsCombatLocked() || Source->IsGuarding() || GetCooldownRemaining(Element) > 0.f
		|| !Source->GetVitals()->SpendEnergy(Ability->EnergyCost))
	{
		return false;
	}

	const float Now = World->GetTimeSeconds();
	ReadyAt.Add(Element, Now + FMath::Max(0.1f, Ability->Cooldown));
	NextCastAt = Now + 0.24f;

	const FVector Origin = Source->GetActorLocation();
	Source->GetCombatStyle()->PlaySound(ENinjaStyleSound::Energy, Origin);
	const AController* Controller = Source->GetController();
	const FVector Direction = Controller ? FRotator(0.0, Controller->GetControlRotation().Yaw, 0.0).Vector()
		: Source->GetActorForwardVector().GetSafeNormal2D();
	const double Distance = FMath::Max(50.f, Ability->Range);
	const float Radius = FMath::Clamp(Ability->HitRadius, 10.f, 400.f);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(NinjaElementalCast), false, Source);
	if (const ANINJA_ARENAGameMode* Mode = World->GetAuthGameMode<ANINJA_ARENAGameMode>())
	{
		Mode->IgnoreAlliesForTrace(*Source, Query);
	}
	FHitResult Cover;
	const bool bHasCover = World->LineTraceSingleByChannel(Cover, Origin, Origin + Direction * Distance, ECC_Visibility, Query);
	const double Length = bHasCover ? Cover.Distance : Distance;
	const FVector Center = Origin + Direction * (Length * 0.5);
	const float SearchRadius = FMath::Sqrt(FMath::Square(Length * 0.5) + FMath::Square(static_cast<double>(Radius)));
	TOptional<FVector> LightningImpact;

	FCollisionObjectQueryParams Objects;
	if (Ability->bAffectsCharacters)
	{
		Objects.AddObjectTypesToQuery(ECC_Pawn);
	}
	if (Ability->bAffectsObjects)
	{
		Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	}

	if (Ability->bAffectsCharacters || Ability->bAffectsObjects)
	{
		TArray<FOverlapResult> Hits;
		World->OverlapMultiByObjectType(Hits, Center, FQuat::Identity, Objects,
			FCollisionShape::MakeSphere(SearchRadius), Query);
		TSet<AActor*> Checked;
		for (const FOverlapResult& Hit : Hits)
		{
			AActor* Target = Hit.GetActor();
			if (!IsValid(Target) || Target == Source || Source->IsCombatAlly(Target) || Checked.Contains(Target)
				|| (Target->IsA<ANINJA_ARENACharacter>() && !Ability->bAffectsCharacters)
				|| (Target->IsA<ANinjaElementProp>() && !Ability->bAffectsObjects))
			{
				continue;
			}
			Checked.Add(Target);
			UNinjaElementComponent* Recipient = Target->FindComponentByClass<UNinjaElementComponent>();
			const FVector Offset = Target->GetActorLocation() - Origin;
			const double Along = FVector::DotProduct(Offset, Direction);
			const bool bDirectHit = bHasCover && Cover.GetActor() == Target;
			if (!Recipient || Along <= 0.0 || (!bDirectHit && (Along > Length
				|| (Offset - Direction * Along).SizeSquared() > FMath::Square(static_cast<double>(Radius)))))
			{
				continue;
			}
			FCollisionQueryParams Sight = Query;
			Sight.AddIgnoredActor(Target);
			if (!World->LineTraceTestByChannel(Origin, Target->GetActorLocation(), ECC_Visibility, Sight))
			{
				Recipient->ReceiveElement(*Source, Element, *Ability, Offset.GetSafeNormal2D());
				if (Element == ENinjaElement::Lightning && !LightningImpact.IsSet())
				{
					LightningImpact = Target->GetActorLocation();
				}
			}
		}
	}

	if (ANinjaElementPulse* Pulse = World->SpawnActor<ANinjaElementPulse>(Center, Direction.Rotation()))
	{
		Pulse->Show(Ability->Color, Length, Radius);
	}
	if (LightningImpact.IsSet() && LightningEffect && FApp::CanEverRender() && Scalability::GetEffectsQualityDirect(true) > 0)
	{
		UNiagaraComponent* Flash = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, LightningEffect,
			LightningImpact.GetValue(), Direction.Rotation(), FVector(0.65), true, true, ENCPoolMethod::None);
		if (Flash)
		{
			FTimerHandle Lifetime;
			World->GetTimerManager().SetTimer(Lifetime, FTimerDelegate::CreateWeakLambda(Flash, [Flash]()
			{
				Flash->DestroyComponent();
			}), 0.8f, false);
		}
	}
	return true;
}

void UNinjaElementComponent::ReceiveElement(ANINJA_ARENACharacter& Source, ENinjaElement Element,
	const FNinjaElementAbility& Ability, const FVector& Direction)
{
	AActor* Target = GetOwner();
	ANINJA_ARENACharacter* Fighter = Cast<ANINJA_ARENACharacter>(Target);
	ANinjaElementProp* Prop = Cast<ANinjaElementProp>(Target);
	if ((Fighter && (Fighter->IsRoundLocked() || Fighter->GetVitals()->IsDefeated()))
		|| (Prop && Prop->GetHealth() <= 0.f) || !Target->CanBeDamaged())
	{
		return;
	}

	const bool bWet = IsWet();
	const float Now = GetWorld()->GetTimeSeconds();
	float Damage = FMath::Max(0.f, Ability.Damage);
	if (Element == ENinjaElement::Fire && Prop && Prop->IsFlammable())
	{
		Damage *= 1.6f;
	}
	if (Element == ENinjaElement::Lightning)
	{
		Damage *= (bWet ? 1.5f : 1.f) * ((Prop && Prop->IsCrystal()) ? 1.65f : 1.f);
	}
	if (Damage > 0.f)
	{
		UGameplayStatics::ApplyDamage(Target, Damage, Source.GetController(), &Source, nullptr);
	}
	if ((Fighter && Fighter->GetVitals()->IsDefeated()) || (Prop && Prop->GetHealth() <= 0.f))
	{
		return;
	}

	const float Duration = FMath::Max(0.f, Ability.EffectDuration);
	switch (Element)
	{
	case ENinjaElement::Fire:
		FrozenUntil = 0.f;
		if (bWet)
		{
			WetUntil = 0.f;
		}
		else
		{
			if (!IsBurning())
			{
				NextBurnAt = Now + 0.5f;
			}
			BurnUntil = Now + Duration;
			BurnDamage = Damage * 0.16f;
			BurnSource = &Source;
		}
		break;
	case ENinjaElement::Lightning:
		WetUntil = 0.f;
		ChargedUntil = Now + Duration * ((Prop && Prop->IsCrystal()) ? 4.f : 1.f);
		if (Fighter && Duration > 0.f)
		{
			Fighter->InterruptForElement();
		}
		break;
	case ENinjaElement::Wind:
		if (Fighter)
		{
			Fighter->InterruptForElement(Direction * Ability.Force + FVector(0.0, 0.0, 125.0));
		}
		else if (Prop)
		{
			Prop->Push(Direction, Ability.Force);
		}
		break;
	case ENinjaElement::Ice:
		FrozenUntil = Now + Duration * (bWet ? 1.5f : 1.f);
		WetUntil = 0.f;
		BurnUntil = 0.f;
		BurnSource.Reset();
		break;
	case ENinjaElement::Water:
		BurnUntil = 0.f;
		BurnSource.Reset();
		WetUntil = Now + Duration;
		break;
	}
	RefreshStatus();
}

void UNinjaElementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < BurnUntil && Now >= NextBurnAt)
	{
		NextBurnAt = Now + 0.5f;
		ANINJA_ARENACharacter* Source = BurnSource.Get();
		UGameplayStatics::ApplyDamage(GetOwner(), BurnDamage, Source ? Source->GetController() : nullptr, Source, nullptr);
	}
	RefreshStatus();
}

uint8 UNinjaElementComponent::GetStatusMask() const
{
	return (IsBurning() ? 1 : 0) | (IsWet() ? 2 : 0) | (IsFrozen() ? 4 : 0) | (IsCharged() ? 8 : 0);
}

void UNinjaElementComponent::RefreshStatus()
{
	const uint8 Status = GetStatusMask();
	if (Status == LastStatusMask)
	{
		return;
	}
	LastStatusMask = Status;
	SetComponentTickEnabled(Status != 0);
	if (ANINJA_ARENACharacter* Fighter = Cast<ANINJA_ARENACharacter>(GetOwner()))
	{
		Fighter->RefreshMovementSpeed();
	}
	else if (ANinjaElementProp* Prop = Cast<ANinjaElementProp>(GetOwner()))
	{
		Prop->RefreshVisual();
	}
}

void UNinjaElementComponent::ResetElementState()
{
	ReadyAt.Reset();
	NextCastAt = BurnUntil = NextBurnAt = BurnDamage = WetUntil = FrozenUntil = ChargedUntil = 0.f;
	BurnSource.Reset();
	RefreshStatus();
}

const TCHAR* UNinjaElementComponent::GetStatusLabel() const
{
	if (IsCharged()) { return TEXT("CARGA"); }
	if (IsFrozen()) { return TEXT("HIELO"); }
	if (IsBurning()) { return TEXT("QUEMADURA"); }
	if (IsWet()) { return TEXT("MOJADO"); }
	return TEXT("");
}

FLinearColor UNinjaElementComponent::GetStatusColor(const FLinearColor& DefaultColor) const
{
	if (IsCharged()) { return FLinearColor(0.96f, 0.79f, 1.f); }
	if (IsFrozen()) { return FLinearColor(0.55f, 0.91f, 1.f); }
	if (IsBurning()) { return FLinearColor(1.f, 0.22f, 0.03f); }
	if (IsWet()) { return FLinearColor(0.10f, 0.52f, 1.f); }
	return DefaultColor;
}
