// Director's Cut - canvas HUD.

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
#include "CanvasItem.h"
#include "Fonts/SlateFontInfo.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FLinearColor Rose(1.f, 0.55f, 0.7f);
	const FLinearColor Gold(1.f, 0.72f, 0.2f);
	const FLinearColor Good(0.35f, 1.f, 0.45f);
	const FLinearColor Bad(1.f, 0.3f, 0.25f);
	const FLinearColor Muted(0.7f, 0.74f, 0.78f);
	const FLinearColor Panel(0.f, 0.f, 0.f, 0.6f);
	const FLinearColor Live(0.9f, 0.08f, 0.08f);
	constexpr float GaugeLinger = 1.5f;
	/** Width of the preparation reference-cut inset, as a share of the screen width. */
	constexpr float InsetShare = 0.24f;
	// Typewriter ink on the storyboard page.
	const FLinearColor Ink(0.12f, 0.09f, 0.07f);
	const FLinearColor InkFaded(0.38f, 0.3f, 0.22f);
	const FLinearColor InkRed(0.55f, 0.08f, 0.06f);
}

ADCZombieHUD::ADCZombieHUD()
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> StaticIcon(TEXT("/Game/DirectorsCut/UI/Icons/Vintage_Film_Camera_Rotation_Icon-1.Vintage_Film_Camera_Rotation_Icon-1"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> DollyIcon(TEXT("/Game/DirectorsCut/UI/Icons/Vintage_Film_Camera_on_Rails-2.Vintage_Film_Camera_on_Rails-2"));
	static ConstructorHelpers::FObjectFinder<UFont> Bree(TEXT("/Game/DirectorsCut/UI/Fonts/Bree_Serif__1_/BreeSerif-Regular_Font.BreeSerif-Regular_Font"));
	TextFont = Bree.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> Paper(TEXT("/Game/DirectorsCut/UI/Vintage_Script_Paper_UI_Element.Vintage_Script_Paper_UI_Element"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Monitor(TEXT("/Game/DirectorsCut/UI/SMALLHD_Monitor_Rig_Assembly.SMALLHD_Monitor_Rig_Assembly"));
	MonitorFrame = Monitor.Object;
	StaticCameraIcon = StaticIcon.Object;
	DollyCameraIcon = DollyIcon.Object;
	StoryboardPaper = Paper.Object;
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

void ADCZombieHUD::Label(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale, bool bCenter, bool bShadow, float MaxWidth)
{
	UFont* Font = GetTextFont();
	float FinalScale = Scale * S;
	float W = 0.f, H = 0.f;
	GetTextSize(Text, W, H, Font, FinalScale);
	// Long lines shrink to fit their room (by default the screen) rather than running off it.
	constexpr float EdgeMargin = 16.f;
	const float Room = MaxWidth > 0.f ? MaxWidth : bCenter ? Canvas->ClipX - 2.f * EdgeMargin : Canvas->ClipX - EdgeMargin - X;
	if (W > Room && Room > 0.f)
	{
		FinalScale *= Room / W;
		W = Room;
	}
	if (bCenter)
	{
		X -= W * 0.5f;
	}
	// Ask for the glyphs at the size they're shown. Plain DrawText renders the font once at its base size and stretches
	// that bitmap by the scale, which is what made larger text look pixelated.
	const FSlateFontInfo FontInfo(Font, FMath::Max(Font->LegacyFontSize * FinalScale, 6.f));
	FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Text), FontInfo, Color);
	if (bShadow)
	{
		Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.8f), FVector2D(2.f, 2.f));
	}
	Canvas->DrawItem(Item);
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
			Label(TEXT("CLAPPERBOARD - Enter to call Action!"), P.X, P.Y, FLinearColor::White, 0.7f, true);
		}
	}

	// While shooting, dim everything outside the capture frame (the 70% safe area that scenes must sit in) and frame
	// it with the field monitor, its screen fitted exactly over the capture frame. Drawn first so the HUD stays on top.
	if ((D->GetPhase() == EDCPhase::Countdown || D->GetPhase() == EDCPhase::Filming) && D->IsViewingCamera())
	{
		const float W = Canvas->ClipX;
		const float H = Canvas->ClipY;
		float L, T, R, B;
		GetCaptureRect(L, T, R, B);
		const FLinearColor Mask(0.f, 0.f, 0.f, 0.5f);
		DrawRect(Mask, 0.f, 0.f, W, T);
		DrawRect(Mask, 0.f, B, W, H - B);
		DrawRect(Mask, 0.f, T, L, B - T);
		DrawRect(Mask, R, T, W - R, B - T);

		if (MonitorFrame)
		{
			// Scale so the screen opening covers the frame exactly (the frame has the screen's own aspect).
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

	if (D->IsViewingCamera() && !D->IsReviewing())
	{
		DrawFraming(D);
	}
	else
	{
		DrawRect(FLinearColor::White, CX - 2.f, CY - 2.f, 4.f, 4.f);
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
		const int32 N = FMath::CeilToInt(D->GetCountdownRemaining());
		Label(FString::Printf(TEXT("%d"), FMath::Max(N, 1)), CX, CY - 160.f * S, FLinearColor::White, 4.f, true);
		Label(TEXT("Places, everyone... and keep it quiet."), CX, CY - 40.f * S, Muted, 1.f, true);
	}

	const FString Message = D->GetMessage();
	if (!Message.IsEmpty())
	{
		Label(Message, CX, Canvas->ClipY * 0.22f, FLinearColor::White, 1.f, true);
	}

	if (!D->GetSetupProblem().IsEmpty())
	{
		TArray<FString> Lines;
		D->GetSetupProblem().ParseIntoArrayLines(Lines);
		float Y = 150.f * S;
		for (const FString& Line : Lines)
		{
			Label(TEXT("SETUP: ") + Line, 24.f * S, Y, Bad, 0.7f);
			Y += 26.f * S;
		}
	}

	DrawReferenceInset(D);
	DrawSwitcher(D);
	DrawHelp(D);

	if (UGameplayStatics::IsGamePaused(this))
	{
		DrawRect(Panel, 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
		Label(TEXT("PAUSED - press P to resume"), CX, CY, FLinearColor::White, 1.6f, true);
	}
}

void ADCZombieHUD::DrawStoryboardCard(ADCZombieDirector* D)
{
	// Phase 1: the storyboard - what happens, in order (never when or where), typed on a script page.
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	DrawRect(FLinearColor(0.02f, 0.02f, 0.03f, 0.92f), 0.f, 0.f, W, H);

	// The page: as large as fits, keeping the art's proportions.
	const float Aspect = StoryboardPaper && StoryboardPaper->GetSurfaceHeight() > 0.f
		? StoryboardPaper->GetSurfaceWidth() / StoryboardPaper->GetSurfaceHeight() : 1.5f;
	float PH = H * 0.94f;
	float PW = PH * Aspect;
	if (PW > W * 0.96f)
	{
		PW = W * 0.96f;
		PH = PW / Aspect;
	}
	const float PX = (W - PW) * 0.5f;
	const float PY = (H - PH) * 0.5f;
	if (StoryboardPaper)
	{
		DrawTexture(StoryboardPaper, PX, PY, PW, PH, 0.f, 0.f, 1.f, 1.f);
	}
	else
	{
		DrawRect(FLinearColor(0.93f, 0.87f, 0.74f), PX, PY, PW, PH);
	}

	// Everything below is placed in page fractions and sized with the page, in ink (no drop shadow).
	const float K = PH / (800.f * S);
	auto Type = [&](const FString& Text, float FX, float FY, const FLinearColor& Color, float Scale, bool bCenter)
	{
		Label(Text, PX + PW * FX, PY + PH * FY, Color, Scale * K, bCenter, false, PW * (bCenter ? 0.78f : 0.92f - FX));
	};
	Type(TEXT("STORYBOARD   1/4"), 0.14f, 0.052f, InkFaded, 0.7f, false);
	Type(D->MovieTitle, 0.5f, 0.12f, InkRed, 2.f, true);
	Type(TEXT("A fantasy tragedy on a working film set. Capture every scene - and keep the crew out of the movie."), 0.5f, 0.225f, InkFaded, 0.72f, true);

	const TArray<FDCMultiCamBeat>& Beats = D->GetBeats();
	const float Step = FMath::Min(0.06f, 0.4f / FMath::Max(Beats.Num(), 1));
	float FY = 0.3f;
	for (int32 i = 0; i < Beats.Num(); ++i)
	{
		const ADCBeat* Beat = Beats[i].Beat;
		Type(FString::Printf(TEXT("%d.  %s"), i + 1, *Beat->DisplayName), 0.13f, FY, Ink, 0.85f, false);
		FY += Step;
	}

	Type(TEXT("Your film is graded on how closely it matches the reference cut: camera position, angle and zoom."), 0.5f, 0.76f, InkRed, 0.75f, true);
	Type(TEXT("Press Enter to start preparing"), 0.5f, 0.815f, Ink, 0.85f, true);
	Type(D->bShowReferenceCut ? TEXT("Space: watch the reference cut again")
		: TEXT("H pause/play rehearsal   Q/E scrub   , / . rehearsal speed"), 0.5f, 0.87f, InkFaded, 0.65f, true);
}

void ADCZombieHUD::DrawReferenceInset(ADCZombieDirector* D)
{
	UTextureRenderTarget2D* Inset = D->GetReferenceInset();
	if (!Inset)
	{
		return;
	}
	// Flush in the bottom-right corner, in the film's own 2.12:1 shape (the help lines move left of it).
	const float W = Canvas->ClipX * InsetShare;
	const float H = W / DCFrame::FilmAspect;
	const float X = Canvas->ClipX - W;
	const float Y = Canvas->ClipY - H;
	DrawRect(FLinearColor::Black, X - 4.f * S, Y - 30.f * S, W + 4.f * S, H + 30.f * S);
	Label(TEXT("REFERENCE CUT"), X, Y - 27.f * S, Gold, 0.5f, false, true, W);
	DrawTexture(Inset, X, Y, W, H, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Opaque);
	if (const ADCBeat* Beat = D->GetReferenceBeatAt(D->GetSceneTime()))
	{
		Label(Beat->DisplayName, X + W * 0.5f, Y + H - 28.f * S, FLinearColor::White, 0.5f, true, true, W - 12.f * S);
	}
}

void ADCZombieHUD::DrawReferenceCut(ADCZombieDirector* D)
{
	// The reference cut: the level shot well, for the player to study and try to match.
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const float Bar = FMath::Max((H - W / DCFrame::FilmAspect) * 0.5f, 40.f * S);
	const float Time = D->GetReferenceTime();
	const float Length = FMath::Max(D->GetReferenceLength(), KINDA_SMALL_NUMBER);
	DrawRect(FLinearColor::Black, 0.f, 0.f, W, Bar);
	DrawRect(FLinearColor::Black, 0.f, H - Bar, W, Bar);

	Label(TEXT("REFERENCE CUT"), 24.f * S, Bar * 0.25f, Gold, 0.75f);
	Label(D->MovieTitle, W * 0.5f, Bar * 0.25f, Rose, 0.75f, true);
	Label(TEXT("Study it - then try to shoot it yourself"), W * 0.5f, Bar * 0.25f + 30.f * S, Muted, 0.5f, true);

	// The storyboard line for the beat on screen, as a subtitle.
	if (const ADCBeat* Beat = D->GetReferenceBeat())
	{
		Label(Beat->DisplayName, W * 0.5f, H - Bar - 64.f * S, FLinearColor::White, 1.f, true);
	}

	// Timeline with a gold tick where each beat becomes capturable.
	const float X0 = W * 0.2f;
	const float TW = W * 0.6f;
	const float TY = H - Bar * 0.62f;
	const float TH = 6.f * S;
	DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.2f), X0, TY, TW, TH);
	for (const FDCMultiCamBeat& State : D->GetBeats())
	{
		DrawRect(Gold, X0 + TW * State.Beat->GetCaptureStartTime() / Length - 2.f, TY - 8.f * S, 4.f, TH + 16.f * S);
	}
	DrawRect(Rose, X0, TY, TW * Time / Length, TH);
	DrawRect(FLinearColor::White, X0 + TW * Time / Length - 1.5f, TY - 6.f, 3.f, TH + 12.f);
	Label(FString::Printf(TEXT("%04.1f / %04.1f"), Time, Length), 24.f * S, TY - 10.f * S, Muted, 0.6f);
	{
		const FString Hint = TEXT("Space pause   Q/E scrub   Enter storyboard");
		float HintW = 0.f, HintH = 0.f;
		GetTextSize(Hint, HintW, HintH, GetTextFont(), 0.6f * S);
		Label(Hint, W - 24.f * S - HintW, TY - 10.f * S, Muted, 0.6f);
	}
	if (D->IsReferencePaused())
	{
		Label(TEXT("PAUSED"), W * 0.5f, H * 0.5f - 20.f * S, FLinearColor::White, 1.4f, true);
	}
}

