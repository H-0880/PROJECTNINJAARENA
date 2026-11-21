#include "NinjaCombatFeedback.h"

#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NINJA_ARENACharacter.h"
#include "NinjaCombatStyle.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Misc/App.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	struct FNinjaImpactStyle
	{
		float Stop;
		float Reaction;
		float Angle;
		float Shake;
		float Flash;
		float Wave;
		FLinearColor Color;
	};

	FNinjaImpactStyle GetImpactStyle(ENinjaImpactKind Kind)
	{
		switch (Kind)
		{
		case ENinjaImpactKind::PerfectGuard:
			return { 0.030f, 0.14f, 2.f, 0.44f, 0.38f, 94.f, FLinearColor(0.25f, 1.f, 0.83f) };
		case ENinjaImpactKind::Guard:
			return { 0.016f, 0.12f, 3.f, 0.35f, 0.20f, 0.f, FLinearColor(0.31f, 0.75f, 1.f) };
		case ENinjaImpactKind::Heavy:
			return { 0.052f, 0.28f, 14.f, 1.28f, 0.56f, 205.f, FLinearColor(1.f, 0.46f, 0.15f) };
		case ENinjaImpactKind::Aerial:
			return { 0.024f, 0.18f, 9.f, 0.62f, 0.30f, 0.f, FLinearColor(0.79f, 0.56f, 1.f) };
		case ENinjaImpactKind::Ultimate:
			return { 0.078f, 0.4f, 22.f, 2.2f, 0.86f, 370.f, FLinearColor(0.22f, 0.96f, 0.84f) };
		default:
			return { 0.021f, 0.15f, 7.f, 0.52f, 0.28f, 0.f, FLinearColor(1.f, 0.78f, 0.28f) };
		}
	}

	void SpawnImpactBurst(UWorld* World, const FVector& Location, ENinjaImpactKind Kind)
	{
		if (ANinjaImpactBurst* Burst = World->SpawnActor<ANinjaImpactBurst>(Location, FRotator::ZeroRotator))
		{
			Burst->Configure(Kind);
		}
	}
}

UNinjaCombatFeedbackComponent::UNinjaCombatFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UNinjaCombatFeedbackComponent::Hit(ANINJA_ARENACharacter& Attacker, AActor& Target, ENinjaImpactKind Kind, bool bSpawnBurst)
{
	Attacker.GetCombatFeedback()->ApplyImpact(Kind, false);
	const bool bGuard = Kind == ENinjaImpactKind::Guard || Kind == ENinjaImpactKind::PerfectGuard;
	if (Kind != ENinjaImpactKind::Ultimate)
	{
		const bool bStrong = Kind == ENinjaImpactKind::Heavy;
		Attacker.GetCombatStyle()->PlaySound(bGuard ? ENinjaStyleSound::Energy : (bStrong ? ENinjaStyleSound::Heavy : ENinjaStyleSound::Light),
			Target.GetActorLocation());
	}
	if (ANINJA_ARENACharacter* Fighter = Cast<ANINJA_ARENACharacter>(&Target))
	{
		if (!bGuard)
		{
			Fighter->GetCombatStyle()->StopActionPose();
		}
		Fighter->GetCombatFeedback()->ApplyImpact(Kind, true);
	}
	if (bSpawnBurst)
	{
		const FVector TowardSource = (Attacker.GetActorLocation() - Target.GetActorLocation()).GetSafeNormal2D();
		const FVector Location = Target.GetActorLocation() + TowardSource * 35.0 + FVector(0.0, 0.0, 20.0);
		SpawnImpactBurst(Attacker.GetWorld(), Location, Kind);
		if (Kind == ENinjaImpactKind::PerfectGuard && FApp::CanEverRender())
		{
			static const TSoftObjectPtr<UNiagaraSystem> Spark(FSoftObjectPath(TEXT(
				"/Game/FreeStylizedLaserBeamVFX/VFX/Hit/NS_Hit_01.NS_Hit_01")));
			if (UNiagaraSystem* System = Spark.LoadSynchronous())
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(Attacker.GetWorld(), System, Location,
					TowardSource.Rotation(), FVector(0.65));
			}
		}
	}
}

