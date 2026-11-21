#include "NinjaCombatStyle.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "AnimationRuntime.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NINJA_ARENACharacter.h"
#include "NinjaFighterAnimInstance.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Misc/App.h"
#include "TimerManager.h"
#include "Sound/SoundBase.h"
#include "Retargeter/IKRetargeter.h"
#include "UObject/ConstructorHelpers.h"

UNinjaCombatStyleComponent::UNinjaCombatStyleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.075f;

	static ConstructorHelpers::FObjectFinder<UAnimSequence> First(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Second(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02.MM_Attack_02"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Third(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03.MM_Attack_03"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Charged(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack.MM_ChargedAttack"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Dash(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Dash.MM_Dash"));
	LightStrikes = { First.Object.Get(), Second.Object.Get(), Third.Object.Get() };
	HeavyStrike = Charged.Object;
	DashPose = Dash.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> LightSound(TEXT("/Game/NinjaArena/Audio/SW_NinjaLight.SW_NinjaLight"));
	static ConstructorHelpers::FObjectFinder<USoundBase> HeavySound(TEXT("/Game/NinjaArena/Audio/SW_NinjaHeavy.SW_NinjaHeavy"));
	static ConstructorHelpers::FObjectFinder<USoundBase> DashSound(TEXT("/Game/NinjaArena/Audio/SW_NinjaDash.SW_NinjaDash"));
	static ConstructorHelpers::FObjectFinder<USoundBase> EnergySound(TEXT("/Game/NinjaArena/Audio/SW_NinjaEnergy.SW_NinjaEnergy"));
	static ConstructorHelpers::FObjectFinder<USoundBase> UltimateSound(TEXT("/Game/NinjaArena/Audio/SW_NinjaUltimate.SW_NinjaUltimate"));
	static ConstructorHelpers::FObjectFinder<USoundBase> StepSound(TEXT("/Game/NinjaArena/Audio/SW_NinjaStep.SW_NinjaStep"));
	Sounds = { LightSound.Object.Get(), HeavySound.Object.Get(), DashSound.Object.Get(), EnergySound.Object.Get(),
		UltimateSound.Object.Get(), StepSound.Object.Get(), DashSound.Object.Get() };

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Basic(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	SmoothPart = Sphere.Object;
	FlatPart = Cube.Object;
	PartMaterial = Basic.Object;
}

void UNinjaCombatStyleComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyCostume();
}