void ADCZombieHUD::DrawHeader(ADCZombieDirector* D)
{
	DrawRect(Panel, 0.f, 0.f, Canvas->ClipX, 70.f * S);

	FString PhaseText;
	FLinearColor PhaseColor = FLinearColor::White;
	switch (D->GetPhase())
	{
	case EDCPhase::Preparation: PhaseText = TEXT("2/4 PREPARATION");      PhaseColor = Gold; break;
	case EDCPhase::Countdown:   PhaseText = TEXT("3/4 SHOOTING - STAND BY"); PhaseColor = Gold; break;
	case EDCPhase::Filming:     PhaseText = TEXT("3/4 SHOOTING");         PhaseColor = Bad;  break;
	default: break;
	}
	Label(PhaseText, 24.f * S, 18.f * S, PhaseColor, 1.2f);
	Label(FString::Printf(TEXT("%04.1f / %.1f"), D->GetSceneTime(), D->TakeDuration), Canvas->ClipX * 0.5f, 10.f * S, FLinearColor::White, 1.6f, true);

	if (D->GetPhase() == EDCPhase::Preparation)
	{
		const EDCCamTool Tool = D->GetTool();
		FString ToolText = FString::Printf(TEXT("%s @%.0fcm"), Tool == EDCCamTool::Crane ? TEXT("CRANE") : TEXT("STATIC"), D->GetPendingHeight());
		Label(FString::Printf(TEXT("Cameras %d   [T] %s"), D->GetCameraCount(), *ToolText),
			Canvas->ClipX - 560.f * S, 22.f * S, FLinearColor(1.f, 0.6f, 0.2f), 0.8f);
	}
}

