#include "PNACharacter.h"
#include "PNAMovementComponent.h"
#include "PNALockOnComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"

APNACharacter::APNACharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UPNAMovementComponent>(
        ACharacter::CharacterMovementComponentName))
{
    PrimaryActorTick.bCanEverTick = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;

    PNAMovement = Cast<UPNAMovementComponent>(GetCharacterMovement());
    JumpMaxCount = 2; // double jump

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.f;
    CameraBoom->SocketOffset = FVector(0.f, 55.f, 65.f);
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 10.f;
    CameraBoom->bDoCollisionTest = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

    LockOn = CreateDefaultSubobject<UPNALockOnComponent>(TEXT("LockOn"));
}

void APNACharacter::NotifyControllerChanged()
{
    Super::NotifyControllerChanged();

    if (const APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
                LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                if (MappingContext)
                {
                    Subsystem->AddMappingContext(MappingContext, 0);
                }
            }
        }
    }
}

void APNACharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!EnhancedInput) return;

    if (IA_Move) EnhancedInput->BindAction(IA_Move, ETriggerEvent::Triggered, this, &APNACharacter::Move);
    if (IA_Look) EnhancedInput->BindAction(IA_Look, ETriggerEvent::Triggered, this, &APNACharacter::Look);

    if (IA_Jump)
    {
        EnhancedInput->BindAction(IA_Jump, ETriggerEvent::Started, this, &APNACharacter::JumpStart);
        EnhancedInput->BindAction(IA_Jump, ETriggerEvent::Completed, this, &APNACharacter::JumpStop);
    }
    if (IA_Sprint)
    {
        EnhancedInput->BindAction(IA_Sprint, ETriggerEvent::Started, this, &APNACharacter::SprintStart);
        EnhancedInput->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &APNACharacter::SprintStop);
    }
    if (IA_Dash) EnhancedInput->BindAction(IA_Dash, ETriggerEvent::Started, this, &APNACharacter::Dash);
    if (IA_LockOn) EnhancedInput->BindAction(IA_LockOn, ETriggerEvent::Started, this, &APNACharacter::LockOnPressed);
}

void APNACharacter::Move(const FInputActionValue& Value)
{
    const FVector2D Input = Value.Get<FVector2D>();
    if (!Controller) return;

    const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
    const FRotationMatrix RotationMatrix(YawRotation);

    AddMovementInput(RotationMatrix.GetUnitAxis(EAxis::X), Input.Y);
    AddMovementInput(RotationMatrix.GetUnitAxis(EAxis::Y), Input.X);
}

void APNACharacter::Look(const FInputActionValue& Value)
{
    if (LockOn && LockOn->HasTarget()) return; // camera driven by lock-on
    const FVector2D Input = Value.Get<FVector2D>();
    AddControllerYawInput(Input.X);
    AddControllerPitchInput(Input.Y);
}

void APNACharacter::SprintStart()
{
    if (PNAMovement) PNAMovement->SetWantsSprint(true);
}

void APNACharacter::SprintStop()
{
    if (PNAMovement) PNAMovement->SetWantsSprint(false);
}

void APNACharacter::Dash()
{
    if (PNAMovement) PNAMovement->RequestDash();
}

void APNACharacter::JumpStart() { Jump(); }
void APNACharacter::JumpStop() { StopJumping(); }

void APNACharacter::LockOnPressed()
{
    if (LockOn) LockOn->ToggleLockOn();
}
