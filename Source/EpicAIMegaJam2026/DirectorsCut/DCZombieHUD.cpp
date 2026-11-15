// Director's Cut - canvas HUD, styled like a film set (see DCUI for the building blocks).

#include "DCZombieHUD.h"
#include "DCCrewMember.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "DCBeat.h"
#include "DCCinemaReplay.h"
#include "DCMultiCam.h"
#include "Camera/CameraComponent.h"
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
		if (D->IsCutsceneOverScene())
		{
			DrawPhase(D); // the filming tutorial plays over the live camera and its HUD
		}
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
	// back row) would show in the black bars around it. Paint everything outside the view black - far enough out to
	// cover bars of any size, so it never depends on measuring them; the window clips the rest.
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const float Far = 2.f * FMath::Max(W, H);
	const FLinearColor Black = FLinearColor::Black;
	DCUI::Rect(Canvas, -Far, -Far, W + 2.f * Far, Far, Black);
	DCUI::Rect(Canvas, -Far, H, W + 2.f * Far, Far, Black);
	DCUI::Rect(Canvas, -Far, 0.f, Far, H, Black);
	DCUI::Rect(Canvas, W, 0.f, Far, H, Black);
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

 if (D->IsSetOverview()) { DrawCountdown(D); return; }
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
	if (D->GetPhase() == EDCPhase::Countdown && !D->IsInCutscene())
	{
		DrawCountdown(D);
	}

	// Nothing pops up in the middle of the shot while the camera rolls.
	const FString Message = D->GetMessage();
	const bool bRolling = D->GetPhase() == EDCPhase::Countdown || D->GetPhase() == EDCPhase::Filming;
	if (!Message.IsEmpty() && !bRolling)
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

	DrawFilmingGuide(D);
	DrawReferenceInset(D);
	DrawCoach(D);
	DrawSwitcher(D);
	DrawCheer(D);
	if (D->GetPhase() == EDCPhase::Preparation && !D->IsViewingCamera() && PlayerOwner && PlayerOwner->GetPawn())
	{
		ADCCrewMember* Nearest = nullptr;
		float BestDistance = FMath::Square(360.f);
		for (TActorIterator<ADCCrewMember> It(GetWorld()); It; ++It)
		{
			const float Distance = FVector::DistSquared(PlayerOwner->GetPawn()->GetActorLocation(), It->GetActorLocation());
			if (!It->IsHidden() && Distance < BestDistance && PlayerOwner->LineOfSightTo(*It))
			{ Nearest = *It; BestDistance = Distance; }
		}
		FVector2D Point;
		if (Nearest && PlayerOwner->ProjectWorldLocationToScreen(Nearest->GetActorLocation() + FVector(0, 0, Nearest->HeadHeight), Point))
		{
			const float W = FMath::Min(420.f * S, Canvas->ClipX - 32.f), FontPx = 21.f * S;
			TArray<FString> Lines;
			DCUI::Wrap(Canvas, GetTextFont(), Nearest->GetDialogue(), FontPx, W - 28.f * S, Lines);
			const float H = (40.f + 27.f * Lines.Num()) * S;
			const float X = FMath::Clamp(float(Point.X) - W / 2.f, 16.f, Canvas->ClipX - W - 16.f);
			const float Y = FMath::Clamp(float(Point.Y) - H - 14.f * S, 80.f * S, Canvas->ClipY - H - 16.f);
			DCUI::Panel(Canvas, X, Y, W, H, S, FLinearColor(.015f,.02f,.025f,.94f));
			DCUI::Text(Canvas, GetTextFont(), Nearest->CrewRole, X + 14.f*S, Y + 9.f*S, FontPx, DCUI::Gold);
			for (int32 i = 0; i < Lines.Num(); ++i)
				DCUI::Text(Canvas, GetTextFont(), Lines[i], X + 14.f*S, Y + (37.f + i*27.f)*S, FontPx, DCUI::Ivory);
		}
	}
	if (D->GetPhase() == EDCPhase::Preparation)
	{
		DrawPlanPreview(D); // Keep timeline controls readable throughout the cheer animation.
		FVector2D Joint; // where a nearby camera's stand meets its legs, on screen (for the F prompt)
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
		else if (const ADCMultiCam* Near = D->GetCameraSlot(D->GetNearbyCameraSlot()); Near && ProjectVisible(Near->GetStandJoint(), Joint))
		{
			// The F key floats beside the camera it opens, where its pole meets the tripod legs (so it stays low however
			// tall the stand is), on the side facing the middle of the screen. Only on-screen cameras are interactable.
			const float KeyPx = Px(Small) * 0.82f * 2.f;
			const FVector2D KeySize = DCUI::Measure(Canvas, GetTextFont(), TEXT("F"), KeyPx);
			const float W = KeySize.X + 32.f * S;
			const float H = Px(Small) * 2.4f;
			const float Side = Joint.X > CX ? -1.f : 1.f;
			const float KX = FMath::Clamp(float(Joint.X) + Side * 46.f * S - (Side < 0.f ? W : 0.f), 10.f * S, Canvas->ClipX - W - 10.f * S);
			const float KY = FMath::Clamp(float(Joint.Y) - H * 0.5f, 10.f * S, Canvas->ClipY - H - 10.f * S);
			const float Bob = FMath::Sin(GetWorld()->GetRealTimeSeconds() * 4.f) * 3.f * S;
			DCUI::Rect(Canvas, KX, KY + Bob, W, H, FLinearColor(0.f, 0.f, 0.f, 0.72f));
			DCUI::Outline(Canvas, KX, KY + Bob, W, H, FMath::Max(1.f, 2.f * S), DCUI::Gold);
			DCUI::Text(Canvas, GetTextFont(), TEXT("F"), KX + W * 0.5f, KY + Bob + (H - KeySize.Y) * 0.5f,
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

void ADCZombieHUD::DrawMainMenu(ADCZombieDirector* D)
{
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const float Now = GetWorld()->GetRealTimeSeconds();
	DCUI::Rect(Canvas, 0.f, 0.f, W, H, FLinearColor::Black);

	// The background covers the screen (cropped, never stretched), breathing in and out very slowly. Only the part
	// inside the view is drawn, so none of it shows in the black bars of wider or taller windows.
	if (MainMenuImage && MainMenuImage->GetSurfaceWidth() > 0.f && MainMenuImage->GetSurfaceHeight() > 0.f)
	{
		const float IW = MainMenuImage->GetSurfaceWidth();
		const float IH = MainMenuImage->GetSurfaceHeight();
		const float Cover = FMath::Max(W / IW, H / IH) * (1.04f + 0.02f * FMath::Sin(Now * 0.15f));
		const float UWidth = W / (IW * Cover), VHeight = H / (IH * Cover);
		DrawTexture(MainMenuImage, 0.f, 0.f, W, H, (1.f - UWidth) * 0.5f, (1.f - VHeight) * 0.5f, UWidth, VHeight);
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

	// A dark stage with a warm spotlight where he stands - or, over the live scene, just enough dimming to read him.
	const bool bOverScene = D->IsCutsceneOverScene();
	DCUI::Rect(Canvas, 0.f, 0.f, W, H, FLinearColor(0.035f, 0.03f, 0.04f, bOverScene ? 0.55f : 0.93f));
	for (int32 i = 0; i < 6; ++i)
	{
		DCUI::Disc(Canvas, Spot, H * (0.62f - i * 0.08f), FLinearColor(1.f, 0.78f, 0.45f, 0.035f), 64);
	}

	// Cinema framing: a soft dark edge and slim letterbox bars (not over the scene: they'd hide its HUD).
	DCUI::Vignette(Canvas, 0.f, 0.f, W, H, H * 0.3f, FLinearColor(0.f, 0.f, 0.f, 0.6f));
	const float Bar = bOverScene ? 0.f : H * 0.06f;
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
	// [Bracketed] words are highlighted: wrap the text without the brackets, and remember which characters they marked.
	FString Plain;
	TArray<bool> Highlighted;
	bool bInHighlight = false;
	for (const TCHAR Char : Line->Line)
	{
		if (Char == TEXT('[') || Char == TEXT(']'))
		{
			bInHighlight = Char == TEXT('[');
			continue;
		}
		Plain.AppendChar(Char);
		Highlighted.Add(bInHighlight);
	}
	TArray<FString> Lines;
	DCUI::Wrap(Canvas, GetTextFont(), Plain, TextPx, BW - 2.f * Pad, Lines);
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

	// The line types itself out across its wrapped lines, highlighted words in blue.
	const FLinearColor Highlight(0.05f, 0.33f, 0.92f);
	int32 Remaining = FMath::FloorToInt(Since * ADCMultiCamDirector::CutsceneTypeRate);
	float Y = BY + Pad * 0.7f + NameH;
	int32 Offset = 0;
	for (const FString& Text : Lines)
	{
		const int32 Start = FMath::Max(Plain.Find(Text, ESearchCase::CaseSensitive, ESearchDir::FromStart, Offset), 0);
		Offset = Start + Text.Len();
		const int32 Shown = FMath::Clamp(Remaining, 0, Text.Len());
		float X = BX + Pad + 6.f * S;
		for (int32 Run = 0; Run < Shown;)
		{
			const bool bHighlight = Highlighted.IsValidIndex(Start + Run) && Highlighted[Start + Run];
			int32 RunEnd = Run + 1;
			while (RunEnd < Shown && Highlighted.IsValidIndex(Start + RunEnd) && Highlighted[Start + RunEnd] == bHighlight)
			{
				++RunEnd;
			}
			const FString Piece = Text.Mid(Run, RunEnd - Run);
			DCUI::Text(Canvas, GetTextFont(), Piece, X, Y, TextPx, bHighlight ? Highlight : Ink, EAlign::Left, false);
			X += DCUI::Measure(Canvas, GetTextFont(), Piece, TextPx).X;
			Run = RunEnd;
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

	if (Line->bHighlightShootingUI)
	{
		DrawShootingUIHighlight(D);
	}
	if (Line->bShowCameraControls)
	{
		DrawCameraControls(BX + BW * 0.5f, BY + BH + 28.f * S, Since);
	}
}

void ADCZombieHUD::DrawCameraControls(float CX, float Top, float Since)
{
	const float Now = GetWorld()->GetRealTimeSeconds();
	const float Appear = FMath::SmoothStep(0.15f, 0.55f, Since);
	if (Appear <= 0.f)
	{
		return;
	}
	const FLinearColor Card(0.04f, 0.04f, 0.06f, 0.88f * Appear);
	const FLinearColor Blue(0.35f, 0.66f, 1.f, Appear);
	const FLinearColor MouseBody(0.93f, 0.9f, 0.84f, Appear);
	const FLinearColor Dark(0.09f, 0.08f, 0.09f, Appear);
	const float CardW = 300.f * S, CardH = 250.f * S, Gap = 32.f * S, Radius = 20.f * S;
	const float MouseW = 64.f * S, MouseH = 98.f * S;

	// A mouse: rounded body, the two buttons' seam, and its wheel (WheelColor) with Ticks scrolling over it.
	auto Mouse = [&](float MX, float MY, const FLinearColor& WheelColor, float Ticks)
	{
		DCUI::RoundedRect(Canvas, MX - MouseW * 0.5f, MY, MouseW, MouseH, MouseW * 0.48f, MouseBody);
		DCUI::Line(Canvas, FVector2D(MX, MY + 4.f * S), FVector2D(MX, MY + MouseH * 0.4f), FMath::Max(2.f * S, 1.f), Dark);
		DCUI::Line(Canvas, FVector2D(MX - MouseW * 0.5f + 6.f * S, MY + MouseH * 0.4f), FVector2D(MX + MouseW * 0.5f - 6.f * S, MY + MouseH * 0.4f),
			FMath::Max(2.f * S, 1.f), Dark);
		const float WheelW = 12.f * S, WheelH = 26.f * S, WheelY = MY + 12.f * S;
		DCUI::RoundedRect(Canvas, MX - WheelW * 0.5f, WheelY, WheelW, WheelH, WheelW * 0.5f, WheelColor);
		if (Ticks >= 0.f)
		{
			for (int32 i = 0; i < 3; ++i)
			{
				const float TY = WheelY + 3.f * S + FMath::Fmod(Ticks + i * WheelH / 3.f, WheelH - 6.f * S);
				DCUI::Rect(Canvas, MX - WheelW * 0.3f, TY, WheelW * 0.6f, FMath::Max(2.f * S, 1.f), Dark);
			}
		}
	};
	auto Arrow = [&](FVector2D Tip, FVector2D Dir, float Size, const FLinearColor& Color)
	{
		const FVector2D Side(-Dir.Y, Dir.X);
		DCUI::Triangle(Canvas, Tip, Tip - Dir * Size + Side * Size * 0.7f, Tip - Dir * Size - Side * Size * 0.7f, Color);
	};
	auto CardFrame = [&](float X, const TCHAR* Action, const TCHAR* Result)
	{
		DCUI::RoundedRect(Canvas, X - 2.f * S, Top - 2.f * S, CardW + 4.f * S, CardH + 4.f * S, Radius + 2.f * S, FLinearColor(Blue.R, Blue.G, Blue.B, 0.6f * Appear));
		DCUI::RoundedRect(Canvas, X, Top, CardW, CardH, Radius, Card);
		Label(Action, X + CardW * 0.5f, Top + CardH - Px(Body) * 2.35f, Blue, Body, EAlign::Center, false, CardW - 20.f * S);
		Label(Result, X + CardW * 0.5f, Top + CardH - Px(Small) * 1.45f, FLinearColor(0.85f, 0.82f, 0.76f, Appear), Small, EAlign::Center, false, CardW - 20.f * S);
	};
	const float LeftX = CX - CardW - Gap * 0.5f, RightX = CX + Gap * 0.5f;
	const float MouseTop = Top + 26.f * S;

	// Move the mouse: it glides side to side, with arrows lighting up the way it's going.
	CardFrame(LeftX, TEXT("MOVE THE MOUSE"), TEXT("to aim the camera"));
	const float Sway = FMath::Sin(Now * 2.4f);
	const float MoveX = LeftX + CardW * 0.5f + Sway * 46.f * S;
	const float ArrowY = MouseTop + MouseH * 0.5f;
	Arrow(FVector2D(LeftX + 30.f * S, ArrowY), FVector2D(-1.f, 0.f), 16.f * S, FLinearColor(Blue.R, Blue.G, Blue.B, Appear * (0.35f + 0.65f * FMath::Max(-Sway, 0.f))));
	Arrow(FVector2D(LeftX + CardW - 30.f * S, ArrowY), FVector2D(1.f, 0.f), 16.f * S, FLinearColor(Blue.R, Blue.G, Blue.B, Appear * (0.35f + 0.65f * FMath::Max(Sway, 0.f))));
	Mouse(MoveX, MouseTop, Dark, -1.f);

	// Scroll: the wheel glows and rolls, with arrows above and below pulsing in turn (in, then out).
	CardFrame(RightX, TEXT("SCROLL THE WHEEL"), TEXT("to zoom in & out"));
	const float WheelX = RightX + CardW * 0.5f;
	const float Phase = FMath::Fmod(Now * 0.6f, 1.f);
	const bool bScrollUp = Phase < 0.5f;
	const float Roll = (bScrollUp ? -1.f : 1.f) * FMath::Fmod(Now * 40.f, 26.f) * S;
	Mouse(WheelX, MouseTop, Blue, FMath::Fmod(Roll + 26.f * S, 26.f * S));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Now * 9.f);
	const float WheelMid = MouseTop + 25.f * S;
	Arrow(FVector2D(WheelX + MouseW * 0.5f + 26.f * S, WheelMid - 22.f * S), FVector2D(0.f, -1.f), 14.f * S,
		FLinearColor(Blue.R, Blue.G, Blue.B, Appear * (bScrollUp ? 0.55f + 0.45f * Pulse : 0.25f)));
	Arrow(FVector2D(WheelX + MouseW * 0.5f + 26.f * S, WheelMid + 22.f * S), FVector2D(0.f, 1.f), 14.f * S,
		FLinearColor(Blue.R, Blue.G, Blue.B, Appear * (bScrollUp ? 0.25f : 0.55f + 0.45f * Pulse)));
	Label(bScrollUp ? TEXT("+") : TEXT("-"), WheelX - MouseW * 0.5f - 30.f * S, WheelMid - Px(Large) * 0.6f, Blue, Large, EAlign::Center, false);
}

void ADCZombieHUD::DrawShootingUIHighlight(ADCZombieDirector* D)
{
	// Bring the two panels back up over the dimming, then frame each in pulsing gold with a tag saying what it's for.
	DrawReferenceInset(D);
	DrawSwitcher(D);
	const float Now = GetWorld()->GetRealTimeSeconds();
	const float Pulse = 0.55f + 0.45f * FMath::Sin(Now * 5.f);
	const FLinearColor Ink(0.03f, 0.03f, 0.04f);
	auto Spotlight = [&](FBox2D Box, const FString& Tag, bool bTagBelow)
	{
		const float Grow = (10.f + 4.f * Pulse) * S;
		Box = Box.ExpandBy(FVector2D(Grow, Grow));
		const FVector2D Size = Box.GetSize();
		DCUI::Outline(Canvas, Box.Min.X, Box.Min.Y, Size.X, Size.Y, FMath::Max(4.f * S, 2.f), WithAlpha(DCUI::Gold, 0.4f + 0.6f * Pulse));
		DCUI::Corners(Canvas, Box.Min.X - 6.f * S, Box.Min.Y - 6.f * S, Size.X + 12.f * S, Size.Y + 12.f * S, 26.f * S,
			FMath::Max(5.f * S, 2.f), DCUI::Gold);
		// The tag: a gold label on the frame's edge, kept on screen.
		const FVector2D TagSize = DCUI::Measure(Canvas, GetTextFont(), Tag, Px(Small));
		const float TagW = TagSize.X + 28.f * S;
		const float TagH = TagSize.Y + 12.f * S;
		const float TagX = FMath::Clamp(Box.GetCenter().X - TagW * 0.5f, 8.f * S, Canvas->ClipX - TagW - 8.f * S);
		const float TagY = bTagBelow ? Box.Max.Y + 10.f * S : Box.Min.Y - TagH - 10.f * S;
		DCUI::Rect(Canvas, TagX, TagY, TagW, TagH, DCUI::Gold);
		Label(Tag, TagX + TagW * 0.5f, TagY + 6.f * S, Ink, Small, EAlign::Center, false);
	};
	if (D->GetReferenceInset())
	{
		Spotlight(GetInsetBounds(), TEXT("MATCH THIS REFERENCE CUT"), false);
	}
	int32 Count = 0;
	float BoxW = 0.f;
	FBox2D Switcher;
	if (GetSwitcherLayout(D, Count, BoxW, Switcher))
	{
		Spotlight(Switcher, Count > 1 ? FString::Printf(TEXT("PRESS 1-%d TO SWITCH CAMERAS"), Count) : FString(TEXT("PRESS 1 FOR YOUR CAMERA")), true);
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
	DCUI::Rect(Canvas, X0, TY, TW * Time / Length, 6.f * S, Rose);
 for (float Cut : D->GetReferenceCameraCutTimes(false))
  DCUI::Rect(Canvas, X0 + TW * Cut / Length - 1.5f * S, TY - 8.f * S, 3.f * S, 22.f * S, DCUI::Gold);
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
		// Ticked off (struck through, like a to-do list) once an installed camera matches the scene's reference picture.
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
		// 0 = far off, 1 = there. Red fades out as the shot improves, nothing at all halfway, then blue fades in.
		const float Close = FMath::SmoothStep(0.3f, Perfect, ShownLikeness);
		const float Lean = Close * 2.f - 1.f; // -1 red ... 0 clear ... +1 blue
		const FLinearColor Color = Lean < 0.f ? FarColor : CloseColor;
		// Strong and throbbing while it's wrong, a slow breath once perfect.
		const float Throb = 1.f + 0.18f * FMath::Max(-Lean, 0.f) * FMath::Sin(Now * 7.f);
		const float Breath = bPerfectShot ? 0.85f + 0.15f * FMath::Sin(Now * 3.f) : 1.f;
		const float Alpha = VignetteStrength * FMath::Lerp(1.f, 0.6f, Close) * FMath::Abs(Lean) * Throb * Breath * VignetteFade;
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
 for (float Cut : D->GetReferenceCameraCutTimes(true))
  DCUI::Rect(Canvas, TrackX + TrackW * Cut / Length - 1.5f * S, TY - 7.f * S, 3.f * S, 20.f * S, Rose);
	DCUI::Rect(Canvas, TrackX + TrackW * Time / Length - 1.5f, TY - 7.f * S, 3.f, 20.f * S, DCUI::Ivory);
	if (D->IsViewingCamera())
	{
		const FString Line = Framed.Num() > 0 ? TEXT("Frames: ") + FString::Join(Framed, TEXT(", ")) : TEXT("No mission framed right now");
		Label(Line, X + W * 0.5f, KeyTop + KeySize + RowGap, Framed.Num() > 0 ? DCUI::Good : DCUI::Muted, Body, EAlign::Center, true, W - 2.f * Pad);
	}
}

void ADCZombieHUD::DrawCountdown(ADCZombieDirector* D)
{
 // Fit the open arm and the board together, including compact PIE viewports.
 TGuardValue<float> SlateScale(S, FMath::Min(S, Canvas->ClipY / 650.f));
 const bool bPreparation = D->IsSetOverview();
 const float Duration = bPreparation ? 2.4f : FMath::Max(D->CountdownDuration, .01f);
 const float Remaining = bPreparation ? D->GetPreparationSlateRemaining() : D->GetCountdownRemaining();
 const float T = FMath::Clamp(1.f - Remaining / Duration, 0.f, 1.f);
 const float Enter = FMath::SmoothStep(0.f, .20f, T);
 const float Exit = FMath::SmoothStep(.78f, 1.f, T);
 const float W = 480.f * S, H = 270.f * S, Stick = 38.f * S;
 const float X = (Canvas->ClipX - W) * .5f;
 const float Y = (Canvas->ClipY - H + W * .5f + Stick) * .5f + (1.f - Enter + Exit) * Canvas->ClipY;
 DCUI::Rect(Canvas, 0, 0, Canvas->ClipX, Canvas->ClipY, FLinearColor(0,0,0,.55f * Enter * (1.f - Exit)));
 DCUI::Rect(Canvas, X, Y, W, H, FLinearColor(.025f,.028f,.032f,1));
 DCUI::Outline(Canvas, X, Y, W, H, 2.f*S, DCUI::Muted);
 DCUI::ClapperStripes(Canvas, X, Y, W, Stick);
 const float Angle = -.52f * (1.f - FMath::SmoothStep(.42f, .51f, T));
 const FVector2D Pivot(X, Y - 4.f*S);
 auto Point = [&](float U, float V) { return Pivot + FVector2D(U*FMath::Cos(Angle)-V*FMath::Sin(Angle), U*FMath::Sin(Angle)+V*FMath::Cos(Angle)); };
 for (int32 I=0; I<12; ++I)
 {
  const float L = W*I/12.f, R = W*(I+1)/12.f;
  const FLinearColor Color = I%2 ? DCUI::Ivory : FLinearColor(.03f,.03f,.03f,1);
  DCUI::Triangle(Canvas, Point(L,-Stick), Point(R,-Stick), Point(R,0), Color);
  DCUI::Triangle(Canvas, Point(L,-Stick), Point(R,0), Point(L,0), Color);
 }
 Label(TEXT("DIRECTOR'S CUT"), X+W*.5f, Y+62.f*S, DCUI::Ivory, Large, EAlign::Center);
 DCUI::Line(Canvas,FVector2D(X+24.f*S,Y+118.f*S),FVector2D(X+W-24.f*S,Y+118.f*S),2.f*S,DCUI::Muted);
 Label(bPreparation ? TEXT("CAMERA PREPARATION") : TEXT("ROLL CAMERA"), X+W*.5f,Y+140.f*S,DCUI::Gold,Body,EAlign::Center);
 Label(T < .51f ? TEXT("Quiet on set") : bPreparation ? TEXT("Find your frame") : TEXT("Action!"),X+W*.5f,Y+205.f*S,DCUI::Ivory,Body,EAlign::Center);
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

bool ADCZombieHUD::GetSwitcherLayout(ADCZombieDirector* D, int32& OutCount, float& OutBoxW, FBox2D& OutBounds) const
{
	OutCount = FMath::Max(D->IsUnlimited() ? D->GetSlotCount() : D->MaxCameras, D->GetPlacementSlot() + 1);
	if (OutCount == 0)
	{
		return false;
	}
	const float Gap = 12.f * S;
	OutBoxW = FMath::Min(132.f * S, (Canvas->ClipX * 0.44f - (OutCount - 1) * Gap) / OutCount);
	const float TotalW = OutCount * OutBoxW + (OutCount - 1) * Gap;
	// Slides up off the screen while the reference cut is in focus.
	const FVector2D Min(Canvas->ClipX * 0.5f - TotalW * 0.5f, (18.f - 300.f * FocusEase) * S);
	OutBounds = FBox2D(Min, Min + FVector2D(TotalW, 112.f * S));
	return true;
}

FBox2D ADCZombieHUD::GetInsetBounds() const
{
	// As DrawReferenceInset places it: bottom-right, growing toward the middle while in focus.
	const float W = FMath::Lerp(Canvas->ClipX * InsetShare, FMath::Min(Canvas->ClipX * 0.78f, Canvas->ClipY * 0.60f * DCFrame::FilmAspect), FocusEase);
	const float H = W / DCFrame::FilmAspect;
	const float X = FMath::Lerp(Canvas->ClipX - W, (Canvas->ClipX - W) * 0.5f, FocusEase);
	const float Y = FMath::Lerp(Canvas->ClipY - H, (Canvas->ClipY - H) * 0.42f, FocusEase);
	const float LabelH = Px(Small) * 1.4f;
	return FBox2D(FVector2D(X - 4.f * S, Y - LabelH), FVector2D(X + W, Y + H));
}

void ADCZombieHUD::DrawSwitcher(ADCZombieDirector* D)
{
	// One camera-shaped button per slot, with the placement slot pulsing in gold.
	const int32 PlaceSlot = D->GetPlacementSlot();
	int32 Count = 0;
	float BoxW = 0.f;
	FBox2D Bounds;
	if (!GetSwitcherLayout(D, Count, BoxW, Bounds))
	{
		return;
	}
	const float Gap = 12.f * S;
	const float BoxH = Bounds.GetSize().Y;
	float X = Bounds.Min.X;
	const float Y = Bounds.Min.Y;
	const bool bOnAir = D->GetPhase() == EDCPhase::Filming || D->GetPhase() == EDCPhase::Countdown;
	DrawCutCue(D);
	const float Pulse = 0.65f + 0.35f * FMath::Sin(GetWorld()->GetRealTimeSeconds() * 5.f);
	float CutSeconds = 0.f;
	int32 CutCamera = 0;
	const bool bCue = D->GetCutCue(CutSeconds, CutCamera);

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
		if (bCue && i + 1 == CutCamera)
		{
			Edge = WithAlpha(DCUI::Gold, Pulse);
			Fill = WithAlpha(DCUI::Gold, CutSeconds <= 0.f ? 0.45f * Pulse : 0.15f);
			const float Progress = 1.f - FMath::Clamp(CutSeconds / FMath::Max(D->CutCueLead, 0.01f), 0.f, 1.f);
			DCUI::Rect(Canvas, X, Y + BoxH + 4.f * S, BoxW * Progress, 3.f * S, Edge);
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
	// While filming, the action comes first: a smaller bubble that keeps out of the way of the shot.
	constexpr float FilmingCoachTextSize = 32.f;
	constexpr float FilmingCoachNameSize = 22.f;
	constexpr float FilmingCoachMaxWidth = 760.f;	// layout units
	constexpr float TypewriterRate = 45.f;	// characters per second
}

bool ADCZombieHUD::ShowsCoach(ADCZombieDirector* D) const
{
	return D->bShowDirectorCoach && !D->GetCoachLine().IsEmpty() && !D->IsInCutscene()
		&& (D->GetPhase() == EDCPhase::Countdown || D->GetPhase() == EDCPhase::Filming);
}

float ADCZombieHUD::CoachRight() const
{
	const float H = Canvas->ClipY * PortraitShare;
	return H * (DirectorDefault && DirectorDefault->GetSurfaceHeight() > 0.f ? DirectorDefault->GetSurfaceWidth() / DirectorDefault->GetSurfaceHeight() : 1.2f);
}

void ADCZombieHUD::DrawCoach(ADCZombieDirector* D)
{
	if (!ShowsCoach(D))
	{
		CoachShownSince = -1.f;
		return;
	}
	// He pops up from below when he (re)appears - after the filming tutorial, or as the countdown starts.
	const float Now = GetWorld()->GetRealTimeSeconds();
	if (CoachShownSince < 0.f)
	{
		CoachShownSince = Now;
	}
	const float T = FMath::Clamp((Now - CoachShownSince) / 0.45f, 0.f, 1.f) - 1.f;
	const float Rise = 1.f + 2.70158f * T * T * T + 1.70158f * T * T; // back-out ease
	const float Drop = (1.f - Rise) * Canvas->ClipY * PortraitShare * 1.1f;
	const EDCDirectorMood Mood = D->GetCoachMood();
	UTexture2D* Portrait = Mood == EDCDirectorMood::Joy ? DirectorJoy : Mood == EDCDirectorMood::Disgust ? DirectorDisgust
		: Mood == EDCDirectorMood::Almost ? DirectorAlmost : DirectorDefault;
	if (UTexture2D* Chosen = D->GetCoachPortrait())
	{
		Portrait = Chosen; // a face picked for this line (the wrap-up)
	}
	if (!Portrait)
	{
		Portrait = DirectorDefault;
	}
	const FLinearColor Accent = Mood == EDCDirectorMood::Joy ? DCUI::Good : Mood == EDCDirectorMood::Disgust ? DCUI::Rec
		: Mood == EDCDirectorMood::Almost ? DCUI::Gold : DCUI::Ivory;
	DrawDirectorSpeech(D, Portrait, D->GetCoachLine(), Accent, Now - D->GetCoachLineStart(), Drop);
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
	const bool bFilming = D->GetPhase() == EDCPhase::Countdown || D->GetPhase() == EDCPhase::Filming;
	const float TextSize = bFilming ? FilmingCoachTextSize : CoachTextSize;
	const float NameSize = bFilming ? FilmingCoachNameSize : Small;
	// Each face is drawn at its own proportions (the faces aren't all the same shape); the bubble keeps clear of the
	// widest of this face and the default one, so it doesn't jump left for a narrower face.
	const float FaceH = Canvas->ClipY * PortraitShare;
	const float FaceAspect = Portrait && Portrait->GetSurfaceHeight() > 0.f ? Portrait->GetSurfaceWidth() / Portrait->GetSurfaceHeight() : 0.f;
	const float FaceW = FaceAspect > 0.f ? FaceH * FaceAspect : CoachRight();
	const float PortraitW = FMath::Max(CoachRight(), FaceW);
	const float Pad = (bFilming ? 14.f : 22.f) * S;
	// Clear of the director's sprite, so it never covers the bubble's start.
	const float X = PortraitW + 18.f * S;
	const float Right = D->GetReferenceInset() ? Canvas->ClipX * (1.f - InsetShare) - 16.f * S : Canvas->ClipX - 24.f * S;
	const float Lift = D->GetPhase() == EDCPhase::Preparation
		? Canvas->ClipX * InsetShare / DCFrame::FilmAspect + Px(Small) * 1.4f + 24.f * S : 0.f;
	const float H = Px(TextSize) * 1.3f + Px(NameSize) * 1.2f + 2.f * Pad;
	const float Y = Canvas->ClipY - 20.f * S - H + Drop - Lift;
	const float W = FMath::Max(bFilming ? FMath::Min(Right - X, FilmingCoachMaxWidth * S) : Right - X, 200.f * S);
	const FLinearColor Paper(0.96f, 0.93f, 0.86f);
	const FLinearColor InkColor(0.08f, 0.07f, 0.07f);
	// A rounded speech bubble: a soft dark edge, the mood colour showing as a sliver down its left side, cream paper on
	// top, and a tail pointing back at the director's face.
	const float Radius = FMath::Min(18.f * S, H * 0.45f);
	const float Edge = FMath::Max(2.f * S, 1.f);
	const float AccentW = FMath::Max(6.f * S, 3.f);
	DCUI::RoundedRect(Canvas, X - Edge, Y - Edge, W + 2.f * Edge, H + 2.f * Edge, Radius + Edge, FLinearColor(0.f, 0.f, 0.f, 0.35f));
	DCUI::Triangle(Canvas, FVector2D(X + Radius, Y + H * 0.3f), FVector2D(X + Radius, Y + H * 0.65f), FVector2D(PortraitW * 0.9f, Y - H * 0.35f + Lift), Paper);
	DCUI::RoundedRect(Canvas, X, Y, W, H, Radius, Accent);
	DCUI::RoundedRect(Canvas, X + AccentW, Y, W - AccentW, H, Radius, Paper);
	Label(TEXT("THE DIRECTOR"), X + Pad, Y + Pad * 0.6f, FLinearColor(0.55f, 0.08f, 0.06f), NameSize, EAlign::Left, false, W - 2.f * Pad);

	// The line types itself out.
	const int32 Shown = FMath::Clamp(FMath::FloorToInt(Since * TypewriterRate), 0, Line.Len());
	// Size the line for the whole sentence so the text doesn't shrink as it types.
	const FVector2D Full = DCUI::Measure(Canvas, GetTextFont(), Line, Px(TextSize));
	const float TextPx = Full.X > W - 2.f * Pad ? Px(TextSize) * (W - 2.f * Pad) / Full.X : Px(TextSize);
	DCUI::Text(Canvas, GetTextFont(), Line.Left(Shown), X + Pad + 6.f * S, Y + Pad * 0.6f + Px(NameSize) * 1.2f, TextPx, InkColor, EAlign::Left, false);

	// The director himself, standing on the bottom edge; a little bounce when his mood changes.
	if (Portrait)
	{
		const float Pop = 1.f + 0.05f * FMath::Clamp(1.f - Since / 0.2f, 0.f, 1.f);
		const float PH = FaceH * Pop;
		const float PW = FaceW * Pop;
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
   // Where the stand's pole meets its tripod legs - a little above the ground, the same at any height - beside it on
   // the side facing the middle of the screen.
   if (FocusProgress == 0.f && ScrollWheelIcon && ProjectVisible(D->GetPlacementJointLocation(), P))
   {
    const float IconSize = 76.f * S;
    const float Side = P.X > Canvas->ClipX * 0.5f ? -1.f : 1.f;
    const float IX = FMath::Clamp(float(P.X) + Side * 40.f * S - (Side < 0.f ? IconSize : 0.f), 10.f * S, Canvas->ClipX - IconSize - 10.f * S);
    const float IY = FMath::Clamp(float(P.Y) - IconSize * 0.5f, 100.f * S, Canvas->ClipY - 180.f * S);
    DrawTexture(ScrollWheelIcon, IX, IY, IconSize, IconSize, 0.f, 0.f, 1.f, 1.f);
   }
  }
 }

}

void ADCZombieHUD::DrawFilmingGuide(ADCZombieDirector* D)
{
	FVector2D Aim;
	float Ratio;
	if (FocusEase > 0.05f || !D->GetFilmingGuide(Aim, Ratio)) return;
	float L, T, R, B;
	GetCaptureRect(L, T, R, B);
	const FVector2D Center((L + R) * 0.5f, (T + B) * 0.5f);
	const float Now = GetWorld()->GetRealTimeSeconds();
	const float Pulse = 0.7f + 0.3f * FMath::Sin(Now * 6.f);
	if (Ratio > 0.f)
	{
		const bool bMatched = FMath::Abs(FMath::Loge(Ratio)) < 0.06f;
		const FLinearColor Color = bMatched ? DCUI::Good : WithAlpha(DCUI::Gold, Pulse);
		// The extra frame represents subject magnification: too tight grows OUTSIDE the film.
		// Compress extreme errors so all four corners remain on screen.
		const float MaxScale = FMath::Min((Canvas->ClipX - 16.f * S) / (R - L), (Canvas->ClipY - 16.f * S) / (B - T));
		const float Scale = Ratio > 1.f ? 1.f + (MaxScale - 1.f) * (1.f - 1.f / Ratio) : FMath::Max(0.3f, Ratio);
		const float W = (R - L) * Scale, H = (B - T) * Scale;
		DCUI::Corners(Canvas, Center.X - W * 0.5f, Center.Y - H * 0.5f, W, H, 42.f * S, 10.f * S, Color);
	}
	// X/Y are film-width units; screen Y points down. Clamp off-screen targets along the same ray.
	FVector2D Delta(Aim.X * (R - L) * 0.5f, -Aim.Y * (R - L) * 0.5f);
	const float LimitX = (R - L) * 0.42f, LimitY = (B - T) * 0.36f;
	const double Fit = FMath::Max(1., FMath::Max(FMath::Abs(Delta.X) / LimitX, FMath::Abs(Delta.Y) / LimitY));
	Delta /= Fit;
	const FVector2D Target = Center + Delta;
	const bool bAimed = Aim.Size() < 0.045f;
	const FLinearColor Color = bAimed ? DCUI::Good : DCUI::Ivory;
	DCUI::Ring(Canvas, Target, (bAimed ? 10.f : 14.f + 2.f * Pulse) * S, 2.f * S, Color);
	DCUI::Line(Canvas, Center - FVector2D(5.f * S, 0), Center + FVector2D(5.f * S, 0), S, DCUI::Ivory);
	DCUI::Line(Canvas, Center - FVector2D(0, 5.f * S), Center + FVector2D(0, 5.f * S), S, DCUI::Ivory);
	if (!bAimed && Delta.Size() > 24.f * S)
	{
		const FVector2D Dir = Delta.GetSafeNormal(), Side(-Dir.Y, Dir.X);
		const FVector2D Tip = Target - Dir * 20.f * S;
		DCUI::Line(Canvas, Center + Dir * 12.f * S, Tip, 2.f * S, WithAlpha(Color, 0.65f));
		DCUI::Line(Canvas, Tip, Tip - Dir * 12.f * S + Side * 7.f * S, 3.f * S, Color);
		DCUI::Line(Canvas, Tip, Tip - Dir * 12.f * S - Side * 7.f * S, 3.f * S, Color);
		// A traveling dot makes the required mouse movement unambiguous.
		DCUI::Disc(Canvas, Center + Delta * FMath::Frac(Now * 0.85f), 4.f * S, DCUI::Gold);
	}
}

void ADCZombieHUD::DrawCutCue(ADCZombieDirector* D)
{
	float Left = 0.f;
	int32 CameraNumber = 0;
	if (!D->GetCutCue(Left, CameraNumber))
	{
		return;
	}
	int32 Count; float BoxW; FBox2D Bounds;
	if (!GetSwitcherLayout(D, Count, BoxW, Bounds) || CameraNumber < 1 || CameraNumber > Count) return;
	// Count down directly under the camera that will be needed.
	const bool bCut = Left <= 0.f;
	const FString Text = bCut ? FString::Printf(TEXT("Press camera %d!"), CameraNumber) : FString::FromInt(FMath::CeilToInt(Left));
	// Each number (and the call) pops in, then settles.
	const float Shown = bCut ? -Left : FMath::CeilToFloat(Left) - Left;
	const float Pop = 1.f + 0.3f * FMath::Exp(-Shown * 12.f);
	const float TextSize = Small * Pop;
	const FVector2D TextExtent = DCUI::Measure(Canvas, GetTextFont(), Text, Px(TextSize));

	// A pill-shaped speech bubble pointing up at the requested camera, bobbing gently.
	const float PadX = 30.f * S;
	const float PadY = 12.f * S;
	const float BH = TextExtent.Y + 2.f * PadY;
	const float BW = FMath::Max(TextExtent.X + 2.f * PadX, BH);
	const float Tail = 16.f * S;
	const float CX = Bounds.Min.X + (CameraNumber - 1) * (BoxW + 12.f * S) + BoxW * 0.5f;
	const float Bob = FMath::Sin(GetWorld()->GetRealTimeSeconds() * 3.f) * 6.f * S;
	const float BX = CX - BW * 0.5f;
	const float BY = Bounds.Max.Y + Tail + 10.f * S + Bob;
	auto Bubble = [&](float Grow, const FLinearColor& Color)
	{
		const float R = BH * 0.5f + Grow;
		DCUI::Rect(Canvas, BX + BH * 0.5f, BY - Grow, BW - BH, BH + 2.f * Grow, Color);
		DCUI::Disc(Canvas, FVector2D(BX + BH * 0.5f, BY + BH * 0.5f), R, Color);
		DCUI::Disc(Canvas, FVector2D(BX + BW - BH * 0.5f, BY + BH * 0.5f), R, Color);
		DCUI::Triangle(Canvas, FVector2D(CX - Tail - Grow, BY + 2.f * S), FVector2D(CX + Tail + Grow, BY + 2.f * S),
			FVector2D(CX, BY - Tail - Grow * 1.5f), Color);
	};
	const FLinearColor Dark(0.03f, 0.03f, 0.04f, 0.94f);
	Bubble(3.f * S, DCUI::Gold); // outline
	Bubble(0.f, bCut ? DCUI::Gold : Dark);
	Label(Text, CX, BY + (BH - TextExtent.Y) * 0.5f, bCut ? Dark : DCUI::Ivory, TextSize, EAlign::Center, !bCut);
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