void ADCZombieHUD::DrawStoryboardPanel(ADCZombieDirector* D)
{
	const TArray<FDCMultiCamBeat>& Beats = D->GetBeats();
	TArray<FString> Lines;
	float Widest = 0.f;
	for (int32 i = 0; i < Beats.Num(); ++i)
	{
		const FDCMultiCamBeat& State = Beats[i];
		const TCHAR* Mark = State.Status == EDCShotStatus::Captured ? TEXT("[OK]") : State.Status == EDCShotStatus::Missed ? TEXT("[--]") : TEXT("[  ]");
		Lines.Add(FString::Printf(TEXT("%s %d. %s"), Mark, i + 1, *State.Beat->DisplayName));
		float LineW = 0.f, LineH = 0.f;
		GetTextSize(Lines.Last(), LineW, LineH, GetTextFont(), 0.58f * S);
		Widest = FMath::Max(Widest, LineW);
	}
	const float X = Canvas->ClipX - Widest - 36.f * S;
	float Y = 130.f * S;
	const float RowH = 28.f * S;
	DrawRect(Panel, X - 12.f * S, Y - 10.f * S, Widest + 36.f * S, RowH * Beats.Num() + 48.f * S);
	Label(TEXT("STORYBOARD (Tab)"), X, Y, Muted, 0.7f);
	Y += 32.f * S;
	for (int32 i = 0; i < Beats.Num(); ++i)
	{
		const FDCMultiCamBeat& State = Beats[i];
		const FLinearColor Color = State.Status == EDCShotStatus::Captured ? Good : State.Status == EDCShotStatus::Missed ? Muted : FLinearColor::White;
		Label(Lines[i], X, Y, Color, 0.58f);
		Y += RowH;
	}
}

