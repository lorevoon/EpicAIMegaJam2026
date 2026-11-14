// Director's Cut - canvas HUD, styled like a film set (see DCUI for the building blocks).

#include "DCZombieHUD.h"
#include "DCBeat.h"
#include "DCCinemaReplay.h"
#include "DCMultiCam.h"
#include "DCSceneMarks.h"
#include "DCZombieDirector.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

using DCUI::EAlign;

namespace
{
	// Text sizes in pixels at 1080p (before TextScale).
	constexpr float Small = 30.f;
	constexpr float Body = 38.f;
	constexpr float Large = 54.f;
	constexpr float Title = 84.f;
	constexpr float Huge = 220.f;

	const FLinearColor Rose(1.f, 0.55f, 0.7f);
	const FLinearColor Live(0.85f, 0.08f, 0.07f, 0.9f);
	constexpr float GaugeLinger = 1.5f;
	/** Width of the reference-cut inset, as a share of the screen width. */
	constexpr float InsetShare = 0.24f;
	/** Height of the top bar during preparation and shooting (layout units). */
	constexpr float HeaderHeight = 86.f;

	FLinearColor WithAlpha(FLinearColor Color, float Alpha)
	{
		Color.A *= Alpha;
		return Color;
	}
}

ADCZombieHUD::ADCZombieHUD()
{
	static ConstructorHelpers::FObjectFinder<UFont> Bree(TEXT("/Game/DirectorsCut/UI/Fonts/Bree_Serif__1_/BreeSerif-Regular_Font.BreeSerif-Regular_Font"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Monitor(TEXT("/Game/DirectorsCut/UI/SMALLHD_Monitor_Rig_Assembly.SMALLHD_Monitor_Rig_Assembly"));
	TextFont = Bree.Object;
	MonitorFrame = Monitor.Object;
}

ADCZombieDirector* ADCZombieHUD::FindDirector()
{
	if (!Director.IsValid())
	{
		for (TActorIterator<ADCZombieDirector> It(GetWorld()); It; ++It)
		{
			Director = *It;
			break;
		}
	}
	return Director.Get();
}

UFont* ADCZombieHUD::GetTextFont() const
{
	return TextFont ? TextFont.Get() : GEngine->GetLargeFont();
}

FVector2D ADCZombieHUD::Label(const FString& Text, float X, float Y, const FLinearColor& Color, float Size, EAlign Align, bool bShadow, float MaxWidth)
{
	// Never run off the screen: by default a line may use the screen width less a margin.
	constexpr float Edge = 16.f;
	if (MaxWidth <= 0.f)
	{
		MaxWidth = Align == EAlign::Center ? Canvas->ClipX - 2.f * Edge : Align == EAlign::Right ? X - Edge : Canvas->ClipX - Edge - X;
	}
	return DCUI::Text(Canvas, GetTextFont(), Text, X, Y, Px(Size), Color, Align, bShadow, MaxWidth);
}

float ADCZombieHUD::KeyHint(const FString& Key, const FString& Action, float X, float Y, float Size, const FLinearColor& Color, bool bDraw)
{
	const float KeyPx = Px(Size) * 0.82f;
	const FVector2D KeySize = DCUI::Measure(Canvas, GetTextFont(), Key, KeyPx);
	const FVector2D ActionSize = DCUI::Measure(Canvas, GetTextFont(), Action, Px(Size));
	const float Pad = 8.f * S;
	const float BoxW = KeySize.X + 2.f * Pad;
	const float BoxH = Px(Size) * 1.2f;
	if (bDraw)
	{
		DCUI::Rect(Canvas, X, Y, BoxW, BoxH, FLinearColor(1.f, 1.f, 1.f, 0.08f));
		DCUI::Outline(Canvas, X, Y, BoxW, BoxH, FMath::Max(1.f, 1.5f * S), WithAlpha(DCUI::Gold, 0.8f));
		DCUI::Text(Canvas, GetTextFont(), Key, X + Pad, Y + (BoxH - KeySize.Y) * 0.5f, KeyPx, DCUI::Gold, EAlign::Left, false);
		DCUI::Text(Canvas, GetTextFont(), Action, X + BoxW + 10.f * S, Y + (BoxH - ActionSize.Y) * 0.5f, Px(Size), Color);
	}
	return BoxW + 10.f * S + ActionSize.X;
}

void ADCZombieHUD::GetCaptureRect(float& L, float& T, float& R, float& B) const
{
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const FVector2D Half = DCFrame::CaptureExtent(W / FMath::Max(H, 1.f)) * 0.5f;
	L = W * (0.5f - Half.X);
	R = W * (0.5f + Half.X);
	T = H * (0.5f - Half.Y);
	B = H * (0.5f + Half.Y);
}

bool ADCZombieHUD::ProjectVisible(const FVector& World, FVector2D& OutScreen) const
{
	if (!PlayerOwner || !PlayerOwner->PlayerCameraManager)
	{
		return false;
	}
	const FVector CamLoc = PlayerOwner->PlayerCameraManager->GetCameraLocation();
	const FVector CamFwd = PlayerOwner->PlayerCameraManager->GetCameraRotation().Vector();
	if (FVector::DotProduct(World - CamLoc, CamFwd) <= 0.f)
	{
		return false;
	}
	const FVector P = Project(World);
	OutScreen = FVector2D(P.X, P.Y);
	return true;
}

void ADCZombieHUD::DrawHUD()
{
	Super::DrawHUD();

	ADCZombieDirector* D = FindDirector();
	if (!D || !Canvas)
	{
		return;
	}
	S = FMath::Max(Canvas->ClipY / 1080.f, 0.6f) * UIScale;
	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.5f;

	if (D->GetPhase() == EDCPhase::Storyboard)
	{
		if (D->IsReferencePlaying())
		{
			DrawReferenceCut(D);
		}
		else
		{
			DrawStoryboardCard(D);
		}
		return;
	}
	if (D->IsReviewing())
	{
		if (D->GetCinema())
		{
			D->GetCinema()->DrawCinema(this, Canvas, D);
			return;
		}
		DrawPremiere(D);
		if (D->IsFilmFinished())
		{
			DrawResults(D);
		}
		return;
	}

	if (D->GetPhase() == EDCPhase::Preparation && !D->IsViewingCamera())
	{
		FVector2D P;
		if (D->GetClapperboard() && ProjectVisible(D->GetClapperboard()->GetActorLocation() + FVector(0.f, 0.f, 170.f), P))
		{
			KeyHint(TEXT("ENTER"), TEXT("Call ACTION!"), P.X - KeyHint(TEXT("ENTER"), TEXT("Call ACTION!"), 0.f, 0.f, Small, DCUI::Ivory, false) * 0.5f,
				P.Y, Small, DCUI::Ivory);
		}
	}

	// While shooting, dim everything outside the capture frame and frame it with the field monitor, its screen fitted
	// exactly over the capture frame. Drawn first so the HUD stays on top.
	if ((D->GetPhase() == EDCPhase::Countdown || D->GetPhase() == EDCPhase::Filming) && D->IsViewingCamera())
	{
		const float W = Canvas->ClipX;
		const float H = Canvas->ClipY;
		float L, T, R, B;
		GetCaptureRect(L, T, R, B);
		const FLinearColor Mask(0.f, 0.f, 0.f, 0.5f);
		DCUI::Rect(Canvas, 0.f, 0.f, W, T, Mask);
		DCUI::Rect(Canvas, 0.f, B, W, H - B, Mask);
		DCUI::Rect(Canvas, 0.f, T, L, B - T, Mask);
		DCUI::Rect(Canvas, R, T, W - R, B - T, Mask);
		if (MonitorFrame)
		{
			const FVector2D Screen = MonitorScreenMax - MonitorScreenMin + FVector2D(1.f, 1.f);
			const float ScaleX = (R - L) / Screen.X;
			const float ScaleY = (B - T) / Screen.Y;
			DrawTexture(MonitorFrame, L - MonitorScreenMin.X * ScaleX, T - MonitorScreenMin.Y * ScaleY,
				MonitorFrame->GetSurfaceWidth() * ScaleX, MonitorFrame->GetSurfaceHeight() * ScaleY, 0.f, 0.f, 1.f, 1.f);
		}
	}

	DrawHeader(D);
	if (D->bShowNotebook)
	{
		DrawStoryboardPanel(D);
	}
	if (D->IsViewingCamera())
	{
		DrawFraming(D);
	}
	else
	{
		DCUI::Corners(Canvas, CX - 9.f * S, CY - 9.f * S, 18.f * S, 18.f * S, 6.f * S, FMath::Max(1.5f * S, 1.f), DCUI::Ivory);
	}
	if (D->GetPhase() == EDCPhase::Preparation)
	{
		DrawPlanPreview(D);
	}
	if (D->GetPhase() == EDCPhase::Filming)
	{
		DrawGauges(D);
	}
	if (D->GetPhase() == EDCPhase::Countdown)
	{
		DrawCountdown(D);
	}

	const FString Message = D->GetMessage();
	if (!Message.IsEmpty())
	{
		DrawSlatePopup(Message);
	}

	if (!D->GetSetupProblem().IsEmpty())
	{
		TArray<FString> Lines;
		D->GetSetupProblem().ParseIntoArrayLines(Lines);
		float Y = (HeaderHeight + 20.f) * S;
		for (const FString& Line : Lines)
		{
			Y += Label(TEXT("SETUP: ") + Line, 24.f * S, Y, DCUI::Rec, Small).Y;
		}
	}

	DrawReferenceInset(D);
	DrawSwitcher(D);
	DrawHelp(D);

	if (UGameplayStatics::IsGamePaused(this))
	{
		DCUI::Rect(Canvas, 0.f, 0.f, Canvas->ClipX, Canvas->ClipY, FLinearColor(0.f, 0.f, 0.f, 0.7f));
		Label(TEXT("PAUSED"), CX, CY - Px(Title), DCUI::Ivory, Title, EAlign::Center);
		const float W = KeyHint(TEXT("P"), TEXT("Resume"), 0.f, 0.f, Body, DCUI::Ivory, false);
		KeyHint(TEXT("P"), TEXT("Resume"), CX - W * 0.5f, CY + 20.f * S, Body, DCUI::Ivory);
	}
}

// ---------------------------------------------------------------------------------------------
// Storyboard

void ADCZombieHUD::DrawStoryboardCard(ADCZombieDirector* D)
{
	// Phase 1: the storyboard on a clapperboard slate - what happens, in order (never when or where).
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	DCUI::Rect(Canvas, 0.f, 0.f, W, H, FLinearColor(0.01f, 0.01f, 0.015f, 0.82f));

	const TArray<FDCMultiCamBeat>& Beats = D->GetBeats();
	const float Pad = 40.f * S;
	const float PW = FMath::Min(W * 0.78f, 1500.f * S);
	const float Inner = PW - 2.f * Pad;
	const float StickH = 40.f * S, BandH = 28.f * S, Gap = 6.f * S;
	// Height: production, fields, scene list, footer.
	const float FixedH = Pad + Px(Small) * 1.3f + Px(Title) * 1.15f + 24.f * S + (Px(Small) + Px(Body)) * 1.3f + 24.f * S
		+ Px(Small) * 1.4f + 24.f * S + Px(Small) * 1.5f + Px(Body) * 1.5f + Pad;
	const float RowH = FMath::Min(Px(Body) * 1.45f, (H * 0.92f - StickH - BandH - 2.f * Gap - FixedH) / FMath::Max(Beats.Num(), 1));
	const float BodyH = FixedH + RowH * Beats.Num();
	const float PX = (W - PW) * 0.5f;
	float Y = FMath::Max((H - (StickH + Gap + BandH + Gap + BodyH)) * 0.5f, 8.f * S);

	// The clapper: the hinged stick, then the slate's own striped band.
	DCUI::ClapperStripes(Canvas, PX, Y, PW, StickH);
	Y += StickH + Gap;
	DCUI::ClapperStripes(Canvas, PX, Y, PW, BandH);
	Y += BandH + Gap;
	DCUI::Rect(Canvas, PX, Y, PW, BodyH, FLinearColor(0.03f, 0.03f, 0.04f, 0.94f));
	DCUI::Outline(Canvas, PX, Y, PW, BodyH, FMath::Max(2.f * S, 1.f), WithAlpha(DCUI::Ivory, 0.35f));
	auto Divider = [&](float At) { DCUI::Rect(Canvas, PX + Pad, At, Inner, FMath::Max(1.5f * S, 1.f), WithAlpha(DCUI::Ivory, 0.25f)); };
	const float X = PX + Pad;
	Y += Pad;

	Label(TEXT("PRODUCTION"), X, Y, DCUI::Muted, Small);
	Label(TEXT("STORYBOARD  1/4"), PX + PW - Pad, Y, DCUI::Gold, Small, EAlign::Right);
	Y += Px(Small) * 1.3f;
	Label(D->MovieTitle, X, Y, DCUI::Ivory, Title, EAlign::Left, true, Inner);
	Y += Px(Title) * 1.15f + 12.f * S;
	Divider(Y);
	Y += 12.f * S;

	// Slate fields.
	const float Length = D->TakeDuration;
	const TPair<FString, FString> Fields[] = {
		{ TEXT("SCENES"), FString::FromInt(Beats.Num()) },
		{ TEXT("RUNNING TIME"), DCUI::Timecode(Length) },
		{ TEXT("ASPECT"), TEXT("2.12 : 1") },
		{ TEXT("DIRECTOR"), TEXT("YOU") },
	};
	const float ColW = Inner / UE_ARRAY_COUNT(Fields);
	for (int32 i = 0; i < UE_ARRAY_COUNT(Fields); ++i)
	{
		const float FX = X + ColW * i;
		if (i > 0)
		{
			DCUI::Rect(Canvas, FX - 12.f * S, Y, FMath::Max(1.5f * S, 1.f), (Px(Small) + Px(Body)) * 1.25f, WithAlpha(DCUI::Ivory, 0.25f));
		}
		Label(Fields[i].Key, FX, Y, DCUI::Muted, Small, EAlign::Left, true, ColW - 24.f * S);
		Label(Fields[i].Value, FX, Y + Px(Small) * 1.3f, DCUI::Ivory, Body, EAlign::Left, true, ColW - 24.f * S);
	}
	Y += (Px(Small) + Px(Body)) * 1.3f + 12.f * S;
	Divider(Y);
	Y += 12.f * S;

	// The scenes, each with its scene number on a little frame.
	Label(TEXT("SCENES"), X, Y, DCUI::Muted, Small);
	Y += Px(Small) * 1.4f;
	const float BodyPx = FMath::Min(Px(Body), RowH / 1.35f);
	for (int32 i = 0; i < Beats.Num(); ++i)
	{
		const FString Number = FString::Printf(TEXT("SC %d"), i + 1);
		const float ChipW = DCUI::Measure(Canvas, GetTextFont(), TEXT("SC 00"), BodyPx * 0.7f).X + 16.f * S;
		const float ChipH = BodyPx * 1.05f;
		const float RowY = Y + (RowH - ChipH) * 0.5f;
		DCUI::Outline(Canvas, X, RowY, ChipW, ChipH, FMath::Max(1.5f * S, 1.f), DCUI::Gold);
		DCUI::Text(Canvas, GetTextFont(), Number, X + ChipW * 0.5f, RowY + ChipH * 0.12f, BodyPx * 0.7f, DCUI::Gold, EAlign::Center, false);
		DCUI::Text(Canvas, GetTextFont(), Beats[i].Beat->DisplayName, X + ChipW + 18.f * S, Y + (RowH - BodyPx * 1.2f) * 0.5f, BodyPx, DCUI::Ivory,
			EAlign::Left, true, Inner - ChipW - 18.f * S);
		Y += RowH;
	}
	Y += 12.f * S;
	Divider(Y);
	Y += 12.f * S;

	Label(TEXT("Your film is graded on how closely it matches the reference cut: camera position, angle and zoom."),
		PX + PW * 0.5f, Y, DCUI::Muted, Small, EAlign::Center, true, Inner);
	Y += Px(Small) * 1.5f;
	const FString Second = D->bShowReferenceCut ? TEXT("Watch the reference cut") : TEXT("Pause the rehearsal");
	const float W1 = KeyHint(TEXT("ENTER"), TEXT("Start preparing"), 0.f, 0.f, Body, DCUI::Ivory, false);
	const float W2 = KeyHint(D->bShowReferenceCut ? TEXT("SPACE") : TEXT("H"), Second, 0.f, 0.f, Body, DCUI::Ivory, false);
	float HX = PX + (PW - (W1 + W2 + 48.f * S)) * 0.5f;
	HX += KeyHint(TEXT("ENTER"), TEXT("Start preparing"), HX, Y, Body, DCUI::Ivory) + 48.f * S;
	KeyHint(D->bShowReferenceCut ? TEXT("SPACE") : TEXT("H"), Second, HX, Y, Body, DCUI::Ivory);
}

void ADCZombieHUD::DrawReferenceCut(ADCZombieDirector* D)
{
	// The reference cut: the level shot well, letterboxed, for the player to study and try to match.
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const float Bar = FMath::Max((H - W / DCFrame::FilmAspect) * 0.5f, Px(Small) * 2.f);
	const float Time = D->GetReferenceTime();
	const float Length = FMath::Max(D->GetReferenceLength(), KINDA_SMALL_NUMBER);
	DCUI::Rect(Canvas, 0.f, 0.f, W, Bar, FLinearColor::Black);
	DCUI::Rect(Canvas, 0.f, H - Bar, W, Bar, FLinearColor::Black);

	const float TopY = (Bar - Px(Small) * 1.2f) * 0.5f;
	DCUI::Disc(Canvas, FVector2D(32.f * S, Bar * 0.5f), 9.f * S, DCUI::Gold);
	Label(TEXT("REFERENCE CUT"), 50.f * S, TopY, DCUI::Gold, Small);
	Label(D->MovieTitle, W * 0.5f, TopY, DCUI::Ivory, Small, EAlign::Center, true, W * 0.4f);
	Label(TEXT("Study it - then shoot it yourself"), W - 24.f * S, TopY, DCUI::Muted, Small, EAlign::Right);

	// The storyboard line for the beat on screen, as a subtitle.
	if (const ADCBeat* Beat = D->GetReferenceBeat())
	{
		const FVector2D Size = DCUI::Measure(Canvas, GetTextFont(), Beat->DisplayName, Px(Large));
		const float SubY = H - Bar - Px(Large) * 1.9f;
		DCUI::Rect(Canvas, W * 0.5f - FMath::Min(Size.X, W * 0.9f) * 0.5f - 18.f * S, SubY - 6.f * S, FMath::Min(Size.X, W * 0.9f) + 36.f * S, Size.Y + 12.f * S,
			FLinearColor(0.f, 0.f, 0.f, 0.45f));
		Label(Beat->DisplayName, W * 0.5f, SubY, DCUI::Ivory, Large, EAlign::Center, true, W * 0.9f);
	}

	// Timeline with a gold tick where each beat becomes capturable; timecode and controls either side.
	const float BotY = H - Bar + (Bar - Px(Small) * 1.2f) * 0.5f;
	const float X0 = W * 0.3f;
	const float TW = W * 0.34f;
	const float TY = H - Bar * 0.5f - 3.f * S;
	DCUI::Rect(Canvas, X0, TY, TW, 6.f * S, FLinearColor(1.f, 1.f, 1.f, 0.2f));
	for (const FDCMultiCamBeat& State : D->GetBeats())
	{
		DCUI::Rect(Canvas, X0 + TW * State.Beat->GetCaptureStartTime() / Length - 2.f, TY - 8.f * S, 4.f, 22.f * S, DCUI::Gold);
	}
	DCUI::Rect(Canvas, X0, TY, TW * Time / Length, 6.f * S, Rose);
	DCUI::Rect(Canvas, X0 + TW * Time / Length - 1.5f, TY - 8.f * S, 3.f, 22.f * S, DCUI::Ivory);
	Label(DCUI::Timecode(Time) + TEXT("  /  ") + DCUI::Timecode(Length), 24.f * S, BotY, DCUI::Muted, Small, EAlign::Left, true, X0 - 48.f * S);
	{
		float Total = 0.f;
		const TPair<FString, FString> Hints[] = { { TEXT("SPACE"), TEXT("Pause") }, { TEXT("Q/E"), TEXT("Scrub") }, { TEXT("ENTER"), TEXT("Storyboard") } };
		for (const auto& Hint : Hints) Total += KeyHint(Hint.Key, Hint.Value, 0.f, 0.f, Small, DCUI::Muted, false) + 24.f * S;
		float HX = W - 24.f * S - Total + 24.f * S;
		for (const auto& Hint : Hints) HX += KeyHint(Hint.Key, Hint.Value, HX, BotY - Px(Small) * 0.05f, Small, DCUI::Muted) + 24.f * S;
	}
	if (D->IsReferencePaused())
	{
		Label(TEXT("PAUSED"), W * 0.5f, H * 0.5f - Px(Large) * 0.6f, DCUI::Ivory, Large, EAlign::Center);
	}
}

// ---------------------------------------------------------------------------------------------
// Preparation and shooting

void ADCZombieHUD::DrawHeader(ADCZombieDirector* D)
{
	const float W = Canvas->ClipX;
	const float HB = HeaderHeight * S;
	DCUI::Rect(Canvas, 0.f, 0.f, W, HB, FLinearColor(0.f, 0.f, 0.f, 0.55f));
	DCUI::Rect(Canvas, 0.f, HB - 1.f, W, 1.f, DCUI::Hairline);
	const float Mid = HB * 0.5f;

	// Left: a clapper chip and the phase, like a slate's scene field.
	const EDCPhase Phase = D->GetPhase();
	const float ChipW = 60.f * S, ChipH = 26.f * S;
	float X = 24.f * S;
	DCUI::ClapperStripes(Canvas, X, Mid - ChipH * 0.5f, ChipW, ChipH);
	X += ChipW + 16.f * S;
	if (Phase == EDCPhase::Filming)
	{
		const bool bBlink = D->IsRecordingPaused() || FMath::Fmod(GetWorld()->GetRealTimeSeconds(), 1.f) < 0.6f;
		DCUI::Disc(Canvas, FVector2D(X + 10.f * S, Mid), 10.f * S, bBlink ? DCUI::Rec : WithAlpha(DCUI::Rec, 0.3f));
		X += 30.f * S;
	}
	const FString PhaseText = Phase == EDCPhase::Preparation ? TEXT("SCENE 2  |  PREPARATION")
		: Phase == EDCPhase::Countdown ? TEXT("SCENE 3  |  STAND BY") : TEXT("REC  |  SCENE 3  |  SHOOTING");
	Label(PhaseText, X, Mid - Px(Body) * 0.62f, Phase == EDCPhase::Filming ? DCUI::Rec : DCUI::Gold, Body, EAlign::Left, true, W * 0.33f);

	// Centre: timecode.
	const FString Tc = DCUI::Timecode(D->GetSceneTime());
	const FVector2D TcSize = DCUI::Measure(Canvas, GetTextFont(), Tc, Px(Large));
	Label(Tc, W * 0.5f, Mid - Px(Large) * 0.62f, DCUI::Ivory, Large, EAlign::Center);
	Label(TEXT("/ ") + DCUI::Timecode(D->TakeDuration), W * 0.5f + TcSize.X * 0.5f + 12.f * S, Mid - Px(Small) * 0.4f, DCUI::Muted, Small);

	// Right: the camera about to be placed.
	if (Phase == EDCPhase::Preparation)
	{
		const EDCCamTool Tool = D->GetTool();
		Label(FString::Printf(TEXT("%s  %.0f cm   |   %d camera%s"), Tool == EDCCamTool::Crane ? TEXT("CRANE") : TEXT("STATIC"),
			D->GetPendingHeight(), D->GetCameraCount(), D->GetCameraCount() == 1 ? TEXT("") : TEXT("s")),
			W - 24.f * S, Mid - Px(Small) * 0.62f, DCUI::Ivory, Small, EAlign::Right, true, W * 0.28f);
	}
}

void ADCZombieHUD::DrawStoryboardPanel(ADCZombieDirector* D)
{
	// The storyboard notebook (Tab): a film strip of the scenes, top right.
	const TArray<FDCMultiCamBeat>& Beats = D->GetBeats();
	const float W = Canvas->ClipX * 0.25f;
	const float X = Canvas->ClipX - W - 20.f * S;
	float Y = (HeaderHeight + 18.f) * S;
	const float Band = 14.f * S;
	const float RowH = Px(Small) * 1.35f;
	const float H = Band * 2.f + Px(Small) * 1.5f + RowH * Beats.Num() + 16.f * S;
	DCUI::Rect(Canvas, X, Y, W, H, DCUI::Fill);
	DCUI::Sprockets(Canvas, X, Y, W, Band);
	DCUI::Sprockets(Canvas, X, Y + H - Band, W, Band);
	Y += Band + 6.f * S;
	Label(TEXT("STORYBOARD"), X + 16.f * S, Y, DCUI::Gold, Small);
	Label(TEXT("Tab"), X + W - 16.f * S, Y, DCUI::Muted, Small, EAlign::Right);
	Y += Px(Small) * 1.5f;
	for (int32 i = 0; i < Beats.Num(); ++i)
	{
		const FDCMultiCamBeat& State = Beats[i];
		const float Box = Px(Small) * 0.6f;
		const float BY = Y + (RowH - Box) * 0.5f;
		if (State.Status == EDCShotStatus::Captured)
		{
			DCUI::Rect(Canvas, X + 16.f * S, BY, Box, Box, DCUI::Good);
		}
		else if (State.Status == EDCShotStatus::Missed)
		{
			DCUI::Rect(Canvas, X + 16.f * S, BY, Box, Box, WithAlpha(DCUI::Rec, 0.6f));
		}
		else
		{
			DCUI::Outline(Canvas, X + 16.f * S, BY, Box, Box, FMath::Max(1.5f * S, 1.f), DCUI::Muted);
		}
		const FLinearColor Color = State.Status == EDCShotStatus::Captured ? DCUI::Good : State.Status == EDCShotStatus::Missed ? DCUI::Muted : DCUI::Ivory;
		Label(FString::Printf(TEXT("%d. %s"), i + 1, *State.Beat->DisplayName), X + 16.f * S + Box + 12.f * S, Y + (RowH - Px(Small) * 1.2f) * 0.5f,
			Color, Small, EAlign::Left, true, W - Box - 44.f * S);
		Y += RowH;
	}
}

void ADCZombieHUD::DrawFraming(ADCZombieDirector* D)
{
	bool bRolling = false;
	for (const FDCMultiCamBeat& State : D->GetBeats())
	{
		bRolling |= D->GetPhase() == EDCPhase::Filming && State.Status == EDCShotStatus::Pending && State.Hold > 0.f;
	}
	float L, T, R, B;
	GetCaptureRect(L, T, R, B);
	DCUI::Corners(Canvas, L, T, R - L, B - T, 48.f * S, FMath::Max(3.f * S, 2.f), bRolling ? DCUI::Good : WithAlpha(DCUI::Ivory, 0.55f));
	DCUI::Corners(Canvas, Canvas->ClipX * 0.5f - 9.f * S, Canvas->ClipY * 0.5f - 9.f * S, 18.f * S, 18.f * S, 6.f * S, FMath::Max(1.5f * S, 1.f),
		WithAlpha(DCUI::Ivory, 0.7f));

	// Recording state inside the frame's top-right corner.
	if (D->GetPhase() == EDCPhase::Filming)
	{
		const float RX = R - 24.f * S;
		const float RY = T + 20.f * S;
		if (D->IsRecordingPaused())
		{
			const float BarW = 12.f * S, BarH = Px(Body);
			DCUI::Rect(Canvas, RX - 2.f * BarW - 10.f * S, RY, BarW, BarH, DCUI::Ivory);
			DCUI::Rect(Canvas, RX - BarW, RY, BarW, BarH, DCUI::Ivory);
			Label(TEXT("PAUSED"), RX - 2.f * BarW - 24.f * S, RY, DCUI::Ivory, Body, EAlign::Right);
		}
		else
		{
			const FVector2D Size = Label(TEXT("REC"), RX, RY, DCUI::Ivory, Body, EAlign::Right);
			if (FMath::Fmod(GetWorld()->GetRealTimeSeconds(), 1.f) < 0.6f)
			{
				DCUI::Disc(Canvas, FVector2D(RX - Size.X - 22.f * S, RY + Size.Y * 0.5f), 11.f * S, DCUI::Rec);
			}
		}
	}

	if (const ADCMultiCam* Cam = D->GetCameraSlot(D->GetViewingSlot()))
	{
		const float Mm = 18.f / FMath::Tan(FMath::DegreesToRadians(Cam->GetFOV() * 0.5f));
		Label(FString::Printf(TEXT("CAM %d   |   %.0f mm   |   %s   |   WASD within %.1f m"), Cam->GetNumber(), Mm,
			Cam->GetRig() == EDCCamRig::Crane ? TEXT("CRANE") : TEXT("STATIC"), Cam->GetMovementRadius() / 100.f),
			L, T - Px(Small) * 1.45f, D->GetPhase() == EDCPhase::Filming ? DCUI::Rec : DCUI::Ivory, Small, EAlign::Left, true, R - L);
	}
}

void ADCZombieHUD::DrawPlanPreview(ADCZombieDirector* D)
{
	// The rehearsal timeline, under the top bar.
	const float W = FMath::Min(820.f * S, Canvas->ClipX * 0.5f);
	const float X = (Canvas->ClipX - W) * 0.5f;
	const float Y = (HeaderHeight + 18.f) * S;
	const float Pad = 16.f * S;
	TArray<FString> Framed;
	if (D->IsViewingCamera())
	{
		D->GetPlanPreview(Framed);
	}
	const float H = Pad * 2.f + Px(Small) * 1.3f + 22.f * S + (D->IsViewingCamera() ? Px(Body) * 1.4f : 0.f);
	DCUI::Panel(Canvas, X, Y, W, H, S);

	const float Length = FMath::Max(D->TakeDuration, KINDA_SMALL_NUMBER);
	const float Time = D->GetSceneTime();
	Label(FString::Printf(TEXT("REHEARSAL  |  %s  |  %.1fx"), D->IsRehearsalPlaying() ? TEXT("PLAYING") : TEXT("PAUSED"), D->GetRehearsalSpeed()),
		X + Pad, Y + Pad, DCUI::Gold, Small, EAlign::Left, true, W * 0.6f);
	Label(DCUI::Timecode(Time), X + W - Pad, Y + Pad, DCUI::Ivory, Small, EAlign::Right);
	const float TY = Y + Pad + Px(Small) * 1.3f + 6.f * S;
	DCUI::Rect(Canvas, X + Pad, TY, W - 2.f * Pad, 6.f * S, FLinearColor(1.f, 1.f, 1.f, 0.2f));
	DCUI::Rect(Canvas, X + Pad, TY, (W - 2.f * Pad) * Time / Length, 6.f * S, DCUI::Gold);
	DCUI::Rect(Canvas, X + Pad + (W - 2.f * Pad) * Time / Length - 1.5f, TY - 7.f * S, 3.f, 20.f * S, DCUI::Ivory);
	if (D->IsViewingCamera())
	{
		const FString Line = Framed.Num() > 0 ? TEXT("Frames: ") + FString::Join(Framed, TEXT(", ")) : TEXT("No storyboard scene framed right now");
		Label(Line, X + W * 0.5f, TY + 22.f * S, Framed.Num() > 0 ? DCUI::Good : DCUI::Muted, Body, EAlign::Center, true, W - 2.f * Pad);
	}
}

void ADCZombieHUD::DrawGauges(ADCZombieDirector* D)
{
	const float BarW = 460.f * S;
	const float BarH = 12.f * S;
	float Y = Canvas->ClipY * 0.58f;
	const float X = Canvas->ClipX * 0.5f - BarW * 0.5f;
	for (const FDCMultiCamBeat& State : D->GetBeats())
	{
		const bool bFilling = State.Status == EDCShotStatus::Pending && State.Hold > 0.f;
		const bool bJustCaptured = State.Status == EDCShotStatus::Captured && D->GetSceneTime() - State.LastProgressTime < GaugeLinger;
		if (!bFilling && !bJustCaptured)
		{
			continue;
		}
		const float Fill = bJustCaptured ? 1.f : FMath::Clamp(State.Hold / State.Beat->RequiredHold, 0.f, 1.f);
		const FLinearColor Color = bJustCaptured ? DCUI::Good : Rose;
		Y += Label(bJustCaptured ? FString::Printf(TEXT("%s  -  IN THE CAN"), *State.Beat->DisplayName) : State.Beat->DisplayName,
			Canvas->ClipX * 0.5f, Y, Color, Body, EAlign::Center).Y;
		DCUI::Rect(Canvas, X, Y, BarW, BarH, DCUI::Fill);
		DCUI::Rect(Canvas, X, Y, BarW * Fill, BarH, Color);
		Y += BarH + 14.f * S;
	}
}

void ADCZombieHUD::DrawCountdown(ADCZombieDirector* D)
{
	// A film-leader countdown: rings, crosshair, a sweeping hand and the number.
	const FVector2D C(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	const float R = 190.f * S;
	const float Remaining = D->GetCountdownRemaining();
	const int32 N = FMath::Max(FMath::CeilToInt(Remaining), 1);
	const float Sweep = 360.f * (1.f - (Remaining - FMath::FloorToFloat(Remaining - KINDA_SMALL_NUMBER)));
	DCUI::Disc(Canvas, C, R, FLinearColor(0.f, 0.f, 0.f, 0.45f), 64);
	DCUI::Wedge(Canvas, C, R, -90.f, Sweep, FLinearColor(1.f, 1.f, 1.f, 0.16f), 64);
	DCUI::Ring(Canvas, C, R, 5.f * S, DCUI::Ivory);
	DCUI::Ring(Canvas, C, R * 0.8f, 2.5f * S, WithAlpha(DCUI::Ivory, 0.7f));
	DCUI::Line(Canvas, C - FVector2D(R * 1.15f, 0.f), C + FVector2D(R * 1.15f, 0.f), FMath::Max(2.f * S, 1.f), WithAlpha(DCUI::Ivory, 0.5f));
	DCUI::Line(Canvas, C - FVector2D(0.f, R * 1.15f), C + FVector2D(0.f, R * 1.15f), FMath::Max(2.f * S, 1.f), WithAlpha(DCUI::Ivory, 0.5f));
	const FVector2D NumSize = DCUI::Measure(Canvas, GetTextFont(), FString::FromInt(N), Px(Huge));
	Label(FString::FromInt(N), C.X, C.Y - NumSize.Y * 0.5f, DCUI::Ivory, Huge, EAlign::Center);
	Label(TEXT("Places, everyone... and quiet on set."), C.X, C.Y + R + 28.f * S, DCUI::Ivory, Body, EAlign::Center);
}

void ADCZombieHUD::DrawSlatePopup(const FString& Message)
{
	// A message on a small clapperboard: striped stick over a dark slate.
	const float MaxW = Canvas->ClipX * 0.8f;
	const FVector2D Size = DCUI::Measure(Canvas, GetTextFont(), Message, Px(Body));
	const float Pad = 28.f * S;
	const float W = FMath::Min(Size.X + 2.f * Pad, MaxW);
	const float X = (Canvas->ClipX - W) * 0.5f;
	float Y = Canvas->ClipY * 0.3f;
	const float StickH = 18.f * S;
	DCUI::ClapperStripes(Canvas, X, Y, W, StickH);
	Y += StickH + 4.f * S;
	const float H = Size.Y + 2.f * Pad * 0.7f;
	DCUI::Rect(Canvas, X, Y, W, H, FLinearColor(0.03f, 0.03f, 0.04f, 0.9f));
	DCUI::Outline(Canvas, X, Y, W, H, FMath::Max(1.5f * S, 1.f), WithAlpha(DCUI::Ivory, 0.3f));
	Label(Message, Canvas->ClipX * 0.5f, Y + Pad * 0.7f, DCUI::Ivory, Body, EAlign::Center, true, W - 2.f * Pad);
}

void ADCZombieHUD::DrawSwitcher(ADCZombieDirector* D)
{
	// One slate per camera (number + camera type). While placing, the camera about to be placed already has its
	// slate, pulsing, showing the selected type (T switches it).
	const int32 PlaceSlot = D->GetPlacementSlot();
	const int32 Count = FMath::Max(D->IsUnlimited() ? D->GetSlotCount() : D->MaxCameras, PlaceSlot + 1);
	if (Count == 0)
	{
		return;
	}
	const float Gap = 12.f * S;
	const float BoxW = FMath::Min(132.f * S, (Canvas->ClipX * 0.6f - (Count - 1) * Gap) / Count);
	const float BoxH = 112.f * S;
	float X = Canvas->ClipX * 0.5f - (Count * BoxW + (Count - 1) * Gap) * 0.5f;
	const float Y = HelpTop(D) - BoxH - 20.f * S;
	const bool bOnAir = D->GetPhase() == EDCPhase::Filming || D->GetPhase() == EDCPhase::Countdown;
	const float Pulse = 0.65f + 0.35f * FMath::Sin(GetWorld()->GetRealTimeSeconds() * 5.f);

	for (int32 i = 0; i < Count; ++i)
	{
		const ADCMultiCam* Cam = D->GetCameraSlot(i);
		const bool bPlacing = i == PlaceSlot;
		FLinearColor Fill = DCUI::Fill;
		FLinearColor Edge = Cam ? DCUI::Ivory : DCUI::Muted;
		if (D->GetViewingSlot() == i)
		{
			Fill = bOnAir ? Live : FLinearColor(0.8f, 0.58f, 0.12f, 0.85f);
		}
		else if (bPlacing)
		{
			Fill = WithAlpha(DCUI::Gold, 0.25f * Pulse);
			Edge = WithAlpha(DCUI::Gold, Pulse);
		}
		// A thin clapper stripe across the top of each camera slate.
		DCUI::ClapperStripes(Canvas, X, Y, BoxW, 10.f * S);
		DCUI::Rect(Canvas, X, Y + 12.f * S, BoxW, BoxH - 12.f * S, Fill);
		DCUI::Outline(Canvas, X, Y + 12.f * S, BoxW, BoxH - 12.f * S, bPlacing ? FMath::Max(2.5f * S, 2.f) : FMath::Max(1.5f * S, 1.f), Edge);
		Label(FString::FromInt(i + 1), X + BoxW * 0.5f, Y + 14.f * S, Edge, Large, EAlign::Center, true, BoxW - 8.f * S);

		// The placed camera's type, or (for the slate being placed) the type the next click will build.
		if (Cam || bPlacing)
		{
			const bool bCrane = Cam ? Cam->GetRig() == EDCCamRig::Crane : D->GetTool() == EDCCamTool::Crane;
			Label(bCrane ? TEXT("CRANE") : TEXT("STATIC"), X + BoxW * 0.5f, Y + 14.f * S + Px(Large) * 1.05f, Edge, Small * 0.8f,
				EAlign::Center, true, BoxW - 8.f * S);
		}
		if (bPlacing)
		{
			const float HW = KeyHint(TEXT("T"), TEXT("Switch"), 0.f, 0.f, Small * 0.8f, DCUI::Gold, false);
			KeyHint(TEXT("T"), TEXT("Switch"), X + (BoxW - HW) * 0.5f, Y - Px(Small * 0.8f) * 1.45f, Small * 0.8f, DCUI::Gold);
		}
		X += BoxW + Gap;
	}
}

void ADCZombieHUD::GetHelpLines(ADCZombieDirector* D, TArray<TArray<TPair<FString, FString>>>& OutLines) const
{
	using FHint = TPair<FString, FString>;
	switch (D->GetPhase())
	{
	case EDCPhase::Preparation:
		if (D->IsViewingCamera())
		{
			OutLines.Add({ FHint(TEXT("MOUSE"), TEXT("Turn")), FHint(TEXT("WHEEL"), TEXT("Zoom")), FHint(TEXT("WASD"), TEXT("Move nearby")) });
			OutLines.Add({ FHint(TEXT("Q/E"), TEXT("Scrub")), FHint(TEXT("1-9"), TEXT("Switch camera")), FHint(TEXT("RMB"), TEXT("Step back")) });
		}
		else
		{
			OutLines.Add({ FHint(TEXT("T"), TEXT("Camera type")), FHint(TEXT("LMB"), TEXT("Place")), FHint(TEXT("RMB"), TEXT("Remove")),
				FHint(TEXT("WHEEL"), TEXT("Height")), FHint(TEXT("C"), TEXT("Clear")) });
			OutLines.Add({ FHint(TEXT("1-9"), TEXT("Look through")), FHint(TEXT("Q/E"), TEXT("Scrub")), FHint(TEXT("H"), TEXT("Rehearsal")),
				FHint(TEXT("TAB"), TEXT("Storyboard")) });
			OutLines.Add({ FHint(TEXT("ENTER"), D->IsNearClapper() ? TEXT("Hit the clapperboard - ACTION!") : TEXT("Action!")) });
		}
		break;
	case EDCPhase::Countdown:
	case EDCPhase::Filming:
		OutLines.Add({ FHint(TEXT("1-9"), TEXT("Cut")), FHint(TEXT("MOUSE"), TEXT("Aim")), FHint(TEXT("WHEEL"), TEXT("Zoom")), FHint(TEXT("WASD"), TEXT("Move")) });
		OutLines.Add({ FHint(TEXT("HOLD LMB"), TEXT("Pause recording")), FHint(TEXT("R"), TEXT("Restart")), FHint(TEXT("P"), TEXT("Pause")) });
		break;
	default:
		break;
	}
}

float ADCZombieHUD::HelpTop(ADCZombieDirector* D) const
{
	TArray<TArray<TPair<FString, FString>>> Lines;
	GetHelpLines(D, Lines);
	return Canvas->ClipY - Lines.Num() * Px(Small) * 1.5f - 16.f * S;
}

void ADCZombieHUD::DrawHelp(ADCZombieDirector* D)
{
	TArray<TArray<TPair<FString, FString>>> Lines;
	GetHelpLines(D, Lines);
	// With the reference inset in the bottom-right corner, centre the help in the space to its left.
	const float Room = D->GetReferenceInset() ? Canvas->ClipX * (1.f - InsetShare) - 24.f * S : Canvas->ClipX - 32.f * S;
	const float CenterX = D->GetReferenceInset() ? Room * 0.5f + 8.f * S : Canvas->ClipX * 0.5f;
	const float Spacing = 26.f * S;
	float Y = HelpTop(D);
	for (const TArray<TPair<FString, FString>>& Line : Lines)
	{
		// Shrink a line that doesn't fit its room.
		float Width = -Spacing;
		for (const auto& Hint : Line) Width += KeyHint(Hint.Key, Hint.Value, 0.f, 0.f, Small, DCUI::Ivory, false) + Spacing;
		const float Size = Width > Room ? Small * Room / Width : Small;
		Width = -Spacing;
		for (const auto& Hint : Line) Width += KeyHint(Hint.Key, Hint.Value, 0.f, 0.f, Size, DCUI::Ivory, false) + Spacing;
		float X = CenterX - Width * 0.5f;
		for (const auto& Hint : Line) X += KeyHint(Hint.Key, Hint.Value, X, Y, Size, DCUI::Ivory) + Spacing;
		Y += Px(Small) * 1.5f;
	}
}

void ADCZombieHUD::DrawReferenceInset(ADCZombieDirector* D)
{
	UTextureRenderTarget2D* Inset = D->GetReferenceInset();
	if (!Inset)
	{
		return;
	}
	// Flush in the bottom-right corner, in the film's own 2.12:1 shape (the help moves left of it).
	const float W = Canvas->ClipX * InsetShare;
	const float H = W / DCFrame::FilmAspect;
	const float X = Canvas->ClipX - W;
	const float Y = Canvas->ClipY - H;
	const float LabelH = Px(Small) * 1.4f;
	DCUI::Rect(Canvas, X - 4.f * S, Y - LabelH, W + 4.f * S, H + LabelH, FLinearColor::Black);
	DCUI::Disc(Canvas, FVector2D(X + 12.f * S, Y - LabelH * 0.5f), 7.f * S, DCUI::Gold);
	Label(TEXT("REFERENCE CUT"), X + 26.f * S, Y - LabelH + (LabelH - Px(Small) * 1.2f) * 0.5f, DCUI::Gold, Small, EAlign::Left, true, W - 30.f * S);
	DrawTexture(Inset, X, Y, W, H, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Opaque);
	if (const ADCBeat* Beat = D->GetReferenceBeatAt(D->GetSceneTime()))
	{
		Label(Beat->DisplayName, X + W * 0.5f, Y + H - Px(Small) * 1.4f, DCUI::Ivory, Small, EAlign::Center, true, W - 16.f * S);
	}
}

// ---------------------------------------------------------------------------------------------
// Review without a cinema (the cinema draws its own presentation)

void ADCZombieHUD::DrawResults(ADCZombieDirector* D)
{
	const TArray<FDCMultiCamBeat>& Beats = D->GetBeats();
	const float W = FMath::Min(1200.f * S, Canvas->ClipX * 0.9f);
	const float X = (Canvas->ClipX - W) * 0.5f;
	const float Pad = 36.f * S;
	const float CX = Canvas->ClipX * 0.5f;
	const float RowH = Px(Small) * 1.3f;
	const float H = Pad * 2.f + Px(Large) * 1.2f + Px(Body) * 1.5f + RowH * Beats.Num() + Px(Body) * 1.6f + Px(Large) * 1.4f + Px(Small) * 3.f;
	float Y = FMath::Max((Canvas->ClipY - H) * 0.5f, 10.f * S);
	DCUI::ClapperStripes(Canvas, X, Y, W, 36.f * S);
	Y += 40.f * S;
	DCUI::Rect(Canvas, X, Y, W, H, FLinearColor(0.03f, 0.03f, 0.04f, 0.94f));
	Y += Pad;

	const FDCMatch& Match = D->GetMatch();
	const bool bGood = D->IsGoodTake();
	Label(D->MovieTitle, CX, Y, DCUI::Ivory, Large, EAlign::Center, true, W - 2.f * Pad);
	Y += Px(Large) * 1.2f;
	Label(FString::Printf(TEXT("GRADE %s  -  %s"), *D->GetGrade(), *D->GetCutName()), CX, Y, bGood ? DCUI::Gold : DCUI::Rec, Body, EAlign::Center);
	Y += Px(Body) * 1.5f;
	for (int32 i = 0; i < Beats.Num(); ++i)
	{
		const FDCMultiCamBeat& State = Beats[i];
		const bool bGot = State.Status == EDCShotStatus::Captured;
		Label(FString::Printf(TEXT("SC %d   %s"), i + 1, *State.Beat->DisplayName), X + Pad, Y, bGot ? DCUI::Ivory : DCUI::Muted, Small, EAlign::Left, true, W * 0.65f);
		Label(bGot ? FString::Printf(TEXT("IN THE CAN  (CAM %d)"), State.CapturedBy) : FString(TEXT("MISSED")), X + W - Pad, Y,
			bGot ? DCUI::Good : DCUI::Rec, Small, EAlign::Right);
		Y += RowH;
	}
	Y += Px(Body) * 0.3f;
	Label(FString::Printf(TEXT("Position %.0f%%   |   Angle %.0f%%   |   Zoom %.0f%%   |   %d cuts"),
		Match.Position * 100.f, Match.Angle * 100.f, Match.Zoom * 100.f, D->GetCutCount()), CX, Y, DCUI::Ivory, Body, EAlign::Center, true, W - 2.f * Pad);
	Y += Px(Body) * 1.3f;
	Label(FString::Printf(TEXT("MATCH %.0f%%   (best %.0f%%)"), Match.Overall * 100.f, D->GetBestMatch() * 100.f), CX, Y, bGood ? DCUI::Gold : DCUI::Rec, Large, EAlign::Center);
	Y += Px(Large) * 1.4f;
	const float Weakest = FMath::Min3(Match.Position, Match.Angle, Match.Zoom);
	Label(D->GetGradeRank() == 0 ? TEXT("Shot for shot, that's the reference cut.")
		: Weakest == Match.Position ? TEXT("Your cameras stood far from the reference spots.")
		: Weakest == Match.Angle ? TEXT("Your cameras pointed elsewhere - match the reference framing.")
		: TEXT("Your lenses differ - zoom like the reference cut."), CX, Y, DCUI::Muted, Small, EAlign::Center, true, W - 2.f * Pad);
	Y += Px(Small) * 1.6f;
	const float W1 = KeyHint(TEXT("SPACE"), TEXT("Watch again"), 0.f, 0.f, Small, DCUI::Ivory, false);
	const float W2 = KeyHint(TEXT("ENTER"), TEXT("Back to the set"), 0.f, 0.f, Small, DCUI::Ivory, false);
	const float HX = CX - (W1 + W2 + 40.f * S) * 0.5f;
	KeyHint(TEXT("SPACE"), TEXT("Watch again"), HX, Y, Small, DCUI::Ivory);
	KeyHint(TEXT("ENTER"), TEXT("Back to the set"), HX + W1 + 40.f * S, Y, Small, DCUI::Ivory);
}

void ADCZombieHUD::DrawPremiere(ADCZombieDirector* D)
{
	// The finished film: letterboxed to the film's aspect (the bars match the engine's), no gameplay HUD.
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const float Bar = FMath::Max((H - W / DCFrame::FilmAspect) * 0.5f, Px(Small) * 2.f);
	const float Time = D->GetSceneTime();
	const float Length = FMath::Max(D->GetPremiereLength(), KINDA_SMALL_NUMBER);
	DCUI::Rect(Canvas, 0.f, 0.f, W, Bar, FLinearColor::Black);
	DCUI::Rect(Canvas, 0.f, H - Bar, W, Bar, FLinearColor::Black);

	if (Time < 3.5f)
	{
		Label(D->MovieTitle, W * 0.5f, H * 0.38f, DCUI::Ivory, Title, EAlign::Center);
		Label(TEXT("A FILM BY YOU"), W * 0.5f, H * 0.38f + Px(Title) * 1.3f, DCUI::Gold, Body, EAlign::Center);
	}

	const float X0 = W * 0.3f;
	const float TW = W * 0.34f;
	const float TY = H - Bar * 0.5f - 3.f * S;
	DCUI::Rect(Canvas, X0, TY, TW, 6.f * S, FLinearColor(1.f, 1.f, 1.f, 0.2f));
	DCUI::Rect(Canvas, X0, TY, TW * Time / Length, 6.f * S, Rose);
	FString JustCaptured;
	for (const FDCMultiCamBeat& State : D->GetBeats())
	{
		if (State.Status != EDCShotStatus::Captured)
		{
			continue;
		}
		DCUI::Rect(Canvas, X0 + TW * State.CapturedFilmTime / Length - 2.f, TY - 8.f * S, 4.f, 22.f * S, DCUI::Gold);
		if (!D->IsFilmFinished() && Time >= State.CapturedFilmTime && Time - State.CapturedFilmTime < 2.5f)
		{
			JustCaptured = FString::Printf(TEXT("IN THE CAN: %s"), *State.Beat->DisplayName);
		}
	}
	DCUI::Rect(Canvas, X0 + TW * Time / Length - 1.5f, TY - 8.f * S, 3.f, 22.f * S, DCUI::Ivory);
	if (!JustCaptured.IsEmpty())
	{
		Label(JustCaptured, W * 0.5f, H - Bar - Px(Body) * 1.8f, DCUI::Gold, Body, EAlign::Center);
	}
	const float BotY = H - Bar + (Bar - Px(Small) * 1.2f) * 0.5f;
	Label(DCUI::Timecode(Time) + TEXT("  /  ") + DCUI::Timecode(Length), 24.f * S, BotY, DCUI::Muted, Small, EAlign::Left, true, X0 - 48.f * S);
	Label(TEXT("SPACE pause   Q/E scrub   ENTER results"), W - 24.f * S, BotY, DCUI::Muted, Small, EAlign::Right, true, W - X0 - TW - 48.f * S);
	Label(D->MovieTitle, W * 0.5f, (Bar - Px(Small) * 1.2f) * 0.5f, DCUI::Ivory, Small, EAlign::Center);
	if (D->IsPremierePaused() && !D->IsFilmFinished())
	{
		Label(TEXT("PAUSED"), W * 0.5f, H * 0.5f - Px(Large) * 0.6f, DCUI::Ivory, Large, EAlign::Center);
	}
}
