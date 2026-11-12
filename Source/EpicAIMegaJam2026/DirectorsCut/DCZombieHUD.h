// Director's Cut - canvas HUD: storyboard, rehearsal, planning preview, zombie alarm, results.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DCZombieHUD.generated.h"

class ADCZombieDirector;

UCLASS()
class ADCZombieHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawStoryboardCard(ADCZombieDirector* D);
	void DrawHeader(ADCZombieDirector* D);
	void DrawStoryboardPanel(ADCZombieDirector* D);
	void DrawFraming(ADCZombieDirector* D);
	void DrawPlanPreview(ADCZombieDirector* D);
	void DrawGauges(ADCZombieDirector* D);
	void DrawZombieAlarm(ADCZombieDirector* D);
	void DrawSwitcher(ADCZombieDirector* D);
	void DrawHelp(ADCZombieDirector* D);
	void DrawResults(ADCZombieDirector* D);
	void DrawPremiere(ADCZombieDirector* D);

	void Label(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale = 1.f, bool bCenter = false);
	bool ProjectVisible(const FVector& World, FVector2D& OutScreen) const;

	ADCZombieDirector* FindDirector();

	TWeakObjectPtr<ADCZombieDirector> Director;
	float S = 1.f;
};