const FNinjaFighterProfile& UNinjaCombatStyleComponent::GetProfile(ENinjaFighter Fighter)
{
	static const FNinjaFighterProfile Junco{
		TEXT("JUNCO"), FLinearColor(0.027f, 0.135f, 0.115f), FLinearColor(0.10f, 0.90f, 0.63f),
		FLinearColor(0.021f, 0.037f, 0.045f), TEXT("Junco"), {}, {1.28f, 1.28f, 1.28f}, 1.f, 1.f
	};
	static const FNinjaFighterProfile Ascua{
		TEXT("ASCUA"), FLinearColor(0.19f, 0.035f, 0.018f), FLinearColor(1.f, 0.33f, 0.095f),
		FLinearColor(0.042f, 0.021f, 0.022f), TEXT("Ascua"), {}, {1.28f, 1.28f, 1.28f}, 1.f, 1.f
	};
	static const FNinjaFighterProfile Kuroboshi{
		TEXT("KUROBOSHI"), FLinearColor(0.025f, 0.035f, 0.095f), FLinearColor(0.24f, 0.68f, 1.f),
		FLinearColor(0.013f, 0.020f, 0.044f), TEXT("Kuroboshi"), {0.92f, 1.25f, 1.80f}, {1.48f, 1.36f, 1.14f}, 1.f, 1.f
	};
	static const FNinjaFighterProfile Raizen{
		TEXT("RAIZEN"), FLinearColor(0.11f, 0.052f, 0.023f), FLinearColor(1.f, 0.58f, 0.15f),
		FLinearColor(0.055f, 0.060f, 0.066f), TEXT("Raizen"), {1.06f, 1.18f, 1.44f}, {1.18f, 1.12f, 1.02f}, 1.16f, 1.22f
	};
	static const FNinjaFighterProfile AkiraVioleta{
		TEXT("AKIRA VIOLETA"), FLinearColor(0.115f, 0.024f, 0.26f), FLinearColor(0.78f, 0.32f, 1.f),
		FLinearColor(0.028f, 0.012f, 0.056f), TEXT("AkiraVioleta"), {0.82f, 1.02f, 1.32f}, {1.72f, 1.64f, 1.5f}, 0.83f, 0.88f
	};
	static const FNinjaFighterProfile Byakuro{
		TEXT("BYAKURO"), FLinearColor(0.095f, 0.10f, 0.13f), FLinearColor(0.79f, 0.93f, 1.f),
		FLinearColor(0.62f, 0.65f, 0.70f), TEXT("Byakuro"), {1.02f, 1.08f, 1.5f}, {1.24f, 1.18f, 1.15f}, 1.10f, 1.16f
	};
	static const FNinjaFighterProfile TripoA{
		TEXT("TRIPO A"), FLinearColor(0.14f, 0.075f, 0.04f), FLinearColor(1.f, 0.72f, 0.38f),
		FLinearColor(0.09f, 0.06f, 0.05f), TEXT("TripoA"), {0.95f, 1.11f, 1.42f}, {1.32f, 1.35f, 1.25f}, 0.98f, 1.f, TEXT("TripoA")
	};
	static const FNinjaFighterProfile TripoB{
		TEXT("TRIPO B"), FLinearColor(0.08f, 0.11f, 0.23f), FLinearColor(0.52f, 0.72f, 1.f),
		FLinearColor(0.05f, 0.07f, 0.13f), TEXT("TripoB"), {0.89f, 1.05f, 1.64f}, {1.45f, 1.34f, 1.16f}, 1.04f, 1.10f, TEXT("TripoB")
	};
	switch (Fighter)
	{
	case ENinjaFighter::Ascua: return Ascua;
	case ENinjaFighter::Kuroboshi: return Kuroboshi;
	case ENinjaFighter::Raizen: return Raizen;
	case ENinjaFighter::AkiraVioleta: return AkiraVioleta;
	case ENinjaFighter::Byakuro: return Byakuro;
	case ENinjaFighter::TripoA: return TripoA;
	case ENinjaFighter::TripoB: return TripoB;
	default: return Junco;
	}
}

void UNinjaCombatStyleComponent::SetFighter(ENinjaFighter NewFighter)
{
	if (FighterId == NewFighter)
	{
		return;
	}
	FighterId = NewFighter;
	if (!HasBegunPlay())
	{
		return;
	}
	for (UStaticMeshComponent* Part : CostumeParts)
	{
		if (IsValid(Part))
		{
			GetOwner()->RemoveInstanceComponent(Part);
			Part->DestroyComponent();
		}
	}
	CostumeParts.Reset();
	ApplyCostume();
}