void ADCZombieHUD::DrawFraming(ADCZombieDirector* D)
{
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	bool bRolling = false;
	for (const FDCMultiCamBeat& State : D->GetBeats())
	{
		bRolling |= D->GetPhase() == EDCPhase::Filming && State.Status == EDCShotStatus::Pending && State.Hold > 0.f;
	}
	const FLinearColor BracketColor = bRolling ? Good : FLinearColor(1.f, 1.f, 1.f, 0.45f);

	float L, T, R, B;
	GetCaptureRect(L, T, R, B);
	const float Arm = 40.f * S;
	const float Th = 3.f;
	DrawLine(L, T, L + Arm, T, BracketColor, Th); DrawLine(L, T, L, T + Arm, BracketColor, Th);
	DrawLine(R, T, R - Arm, T, BracketColor, Th); DrawLine(R, T, R, T + Arm, BracketColor, Th);
	DrawLine(L, B, L + Arm, B, BracketColor, Th); DrawLine(L, B, L, B - Arm, BracketColor, Th);
	DrawLine(R, B, R - Arm, B, BracketColor, Th); DrawLine(R, B, R, B - Arm, BracketColor, Th);
	DrawRect(BracketColor, W * 0.5f - 2.f, H * 0.5f - 2.f, 4.f, 4.f);

	// Recording state in the frame's top-right corner: a pause symbol while paused, a blinking REC otherwise.
	if (D->GetPhase() == EDCPhase::Filming)
	{
		const float IconX = R - 110.f * S;
		const float IconY = T + 18.f * S;
		if (D->IsRecordingPaused())
		{
			const float BarW = 14.f * S;
			const float BarH = 44.f * S;
			DrawRect(FLinearColor::White, IconX, IconY, BarW, BarH);
			DrawRect(FLinearColor::White, IconX + BarW + 12.f * S, IconY, BarW, BarH);
			Label(TEXT("PAUSED"), IconX + 20.f * S, IconY + BarH + 6.f * S, FLinearColor::White, 0.65f, true);
		}
		else if (FMath::Fmod(GetWorld()->GetTimeSeconds(), 1.f) < 0.6f)
		{
			Label(TEXT("* REC"), IconX, IconY, Bad, 1.f);
		}
	}

	if (const ADCMultiCam* Cam = D->GetCameraSlot(D->GetViewingSlot()))
	{
		FString Rig = FString::Printf(TEXT("   %s | WASD within %.1fm"), Cam->GetRig() == EDCCamRig::Crane ? TEXT("CRANE") : TEXT("STATIC"), Cam->GetMovementRadius()/100.f);
		Label(FString::Printf(TEXT("CAM %d   %.0fmm%s"), Cam->GetNumber(), 18.f / FMath::Tan(FMath::DegreesToRadians(Cam->GetFOV() * 0.5f)), *Rig),
			L, T - 34.f * S, D->GetPhase() == EDCPhase::Filming ? Bad : FLinearColor::White, 0.8f);
	}
}

