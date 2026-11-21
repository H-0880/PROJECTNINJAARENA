#include "Modes/PNADuelConstraintComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UPNADuelConstraintComponent::UPNADuelConstraintComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPNADuelConstraintComponent::BeginPlay()
{
    Super::BeginPlay();
    ApplyConstraint();
}

void UPNADuelConstraintComponent::Configure(bool bNewEnabled, float NewPlaneY)
{
    bEnabled = bNewEnabled;
    PlaneY = NewPlaneY;
    ApplyConstraint();
}

void UPNADuelConstraintComponent::SetEnabled(bool bNewEnabled)
{
    if (bEnabled == bNewEnabled)
    {
        return;
    }

    bEnabled = bNewEnabled;
    ApplyConstraint();
}

void UPNADuelConstraintComponent::SetPlaneY(float NewPlaneY)
{
    PlaneY = NewPlaneY;
    ApplyConstraint();
}

FVector UPNADuelConstraintComponent::ProjectToPlane(const FVector& Location) const
{
    if (!bEnabled)
    {
        return Location;
    }

    FVector Out = Location;
    Out.Y = PlaneY;
    return Out;
}

void UPNADuelConstraintComponent::ApplyConstraint()
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character)
    {
        return;
    }

    UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
    if (!Movement)
    {
        return;
    }

    if (bEnabled)
    {
        Movement->SetPlaneConstraintNormal(FVector::YAxisVector);
        Movement->SetPlaneConstraintOrigin(FVector(0.f, PlaneY, 0.f));
        Movement->SetPlaneConstraintEnabled(true);

        FVector Location = Character->GetActorLocation();
        Location.Y = PlaneY;
        Character->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
    }
    else
    {
        Movement->SetPlaneConstraintEnabled(false);
    }
}