void UNinjaCombatStyleComponent::ApplyCostume()
{
	USkeletalMeshComponent* Body = CastChecked<ANINJA_ARENACharacter>(GetOwner())->GetMesh();
	RemoveFighterModel(*Body);
	if (ApplyFighterModel(*Body))
	{
		TintedSlotCount = 0;
		return;
	}
	const FNinjaFighterProfile& Profile = GetProfile();
	const bool bAscua = FighterId == ENinjaFighter::Ascua;
	const FName PaintTint(TEXT("Paint Tint"));
	TintedSlotCount = 0;
	for (int32 Slot = 0; Slot < Body->GetNumMaterials(); ++Slot)
	{
		UMaterialInterface* Material = Body->GetMaterial(Slot);
		FLinearColor Existing;
		if (Material && Material->GetVectorParameterValue(FMaterialParameterInfo(PaintTint), Existing))
		{
			if (UMaterialInstanceDynamic* Tint = Body->CreateDynamicMaterialInstance(Slot))
			{
				Tint->SetVectorParameterValue(PaintTint, Profile.Armor);
				Tint->SetVectorParameterValue(TEXT("LogoTint"), Profile.Mask);
				++TintedSlotCount;
			}
		}
	}

	AddCostumePart(SmoothPart, TEXT("head"), FVector(6.0, 0.0, 4.6), FVector(0.170, 0.192, 0.155), Profile.Mask);
	AddCostumePart(FlatPart, TEXT("head"), FVector(13.8, 0.0, 8.1), FVector(0.025, 0.154, 0.030), Profile.Accent);
	AddCostumePart(SmoothPart, TEXT("neck_01"), FVector(0.0, 0.0, 0.5), FVector(0.23, 0.31, 0.09), Profile.Armor);
	AddCostumePart(SmoothPart, bAscua ? TEXT("upperarm_r") : TEXT("upperarm_l"),
		FVector(0.0, bAscua ? 3.3 : -3.3, 5.0), FVector(0.21, 0.128, 0.098), Profile.Accent);
	if (FighterId == ENinjaFighter::Kuroboshi)
	{
		AddCostumePart(FlatPart, TEXT("hand_r"), FVector(0, 0, -6), FVector(0.045, 0.045, 0.16), Profile.Mask);
		AddCostumePart(FlatPart, TEXT("hand_r"), FVector(0, 0, -15), FVector(0.12, 0.21, 0.035), Profile.Armor);
		AddCostumePart(FlatPart, TEXT("hand_r"), FVector(0, 0, -46), FVector(0.025, 0.055, 0.60), Profile.Accent);
		AddCostumePart(FlatPart, TEXT("hand_r"), FVector(0, 1.5, -45), FVector(0.013, 0.028, 0.57), Profile.Mask);
		AddCostumePart(SmoothPart, TEXT("spine_03"), FVector(10, 0, 0), FVector(0.075, 0.24, 0.19), Profile.Armor);
		AddCostumePart(FlatPart, TEXT("lowerarm_l"), FVector(1, -3, 0), FVector(0.14, 0.035, 0.13), Profile.Armor);
		AddCostumePart(FlatPart, TEXT("lowerarm_r"), FVector(1, 3, 0), FVector(0.14, 0.035, 0.13), Profile.Armor);
	}
	else if (FighterId == ENinjaFighter::Raizen)
	{
		AddCostumePart(SmoothPart, TEXT("spine_03"), FVector(8, 0, 0), FVector(0.14, 0.28, 0.25), Profile.Mask);
		AddCostumePart(SmoothPart, TEXT("upperarm_r"), FVector(0, 3, 5), FVector(0.26, 0.18, 0.13), Profile.Accent);
		AddCostumePart(SmoothPart, TEXT("upperarm_l"), FVector(0, -3, 5), FVector(0.26, 0.18, 0.13), Profile.Accent);
		AddCostumePart(FlatPart, TEXT("lowerarm_r"), FVector(4, 2, 0), FVector(0.20, 0.13, 0.15), Profile.Armor);
		AddCostumePart(FlatPart, TEXT("lowerarm_l"), FVector(4, -2, 0), FVector(0.20, 0.13, 0.15), Profile.Armor);
		AddCostumePart(FlatPart, TEXT("spine_01"), FVector(5, 0, 0), FVector(0.16, 0.31, 0.07), Profile.Accent);
	}
	else if (FighterId == ENinjaFighter::AkiraVioleta)
	{
		AddCostumePart(SmoothPart, TEXT("neck_01"), FVector(-12, -4, -5), FVector(0.32, 0.075, 0.075), Profile.Accent);
		AddCostumePart(SmoothPart, TEXT("neck_01"), FVector(-16, 4, -9), FVector(0.36, 0.065, 0.065), Profile.Armor);
		AddCostumePart(SmoothPart, TEXT("calf_l"), FVector(4, 0, -4), FVector(0.10, 0.12, 0.29), Profile.Armor);
		AddCostumePart(SmoothPart, TEXT("calf_r"), FVector(4, 0, -4), FVector(0.10, 0.12, 0.29), Profile.Armor);
		AddCostumePart(FlatPart, TEXT("thigh_l"), FVector(2, -7, 0), FVector(0.18, 0.035, 0.045), Profile.Accent);
		AddCostumePart(FlatPart, TEXT("thigh_r"), FVector(2, 7, 0), FVector(0.18, 0.035, 0.045), Profile.Accent);
	}
	else if (FighterId == ENinjaFighter::Byakuro)
	{
		AddCostumePart(SmoothPart, TEXT("head"), FVector(-5, 0, 1), FVector(0.23, 0.24, 0.19), Profile.Mask);
		AddCostumePart(SmoothPart, TEXT("spine_03"), FVector(-10, 0, 7), FVector(0.15, 0.26, 0.26), Profile.Mask);
		for (const FName Hand : {FName(TEXT("hand_l")), FName(TEXT("hand_r"))})
		{
			for (int32 Claw = -1; Claw <= 1; ++Claw)
			{
				AddCostumePart(FlatPart, Hand, FVector(9, Claw * 2.8, -3), FVector(0.13, 0.012, 0.015), Profile.Accent);
			}
		}
	}
}

