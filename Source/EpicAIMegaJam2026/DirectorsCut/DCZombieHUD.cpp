// Director's Cut - canvas HUD, styled like a film set (see DCUI for the building blocks).

#include "DCZombieHUD.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "DCBeat.h"
#include "DCCinemaReplay.h"
#include "DCMultiCam.h"
#include "DCSceneMarks.h"
#include "DCZombieDirector.h"
#include "Camera/PlayerCameraManager.h"
#include "CanvasTypes.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
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
	/** How long the perfect-shot burst plays, and the least time between two of them. */
	constexpr float PerfectBurstLength = 1.6f;
	constexpr float PerfectBurstCooldown = 2.f;
	/** Below the perfect threshold by this much before the shot stops counting as perfect. */
	constexpr float PerfectGive = 0.06f;
	/** How long a storyboard scene's tick-off animation plays. */
	constexpr float TickOffSeconds = 1.2f;
	const FLinearColor FarColor(1.f, 0.08f, 0.06f);
	const FLinearColor CloseColor(0.1f, 0.55f, 1.f);
	const FLinearColor PerfectGlow(0.55f, 0.85f, 1.f);

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
	static ConstructorHelpers::FObjectFinder<UTexture2D> DirDefault(TEXT("/Game/DirectorsCut/UI/Director/Director_Default.Director_Default"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> DirAlmost(TEXT("/Game/DirectorsCut/UI/Director/Director_Almost.Director_Almost"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> DirDisgust(TEXT("/Game/DirectorsCut/UI/Director/Director_Disgust.Director_Disgust"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> DirJoy(TEXT("/Game/DirectorsCut/UI/Director/Director_Joy.Director_Joy"));
	DirectorDefault = DirDefault.Object;
	DirectorAlmost = DirAlmost.Object;
	DirectorDisgust = DirDisgust.Object;
	DirectorJoy = DirJoy.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> DirThatsIt(TEXT("/Game/DirectorsCut/UI/Director/Director_ThatsIt.Director_ThatsIt"));
	DirectorThatsIt = DirThatsIt.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> Menu(TEXT("/Game/DirectorsCut/UI/MainMenu.MainMenu"));
	MainMenuImage = Menu.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> TitleTypography(TEXT("/Game/DirectorsCut/UI/TitleTypography.TitleTypography"));
	TitleImage = TitleTypography.Object;
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
	const float Pad = 14.f * S;
	const float BoxW = KeySize.X + 2.f * Pad;
	const float BoxH = Px(Size) * 1.2f;
	if (bDraw)
	{
		DCUI::Rect(Canvas, X, Y, BoxW, BoxH, FLinearColor(1.f, 1.f, 1.f, 0.08f));
		DCUI::Outline(Canvas, X, Y, BoxW, BoxH, FMath::Max(1.f, 1.5f * S), WithAlpha(DCUI::Gold, 0.8f));
		DCUI::Text(Canvas, GetTextFont(), Key, X + Pad, Y + (BoxH - KeySize.Y) * 0.5f, KeyPx, DCUI::Gold, EAlign::Left, false);
		DCUI::Text(Canvas, GetTextFont(), Action, X + BoxW + 18.f * S, Y + (BoxH - ActionSize.Y) * 0.5f, Px(Size), Color);
	}
	return BoxW + 18.f * S + ActionSize.X;
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
	if (D->IsInMainMenu())
	{
		DrawMainMenu(D);
	}
	else if (D->IsInCutscene())
	{
		DrawCutscene(D);
	}
	else
	{
		DrawPhase(D);
	}
	// Fades between phases go over everything.
	if (D->GetFadeAlpha() > 0.f)
	{
		DCUI::Rect(Canvas, 0.f, 0.f, Canvas->ClipX, Canvas->ClipY, FLinearColor(0.f, 0.f, 0.f, D->GetFadeAlpha()));
	}
	MaskOutsideView();
}

void ADCZombieHUD::MaskOutsideView()
{
	// The canvas doesn't clip: anything drawn past the 16:9 view (a drifting background, a spotlight, the cinema's
	// back row) would show in the black bars around it. Paint the bars back to black.
	const FVector2D Bars = GetBarsOffset();
	if (Bars.X <= 0.f && Bars.Y <= 0.f)
	{
		return;
	}
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const float Pad = 2.f; // past the window edge, so nothing peeks through at fractional offsets
	const FLinearColor Black = FLinearColor::Black;
	DCUI::Rect(Canvas, -Bars.X - Pad, -Bars.Y - Pad, W + 2.f * (Bars.X + Pad), Bars.Y + Pad, Black);
	DCUI::Rect(Canvas, -Bars.X - Pad, H, W + 2.f * (Bars.X + Pad), Bars.Y + Pad, Black);
	DCUI::Rect(Canvas, -Bars.X - Pad, 0.f, Bars.X + Pad, H, Black);
	DCUI::Rect(Canvas, W, 0.f, Bars.X + Pad, H, Black);
}

void ADCZombieHUD::DrawPhase(ADCZombieDirector* D)
{
	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.5f;

	if (D->GetPhase() == EDCPhase::Reference)
	{
		if (D->IsReferencePlaying())
		{
			DrawReferenceCut(D);
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

 if (D->IsSetOverview()) return;
 const bool bPrep = D->GetPhase() == EDCPhase::Preparation;
 FocusProgress = bPrep ? FMath::Clamp(FocusProgress + (D->IsReferenceFocused() ? 1.f : -1.f) * GetWorld()->GetDeltaSeconds() / 0.35f, 0.f, 1.f) : 0.f;
 FocusEase = FMath::SmoothStep(0.f, 1.f, FocusProgress);
 if (FocusProgress > 0.f)
 {
  DCUI::Rect(Canvas, 0.f, 0.f, Canvas->ClipX, Canvas->ClipY, FLinearColor(0.f, 0.f, 0.f, 0.72f * FocusEase));
  DrawMissionPanel(D);
  DrawSwitcher(D);
  DrawReferenceInset(D);
  DrawPlanPreview(D);
  return;
 }

	// While viewing a camera, dim everything outside the capture frame and frame it with the field monitor, its screen fitted
	// exactly over the capture frame. Drawn first so the HUD stays on top.
	if ((D->GetPhase() == EDCPhase::Preparation || D->GetPhase() == EDCPhase::Countdown || D->GetPhase() == EDCPhase::Filming) && D->IsViewingCamera())
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
		DrawLikenessVignette(D);
	}
	else
	{
		ResetLikenessVignette();
	}

	if (D->GetPhase() == EDCPhase::Preparation)
	{
		DrawMissionPanel(D);
	}
	if (D->IsViewingCamera())
	{
		DrawFraming(D);
	}
	else
	{
		DCUI::Corners(Canvas, CX - 9.f * S, CY - 9.f * S, 18.f * S, 18.f * S, 6.f * S, FMath::Max(1.5f * S, 1.f), DCUI::Ivory);
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
	DrawCoach(D);
	DrawSwitcher(D);
	DrawCheer(D);
	if (D->GetPhase() == EDCPhase::Preparation)
	{
		DrawPlanPreview(D); // Keep timeline controls readable throughout the cheer animation.
		if (D->IsViewingCamera())
		{
			if (D->IsPlacementPending())
			{
				// Rasterize at a fixed font size and animate its float render scale: no integer font-size steps.
				const float Pulse = 1.f + 0.03f * (1.f + FMath::Sin(GetWorld()->GetRealTimeSeconds() * 2.f));
				const TCHAR* Parts[] = { TEXT("Press"), TEXT("Left Click"), TEXT("to confirm install or"), TEXT("Esc"), TEXT("to cancel") };
				float Widths[5];
				float Total = 0.f;
				const float FontPx = Px(Small);
				const float KeyPad = 14.f * S;
                const float PartGap = 18.f * S;
				const float TextH = DCUI::Measure(Canvas, GetTextFont(), TEXT("Left Click"), FontPx).Y;
				for (int32 i = 0; i < 5; ++i)
				{
					Widths[i] = DCUI::Measure(Canvas, GetTextFont(), Parts[i], FontPx).X + ((i == 1 || i == 3) ? KeyPad * 2.f : 0.f);
					Total += Widths[i] + (i < 4 ? PartGap : 0.f) + (i == 2 ? 20.f * S : 0.f);
				}
				const float Fit = FMath::Min(1.f, Canvas->ClipX * 0.8f / (Total * 1.06f));
				const float Scale = Pulse * Fit;
				const float CenterY = 150.f * S + TextH * 0.5f;
				const float PanelW = Total * 1.06f * Fit + 24.f * S;
				const float PanelH = (TextH + KeyPad * 2.f) * 1.06f * Fit + 12.f * S;
				DCUI::Rect(Canvas, CX - PanelW * 0.5f, CenterY - PanelH * 0.5f, PanelW, PanelH, FLinearColor(0.f, 0.f, 0.f, 0.5f));
				float X = CX - Total * Scale * 0.5f;
				for (int32 i = 0; i < 5; ++i)
				{
					const bool bKey = i == 1 || i == 3;
                    const FLinearColor KeyColor = i == 1 ? DCUI::Good : DCUI::Rec;
					if (bKey)
					{
						DCUI::Outline(Canvas, X, CenterY - (TextH * 0.5f + KeyPad) * Scale,
							Widths[i] * Scale, (TextH + KeyPad * 2.f) * Scale, FMath::Max(1.f, 1.5f * S), KeyColor);
					}
					DCUI::Text(Canvas, GetTextFont(), Parts[i], X + (bKey ? KeyPad * Scale : 0.f), CenterY - TextH * Scale * 0.5f,
						FontPx, bKey ? KeyColor : DCUI::Ivory, EAlign::Left, false, 0.f, Scale);
					X += (Widths[i] + PartGap + (i == 2 ? 20.f * S : 0.f)) * Scale;
				}
			}
			else
			{
				Label(TEXT("Press Esc to exit camera"), CX, 150.f * S, DCUI::Ivory, Small, EAlign::Center);
			}
		}
		else if (D->GetNearbyCameraSlot() != INDEX_NONE)
		{
			const float KeyPx = Px(Small) * 0.82f * 2.f;
			const FVector2D KeySize = DCUI::Measure(Canvas, GetTextFont(), TEXT("F"), KeyPx);
			const float W = KeySize.X + 32.f * S;
			const float H = Px(Small) * 2.4f;
			const float Y = CY + 50.f * S;
			DCUI::Rect(Canvas, CX - W * 0.5f, Y, W, H, FLinearColor(0.f, 0.f, 0.f, 0.72f));
			DCUI::Outline(Canvas, CX - W * 0.5f, Y, W, H, FMath::Max(1.f, 2.f * S), DCUI::Gold);
			DCUI::Text(Canvas, GetTextFont(), TEXT("F"), CX, Y + (H - KeySize.Y) * 0.5f,
				KeyPx, DCUI::Gold, EAlign::Center, false);
		}
	}

	if (UGameplayStatics::IsGamePaused(this))
	{
		DCUI::Rect(Canvas, 0.f, 0.f, Canvas->ClipX, Canvas->ClipY, FLinearColor(0.f, 0.f, 0.f, 0.7f));
		Label(TEXT("PAUSED"), CX, CY - Px(Title), DCUI::Ivory, Title, EAlign::Center);
		const float W = KeyHint(TEXT("P"), TEXT("Resume"), 0.f, 0.f, Body, DCUI::Ivory, false);
		KeyHint(TEXT("P"), TEXT("Resume"), CX - W * 0.5f, CY + 20.f * S, Body, DCUI::Ivory);
	}
}

// ---------------------------------------------------------------------------------------------
// Main menu and cutscenes

FVector2D ADCZombieHUD::GetBarsOffset() const
{
	// Exactly where the engine put the canvas in the window (past the left and top bars): its translation.
	if (Canvas && Canvas->Canvas)
	{
		const auto& Stack = Canvas->Canvas->GetTransformStack();
		if (Stack.Num() > 0)
		{
			return FVector2D(Stack.Top().GetMatrix().GetOrigin());
		}
	}
	return FVector2D::ZeroVector;
}

void ADCZombieHUD::DrawMainMenu(ADCZombieDirector* D)
{
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const float Now = GetWorld()->GetRealTimeSeconds();
	DCUI::Rect(Canvas, 0.f, 0.f, W, H, FLinearColor::Black);

	// The background covers the screen (cropped, never stretched), breathing in and out very slowly.
	if (MainMenuImage && MainMenuImage->GetSurfaceWidth() > 0.f && MainMenuImage->GetSurfaceHeight() > 0.f)
	{
		const float IW = MainMenuImage->GetSurfaceWidth();
		const float IH = MainMenuImage->GetSurfaceHeight();
		const float Cover = FMath::Max(W / IW, H / IH) * (1.04f + 0.02f * FMath::Sin(Now * 0.15f));
		DrawTexture(MainMenuImage, (W - IW * Cover) * 0.5f, (H - IH * Cover) * 0.5f, IW * Cover, IH * Cover, 0.f, 0.f, 1.f, 1.f);
	}
	const FDCMainMenuLayout& Layout = D->MainMenu;

	// The title typography, bottom-left, its proportions kept.
	if (TitleImage && TitleImage->GetSurfaceWidth() > 0.f)
	{
		const float TW = W * Layout.TitleWidth;
		const float TH = TW * TitleImage->GetSurfaceHeight() / TitleImage->GetSurfaceWidth();
		DrawTexture(TitleImage, W * Layout.TitleCorner.X, H * Layout.TitleCorner.Y - TH, TW, TH, 0.f, 0.f, 1.f, 1.f);
	}

	// "Press [ENTER] to Start", bottom-right and right-aligned: the key in a dark key cap, the words shadowed so they
	// read over the artwork, a thin gold rule underneath; it all breathes gently.
	const float TextPx = Px(Layout.PromptTextSize);
	const float KeyPx = TextPx * 0.8f;
	const FString Before = TEXT("Press"), Key = TEXT("ENTER"), After = TEXT("to Start");
	const float Gap = TextPx * 0.35f, KeyPad = TextPx * 0.4f;
	const float BeforeW = DCUI::Measure(Canvas, GetTextFont(), Before, TextPx).X;
	const float AfterW = DCUI::Measure(Canvas, GetTextFont(), After, TextPx).X;
	const float KeyW = DCUI::Measure(Canvas, GetTextFont(), Key, KeyPx).X + 2.f * KeyPad;
	const float KeyH = TextPx * 1.35f;
	const float TotalW = BeforeW + Gap + KeyW + Gap + AfterW;
	const float Right = W * Layout.PromptCorner.X, MidY = H * Layout.PromptCorner.Y;
	const float Breath = 0.7f + 0.3f * FMath::Sin(Now * 2.5f);
	float X = Right - TotalW;
	DCUI::Text(Canvas, GetTextFont(), Before, X, MidY - TextPx * 0.6f, TextPx, WithAlpha(DCUI::Ivory, Breath));
	X += BeforeW + Gap;
	DCUI::Rect(Canvas, X, MidY - KeyH * 0.5f, KeyW, KeyH, FLinearColor(0.02f, 0.02f, 0.03f, 0.85f));
	DCUI::Outline(Canvas, X, MidY - KeyH * 0.5f, KeyW, KeyH, FMath::Max(2.5f * S, 2.f), DCUI::Gold);
	DCUI::Text(Canvas, GetTextFont(), Key, X + KeyPad, MidY - KeyPx * 0.6f, KeyPx, DCUI::Gold, EAlign::Left, false);
	X += KeyW + Gap;
	DCUI::Text(Canvas, GetTextFont(), After, X, MidY - TextPx * 0.6f, TextPx, WithAlpha(DCUI::Ivory, Breath));
	DCUI::Rect(Canvas, Right - TotalW, MidY + KeyH * 0.5f + 10.f * S, TotalW, FMath::Max(3.f * S, 2.f), WithAlpha(DCUI::Gold, Breath));
}

void ADCZombieHUD::DrawCutscene(ADCZombieDirector* D)
{
	const FDCCutsceneLine* Line = D->GetCutsceneLine();
	if (!Line)
	{
		return;
	}
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const float Since = GetWorld()->GetRealTimeSeconds() - D->GetCutsceneLineStart();
	const FVector2D Spot(W * 0.27f, H * 0.6f);

	// A dark stage with a warm spotlight where he stands.
	DCUI::Rect(Canvas, 0.f, 0.f, W, H, FLinearColor(0.035f, 0.03f, 0.04f, 0.93f));
	for (int32 i = 0; i < 6; ++i)
	{
		DCUI::Disc(Canvas, Spot, H * (0.62f - i * 0.08f), FLinearColor(1.f, 0.78f, 0.45f, 0.035f), 64);
	}

	// Cinema framing: a soft dark edge and slim letterbox bars.
	DCUI::Vignette(Canvas, 0.f, 0.f, W, H, H * 0.3f, FLinearColor(0.f, 0.f, 0.f, 0.6f));
	const float Bar = H * 0.06f;
	DCUI::Rect(Canvas, 0.f, 0.f, W, Bar, FLinearColor::Black);
	DCUI::Rect(Canvas, 0.f, H - Bar, W, Bar, FLinearColor::Black);

	// The director, large, standing in front of the bottom bar. Each new line pops him in (back-out ease).
	float PortraitRight = W * 0.4f;
	if (UTexture2D* Portrait = Line->Portrait)
	{
		const float T = FMath::Clamp(Since / 0.35f, 0.f, 1.f) - 1.f;
		const float Pop = 1.f + 2.70158f * T * T * T + 1.70158f * T * T;
		const float PH = H * 0.8f * (0.88f + 0.12f * Pop);
		const float PW = PH * (Portrait->GetSurfaceHeight() > 0.f ? Portrait->GetSurfaceWidth() / Portrait->GetSurfaceHeight() : 1.f);
		const float PX = Spot.X - PW * 0.5f;
		DrawTexture(Portrait, PX, H - PH, PW, PH, 0.f, 0.f, 1.f, 1.f);
		PortraitRight = PX + PW;
	}

	// His speech bubble on the right, sized to the wrapped line.
	const float Pad = 30.f * S;
	const float TextPx = Px(Large * 1.05f);
	const float BX = FMath::Max(PortraitRight - 60.f * S, W * 0.4f);
	const float BW = W * 0.94f - BX;
	TArray<FString> Lines;
	DCUI::Wrap(Canvas, GetTextFont(), Line->Line, TextPx, BW - 2.f * Pad, Lines);
	const float LineH = TextPx * 1.25f;
	const float NameH = Px(Small) * 1.5f;
	const float HintH = Px(Small) * 1.7f;
	const float BH = 2.f * Pad + NameH + Lines.Num() * LineH + HintH;
	const float BY = FMath::Min(H * 0.48f - BH * 0.5f, H - Bar - 30.f * S - BH);
	const FLinearColor Paper(0.96f, 0.93f, 0.86f, 0.98f);
	const FLinearColor Ink(0.08f, 0.07f, 0.07f);
	DCUI::Triangle(Canvas, FVector2D(BX + 2.f, BY + BH * 0.3f), FVector2D(BX + 2.f, BY + BH * 0.58f), FVector2D(BX - 80.f * S, BY + BH * 0.36f), Paper);
	DCUI::Rect(Canvas, BX, BY, BW, BH, Paper);
	DCUI::Rect(Canvas, BX, BY, FMath::Max(7.f * S, 3.f), BH, DCUI::Gold);
	DCUI::Outline(Canvas, BX, BY, BW, BH, FMath::Max(2.f * S, 1.f), FLinearColor(0.f, 0.f, 0.f, 0.35f));
	Label(TEXT("THE DIRECTOR"), BX + Pad, BY + Pad * 0.7f, FLinearColor(0.55f, 0.08f, 0.06f), Small, EAlign::Left, false);

	// The line types itself out across its wrapped lines.
	int32 Remaining = FMath::FloorToInt(Since * ADCMultiCamDirector::CutsceneTypeRate);
	float Y = BY + Pad * 0.7f + NameH;
	for (const FString& Text : Lines)
	{
		if (Remaining > 0)
		{
			DCUI::Text(Canvas, GetTextFont(), Text.Left(Remaining), BX + Pad + 6.f * S, Y, TextPx, Ink, EAlign::Left, false);
		}
		Remaining -= Text.Len() + 1;
		Y += LineH;
	}

	// Which line of how many, and (once it's all typed) how to go on.
	const int32 Count = D->GetCutsceneLineCount();
	const float DotY = BY + BH - Pad - HintH * 0.35f;
	for (int32 i = 0; i < Count; ++i)
	{
		DCUI::Disc(Canvas, FVector2D(BX + Pad + 8.f * S + i * 26.f * S, DotY), 7.f * S,
			i == D->GetCutsceneLineIndex() ? DCUI::Gold : FLinearColor(0.f, 0.f, 0.f, 0.2f), 20);
	}
	if (Remaining >= -1 && FMath::Fmod(GetWorld()->GetRealTimeSeconds(), 1.f) < 0.7f) // -1: every character shown
	{
		const float HintW = KeyHint(TEXT("ENTER"), TEXT("Continue"), 0.f, 0.f, Small, Ink, false);
		const float HintX = BX + BW - Pad - HintW;
		const float HintY = BY + BH - Pad - HintH * 0.85f;
		// A dark key cap, so the gold key reads on the cream paper.
		const float KeyW = DCUI::Measure(Canvas, GetTextFont(), TEXT("ENTER"), Px(Small) * 0.82f).X + 16.f * S;
		DCUI::Rect(Canvas, HintX, HintY, KeyW, Px(Small) * 1.2f, Ink);
		KeyHint(TEXT("ENTER"), TEXT("Continue"), HintX, HintY, Small, Ink);
	}
}

// ---------------------------------------------------------------------------------------------
// Reference film

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
	const FString Map = UGameplayStatics::GetCurrentLevelName(this, true);
	const FString ReferenceTitle = Map.Contains(TEXT("Act1")) ? TEXT("STAGE 1 REFERENCE CUT")
		: Map.Contains(TEXT("Act2")) ? TEXT("STAGE 2 REFERENCE CUT") : TEXT("REFERENCE CUT");
	Label(ReferenceTitle, W * 0.5f, TopY, DCUI::Gold, Small, EAlign::Center);

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
	if (D->IsReferencePaused())
	{
		Label(TEXT("PAUSED"), W * 0.5f, H * 0.5f - Px(Large) * 0.6f, DCUI::Ivory, Large, EAlign::Center);
	}
}

// ---------------------------------------------------------------------------------------------
// Preparation and shooting

void ADCZombieHUD::DrawMissionPanel(ADCZombieDirector* D)
{
	// Preparation missions: always visible at the top right.
	const TArray<FDCMultiCamBeat>& Beats = D->GetBeats();
	const float W = Canvas->ClipX * 0.25f;
	const float X = Canvas->ClipX - W - 20.f * S + (W + 40.f * S) * FocusEase;
	float Y = 18.f * S;
	const float Band = 14.f * S;
	const float RowH = Px(Small) * 1.35f;
	const float H = Band * 2.f + Px(Small) * 1.5f + RowH * Beats.Num() + 16.f * S;
	DCUI::Rect(Canvas, X, Y, W, H, DCUI::Fill);
	DCUI::Sprockets(Canvas, X, Y, W, Band);
	DCUI::Sprockets(Canvas, X, Y + H - Band, W, Band);
 // Fit the entire hint as one row, keeping the key padding and gap proportional.
 const float HintPx = Px(Small);
 const FVector2D PromptSize = DCUI::Measure(Canvas, GetTextFont(), TEXT("Ready to film?"), HintPx);
 const FVector2D EnterSize = DCUI::Measure(Canvas, GetTextFont(), TEXT("ENTER"), HintPx * 0.82f);
 const float HintPad = 14.f * S, HintGap = 20.f * S;
 const float EnterW = EnterSize.X + HintPad * 2.f;
 const float HintScale = FMath::Min(1.f, W / (PromptSize.X + HintGap + EnterW));
 const float HintY = Y + H + 12.f * S;
 const float HintH = FMath::Max(PromptSize.Y, EnterSize.Y + HintPad) * HintScale;
 DCUI::Text(Canvas, GetTextFont(), TEXT("Ready to film?"), X, HintY + (HintH - PromptSize.Y * HintScale) * 0.5f,
  HintPx, DCUI::Ivory, EAlign::Left, true, 0.f, HintScale);
 const float EnterX = X + (PromptSize.X + HintGap) * HintScale;
 DCUI::Outline(Canvas, EnterX, HintY, EnterW * HintScale, HintH, FMath::Max(1.f, 1.5f * S), DCUI::Gold);
 DCUI::Text(Canvas, GetTextFont(), TEXT("ENTER"), EnterX + HintPad * HintScale, HintY + (HintH - EnterSize.Y * HintScale) * 0.5f,
  HintPx * 0.82f, DCUI::Gold, EAlign::Left, false, 0.f, HintScale);
	Y += Band + 6.f * S;
	Label(TEXT("MISSION"), X + 16.f * S, Y, DCUI::Gold, Small);
	Y += Px(Small) * 1.5f;
	// A scene just ticked off: its name swells and is struck through, drawn last so it sits over its neighbours.
	struct FTickOff { FString Name; float TextX, MidY, Age; FVector2D Box; };
	TArray<FTickOff, TInlineAllocator<2>> TickOffs;
	const float Now = GetWorld()->GetRealTimeSeconds();
	for (int32 i = 0; i < Beats.Num(); ++i)
	{
		const FDCMultiCamBeat& State = Beats[i];
		// Ticked off (struck through, like a to-do list) once a camera stands on the scene's reference spot.
		const bool bCovered = D->IsSceneCovered(i);
		const float Age = Now - D->GetSceneCoveredTime(i);
		const bool bTicking = bCovered && State.Status == EDCShotStatus::Pending && D->GetSceneCoveredTime(i) >= 0.f && Age < TickOffSeconds;
		// The box stamps down: big, then snapping to size.
		const float Box = Px(Small) * 0.6f * (bTicking ? 1.f + 0.9f * FMath::Square(1.f - FMath::Clamp(Age / 0.25f, 0.f, 1.f)) : 1.f);
		const float NormalBox = Px(Small) * 0.6f;
		const float BX = X + 16.f * S - (Box - NormalBox) * 0.5f;
		const float BY = Y + (RowH - Box) * 0.5f;
		if (bTicking)
		{
			DCUI::Rect(Canvas, X + 4.f * S, Y, W - 8.f * S, RowH, WithAlpha(DCUI::Gold, 0.35f * FMath::Clamp(1.f - Age / 0.6f, 0.f, 1.f)));
		}
		if (State.Status == EDCShotStatus::Captured)
		{
			DCUI::Rect(Canvas, X + 16.f * S, BY, Box, Box, DCUI::Good);
		}
		else if (bCovered && State.Status == EDCShotStatus::Pending)
		{
			DCUI::Rect(Canvas, BX, BY, Box, Box, DCUI::Gold);
		}
		else if (State.Status == EDCShotStatus::Missed)
		{
			DCUI::Rect(Canvas, X + 16.f * S, BY, Box, Box, WithAlpha(DCUI::Rec, 0.6f));
		}
		else
		{
			DCUI::Outline(Canvas, X + 16.f * S, BY, Box, Box, FMath::Max(1.5f * S, 1.f), DCUI::Muted);
		}
		const FLinearColor Color = State.Status == EDCShotStatus::Captured ? DCUI::Good : State.Status == EDCShotStatus::Missed ? DCUI::Muted
			: bCovered ? DCUI::Muted : DCUI::Ivory;
		const float TextX = X + 16.f * S + NormalBox + 12.f * S;
		const float TextY = Y + (RowH - Px(Small) * 1.2f) * 0.5f;
		const FString Name = FString::Printf(TEXT("%d. %s"), i + 1, *State.Beat->DisplayName);
		if (bTicking)
		{
			TickOffs.Add({ Name, TextX, Y + RowH * 0.5f, Age, FVector2D(X + 16.f * S + NormalBox * 0.5f, Y + RowH * 0.5f) });
		}
		else
		{
			const FVector2D Size = Label(Name, TextX, TextY, Color, Small, EAlign::Left, true, W - NormalBox - 44.f * S);
			if (bCovered)
			{
				DCUI::Rect(Canvas, TextX - 4.f * S, TextY + Size.Y * 0.52f, Size.X + 8.f * S, FMath::Max(2.f * S, 1.5f), DCUI::Gold);
			}
		}
		Y += RowH;
	}
	for (const FTickOff& Tick : TickOffs)
	{
		DrawTickOff(Tick.Name, Tick.TextX, Tick.MidY, Tick.Box, Tick.Age);
	}
}

void ADCZombieHUD::DrawTickOff(const FString& Name, float TextX, float MidY, const FVector2D& Box, float Age)
{
	auto Ease = [](float Alpha, float Exp) { return FMath::InterpEaseOut(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f), Exp); };

	// A ring and a spray of sparkles out of the checkbox.
	const float Ring = FMath::Clamp(Age / 0.5f, 0.f, 1.f);
	if (Ring < 1.f)
	{
		DCUI::Ring(Canvas, Box, Px(Small) * (0.4f + 1.6f * Ease(Ring, 2.f)), FMath::Max(5.f * S * (1.f - Ring), 1.f), WithAlpha(DCUI::Gold, 1.f - Ring), 40);
	}
	const FLinearColor Tints[2] = { DCUI::Gold, DCUI::Ivory };
	FRandomStream Rng(FCrc::StrCrc32(*Name));
	constexpr int32 Count = 14;
	for (int32 i = 0; i < Count; ++i)
	{
		const float Angle = 2.f * PI * (i + Rng.FRand() * 0.7f) / Count;
		const float Reach = Rng.FRandRange(50.f, 120.f) * S;
		const float Life = Rng.FRandRange(0.5f, 0.9f);
		const float Radius = Rng.FRandRange(6.f, 12.f) * S;
		const float K = Age / Life;
		if (K < 1.f)
		{
			const FVector2D P = Box + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Reach * Ease(K, 2.f);
			DCUI::Sparkle(Canvas, P, Radius * (1.f - 0.7f * K), WithAlpha(Tints[i % 2], 1.f - K * K));
		}
	}

	// The name swells up fast, holds, and settles back while a gold stroke strikes it through.
	const float Swell = Age < 0.2f ? Ease(Age / 0.2f, 3.f) : 1.f - FMath::SmoothStep(0.55f, TickOffSeconds, Age);
	const float Size = Small * (1.f + 0.75f * Swell);
	const FVector2D Measured = DCUI::Measure(Canvas, GetTextFont(), Name, Px(Size));
	// Grown names shift left so they stay on screen.
	const float NameX = FMath::Min(TextX, Canvas->ClipX - 16.f * S - Measured.X);
	const float NameY = MidY - Px(Size) * 0.6f;
	const FLinearColor Color = FLinearColor::LerpUsingHSV(DCUI::Muted, DCUI::Gold, Swell);
	const FVector2D Drawn = Label(Name, NameX, NameY, Color, Size, EAlign::Left, true, Canvas->ClipX - 32.f * S);
	const float Strike = Ease((Age - 0.15f) / 0.35f, 2.f);
	if (Strike > 0.f)
	{
		DCUI::Rect(Canvas, NameX - 4.f * S, NameY + Drawn.Y * 0.52f, (Drawn.X + 8.f * S) * Strike, FMath::Max(3.f * S * (1.f + Swell), 1.5f), DCUI::Gold);
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
		const FVector2D Size = Label(TEXT("REC"), RX, RY, DCUI::Ivory, Body, EAlign::Right);
		if (FMath::Fmod(GetWorld()->GetRealTimeSeconds(), 1.f) < 0.6f)
		{
			DCUI::Disc(Canvas, FVector2D(RX - Size.X - 22.f * S, RY + Size.Y * 0.5f), 11.f * S, DCUI::Rec);
		}
	}

}

void ADCZombieHUD::ResetLikenessVignette()
{
	VignetteFade = 0.f;
	bPerfectShot = false;
	LastVignetteTime = -1.f;
}

void ADCZombieHUD::DrawLikenessVignette(ADCZombieDirector* D)
{
	const float Now = GetWorld()->GetRealTimeSeconds();
	const float Dt = LastVignetteTime < 0.f ? 0.f : FMath::Clamp(Now - LastVignetteTime, 0.f, 0.1f);
	LastVignetteTime = Now;

	const float Likeness = D->GetPhase() == EDCPhase::Filming ? D->GetLiveLikeness() : -1.f;
	const bool bReading = Likeness >= 0.f;
	if (bReading)
	{
		// Snap to the first reading, then ease, so the colour sweeps instead of flickering.
		ShownLikeness = VignetteFade <= 0.f ? Likeness : FMath::FInterpTo(ShownLikeness, Likeness, Dt, 5.f);
	}
	VignetteFade = FMath::FInterpConstantTo(VignetteFade, bReading ? 1.f : 0.f, Dt, 3.f);

	// Perfect from the director's threshold; it takes a clear slip below it to lose it again.
	const float Perfect = D->PerfectLikeness;
	const bool bWasPerfect = bPerfectShot;
	bPerfectShot = bReading && Likeness >= (bPerfectShot ? Perfect - PerfectGive : Perfect);
	if (bPerfectShot && !bWasPerfect && Now - PerfectBurstStart > PerfectBurstCooldown)
	{
		PerfectBurstStart = Now;
		++PerfectBurstCount;
	}

	if (VignetteFade > 0.f)
	{
		// Around the whole screen, so it frames the monitor rather than crowding the shot.
		const float W = Canvas->ClipX;
		const float H = Canvas->ClipY;
		// 0 = far off (red), 1 = there (blue), by way of magenta and violet (HSV) so it stays vivid in between.
		const float Close = FMath::SmoothStep(0.3f, Perfect, ShownLikeness);
		const FLinearColor Color = FLinearColor::LerpUsingHSV(FarColor, CloseColor, Close);
		// Strong and throbbing while it's wrong, calmer as it closes in, a slow breath once perfect.
		const float Throb = 1.f + 0.18f * (1.f - Close) * FMath::Sin(Now * 7.f);
		const float Breath = bPerfectShot ? 0.85f + 0.15f * FMath::Sin(Now * 3.f) : 1.f;
		const float Alpha = VignetteStrength * FMath::Lerp(1.f, 0.6f, Close) * Throb * Breath * VignetteFade;
		const float Depth = H * VignetteDepth * FMath::Lerp(1.f, 0.75f, Close);
		// A wide soft layer plus a tight one: falls off like a lens vignette rather than a straight ramp.
		DCUI::Vignette(Canvas, 0.f, 0.f, W, H, Depth, WithAlpha(Color, Alpha * 0.55f));
		DCUI::Vignette(Canvas, 0.f, 0.f, W, H, Depth * 0.35f, WithAlpha(Color, Alpha * 0.6f));
	}
	const float Age = Now - PerfectBurstStart;
	if (Age < PerfectBurstLength)
	{
		float L, T, R, B;
		GetCaptureRect(L, T, R, B);
		DrawPerfectBurst(L, T, R, B, Age);
	}
}

void ADCZombieHUD::DrawPerfectBurst(float L, float T, float R, float B, float Age)
{
	auto Ease = [](float Alpha, float Exp) { return FMath::InterpEaseOut(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f), Exp); };
	const float Fade = 1.f - FMath::Clamp((Age - (PerfectBurstLength - 0.4f)) / 0.4f, 0.f, 1.f);

	// 1. A camera flash over the frame.
	const float Flash = 1.f - FMath::Clamp(Age / 0.3f, 0.f, 1.f);
	if (Flash > 0.f)
	{
		DCUI::Rect(Canvas, L, T, R - L, B - T, FLinearColor(0.8f, 0.93f, 1.f, 0.4f * Flash * Flash));
	}

	// 2. A shockwave: the frame's outline racing outward.
	const float Wave = FMath::Clamp(Age / 0.6f, 0.f, 1.f);
	if (Wave < 1.f)
	{
		const float Grow = 90.f * S * Ease(Wave, 2.f);
		DCUI::Outline(Canvas, L - Grow, T - Grow, R - L + 2.f * Grow, B - T + 2.f * Grow, FMath::Max(10.f * S * (1.f - Wave), 1.f),
			WithAlpha(PerfectGlow, 1.f - Wave));
	}

	// 3. The viewfinder corners snap in, lit up.
	const float Pad = 40.f * S * (1.f - Ease(Age / 0.25f, 3.f));
	DCUI::Corners(Canvas, L - Pad, T - Pad, R - L + 2.f * Pad, B - T + 2.f * Pad, 72.f * S, FMath::Max(6.f * S, 3.f), WithAlpha(PerfectGlow, Fade));

	// 4. Sparkles bursting out of the stamp: fast at first, slowing, drifting down. Seeded per burst so they hold their paths.
	const FVector2D Origin((L + R) * 0.5f, T + (B - T) * 0.3f);
	const FLinearColor Tints[3] = { DCUI::Gold, DCUI::Ivory, PerfectGlow };
	FRandomStream Rng(PerfectBurstCount * 7919);
	constexpr int32 Count = 28;
	for (int32 i = 0; i < Count; ++i)
	{
		const float Angle = 2.f * PI * (i + Rng.FRand() * 0.6f) / Count;
		const float Speed = Rng.FRandRange(260.f, 620.f) * S;
		const float Life = Rng.FRandRange(0.8f, PerfectBurstLength);
		const float Radius = Rng.FRandRange(8.f, 18.f) * S;
		const float K = Age / Life;
		if (K >= 1.f)
		{
			continue;
		}
		const float Dist = Speed * Life * 0.5f * (1.f - FMath::Square(1.f - K));
		const FVector2D P = Origin + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Dist + FVector2D(0.f, 120.f * S * Age * Age);
		const float Twinkle = 0.8f + 0.2f * FMath::Sin(Age * 30.f + i);
		DCUI::Sparkle(Canvas, P, Radius * (1.f - 0.6f * K) * Twinkle, WithAlpha(Tints[i % 3], 1.f - K * K));
	}

	// 5. The stamp: PERFECT! pops in big and settles.
	const float Size = Title * 1.2f * (1.f + 0.6f * (1.f - Ease(Age / 0.3f, 3.f)));
	Label(TEXT("PERFECT!"), Origin.X, Origin.Y - Px(Size) * 0.6f, WithAlpha(FLinearColor(0.78f, 0.93f, 1.f), Fade), Size, EAlign::Center);
}

void ADCZombieHUD::DrawPlanPreview(ADCZombieDirector* D)
{
	// Center the timeline beside the reference inset, aligned to the middle of that window.
	const float W = FMath::Min(820.f * S, Canvas->ClipX * 0.44f);
	const float X = (Canvas->ClipX - W) * 0.5f;
	const float Pad = 16.f * S;
	TArray<FString> Framed;
	if (D->IsViewingCamera())
	{
		D->GetPlanPreview(Framed);
	}
	const float KeySize = Px(Small) * 1.2f;
	const float RowGap = 8.f * S;
	const float H = Pad * 2.f + Px(Small) * 1.3f + RowGap + KeySize + (D->IsViewingCamera() ? Px(Body) * 1.4f : 0.f);
	const float InsetH = Canvas->ClipX * InsetShare / DCFrame::FilmAspect;
	const float CenterY = Canvas->ClipY - (InsetH + Px(Small) * 1.4f) * 0.5f;
	const float Y = FMath::Min(CenterY - H * 0.5f, Canvas->ClipY - H - 12.f * S);
	DCUI::Panel(Canvas, X, Y, W, H, S);

	const float Length = FMath::Max(D->TakeDuration, KINDA_SMALL_NUMBER);
	const float Time = D->GetSceneTime();
 // Center the transport icon and timecode as one group.
 const FString Timecode = DCUI::Timecode(Time);
 const FVector2D TimeSize = DCUI::Measure(Canvas, GetTextFont(), Timecode, Px(Small));
 const float IconH = Px(Small) * 0.85f;
 const float IconW = IconH * 0.8f;
 const float TransportGap = 12.f * S;
 const float IconX = X + (W - IconW - TransportGap - TimeSize.X) * 0.5f;
 const float TransportMidY = Y + Pad + Px(Small) * 1.3f * 0.5f;
 const float IconY = TransportMidY - IconH * 0.5f;
 if (D->IsRehearsalPlaying())
  DCUI::Triangle(Canvas, FVector2D(IconX, IconY), FVector2D(IconX, IconY + IconH),
   FVector2D(IconX + IconH * 0.8f, IconY + IconH * 0.5f), DCUI::Gold);
 else
 {
  DCUI::Rect(Canvas, IconX, IconY, IconH * 0.25f, IconH, DCUI::Gold);
  DCUI::Rect(Canvas, IconX + IconH * 0.55f, IconY, IconH * 0.25f, IconH, DCUI::Gold);
 }
	Label(Timecode, IconX + IconW + TransportGap, TransportMidY - TimeSize.Y * 0.5f, DCUI::Ivory, Small);
	const float KeyTop = Y + Pad + Px(Small) * 1.3f + RowGap;
	const float TY = KeyTop + KeySize * 0.5f - 3.f * S;
	const float KeySpace = KeySize + 12.f * S;
	const float TrackX = X + Pad + KeySpace;
	const float TrackW = W - 2.f * (Pad + KeySpace);
	auto TimelineKey = [&](const TCHAR* Key, float KeyX)
	{
		const float KeyY = TY + 3.f * S - KeySize * 0.5f;
		DCUI::Outline(Canvas, KeyX, KeyY, KeySize, KeySize, FMath::Max(1.5f * S, 1.f), DCUI::Gold);
		const FVector2D Size = DCUI::Measure(Canvas, GetTextFont(), Key, Px(Small) * 0.8f);
		Label(Key, KeyX + KeySize * 0.5f, KeyY + (KeySize - Size.Y) * 0.5f, DCUI::Gold, Small * 0.8f, EAlign::Center);
	};
	TimelineKey(TEXT("Q"), X + Pad);
	TimelineKey(TEXT("E"), X + W - Pad - KeySize);
	DCUI::Rect(Canvas, TrackX, TY, TrackW, 6.f * S, FLinearColor(1.f, 1.f, 1.f, 0.2f));
	DCUI::Rect(Canvas, TrackX, TY, TrackW * Time / Length, 6.f * S, DCUI::Gold);
	DCUI::Rect(Canvas, TrackX + TrackW * Time / Length - 1.5f, TY - 7.f * S, 3.f, 20.f * S, DCUI::Ivory);
	if (D->IsViewingCamera())
	{
		const FString Line = Framed.Num() > 0 ? TEXT("Frames: ") + FString::Join(Framed, TEXT(", ")) : TEXT("No mission framed right now");
		Label(Line, X + W * 0.5f, KeyTop + KeySize + RowGap, Framed.Num() > 0 ? DCUI::Good : DCUI::Muted, Body, EAlign::Center, true, W - 2.f * Pad);
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
	// One camera-shaped button per slot, with the placement slot pulsing in gold.
	const int32 PlaceSlot = D->GetPlacementSlot();
	const int32 Count = FMath::Max(D->IsUnlimited() ? D->GetSlotCount() : D->MaxCameras, PlaceSlot + 1);
	if (Count == 0)
	{
		return;
	}
	const float Gap = 12.f * S;
	const float BoxW = FMath::Min(132.f * S, (Canvas->ClipX * 0.44f - (Count - 1) * Gap) / Count);
	const float BoxH = 112.f * S;
	float X = Canvas->ClipX * 0.5f - (Count * BoxW + (Count - 1) * Gap) * 0.5f;
	const float Y = (18.f - 300.f * FocusEase) * S;
	const bool bOnAir = D->GetPhase() == EDCPhase::Filming || D->GetPhase() == EDCPhase::Countdown;
	if (D->GetPhase() == EDCPhase::Filming)
		Label(TEXT("Press number to switch cameras"), Canvas->ClipX * 0.5f, Y + BoxH + 12.f * S,
			DCUI::Ivory, Small, EAlign::Center);
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
		// Camera silhouette: twin film reels, body and projecting lens.
		const float BodyW = BoxW * 0.78f;
		const float Stroke = FMath::Max((bPlacing ? 2.5f : 1.5f) * S, 1.f);
		for (float ReelX : { 0.24f, 0.57f })
		{
			const float Radius = FMath::Min(16.f * S, BoxW * 0.15f);
			const FVector2D Center(X + BoxW * ReelX, Y + 32.f * S - Radius);
			DCUI::Disc(Canvas, Center, Radius, Fill);
			DCUI::Ring(Canvas, Center, Radius, Stroke, Edge);
		}
		const FVector2D A(X + BodyW, Y + 51.f * S), B(X + BoxW, Y + 38.f * S);
		const FVector2D C(X + BoxW, Y + 88.f * S), E(X + BodyW, Y + 75.f * S);
		DCUI::Triangle(Canvas, A, B, C, Fill);
		DCUI::Triangle(Canvas, A, C, E, Fill);
		DCUI::Line(Canvas, A, B, Stroke, Edge);
		DCUI::Line(Canvas, B, C, Stroke, Edge);
		DCUI::Line(Canvas, C, E, Stroke, Edge);
		DCUI::Rect(Canvas, X, Y + 32.f * S, BodyW, BoxH - 32.f * S, Fill);
		DCUI::Outline(Canvas, X, Y + 32.f * S, BodyW, BoxH - 32.f * S, Stroke, Edge);
		Label(FString::FromInt(i + 1), X + BodyW * 0.5f, Y + 44.f * S, Edge, Large * 0.8f, EAlign::Center, true, BodyW - 8.f * S);

		X += BoxW + Gap;
	}
}


namespace
{
	constexpr float PortraitShare = 0.36f;	// portrait height, as a share of the screen height
	constexpr float CoachTextSize = 50.f;	// the bubble's text: big, so it reads at a glance
	constexpr float TypewriterRate = 45.f;	// characters per second
}

bool ADCZombieHUD::ShowsCoach(ADCZombieDirector* D) const
{
	return D->bShowDirectorCoach && !D->GetCoachLine().IsEmpty()
		&& (D->GetPhase() == EDCPhase::Countdown || D->GetPhase() == EDCPhase::Filming);
}

float ADCZombieHUD::CoachRight() const
{
	const float H = Canvas->ClipY * PortraitShare;
	return H * (DirectorDefault && DirectorDefault->GetSurfaceHeight() > 0.f ? DirectorDefault->GetSurfaceWidth() / DirectorDefault->GetSurfaceHeight() : 1.2f);
}

float ADCZombieHUD::CoachTop() const
{
	const float BubbleH = Px(CoachTextSize) * 1.3f + Px(Small) * 1.2f + 2.f * 22.f * S;
	return Canvas->ClipY - BubbleH - 20.f * S - 12.f * S;
}

void ADCZombieHUD::DrawCoach(ADCZombieDirector* D)
{
	if (!ShowsCoach(D))
	{
		return;
	}
	const EDCDirectorMood Mood = D->GetCoachMood();
	UTexture2D* Portrait = Mood == EDCDirectorMood::Joy ? DirectorJoy : Mood == EDCDirectorMood::Disgust ? DirectorDisgust
		: Mood == EDCDirectorMood::Almost ? DirectorAlmost : DirectorDefault;
	if (!Portrait)
	{
		Portrait = DirectorDefault;
	}
	const FLinearColor Accent = Mood == EDCDirectorMood::Joy ? DCUI::Good : Mood == EDCDirectorMood::Disgust ? DCUI::Rec
		: Mood == EDCDirectorMood::Almost ? DCUI::Gold : DCUI::Ivory;
	DrawDirectorSpeech(D, Portrait, D->GetCoachLine(), Accent, GetWorld()->GetRealTimeSeconds() - D->GetCoachLineStart(), 0.f);
}

void ADCZombieHUD::DrawCheer(ADCZombieDirector* D)
{
	constexpr float RiseSeconds = 0.4f;
	constexpr float SinkSeconds = 0.35f;
	const float Now = GetWorld()->GetRealTimeSeconds();
	const float Up = Now - D->GetCheerUpSince();
	const float Left = D->GetCheerLineStart() + D->CheerSeconds - Now;
	if (D->GetPhase() != EDCPhase::Preparation || D->GetCheerLine().IsEmpty() || Up < 0.f || Left < -SinkSeconds)
	{
		return;
	}
	// Springs up past his spot and settles (back-out ease), then drops away once he's said his piece.
	const float T = FMath::Clamp(Up / RiseSeconds, 0.f, 1.f) - 1.f;
	constexpr float Overshoot = 1.70158f;
	const float Rise = 1.f + (Overshoot + 1.f) * T * T * T + Overshoot * T * T;
	const float Sink = Left < 0.f ? FMath::Square(-Left / SinkSeconds) : 0.f;
	const float Travel = Canvas->ClipY * PortraitShare * 1.1f;
	UTexture2D* Face = D->GetCheerCount() % 2 == 1 ? DirectorThatsIt : DirectorJoy;
	DrawDirectorSpeech(D, Face ? Face : DirectorJoy.Get(), D->GetCheerLine(), DCUI::Gold, Now - D->GetCheerLineStart(),
		(1.f - Rise + Sink) * Travel);
}

void ADCZombieHUD::DrawDirectorSpeech(ADCZombieDirector* D, UTexture2D* Portrait, const FString& Line, const FLinearColor& Accent, float Since, float Drop)
{
	// The bubble runs along the bottom, from the director to the reference inset (or the screen edge).
	const float PortraitW = CoachRight();
	const float Pad = 22.f * S;
	const float X = PortraitW * 0.8f;
	const float Right = D->GetReferenceInset() ? Canvas->ClipX * (1.f - InsetShare) - 16.f * S : Canvas->ClipX - 24.f * S;
	const float Lift = D->GetPhase() == EDCPhase::Preparation
		? Canvas->ClipX * InsetShare / DCFrame::FilmAspect + Px(Small) * 1.4f + 24.f * S : 0.f;
	const float Y = CoachTop() + 12.f * S + Drop - Lift;
	const float W = FMath::Max(Right - X, 200.f * S);
	const float H = Canvas->ClipY - 20.f * S - (CoachTop() + 12.f * S);
	const FLinearColor Paper(0.96f, 0.93f, 0.86f, 0.97f);
	const FLinearColor InkColor(0.08f, 0.07f, 0.07f);
	// Tail pointing back at the director's face.
	DCUI::Triangle(Canvas, FVector2D(X + 2.f, Y + H * 0.25f), FVector2D(X + 2.f, Y + H * 0.6f), FVector2D(PortraitW * 0.62f, Y - H * 0.35f + Lift), Paper);
	DCUI::Rect(Canvas, X, Y, W, H, Paper);
	DCUI::Rect(Canvas, X, Y, FMath::Max(6.f * S, 3.f), H, Accent);
	DCUI::Outline(Canvas, X, Y, W, H, FMath::Max(2.f * S, 1.f), FLinearColor(0.f, 0.f, 0.f, 0.35f));
	Label(TEXT("THE DIRECTOR"), X + Pad, Y + Pad * 0.6f, FLinearColor(0.55f, 0.08f, 0.06f), Small, EAlign::Left, false, W - 2.f * Pad);

	// The line types itself out.
	const int32 Shown = FMath::Clamp(FMath::FloorToInt(Since * TypewriterRate), 0, Line.Len());
	// Size the line for the whole sentence so the text doesn't shrink as it types.
	const FVector2D Full = DCUI::Measure(Canvas, GetTextFont(), Line, Px(CoachTextSize));
	const float TextPx = Full.X > W - 2.f * Pad ? Px(CoachTextSize) * (W - 2.f * Pad) / Full.X : Px(CoachTextSize);
	DCUI::Text(Canvas, GetTextFont(), Line.Left(Shown), X + Pad + 6.f * S, Y + Pad * 0.6f + Px(Small) * 1.2f, TextPx, InkColor, EAlign::Left, false);

	// The director himself, standing on the bottom edge; a little bounce when his mood changes.
	if (Portrait)
	{
		const float Pop = 1.f + 0.05f * FMath::Clamp(1.f - Since / 0.2f, 0.f, 1.f);
		const float PH = Canvas->ClipY * PortraitShare * Pop;
		const float PW = PortraitW * Pop;
		DrawTexture(Portrait, 0.f, Canvas->ClipY - PH + Drop, PW, PH, 0.f, 0.f, 1.f, 1.f);
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
	const float W = FMath::Lerp(Canvas->ClipX * InsetShare, FMath::Min(Canvas->ClipX * 0.78f, Canvas->ClipY * 0.60f * DCFrame::FilmAspect), FocusEase);
 const float H = W / DCFrame::FilmAspect;
 const float X = FMath::Lerp(Canvas->ClipX - W, (Canvas->ClipX - W) * 0.5f, FocusEase);
 const float Y = FMath::Lerp(Canvas->ClipY - H, (Canvas->ClipY - H) * 0.42f, FocusEase);
	const float LabelH = Px(Small) * 1.4f;
	DCUI::Rect(Canvas, X - 4.f * S, Y - LabelH, W + 4.f * S, H + LabelH, FLinearColor::Black);
	DCUI::Disc(Canvas, FVector2D(X + 12.f * S, Y - LabelH * 0.5f), 7.f * S, DCUI::Gold);
 const bool bShowTab = D->GetPhase() == EDCPhase::Preparation;
 const float TabPx = Px(Small) * 0.70f;
 const FVector2D TabSize = DCUI::Measure(Canvas, GetTextFont(), TEXT("Tab"), TabPx);
 const float TabW = TabSize.X + 24.f * S;
 const float TabH = LabelH - 8.f * S;
 const float TabX = X + W - TabW - 8.f * S;
 Label(TEXT("REFERENCE CUT"), X + 26.f * S, Y - LabelH + (LabelH - Px(Small) * 1.2f) * 0.5f, DCUI::Gold, Small, EAlign::Left, true,
  W - 30.f * S - (bShowTab ? TabW + 16.f * S : 0.f));
 if (bShowTab)
 {
  DCUI::Outline(Canvas, TabX, Y - LabelH + 4.f * S, TabW, TabH, FMath::Max(1.f, 1.5f * S), DCUI::Gold);
  DCUI::Text(Canvas, GetTextFont(), TEXT("Tab"), TabX + 12.f * S, Y - LabelH * 0.5f - TabSize.Y * 0.5f, TabPx, DCUI::Gold, EAlign::Left, false);
 }
	DrawTexture(Inset, X, Y, W, H, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Opaque);
	if (const ADCBeat* Beat = D->GetReferenceBeatAt(D->GetSceneTime()))
	{
		Label(Beat->DisplayName, X + W * 0.5f, Y + H - Px(Small) * 1.4f, DCUI::Ivory, Small, EAlign::Center, true, W - 16.f * S);
	}

 if (FocusProgress < 1.f)
 {
  if (UTextureRenderTarget2D* Preview = D->GetPlacementPreview())
  {
   const float PreviewW = Canvas->ClipX * InsetShare;
   const float PreviewH = PreviewW / DCFrame::FilmAspect;
   const float PreviewX = Canvas->ClipX - PreviewW + (PreviewW + 12.f * S) * FocusEase;
   const float PY = Canvas->ClipY - PreviewH * 2.f - LabelH;
   DCUI::Rect(Canvas, PreviewX, PY - LabelH, PreviewW, PreviewH + LabelH, FLinearColor::Black);
   DCUI::Disc(Canvas, FVector2D(PreviewX + 12.f * S, PY - LabelH * 0.5f), 7.f * S, DCUI::Gold);
   Label(TEXT("PLACEMENT PREVIEW"), PreviewX + 26.f * S, PY - LabelH, DCUI::Gold, Small, EAlign::Left, true, PreviewW - 30.f * S);
   DrawTexture(Preview, PreviewX, PY, PreviewW, PreviewH, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Opaque);
   if (!ScrollWheelIcon) ScrollWheelIcon = FImageUtils::ImportFileAsTexture2D(FPaths::ProjectContentDir() / TEXT("DirectorsCut/RawUI/mouse-scroll-wheel-icon.png"));
   FVector2D P;
   if (FocusProgress == 0.f && ScrollWheelIcon && ProjectVisible(D->GetPlacementHintLocation(), P))
   {
    const float IX = FMath::Clamp(float(P.X) + 70.f * S, 10.f * S, Canvas->ClipX - 110.f * S);
    const float IY = FMath::Clamp(float(P.Y), 100.f * S, Canvas->ClipY - 180.f * S);
    DrawTexture(ScrollWheelIcon, IX, IY, 76.f * S, 76.f * S, 0.f, 0.f, 1.f, 1.f);
   }
  }
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
	Label(FString::Printf(TEXT("Framing %.0f%%   |   Size %.0f%%   |   Viewpoint %.0f%%   |   %d cuts"),
		Match.Framing * 100.f, Match.Size * 100.f, Match.Viewpoint * 100.f, D->GetCutCount()), CX, Y, DCUI::Ivory, Body, EAlign::Center, true, W - 2.f * Pad);
	Y += Px(Body) * 1.3f;
	Label(FString::Printf(TEXT("MATCH %.0f%%   (best %.0f%%)"), Match.Overall * 100.f, D->GetBestMatch() * 100.f), CX, Y, bGood ? DCUI::Gold : DCUI::Rec, Large, EAlign::Center);
	Y += Px(Large) * 1.4f;
	const float Weakest = FMath::Min3(Match.Framing, Match.Size, Match.Viewpoint);
	Label(D->GetGradeRank() == 0 ? TEXT("Shot for shot, that's the reference cut.")
		: Weakest == Match.Framing ? TEXT("The actors sat in the wrong part of the frame - aim like the reference cut.")
		: Weakest == Match.Size ? TEXT("The actors were the wrong size - get closer or zoom to match.")
		: TEXT("You filmed them from the wrong side - place cameras where the reference looks from."), CX, Y, DCUI::Muted, Small, EAlign::Center, true, W - 2.f * Pad);
	Y += Px(Small) * 1.6f;

}

void ADCZombieHUD::DrawPremiere(ADCZombieDirector* D)
{
	// The finished film: letterboxed to the film's aspect (the bars match the engine's), no gameplay HUD.
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const float Bar = FMath::Max((H - W / DCFrame::FilmAspect) * 0.5f, Px(Small) * 2.f);
	const float Time = D->GetSceneTime();
	DCUI::Rect(Canvas, 0.f, 0.f, W, Bar, FLinearColor::Black);
	DCUI::Rect(Canvas, 0.f, H - Bar, W, Bar, FLinearColor::Black);

	if (Time < 3.5f)
	{
		Label(D->MovieTitle, W * 0.5f, H * 0.38f, DCUI::Ivory, Title, EAlign::Center);
		Label(TEXT("A FILM BY YOU"), W * 0.5f, H * 0.38f + Px(Title) * 1.3f, DCUI::Gold, Body, EAlign::Center);
	}

	Label(D->MovieTitle, W * 0.5f, (Bar - Px(Small) * 1.2f) * 0.5f, DCUI::Ivory, Small, EAlign::Center);
	if (D->IsPremierePaused() && !D->IsFilmFinished())
	{
		Label(TEXT("PAUSED"), W * 0.5f, H * 0.5f - Px(Large) * 0.6f, DCUI::Ivory, Large, EAlign::Center);
	}
}
