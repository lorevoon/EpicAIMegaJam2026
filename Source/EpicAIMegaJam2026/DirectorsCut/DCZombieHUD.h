// Director's Cut - canvas HUD: storyboard, preparation timeline, shooting frame and zombie alarm, review (film + evaluation).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DCZombieHUD.generated.h"

class ADCZombieDirector;
class UTexture2D;

UCLASS()
class ADCZombieHUD : public AHUD
{
	GENERATED_BODY()

public:
	ADCZombieHUD();

	virtual void DrawHUD() override;

	/** Camera-type icons on the switcher buttons. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI")
	TObjectPtr<UTexture2D> StaticCameraIcon;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI")
	TObjectPtr<UTexture2D> DollyCameraIcon;

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