void ADCZombieHUD::DrawPlanPreview(ADCZombieDirector* D)
{
	const float CX = Canvas->ClipX * 0.5f;
	float Y = Canvas->ClipY * 0.48f;
	const float Length = FMath::Max(D->TakeDuration, KINDA_SMALL_NUMBER);
	const float Time = D->GetSceneTime();
	Label(FString::Printf(TEXT("REHEARSAL %s   %.1fx   %.1fs / %.0fs   H play/pause   ,/. speed   Q/E scrub"),
		D->IsRehearsalPlaying() ? TEXT("PLAYING") : TEXT("PAUSED"), D->GetRehearsalSpeed(), Time, Length), CX, Y, Gold, 0.65f, true);
	const float TW = 640.f * S;
	const float X0 = CX - TW * 0.5f;
	const float TY = Y + 32.f * S;
	const float TH = 6.f * S;
	DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.25f), X0, TY, TW, TH);
	DrawRect(Gold, X0, TY, TW * Time / Length, TH);
	DrawRect(FLinearColor::White, X0 + TW * Time / Length - 1.5f, TY - 6.f * S, 3.f, TH + 12.f * S);
	Y = TY + 4.f * S;
	if (!D->IsViewingCamera())
	{
		return;
	}
	TArray<FString> Framed;
	D->GetPlanPreview(Framed);
	Y += 32.f * S;
	if (Framed.Num() > 0)
	{
		Label(TEXT("This camera frames: ") + FString::Join(Framed, TEXT(", ")), CX, Y, Good, 0.75f, true);
	}
	else
	{
		Label(TEXT("No storyboard scene framed at this moment"), CX, Y, Muted, 0.7f, true);
	}
}

