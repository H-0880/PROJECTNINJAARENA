#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PNAAdvancedCombatComponent.generated.h"

class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class EPNAAdvancedDefenseResult : uint8
{
    None,
    Invulnerable,
    Parried,
    Armored
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPNAAdvancedTraceHit, AActor*, HitActor, const FHitResult&, HitResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPNAParryTriggered, AActor*, Attacker);

/**
 * Optional advanced combat layer for NINJA_ARENA.
 * It is deliberately additive: the existing character combat remains authoritative.
 * Animation Notifies can open/close trace and defensive windows without duplicating
 * health, energy, lock-on or movement systems.
 */
UCLASS(ClassGroup=(PNA), meta=(BlueprintSpawnableComponent))
class NINJA_ARENA_API UPNAAdvancedCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPNAAdvancedCombatComponent();

    UPROPERTY(BlueprintAssignable, Category="PNA|Combat|Trace")
    FPNAAdvancedTraceHit OnTraceHit;

    UPROPERTY(BlueprintAssignable, Category="PNA|Combat|Defense")
    FPNAParryTriggered OnParryTriggered;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Combat|Trace", meta=(ClampMin="1.0"))
    float TraceRadius = 12.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Combat|Trace")
    FName TraceStartSocket = TEXT("hand_r");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Combat|Trace")
    FName TraceEndSocket = TEXT("hand_r");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Combat|Trace")
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PNA|Combat|Debug")
    bool bDebugTrace = false;

    UFUNCTION(BlueprintCallable, Category="PNA|Combat|Trace")
    void BeginTraceWindow(FName StartSocket, FName EndSocket, float Radius = 12.f);

    UFUNCTION(BlueprintCallable, Category="PNA|Combat|Trace")
    void EndTraceWindow();

    UFUNCTION(BlueprintCallable, Category="PNA|Combat|Defense")
    void SetInvulnerable(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category="PNA|Combat|Defense")
    void SetHyperArmor(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category="PNA|Combat|Defense")
    void SetParryWindow(bool bEnabled);

    UFUNCTION(BlueprintPure, Category="PNA|Combat|Defense")
    bool IsInvulnerable() const { return bInvulnerable; }

    UFUNCTION(BlueprintPure, Category="PNA|Combat|Defense")
    bool HasHyperArmor() const { return bHyperArmor; }

    UFUNCTION(BlueprintPure, Category="PNA|Combat|Defense")
    bool IsParryWindowOpen() const { return bParryWindow; }

    /** Returns the advanced defensive result before the legacy guard/damage path runs. */
    EPNAAdvancedDefenseResult EvaluateIncomingMelee(AActor* Attacker);

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    TWeakObjectPtr<USkeletalMeshComponent> OwnerMesh;
    FVector PreviousStart = FVector::ZeroVector;
    FVector PreviousEnd = FVector::ZeroVector;
    bool bTraceActive = false;
    bool bTraceInitialized = false;
    bool bInvulnerable = false;
    bool bHyperArmor = false;
    bool bParryWindow = false;
    TSet<TWeakObjectPtr<AActor>> HitActorsThisWindow;

    void TraceSegment(const FVector& Previous, const FVector& Current);
};
