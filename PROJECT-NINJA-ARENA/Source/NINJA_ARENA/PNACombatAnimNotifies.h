#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "PNACombatAnimNotifies.generated.h"

UCLASS(meta=(DisplayName="PNA Weapon Trace Window"))
class NINJA_ARENA_API UANS_PNAWeaponTrace : public UAnimNotifyState
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Trace") FName StartSocket = TEXT("hand_r");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Trace") FName EndSocket = TEXT("hand_r");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Trace", meta=(ClampMin="1.0")) float Radius = 12.f;
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};

UCLASS(meta=(DisplayName="PNA I-Frames"))
class NINJA_ARENA_API UANS_PNAIFrames : public UAnimNotifyState
{
    GENERATED_BODY()
public:
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};

UCLASS(meta=(DisplayName="PNA Hyper Armor"))
class NINJA_ARENA_API UANS_PNAHyperArmor : public UAnimNotifyState
{
    GENERATED_BODY()
public:
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};

UCLASS(meta=(DisplayName="PNA Parry Window"))
class NINJA_ARENA_API UANS_PNAParryWindow : public UAnimNotifyState
{
    GENERATED_BODY()
public:
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
