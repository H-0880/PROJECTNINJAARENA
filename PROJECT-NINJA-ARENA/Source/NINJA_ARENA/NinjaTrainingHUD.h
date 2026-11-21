#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "NinjaTrainingHUD.generated.h"

class ANINJA_ARENACharacter;
class ANINJA_ARENAGameMode;

UCLASS()
class ANinjaTrainingHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawDemoMenu(const ANINJA_ARENAGameMode& Mode);
	void DrawFighterSelection(const ANINJA_ARENACharacter& Fighter, const ANINJA_ARENAGameMode& Mode, bool bSecond);
	void DrawFighterPanel(const ANINJA_ARENACharacter& Fighter, const TCHAR* Name,
		float Left, float Width, FLinearColor Accent, bool bWarning);
};
