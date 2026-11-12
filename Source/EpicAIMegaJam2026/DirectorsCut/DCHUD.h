// Director's Cut prototype - canvas HUD. Reads director state only; owns no timing or scoring.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DCHUD.generated.h"

class ADCTakeDirector;
class ADCCharacter;
struct FDCShot;

UCLASS()
class ADCHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawHeader(ADCTakeDirector* D, ADCCharacter* Player);
	void DrawTimeline(ADCTakeDirector* D);
	void DrawShotList(ADCTakeDirector* D);
	void DrawWorldLabels(ADCTakeDirector* D);
	void DrawFraming(ADCTakeDirector* D);
	void DrawHelp(ADCTakeDirector* D, ADCCharacter* Player);
	void DrawResults(ADCTakeDirector* D);

	void Label(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale = 1.f, bool bCenter = false);
	bool ProjectVisible(const FVector& World, FVector2D& OutScreen) const;
	static FLinearColor ShotColor(const FDCShot& Shot);
	static FString ShotWindow(const FDCShot& Shot);

	ADCTakeDirector* FindDirector();

	TWeakObjectPtr<ADCTakeDirector> Director;
	float S = 1.f;
};
