#pragma once

#include "Animation/AnimInstance.h"
#include "NinjaFighterAnimInstance.generated.h"

class UIKRetargeter;

UCLASS(Transient)
class UNinjaFighterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	void SetRetargetAsset(UIKRetargeter* Asset);

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UIKRetargeter> RetargetAsset;
};