void ADCZombieHUD::DrawGauges(ADCZombieDirector* D)
{
	const float BarW = 340.f * S;
	const float BarH = 12.f * S;
	float Y = Canvas->ClipY * 0.52f;
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
		const FLinearColor Color = bJustCaptured ? Good : Rose;
		Label(bJustCaptured ? FString::Printf(TEXT("%s - IN THE CAN"), *State.Beat->DisplayName) : State.Beat->DisplayName,
			Canvas->ClipX * 0.5f, Y, Color, 0.7f, true);
		Y += 26.f * S;
		DrawRect(Panel, X, Y, BarW, BarH);
		DrawRect(Color, X, Y, BarW * Fill, BarH);
		Y += BarH + 12.f * S;
	}
}

void ADCZombieHUD::DrawSwitcher(ADCZombieDirector* D)
{
	// One button per camera (number + camera-type icon); shrinks to fit when there are many. While placing, the
	// camera about to be placed already has its button, pulsing, with the icon of the selected type (T switches it).
	const int32 PlaceSlot = D->GetPlacementSlot();
	const int32 Count = FMath::Max(D->IsUnlimited() ? D->GetSlotCount() : D->MaxCameras, PlaceSlot + 1);
	if (Count == 0)
	{
		return;
	}
	const float Gap = 10.f * S;
	const float BoxW = FMath::Min(110.f * S, (Canvas->ClipX * 0.8f - (Count - 1) * Gap) / Count);
	const float BoxH = 84.f * S;
	float X = Canvas->ClipX * 0.5f - (Count * BoxW + (Count - 1) * Gap) * 0.5f;
	const float Y = Canvas->ClipY - BoxH - 160.f * S;
	const bool bOnAir = D->GetPhase() == EDCPhase::Filming || D->GetPhase() == EDCPhase::Countdown;
	const float Pulse = 0.65f + 0.35f * FMath::Sin(GetWorld()->GetRealTimeSeconds() * 5.f);

	for (int32 i = 0; i < Count; ++i)
	{
		const ADCMultiCam* Cam = D->GetCameraSlot(i);
		const bool bPlacing = i == PlaceSlot;
		FLinearColor Fill = Panel;
		FLinearColor Edge = Cam ? FLinearColor::White : Muted;
		if (D->GetViewingSlot() == i)
		{
			Fill = bOnAir ? Live : FLinearColor(0.8f, 0.6f, 0.1f, 0.85f);
		}
		else if (bPlacing)
		{
			Fill = FLinearColor(Gold.R, Gold.G, Gold.B, 0.25f * Pulse);
			Edge = FLinearColor(Gold.R, Gold.G, Gold.B, Pulse);
		}
		DrawRect(Fill, X, Y, BoxW, BoxH);
		const float Thick = bPlacing ? 2.f : 1.f;
		DrawLine(X, Y, X + BoxW, Y, Edge, Thick);
		DrawLine(X, Y + BoxH, X + BoxW, Y + BoxH, Edge, Thick);
		DrawLine(X, Y, X, Y + BoxH, Edge, Thick);
		DrawLine(X + BoxW, Y, X + BoxW, Y + BoxH, Edge, Thick);
		Label(FString::Printf(TEXT("%d"), i + 1), X + BoxW * 0.5f, Y + 2.f * S, Edge, 1.1f, true);

		// The placed camera's type, or (for the button being placed) the type the next click will build.
		if (Cam || bPlacing)
		{
			const bool bCrane = Cam ? Cam->GetRig() == EDCCamRig::Crane : D->GetTool() == EDCCamTool::Crane;
			if (bCrane) Label(TEXT("CRANE"), X + BoxW*.5f, Y + 45.f*S, Edge, .55f, true);
			if (UTexture2D* Icon = bCrane ? nullptr : StaticCameraIcon)
			{
				const float Aspect = Icon->GetSurfaceHeight() > 0.f ? Icon->GetSurfaceWidth() / Icon->GetSurfaceHeight() : 1.f;
				float IconH = BoxH - 40.f * S;
				float IconW = IconH * Aspect;
				if (IconW > BoxW - 10.f * S)
				{
					IconW = BoxW - 10.f * S;
					IconH = IconW / Aspect;
				}
				DrawTexture(Icon, X + (BoxW - IconW) * 0.5f, Y + 36.f * S + (BoxH - 40.f * S - IconH) * 0.5f, IconW, IconH,
					0.f, 0.f, 1.f, 1.f, bPlacing && !Cam ? Edge : FLinearColor::White);
			}
		}
		if (bPlacing)
		{
			Label(TEXT("T switch"), X + BoxW * 0.5f, Y + BoxH + 4.f * S, Gold, 0.5f, true);
		}
		X += BoxW + Gap;
	}
}