void UNinjaCombatFeedbackComponent::BeginHitStop(float Duration)
{
	ANINJA_ARENACharacter* Fighter = CastChecked<ANINJA_ARENACharacter>(GetOwner());
	if (!bHitStopped)
	{
		bHitStopped = true;
		SavedTimeDilation = Fighter->CustomTimeDilation;
		SavedAnimRate = Fighter->GetMesh()->GlobalAnimRateScale;
		Fighter->CustomTimeDilation = SavedTimeDilation * 0.08f;
		Fighter->GetMesh()->GlobalAnimRateScale = 0.f;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	HitStopUntil = FMath::Max(HitStopUntil, Now + Duration);
	GetWorld()->GetTimerManager().SetTimer(HitStopTimer, this, &UNinjaCombatFeedbackComponent::EndHitStop,
		static_cast<float>(HitStopUntil - Now), false);
}

void UNinjaCombatFeedbackComponent::EndHitStop()
{
	if (bHitStopped)
	{
		ANINJA_ARENACharacter* Fighter = CastChecked<ANINJA_ARENACharacter>(GetOwner());
		Fighter->CustomTimeDilation = SavedTimeDilation;
		Fighter->GetMesh()->GlobalAnimRateScale = SavedAnimRate;
		bHitStopped = false;
	}
	HitStopUntil = 0.0;
}

void UNinjaCombatFeedbackComponent::ApplyImpact(ENinjaImpactKind Kind, bool bHitTarget)
{
	const FNinjaImpactStyle Style = GetImpactStyle(Kind);
	const double Now = GetWorld()->GetTimeSeconds();
	BeginHitStop(Style.Stop);
	if (bHitTarget)
	{
		ANINJA_ARENACharacter* Fighter = CastChecked<ANINJA_ARENACharacter>(GetOwner());
		if (!bReacting)
		{
			MeshHome = Fighter->GetMesh()->GetRelativeRotation();
		}
		bReacting = true;
		ReactionStart = Now;
		ReactionDuration = Style.Reaction;
		ReactionAngle = Style.Angle;
		SetComponentTickEnabled(true);
	}
	if (bEnableCameraShake && CaptureLocalCamera())
	{
		ShakeStart = Now;
		ShakeDuration = Style.Reaction;
		ShakePower = Style.Shake;
		SetComponentTickEnabled(true);
	}
}

UCameraComponent* UNinjaCombatFeedbackComponent::CaptureLocalCamera()
{
	const ANINJA_ARENACharacter* Fighter = CastChecked<ANINJA_ARENACharacter>(GetOwner());
	if (!Fighter->IsPlayerControlled() || !Fighter->IsLocallyControlled())
	{
		return nullptr;
	}
	UCameraComponent* Camera = Fighter->GetFollowCamera();
	if (!bCameraCaptured)
	{
		bCameraCaptured = true;
		CameraHome = Camera->GetRelativeLocation();
		CameraHomeRotation = Camera->GetRelativeRotation();
		CameraHomeFov = Camera->FieldOfView;
	}
	return Camera;
}

void UNinjaCombatFeedbackComponent::BeginCameraCue(float FovOffset, const FVector& Offset, float Duration)
{
	if (bEnableActionCamera && CaptureLocalCamera())
	{
		CueFov = FovOffset;
		CueOffset = Offset;
		CueUntil = GetWorld()->GetTimeSeconds() + Duration;
		SetComponentTickEnabled(true);
	}
}

void UNinjaCombatFeedbackComponent::CuePursuit()
{
	BeginCameraCue(9.f, FVector(-12.0, 16.0, 9.0), 0.46f);
}

void UNinjaCombatFeedbackComponent::CueUltimateCharge()
{
	BeginCameraCue(-12.f, FVector(16.0, 22.0, 4.0), 0.52f);
}

void UNinjaCombatFeedbackComponent::CueUltimateRelease(const FVector& Location)
{
	BeginCameraCue(5.f, FVector(-14.0, 23.0, 10.0), 0.36f);
	ApplyImpact(ENinjaImpactKind::Ultimate, false);
	SpawnImpactBurst(GetWorld(), Location, ENinjaImpactKind::Ultimate);
}

void UNinjaCombatFeedbackComponent::ReleaseCameraCue()
{
	CueUntil = 0.0;
}

void UNinjaCombatFeedbackComponent::RestoreReaction()
{
	if (bReacting)
	{
		CastChecked<ANINJA_ARENACharacter>(GetOwner())->GetMesh()->SetRelativeRotation(MeshHome);
		bReacting = false;
	}
}

void UNinjaCombatFeedbackComponent::RestoreCamera()
{
	if (bCameraCaptured)
	{
		UCameraComponent* Camera = CastChecked<ANINJA_ARENACharacter>(GetOwner())->GetFollowCamera();
		Camera->SetRelativeLocationAndRotation(CameraHome, CameraHomeRotation);
		Camera->SetFieldOfView(CameraHomeFov);
		bCameraCaptured = false;
	}
	CueUntil = 0.0;
	ShakeDuration = 0.f;
	CurrentOffset = FVector::ZeroVector;
	CurrentFov = 0.f;
}

void UNinjaCombatFeedbackComponent::ResetFeedback()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitStopTimer);
	}
	EndHitStop();
	RestoreReaction();
	RestoreCamera();
	SetComponentTickEnabled(false);
}

void UNinjaCombatFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ResetFeedback();
	Super::EndPlay(EndPlayReason);
}

void UNinjaCombatFeedbackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const double Now = GetWorld()->GetTimeSeconds();
	const ANINJA_ARENACharacter* Fighter = CastChecked<ANINJA_ARENACharacter>(GetOwner());
	if (bReacting)
	{
		const double Progress = (Now - ReactionStart) / ReactionDuration;
		if (Progress >= 1.0)
		{
			RestoreReaction();
		}
		else
		{
			const double Tilt = FMath::Sin(UE_PI * Progress) * ReactionAngle;
			Fighter->GetMesh()->SetRelativeRotation(MeshHome + FRotator(-Tilt, 0.0, Tilt * 0.35));
		}
	}

	if (bCameraCaptured)
	{
		if (!Fighter->IsPlayerControlled() || !Fighter->IsLocallyControlled())
		{
			RestoreCamera();
		}
		else
		{
			const bool bCueActive = Now < CueUntil;
			const float WorldDelta = GetWorld()->GetDeltaSeconds();
			const float FrameDelta = FMath::Clamp(WorldDelta, 0.f, 0.05f);
			CurrentFov = FMath::FInterpTo(CurrentFov, bCueActive ? CueFov : 0.f, FrameDelta, 13.f);
			CurrentOffset = FMath::VInterpTo(CurrentOffset, bCueActive ? CueOffset : FVector::ZeroVector, FrameDelta, 12.f);
			const double ShakeAge = Now - ShakeStart;
			const bool bShaking = ShakeAge < ShakeDuration;
			if (!bCueActive && !bShaking && FMath::Abs(CurrentFov) < 0.05f && CurrentOffset.IsNearlyZero(0.1))
			{
				RestoreCamera();
			}
			else
			{
				const double Amplitude = bShaking ? ShakePower * (1.0 - ShakeAge / ShakeDuration) : 0.0;
				const double Wobble = FMath::Sin(ShakeAge * 68.0) * Amplitude;
				UCameraComponent* Camera = Fighter->GetFollowCamera();
				Camera->SetRelativeLocation(CameraHome + CurrentOffset + FVector(0.0, Wobble * 2.1, Wobble));
				Camera->SetRelativeRotation(CameraHomeRotation + FRotator(Wobble * 0.45, 0.0, Wobble * 0.75));
				Camera->SetFieldOfView(FMath::Clamp(CameraHomeFov + CurrentFov, 40.f, 130.f));
			}
		}
	}
	if (!bReacting && !bCameraCaptured)
	{
		SetComponentTickEnabled(false);
	}
}