bool UNinjaCombatStyleComponent::ApplyFighterModel(USkeletalMeshComponent& Body)
{
	const TCHAR* Model = GetProfile().ModelFolder;
	const USkeletalMesh* Source = Body.GetSkeletalMeshAsset();
	if (!Model || !Source)
	{
		return false;
	}
	const FString Folder = FString::Printf(TEXT("/Game/NinjaArena/ModelosJugables/%s"), Model);
	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("%s/SK_%s.SK_%s"), *Folder, Model, Model));
	UIKRetargeter* Retarget = LoadObject<UIKRetargeter>(nullptr, *FString::Printf(TEXT("%s/RTG_Ninja_a_%s.RTG_Ninja_a_%s"), *Folder, Model, Model));
	if (!Mesh || !Retarget || Mesh->GetBounds().BoxExtent.Z <= UE_KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const auto FacingYaw = [](const USkeletalMesh& Asset)
	{
		const FReferenceSkeleton& Skeleton = Asset.GetRefSkeleton();
		const auto Thigh = [&Skeleton](bool bLeft) -> int32
		{
			const FString Mixamo = bLeft ? TEXT("LeftUpLeg") : TEXT("RightUpLeg");
			const FString UE = bLeft ? TEXT("thigh_l") : TEXT("thigh_r");
			const FString Tripo = bLeft ? TEXT("L_Thigh") : TEXT("R_Thigh");
			for (int32 Bone = 0; Bone < Skeleton.GetNum(); ++Bone)
			{
				const FString Name = Skeleton.GetBoneName(Bone).ToString();
				if (Name == UE || Name == Tripo || Name.EndsWith(Mixamo))
				{
					return Bone;
				}
			}
			return INDEX_NONE;
		};
		const int32 Left = Thigh(true);
		const int32 Right = Thigh(false);
		if (Left == INDEX_NONE || Right == INDEX_NONE)
		{
			return TOptional<double>();
		}
		const FVector Across = FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, Right).GetLocation()
			- FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, Left).GetLocation();
		return TOptional<double>(FVector::CrossProduct(Across, FVector::UpVector).Rotation().Yaw);
	};

	const auto SourceBounds = Source->GetBounds();
	const auto TargetBounds = Mesh->GetBounds();
	const double Scale = SourceBounds.BoxExtent.Z / TargetBounds.BoxExtent.Z;
	const double Bottom = SourceBounds.Origin.Z - SourceBounds.BoxExtent.Z
		- (TargetBounds.Origin.Z - TargetBounds.BoxExtent.Z) * Scale;
	const TOptional<double> SourceYaw = FacingYaw(*Source);
	const TOptional<double> TargetYaw = FacingYaw(*Mesh);
	const double Yaw = SourceYaw.IsSet() && TargetYaw.IsSet() ? FMath::UnwindDegrees(SourceYaw.GetValue() - TargetYaw.GetValue()) : 0.0;

	USkeletalMeshComponent* Visual = NewObject<USkeletalMeshComponent>(GetOwner());
	Visual->SetupAttachment(&Body);
	Visual->SetRelativeTransform(FTransform(FRotator(0.0, Yaw, 0.0), FVector(0.0, 0.0, Bottom), FVector(Scale)));
	Visual->SetCollisionProfileName(TEXT("NoCollision"));
	Visual->SetGenerateOverlapEvents(false);
	Visual->SetSkeletalMesh(Mesh);
	Visual->SetAnimInstanceClass(UNinjaFighterAnimInstance::StaticClass());
	Visual->AddTickPrerequisiteComponent(&Body);
	Visual->RegisterComponent();
	UNinjaFighterAnimInstance* Instance = Cast<UNinjaFighterAnimInstance>(Visual->GetAnimInstance());
	if (!Instance)
	{
		Visual->DestroyComponent();
		return false;
	}
	Instance->SetRetargetAsset(Retarget);
	OriginalTickOption = Body.VisibilityBasedAnimTickOption;
	bOriginalVisible = Body.IsVisible();
	Body.VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Body.SetVisibility(false);
	FighterModel = Visual;
	return true;
}

void UNinjaCombatStyleComponent::RemoveFighterModel(USkeletalMeshComponent& Body)
{
	if (FighterModel)
	{
		FighterModel->DestroyComponent();
		FighterModel = nullptr;
		Body.VisibilityBasedAnimTickOption = OriginalTickOption;
		Body.SetVisibility(bOriginalVisible);
	}
}

