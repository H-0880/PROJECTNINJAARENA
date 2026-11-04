#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PNACharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UPNAMovementComponent;
class UPNALockOnComponent;
struct FInputActionValue;

UCLASS()
class PROJECTNINJAARENA_API APNACharacter : public ACharacter
{
    GENERATED_BODY()

public:
    APNACharacter(const FObjectInitializer& ObjectInitializer);

public:
    UFUNCTION(BlueprintPure, Category="PNA|Movement")
    UPNAMovementComponent* GetPNAMovement() const { return PNAMovement; }

protected:
    virtual void NotifyControllerChanged() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void SprintStart();
    void SprintStop();
    void Dash();
    void JumpStart();
    void JumpStop();
    void LockOnPressed();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PNA|Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PNA|Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PNA|Movement")
    TObjectPtr<UPNAMovementComponent> PNAMovement;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PNA|LockOn")
    TObjectPtr<UPNALockOnComponent> LockOn;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Input")
    TObjectPtr<UInputMappingContext> MappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Input")
    TObjectPtr<UInputAction> IA_Move;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Input")
    TObjectPtr<UInputAction> IA_Look;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Input")
    TObjectPtr<UInputAction> IA_Jump;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Input")
    TObjectPtr<UInputAction> IA_Sprint;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Input")
    TObjectPtr<UInputAction> IA_Dash;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PNA|Input")
    TObjectPtr<UInputAction> IA_LockOn;
};