ANinjaImpactBurst::ANinjaImpactBurst()
{
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Flash = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Flash"));
	Flash->SetupAttachment(Root);
	Wave = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Shockwave"));
	Wave->SetupAttachment(Root);
	SetCanBeDamaged(false);
	for (UStaticMeshComponent* Mesh : { Flash.Get(), static_cast<UStaticMeshComponent*>(Wave.Get()) })
	{
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCastShadow(false);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ColorMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	Flash->SetStaticMesh(Sphere.Object);
	Wave->SetStaticMesh(Cube.Object);
	Flash->SetMaterial(0, ColorMaterial.Object);
	Wave->SetMaterial(0, ColorMaterial.Object);
}

void ANinjaImpactBurst::Configure(ENinjaImpactKind Kind)
{
	const FNinjaImpactStyle Style = GetImpactStyle(Kind);
	StartTime = GetWorld()->GetTimeSeconds();
	FlashSize = Style.Flash;
	WaveRadius = Style.Wave;
	EffectDuration = WaveRadius > 0.f ? (Kind == ENinjaImpactKind::Ultimate ? 0.62f : 0.44f) : 0.17f;
	Flash->SetRelativeScale3D(FVector(FlashSize));
	if (UMaterialInstanceDynamic* Tint = Flash->CreateDynamicMaterialInstance(0))
	{
		Tint->SetVectorParameterValue(TEXT("Color"), Style.Color);
	}
	if (WaveRadius > 0.f)
	{
		Wave->SetRelativeScale3D(FVector(0.42, 0.42, 1.0));
		const FVector Origin = GetActorLocation();
		FHitResult Floor;
		const FCollisionQueryParams Query(SCENE_QUERY_STAT(NinjaImpactFloor), false, this);
		if (GetWorld()->LineTraceSingleByChannel(Floor, Origin, Origin - FVector(0.0, 0.0, 320.0), ECC_Visibility, Query))
		{
			Wave->SetWorldLocation(Floor.ImpactPoint + FVector(0.0, 0.0, 6.0));
		}
		if (UMaterialInstanceDynamic* Tint = Wave->CreateDynamicMaterialInstance(0))
		{
			Tint->SetVectorParameterValue(TEXT("Color"), Style.Color);
		}
		for (int32 Segment = 0; Segment < 16; ++Segment)
		{
			const double Angle = Segment * 22.5;
			const FVector Radial = FRotator(0.0, Angle, 0.0).Vector();
			Wave->AddInstance(FTransform(FRotator(0.0, Angle + 90.0, 0.0), Radial * 50.0, FVector(0.196, 0.035, 0.022)));
		}
	}
	SetLifeSpan(EffectDuration);
}

void ANinjaImpactBurst::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const double Age = GetWorld()->GetTimeSeconds() - StartTime;
	const double FlashAmount = FMath::Clamp(1.0 - Age / 0.15, 0.0, 1.0);
	Flash->SetVisibility(FlashAmount > 0.0, false);
	Flash->SetRelativeScale3D(FVector(FMath::Max(0.01, FlashSize * FlashAmount)));
	if (WaveRadius > 0.f)
	{
		const double Progress = FMath::Clamp(Age / EffectDuration, 0.0, 1.0);
		const double Scale = FMath::Lerp(0.42, WaveRadius / 50.0, FMath::InterpEaseOut(0.0, 1.0, Progress, 2.0));
		Wave->SetWorldScale3D(FVector(Scale, Scale, 1.0));
	}
}
