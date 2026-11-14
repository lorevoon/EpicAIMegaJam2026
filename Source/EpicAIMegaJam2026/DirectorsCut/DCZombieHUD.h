// Director's Cut - canvas HUD, styled like a film set: slates, viewfinder marks, timecode, film-leader countdown.
// Storyboard (slate + reference cut), preparation, shooting (field monitor), review (film + evaluation).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DCUI.h"
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

	/** Overall size of panels and spacing, on top of scaling with the screen height. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI", meta = (ClampMin = "0.5", ClampMax = "3"))
	float UIScale = 1.f;

	/** Size of all HUD text, on top of UIScale. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI", meta = (ClampMin = "0.5", ClampMax = "3"))
	float TextScale = 1.f;

	/** Typeface for all HUD text (a runtime font, so it stays sharp at any size). Falls back to the engine font. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI")
	TObjectPtr<UFont> TextFont;

	/** Field monitor drawn around the capture frame while shooting; its screen must be transparent. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI")
	TObjectPtr<UTexture2D> MonitorFrame;

	/** The film's director, bottom-left while shooting: one portrait per mood (see EDCDirectorMood). */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI")
	TObjectPtr<UTexture2D> DirectorDefault;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI")
	TObjectPtr<UTexture2D> DirectorAlmost;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI")
	TObjectPtr<UTexture2D> DirectorDisgust;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI")
	TObjectPtr<UTexture2D> DirectorJoy;

	/** Preparation: he pops up to praise a scene that got its camera, alternating this face with DirectorJoy. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI")
	TObjectPtr<UTexture2D> DirectorThatsIt;

	/** The monitor art's screen opening, in texture pixels (top-left and bottom-right, inclusive). */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI")
	FVector2D MonitorScreenMin = FVector2D(155.f, 130.f);

	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI")
	FVector2D MonitorScreenMax = FVector2D(1066.f, 559.f);

	/** Shooting: how far the vignette reaches in from the screen edges, as a share of the screen height. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI", meta = (ClampMin = "0.05", ClampMax = "0.5"))
	float VignetteDepth = 0.3f;

	/** Shooting: opacity of the vignette at the screen edges when the shot is far off (it softens as the shot improves). */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|UI", meta = (ClampMin = "0", ClampMax = "1"))
	float VignetteStrength = 0.65f;

	UFont* GetTextFont() const;

private:
	void DrawStoryboardCard(ADCZombieDirector* D);
	void DrawReferenceCut(ADCZombieDirector* D);
	void DrawHeader(ADCZombieDirector* D);
	void DrawStoryboardPanel(ADCZombieDirector* D);
	/** A storyboard scene being ticked off: its name swells and is struck through, with a ring and sparkles from its box. */
	void DrawTickOff(const FString& Name, float TextX, float MidY, const FVector2D& Box, float Age);
	void DrawFraming(ADCZombieDirector* D);
	void DrawPlanPreview(ADCZombieDirector* D);
	void DrawGauges(ADCZombieDirector* D);
	void DrawCountdown(ADCZombieDirector* D);
	/** A pop-up message on a little clapperboard. */
	void DrawSlatePopup(const FString& Message);
	void DrawSwitcher(ADCZombieDirector* D);
	void DrawHelp(ADCZombieDirector* D);
	/** Preparation and shooting: the reference cut in the bottom-right corner, in step with the scene clock. */
	void DrawReferenceInset(ADCZombieDirector* D);
	void DrawResults(ADCZombieDirector* D);
	void DrawPremiere(ADCZombieDirector* D);

	/** Text at Size pixels (at 1080p, before TextScale). Long lines shrink to MaxWidth (or the screen). */
	FVector2D Label(const FString& Text, float X, float Y, const FLinearColor& Color, float Size,
		DCUI::EAlign Align = DCUI::EAlign::Left, bool bShadow = true, float MaxWidth = 0.f);
	/** Pixel size of HUD text of the given Size. */
	float Px(float Size) const { return Size * S * TextScale; }
	/** A key in a little outlined box followed by what it does; returns the total width. */
	float KeyHint(const FString& Key, const FString& Action, float X, float Y, float Size, const FLinearColor& Color, bool bDraw = true);
	/** The help lines for the current phase (key, action pairs per line). */
	void GetHelpLines(ADCZombieDirector* D, TArray<TArray<TPair<FString, FString>>>& OutLines) const;
	bool ProjectVisible(const FVector& World, FVector2D& OutScreen) const;
	/** The capture frame on the canvas (DCFrame::CaptureExtent for this screen). */
	void GetCaptureRect(float& L, float& T, float& R, float& B) const;
	float HelpTop(ADCZombieDirector* D) const;

	/** Shooting: the director's portrait (bottom-left) and his chat bubble along the bottom of the screen. */
	void DrawCoach(ADCZombieDirector* D);
	/** Preparation: the director pops up from below to praise a scene that just got its camera, then sinks away. */
	void DrawCheer(ADCZombieDirector* D);
	/** The director's portrait (bottom-left) and his bubble, typing Line out; Drop moves them both down (off screen when large). */
	void DrawDirectorSpeech(ADCZombieDirector* D, UTexture2D* Portrait, const FString& Line, const FLinearColor& Accent, float Since, float Drop);
	bool ShowsCoach(ADCZombieDirector* D) const;
	/** Where the coach's area ends: the portrait's right edge, and the top of his bubble. */
	float CoachRight() const;
	float CoachTop() const;

	/**
	 * Shooting: a vignette around the screen, red while the shot is far from the reference cut and turning blue
	 * as it gets close; reaching a perfect shot sets off a burst (flash, shockwave, sparkles and a PERFECT! stamp).
	 */
	void DrawLikenessVignette(ADCZombieDirector* D);
	void DrawPerfectBurst(float L, float T, float R, float B, float Age);
	/** Fades the vignette out when there's nothing to compare (between shots, paused, not filming). */
	void ResetLikenessVignette();

	ADCZombieDirector* FindDirector();

	TWeakObjectPtr<ADCZombieDirector> Director;
	/** The likeness shown (eased toward the live one so the colour doesn't flicker), and the vignette's fade. */
	float ShownLikeness = 0.f;
	float VignetteFade = 0.f;
	float LastVignetteTime = -1.f;
	/** In the perfect zone right now (with some give, so it doesn't flicker in and out at the edge). */
	bool bPerfectShot = false;
	float PerfectBurstStart = -100.f;
	int32 PerfectBurstCount = 0;
	/** Layout unit: 1 at 1080p, times UIScale. */
	float S = 1.f;
};
