// Director's Cut prototype 3 - canvas HUD: camera switcher, notebook, gauges for the live feed.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DCMultiCamHUD.generated.h"

class ADCMultiCamDirector;
struct FDCMultiCamBeat;

UCLASS()
class ADCMultiCamHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawHeader(ADCMultiCamDirector* D);
	void DrawTimeline(ADCMultiCamDirector* D);
	void DrawNotebook(ADCMultiCamDirector* D);
	void DrawFraming(ADCMultiCamDirector* D);
	void DrawGauges(ADCMultiCamDirector* D);
	void DrawSwitcher(ADCMultiCamDirector* D);
	void DrawHelp(ADCMultiCamDirector* D);
	void DrawResults(ADCMultiCamDirector* D);

	void Label(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale = 1.f, bool bCenter = false);
	bool ProjectVisible(const FVector& World, FVector2D& OutScreen) const;
	static FLinearColor BeatColor(const FDCMultiCamBeat& State);

	ADCMultiCamDirector* FindDirector();

	TWeakObjectPtr<ADCMultiCamDirector> Director;
	float S = 1.f;
};
