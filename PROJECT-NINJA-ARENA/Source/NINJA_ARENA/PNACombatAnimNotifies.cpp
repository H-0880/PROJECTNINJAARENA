#include "PNACombatAnimNotifies.h"

#include "GameFramework/Actor.h"
#include "PNAAdvancedCombatComponent.h"

static UPNAAdvancedCombatComponent* GetPNACombatExtension(USkeletalMeshComponent* MeshComp)
{
    AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
    return Owner ? Owner->FindComponentByClass<UPNAAdvancedCombatComponent>() : nullptr;
}

void UANS_PNAWeaponTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
    if (UPNAAdvancedCombatComponent* Combat = GetPNACombatExtension(MeshComp))
    {
        Combat->BeginTraceWindow(StartSocket, EndSocket, Radius);
    }
}

void UANS_PNAWeaponTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);
    if (UPNAAdvancedCombatComponent* Combat = GetPNACombatExtension(MeshComp)) Combat->EndTraceWindow();
}

void UANS_PNAIFrames::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
    if (UPNAAdvancedCombatComponent* Combat = GetPNACombatExtension(MeshComp)) Combat->SetInvulnerable(true);
}
void UANS_PNAIFrames::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);
    if (UPNAAdvancedCombatComponent* Combat = GetPNACombatExtension(MeshComp)) Combat->SetInvulnerable(false);
}

void UANS_PNAHyperArmor::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
    if (UPNAAdvancedCombatComponent* Combat = GetPNACombatExtension(MeshComp)) Combat->SetHyperArmor(true);
}
void UANS_PNAHyperArmor::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);
    if (UPNAAdvancedCombatComponent* Combat = GetPNACombatExtension(MeshComp)) Combat->SetHyperArmor(false);
}

void UANS_PNAParryWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
    if (UPNAAdvancedCombatComponent* Combat = GetPNACombatExtension(MeshComp)) Combat->SetParryWindow(true);
}
void UANS_PNAParryWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);
    if (UPNAAdvancedCombatComponent* Combat = GetPNACombatExtension(MeshComp)) Combat->SetParryWindow(false);
}
