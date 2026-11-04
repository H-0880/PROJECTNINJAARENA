#include "PNALockOnComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

UPNALockOnComponent::UPNALockOnComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UPNALockOnComponent::ToggleLockOn()
{
    if (HasTarget())
    {
        ClearTarget();
        return;
    }

    if (AActor* Best = FindBestTarget())
    {
        Target = Best;
        SetStrafeMode(true);
        SetComponentTickEnabled(true);
    }
}

void UPNALockOnComponent::ClearTarget()
{
    Target.Reset();
    SetStrafeMode(false);
    SetComponentTickEnabled(false);
}

AActor* UPNALockOnComponent::FindBestTarget() const
{
    const APawn* Pawn = Cast<APawn>(GetOwner());
    if (!Pawn || !Pawn->GetController() || !GetWorld()) return nullptr;

    const FVector Origin = Pawn->GetActorLocation();
    const FVector AimDir = Pawn->GetController()->GetControlRotation().Vector();
    const float CosLimit = FMath::Cos(FMath::DegreesToRadians(MaxAcquireAngleDegrees));

    TArray<AActor*> Candidates;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), TargetTag, Candidates);

    AActor* Best = nullptr;
    float BestScore = TNumericLimits<float>::Max();

    for (AActor* Candidate : Candidates)
    {
        if (!Candidate || Candidate == GetOwner()) continue;

        const FVector ToTarget = Candidate->GetActorLocation() - Origin;
        const float Distance = ToTarget.Size();
        if (Distance > AcquireDistance || Distance < KINDA_SMALL_NUMBER) continue;

        const float Dot = FVector::DotProduct(AimDir, ToTarget / Distance);
        if (Dot < CosLimit) continue;

        FHitResult Hit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(PNALockOnLOS), false, GetOwner());
        Params.AddIgnoredActor(Candidate);
        if (GetWorld()->LineTraceSingleByChannel(Hit, Origin, Candidate->GetActorLocation(), ECC_Visibility, Params))
        {
            continue; // blocked
        }

        // Lower is better: prefer centered targets, then closer ones.
        const float Score = (1.f - Dot) * 2000.f + Distance;
        if (Score < BestScore)
        {
            BestScore = Score;
            Best = Candidate;
        }
    }
    return Best;
}

void UPNALockOnComponent::SetStrafeMode(bool bEnabled)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    UCharacterMovementComponent* Move = Character ? Character->GetCharacterMovement() : nullptr;
    if (!Move) return;

    if (bEnabled)
    {
        bSavedOrientToMovement = Move->bOrientRotationToMovement;
        bSavedUseDesiredRotation = Move->bUseControllerDesiredRotation;
        Move->bOrientRotationToMovement = false;
        Move->bUseControllerDesiredRotation = true;
    }
    else
    {
        Move->bOrientRotationToMovement = bSavedOrientToMovement;
        Move->bUseControllerDesiredRotation = bSavedUseDesiredRotation;
    }
}

void UPNALockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    const APawn* Pawn = Cast<APawn>(GetOwner());
    AController* Controller = Pawn ? Pawn->GetController() : nullptr;
    if (!Controller || !Target.IsValid())
    {
        ClearTarget();
        return;
    }

    const FVector TargetLocation = Target->GetActorLocation() + FVector(0.f, 0.f, AimHeightOffset);
    if (FVector::Dist(Pawn->GetActorLocation(), TargetLocation) > BreakDistance)
    {
        ClearTarget();
        return;
    }

    const FVector Eye = Pawn->GetActorLocation() + FVector(0.f, 0.f, 60.f);
    const FRotator Desired = (TargetLocation - Eye).Rotation();
    const FRotator Smoothed = FMath::RInterpTo(Controller->GetControlRotation(), Desired, DeltaTime, RotationInterpSpeed);
    Controller->SetControlRotation(Smoothed);
}