void UNinjaCombatStyleComponent::AddCostumePart(UStaticMesh* Shape, FName Bone, const FVector& Offset, const FVector& Scale,
	const FLinearColor& Color)
{
	ACharacter* Fighter = CastChecked<ACharacter>(GetOwner());
	USkeletalMeshComponent* Body = Fighter->GetMesh();
	if (!Shape || !Body->DoesSocketExist(Bone))
	{
		return;
	}

	const FTransform BoneWorld = Body->GetSocketTransform(Bone, RTS_World);
	const FQuat ActorRotation = Fighter->GetActorQuat();
	const FTransform StyleInWorld(ActorRotation, BoneWorld.GetLocation() + ActorRotation.RotateVector(Offset), Scale);
	UStaticMeshComponent* Accessory = NewObject<UStaticMeshComponent>(Fighter);
	Fighter->AddInstanceComponent(Accessory);
	Accessory->SetupAttachment(Body, Bone);
	Accessory->SetMobility(EComponentMobility::Movable);
	Accessory->SetStaticMesh(Shape);
	Accessory->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Accessory->SetGenerateOverlapEvents(false);
	Accessory->SetCanEverAffectNavigation(false);
	Accessory->SetCastShadow(false);
	Accessory->SetRelativeTransform(StyleInWorld.GetRelativeTransform(BoneWorld));
	Accessory->SetMaterial(0, PartMaterial);
	Accessory->RegisterComponent();
	if (UMaterialInstanceDynamic* Tint = Accessory->CreateDynamicMaterialInstance(0))
	{
		Tint->SetVectorParameterValue(TEXT("Color"), Color);
	}
	CostumeParts.Add(Accessory);
}

void UNinjaCombatStyleComponent::PlayPose(UAnimSequenceBase* Sequence, float Rate, float StartTime)
{
	const ACharacter* Fighter = CastChecked<ACharacter>(GetOwner());
	const USkeletalMeshComponent* Body = Fighter->GetMesh();
	const USkeletalMesh* SkeletalMesh = Body->GetSkeletalMeshAsset();
	UAnimInstance* Instance = Body->GetAnimInstance();
	if (!Sequence || !Instance || !SkeletalMesh || Sequence->GetSkeleton() != SkeletalMesh->GetSkeleton())
	{
		return;
	}
	StopActionPose(0.065f);
	static const FName DefaultSlot(TEXT("DefaultSlot"));
	if (UAnimMontage* Montage = Instance->PlaySlotAnimationAsDynamicMontage(Sequence, DefaultSlot, 0.035f, 0.12f,
		Rate, 1, -1.f, StartTime))
	{
		if (FAnimMontageInstance* Active = Instance->GetActiveInstanceForMontage(Montage))
		{
			Active->PushDisableRootMotion();
		}
		ActionMontage = Montage;
		LastSequence = Sequence;
	}
}

void UNinjaCombatStyleComponent::PlayStrike(int32 Step, bool bHeavy, float GroundWindup)
{
	const int32 Index = FMath::Clamp(Step - 1, 0, 2);
	if (GetProfile().ModelFolder && GroundWindup > 0.f)
	{
		static const TCHAR* Capoeira[] = {TEXT("pontera"), TEXT("martelo_2"), TEXT("armada"), TEXT("chapa_giratoria")};
		static const float ContactFraction[] = {0.43f, 0.45f, 0.47f, 0.48f};
		const int32 Clip = bHeavy ? 3 : Index;
		const FString Name = FString::Printf(TEXT("NA_AN_%s"), Capoeira[Clip]);
		const FString Path = FString::Printf(TEXT("/Game/NinjaArena/ModelosJugables/Capoeira/%s.%s"), *Name, *Name);
		if (UAnimSequence* Sequence = LoadObject<UAnimSequence>(nullptr, *Path))
		{
			const float Rate = bHeavy ? 1.45f : GetProfile().LightPlayRate[Index];
			const float Contact = Sequence->GetPlayLength() * ContactFraction[Clip];
			PlayPose(Sequence, Rate, FMath::Max(0.f, Contact - GroundWindup * Rate));
			return;
		}
	}
	const bool bSpectral = FighterId == ENinjaFighter::Kuroboshi;
	PlayPose(bHeavy ? HeavyStrike.Get() : LightStrikes[Index].Get(),
		bHeavy ? 1.48f / GetProfile().RecoveryScale : GetProfile().LightPlayRate[Index],
		bHeavy ? 0.20f : (bSpectral ? (Index == 2 ? 0.04f : 0.09f) : 0.07f));
}