void ADCZombieHUD::DrawHelp(ADCZombieDirector* D)
{
	TArray<FString> Lines;
	switch (D->GetPhase())
	{
	case EDCPhase::Preparation:
		if (D->IsViewingCamera())
		{
			Lines.Add(TEXT("AIMING - this is where the camera starts every take"));
			Lines.Add(TEXT("Mouse turn   Wheel zoom   Q/E scrub the scene"));
			Lines.Add(TEXT("WASD move nearby   1-9 / [ ] switch cameras   RMB step back | Preset autosaved"));
		}
		else
		{
			Lines.Add(TEXT("T camera type   LMB place   RMB remove   Wheel height   C clear"));
			Lines.Add(TEXT("Static / Crane: one click to place   1-9 or [ ] look through"));
			Lines.Add(D->IsNearClapper() ? TEXT("Enter: hit the clapperboard and call ACTION!")
				: TEXT("Q/E scrub   H rehearsal   Tab storyboard   Shift sprint   Space jump   Enter: Action!"));
		}
		break;
	case EDCPhase::Countdown:
	case EDCPhase::Filming:
		Lines.Add(TEXT("1-9 or [ ] cut camera   Mouse aims   Wheel zooms   hold LMB pause recording"));
		Lines.Add(TEXT("WASD move nearby   R restart   P pause"));
		break;
	default:
		break;
	}
	// With the reference inset in the bottom-right corner, centre the help in the space to its left.
	const float Room = D->GetReferenceInset() ? Canvas->ClipX * (1.f - InsetShare) - 24.f * S : Canvas->ClipX - 32.f;
	const float CenterX = D->GetReferenceInset() ? Room * 0.5f : Canvas->ClipX * 0.5f;
	float Y = Canvas->ClipY - (Lines.Num() * 30.f + 20.f) * S;
	for (const FString& Line : Lines)
	{
		Label(Line, CenterX, Y, FLinearColor::White, 0.72f, true, true, Room);
		Y += 30.f * S;
	}
}

void ADCZombieHUD::DrawResults(ADCZombieDirector* D)
{
	const TArray<FDCMultiCamBeat>& Beats = D->GetBeats();
	const float W = 1000.f * S;
	const float X = Canvas->ClipX * 0.5f - W * 0.5f;
	float Y = Canvas->ClipY * 0.1f;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.88f), X, Y, W, (300.f + Beats.Num() * 34.f) * S);

	const float CX = Canvas->ClipX * 0.5f;
	Y += 16.f * S;
	Label(D->MovieTitle, CX, Y, Rose, 1.8f, true);
	Y += 64.f * S;
	const FDCMatch& Match = D->GetMatch();
	const bool bGood = D->IsGoodTake();
	Label(FString::Printf(TEXT("GRADE %s  -  %s"), *D->GetGrade(), *D->GetCutName()), CX, Y, bGood ? Gold : Bad, 1.f, true);
	Y += 46.f * S;

	for (int32 i = 0; i < Beats.Num(); ++i)
	{
		const FDCMultiCamBeat& State = Beats[i];
		const bool bGot = State.Status == EDCShotStatus::Captured;
		const FString Line = bGot
			? FString::Printf(TEXT("[OK] %d. %s   (CAM %d)"), i + 1, *State.Beat->DisplayName, State.CapturedBy)
			: FString::Printf(TEXT("[--] %d. %s"), i + 1, *State.Beat->DisplayName);
		Label(Line, X + 40.f * S, Y, bGot ? Good : Muted, 0.62f, false, true, W - 80.f * S);
		Y += 34.f * S;
	}

	Y += 16.f * S;
	Label(FString::Printf(TEXT("Camera position %.0f%%      Angle %.0f%%      Zoom %.0f%%      %d cuts"),
		Match.Position * 100.f, Match.Angle * 100.f, Match.Zoom * 100.f, D->GetCutCount()), CX, Y, FLinearColor::White, 0.8f, true);
	Y += 40.f * S;
	Label(FString::Printf(TEXT("MATCH %.0f%%   (best %.0f%%)"), Match.Overall * 100.f, D->GetBestMatch() * 100.f), CX, Y, bGood ? Gold : Bad, 1.2f, true);
	Y += 56.f * S;
	// A tip about the weakest part of the match.
	const float Weakest = FMath::Min3(Match.Position, Match.Angle, Match.Zoom);
	Label(D->GetGradeRank() == 0 ? TEXT("Shot for shot, that's the reference cut.")
		: Weakest == Match.Position ? TEXT("Your cameras stood far from the reference spots - watch where the reference cut films from.")
		: Weakest == Match.Angle ? TEXT("Your cameras pointed elsewhere - match the reference cut's framing.")
		: TEXT("Your lenses differ - zoom like the reference cut does."), CX, Y, Muted, 0.7f, true);
	Y += 44.f * S;
	Label(TEXT("Space watch the film again   Q/E scrub the film   R / Enter back to preparation (cameras kept)"), CX, Y, FLinearColor::White, 0.72f, true);
}

