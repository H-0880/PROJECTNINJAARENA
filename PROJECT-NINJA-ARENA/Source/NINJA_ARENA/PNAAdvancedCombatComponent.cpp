#include "PNAAdvancedCombatComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

UPNAAdvancedCombatComponent::UPNAAdvancedCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UPNAAdvancedCombatComponent::BeginPlay()
{
    Super::BeginPlay();
    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
        OwnerMesh = Character->GetMesh();
    }
}

void UPNAAdvancedCombatComponent::BeginTraceWindow(FName StartSocket, FName EndSocket, float Radius)
{
    TraceStartSocket = StartSocket;
    TraceEndSocket = EndSocket.IsNone() ? StartSocket : EndSocket;
    TraceRadius = FMath::Max(1.f, Radius);
    HitActorsThisWindow.Reset();
    bTraceActive = true;
    bTraceInitialized = false;
    SetComponentTickEnabled(true);
}

void UPNAAdvancedCombatComponent::EndTraceWindow()
{
    bTraceActive = false;
    bTraceInitialized = false;
    HitActorsThisWindow.Reset();
    SetComponentTickEnabled(false);
}

void UPNAAdvancedCombatComponent::SetInvulnerable(bool bEnabled)
{
    bInvulnerable = bEnabled;
}

void UPNAAdvancedCombatComponent::SetHyperArmor(bool bEnabled)
{
    bHyperArmor = bEnabled;
}

void UPNAAdvancedCombatComponent::SetParryWindow(bool bEnabled)
{
    bParryWindow = bEnabled;
}

EPNAAdvancedDefenseResult UPNAAdvancedCombatComponent::EvaluateIncomingMelee(AActor* Attacker)
{
    if (bInvulnerable)
    {
        return EPNAAdvancedDefenseResult::Invulnerable;
    }
    if (bParryWindow && IsValid(Attacker))
    {
        bParryWindow = false; // One successful parry per window.
        OnParryTriggered.Broadcast(Attacker);
        return EPNAAdvancedDefenseResult::Parried;
    }
    if (bHyperArmor)
    {
        return EPNAAdvancedDefenseResult::Armored;
    }
    return EPNAAdvancedDefenseResult::None;
}

void UPNAAdvancedCombatComponent::TraceSegment(const FVector& Previous, const FVector& Current)
{
    UWorld* World = GetWorld();
    if (!World || Previous.Equals(Current, 0.01f))
    {
        return;
    }

    FCollisionQueryParams Params(SCENE_QUERY_STAT(PNAAdvancedWeaponTrace), false, GetOwner());
    TArray<FHitResult> Hits;
    const FCollisionShape Shape = FCollisionShape::MakeSphere(TraceRadius);
    World->SweepMultiByChannel(Hits, Previous, Current, FQuat::Identity, TraceChannel, Shape, Params);

    if (bDebugTrace)
    {
        DrawDebugLine(World, Previous, Current, FColor::Cyan, false, 0.15f, 0, 1.5f);
        DrawDebugSphere(World, Current, TraceRadius, 12, FColor::Cyan, false, 0.15f);
    }

    for (const FHitResult& Hit : Hits)
    {
        AActor* HitActor = Hit.GetActor();
        if (!IsValid(HitActor) || HitActor == GetOwner() || HitActorsThisWindow.Contains(TWeakObjectPtr<AActor>(HitActor)))
        {
            continue;
        }
        HitActorsThisWindow.Add(TWeakObjectPtr<AActor>(HitActor));
        OnTraceHit.Broadcast(HitActor, Hit);
    }
}

void UPNAAdvancedCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bTraceActive || !OwnerMesh.IsValid())
    {
        return;
    }

    USkeletalMeshComponent* Mesh = OwnerMesh.Get();
    const FVector CurrentStart = Mesh->GetSocketLocation(TraceStartSocket);
    const FVector CurrentEnd = Mesh->GetSocketLocation(TraceEndSocket);

    if (!bTraceInitialized)
    {
        PreviousStart = CurrentStart;
        PreviousEnd = CurrentEnd;
        bTraceInitialized = true;
        return;
    }

    TraceSegment(PreviousStart, CurrentStart);
    if (TraceEndSocket != TraceStartSocket)
    {
        TraceSegment(PreviousEnd, CurrentEnd);
        TraceSegment(CurrentStart, CurrentEnd); // Covers the weapon span on this frame.
    }

    PreviousStart = CurrentStart;
    PreviousEnd = CurrentEnd;
}
