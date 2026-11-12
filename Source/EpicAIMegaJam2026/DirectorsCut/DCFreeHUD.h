// Director's Cut prototype 2 - canvas HUD. Never reveals undiscovered beats.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DCFreeHUD.generated.h"

class ADCFreeDirector;
struct FDCBeatState;

UCLASS()
class ADCFreeHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawHeader(ADCFreeDirector* D);
	void DrawTimeline(ADCFreeDirector* D);
	void DrawNotebook(ADCFreeDirector* D);
	void DrawFraming(ADCFreeDirector* D);
	void DrawGauges(ADCFreeDirector* D);
	void DrawHelp(ADCFreeDirector* D);
	void DrawResults(ADCFreeDirector* D);

	void Label(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale = 1.f, bool bCenter = false);
	bool ProjectVisible(const FVector& World, FVector2D& OutScreen) const;
	static FLinearColor BeatColor(const FDCBeatState& State);

	ADCFreeDirector* FindDirector();

	TWeakObjectPtr<ADCFreeDirector> Director;
	float S = 1.f;
};