void ADCZombieHUD::DrawPremiere(ADCZombieDirector* D)
{
	// The finished film: letterboxed to the film's aspect (the bars match the engine's), no gameplay HUD.
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const float Bar = FMath::Max((H - W / DCFrame::FilmAspect) * 0.5f, 0.f);
	const float Time = D->GetSceneTime();
	const float Length = FMath::Max(D->GetPremiereLength(), KINDA_SMALL_NUMBER);
	DrawRect(FLinearColor::Black, 0.f, 0.f, W, Bar);
	DrawRect(FLinearColor::Black, 0.f, H - Bar, W, Bar);

	// Opening title card.
	if (Time < 3.5f)
	{
		Label(D->MovieTitle, W * 0.5f, H * 0.4f, Rose, 2.4f, true);
		Label(TEXT("a film by you"), W * 0.5f, H * 0.4f + 90.f * S, FLinearColor::White, 0.9f, true);
	}

	// Timeline.
	const float X0 = W * 0.2f;
	const float TW = W * 0.6f;
	const float TY = H - Bar * 0.62f;
	const float TH = 6.f * S;
	DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.2f), X0, TY, TW, TH);
	DrawRect(Rose, X0, TY, TW * Time / Length, TH);
	// Gold marks where each storyboard scene made it into the film; a caption as the film reaches it.
	FString JustCaptured;
	for (const FDCMultiCamBeat& State : D->GetBeats())
	{
		if (State.Status != EDCShotStatus::Captured)
		{
			continue;
		}
		DrawRect(Gold, X0 + TW * State.CapturedFilmTime / Length - 2.f, TY - 8.f * S, 4.f, TH + 16.f * S);
		if (!D->IsFilmFinished() && Time >= State.CapturedFilmTime && Time - State.CapturedFilmTime < 2.5f)
		{
			JustCaptured = FString::Printf(TEXT("IN THE CAN: %s"), *State.Beat->DisplayName);
		}
	}
	DrawRect(FLinearColor::White, X0 + TW * Time / Length - 1.5f, TY - 6.f, 3.f, TH + 12.f);
	if (!JustCaptured.IsEmpty())
	{
		Label(JustCaptured, W * 0.5f, H - Bar - 50.f * S, Gold, 0.85f, true);
	}

	Label(FString::Printf(TEXT("%02d:%04.1f / %02d:%04.1f"), int32(Time) / 60, FMath::Fmod(Time, 60.f), int32(Length) / 60, FMath::Fmod(Length, 60.f)),
		24.f * S, TY - 10.f * S, Muted, 0.7f);
	{
		const FString Hint = TEXT("4/4 REVIEW   Space pause   Q/E scrub   Enter skip to evaluation");
		float HintW = 0.f, HintH = 0.f;
		GetTextSize(Hint, HintW, HintH, GetTextFont(), 0.6f * S);
		Label(Hint, W - 24.f * S - HintW, TY - 10.f * S, Muted, 0.6f);
	}
	Label(D->MovieTitle, W * 0.5f, Bar * 0.35f, Rose, 0.8f, true);
	if (D->IsPremierePaused() && !D->IsFilmFinished())
	{
		Label(TEXT("PAUSED"), W * 0.5f, H * 0.5f - 20.f * S, FLinearColor::White, 1.4f, true);
	}
}