void UNinjaCombatStyleComponent::PlayBladeTrace()
{
	if (FighterId != ENinjaFighter::Kuroboshi || !FApp::CanEverRender())
	{
		return;
	}
	static const TSoftObjectPtr<UNiagaraSystem> Trail(FSoftObjectPath(TEXT(
		"/Game/SwordTrailVFX/VFX/NS_Trail_01.NS_Trail_01")));
	USkeletalMeshComponent* Mesh = CastChecked<ACharacter>(GetOwner())->GetMesh();
	UNiagaraSystem* System = Trail.LoadSynchronous();
	if (!System || !Mesh->DoesSocketExist(TEXT("hand_r")))
	{
		return;
	}
	if (UNiagaraComponent* Effect = UNiagaraFunctionLibrary::SpawnSystemAttached(System, Mesh, TEXT("hand_r"),
		FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true))
	{
		FTimerHandle Cleanup;
		GetWorld()->GetTimerManager().SetTimer(Cleanup,
			FTimerDelegate::CreateWeakLambda(Effect, [Effect]() { Effect->DestroyComponent(); }), 0.45f, false);
	}
}

void UNinjaCombatStyleComponent::PlayDash()
{
	PlayPose(DashPose.Get(), 1.6f);
	PlaySound(ENinjaStyleSound::Dash, GetOwner()->GetActorLocation());
}

void UNinjaCombatStyleComponent::PlayCharge()
{
	PlayPose(HeavyStrike.Get(), 1.22f);
	PlaySound(ENinjaStyleSound::Energy, GetOwner()->GetActorLocation());
}

void UNinjaCombatStyleComponent::PlayWindup(bool bHeavy)
{
	PlayPose(bHeavy ? HeavyStrike.Get() : LightStrikes[0].Get(), bHeavy ? 0.34f : 0.30f);
}

void UNinjaCombatStyleComponent::StopActionPose(float BlendTime)
{
	if (UAnimMontage* Montage = ActionMontage.Get())
	{
		if (UAnimInstance* Instance = CastChecked<ACharacter>(GetOwner())->GetMesh()->GetAnimInstance())
		{
			Instance->Montage_Stop(BlendTime, Montage);
		}
	}
	ActionMontage.Reset();
	LastSequence.Reset();
}

const USoundBase* UNinjaCombatStyleComponent::GetSound(ENinjaStyleSound Sound) const
{
	const int32 Index = static_cast<int32>(Sound);
	return Sounds.IsValidIndex(Index) ? Sounds[Index].Get() : nullptr;
}

void UNinjaCombatStyleComponent::PlaySound(ENinjaStyleSound Sound, const FVector& Location)
{
	const int32 Index = static_cast<int32>(Sound);
	const UWorld* World = GetWorld();
	if (!World || !Sounds.IsValidIndex(Index) || !Sounds[Index])
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	if (Sound != ENinjaStyleSound::Step)
	{
		if (Now < NextSound)
		{
			return;
		}
		NextSound = Now + 0.035;
	}
	const float Volume = (Sound == ENinjaStyleSound::Step || Sound == ENinjaStyleSound::Swing) ? 0.31f : 0.66f;
	const float Pitch = Sound == ENinjaStyleSound::Swing ? 1.30f : (FighterId == ENinjaFighter::Ascua ? 0.94f : 1.04f);
	UGameplayStatics::PlaySoundAtLocation(this, Sounds[Index].Get(), Location, Volume, Pitch);
}

void UNinjaCombatStyleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const ANINJA_ARENACharacter* Fighter = CastChecked<ANINJA_ARENACharacter>(GetOwner());
	const double Now = GetWorld()->GetTimeSeconds();
	const double Speed = Fighter->GetVelocity().Size2D();
	if (Now >= NextFootstep && Speed >= 120.0 && Fighter->GetCharacterMovement()->IsMovingOnGround() && !Fighter->IsCombatLocked())
	{
		const double Pace = FMath::GetMappedRangeValueClamped(FVector2D(120.0, 800.0), FVector2D(0.54, 0.27), Speed);
		NextFootstep = Now + Pace;
		PlaySound(ENinjaStyleSound::Step, Fighter->GetActorLocation() - FVector(0.0, 0.0, 85.0));
	}
}
