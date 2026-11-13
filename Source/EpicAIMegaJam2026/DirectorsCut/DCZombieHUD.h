// Director's Cut - canvas HUD: storyboard, preparation timeline, shooting frame and zombie alarm, review (film + evaluation).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DCZombieHUD.generated.h"

class ADCZombieDirector;
class UFont;
class UTexture2D;

UCLASS()
class ADCZombieHUD : public AHUD
{
	GENERATED_BODY()

public:
	ADCZombieHUD();

	virtual void DrawHUD() override;

	/** Overall size of the HUD (text, panels, and spacing) on top of scaling with the screen height. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI", meta = (ClampMin = "0.5", ClampMax = "3"))
	float UIScale = 1.5f;

	/** Typeface for all HUD text (a runtime font, so it stays sharp at any size). Falls back to the engine font. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI")
	TObjectPtr<UFont> TextFont;

	/** Camera-type icons on the switcher buttons. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI")
	TObjectPtr<UTexture2D> StaticCameraIcon;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI")
	TObjectPtr<UTexture2D> DollyCameraIcon;

	/** The script page the storyboard is typed on. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI")
	TObjectPtr<UTexture2D> StoryboardPaper;

	/** Field monitor drawn around the capture frame while shooting; its screen must be transparent. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI")
	TObjectPtr<UTexture2D> MonitorFrame;

	/** The monitor art's screen opening, in texture pixels (top-left and bottom-right, inclusive). */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI")
	FVector2D MonitorScreenMin = FVector2D(155.f, 130.f);

	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI")
	FVector2D MonitorScreenMax = FVector2D(1066.f, 559.f);

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
	/** Storyboard: the reference cut, letterboxed, with each beat's storyboard line as a subtitle. */
	void DrawReferenceCut(ADCZombieDirector* D);

	/** Draws text; a line too long for MaxWidth (or, by default, the screen) shrinks to fit instead of running off. */
	void Label(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale = 1.f, bool bCenter = false, bool bShadow = true, float MaxWidth = 0.f);
	UFont* GetTextFont() const;
	bool ProjectVisible(const FVector& World, FVector2D& OutScreen) const;
	/** The capture frame on the canvas (DCFrame::CaptureExtent for this screen). */
	void GetCaptureRect(float& L, float& T, float& R, float& B) const;

	ADCZombieDirector* FindDirector();

	TWeakObjectPtr<ADCZombieDirector> Director;
	float S = 1.f;
};
