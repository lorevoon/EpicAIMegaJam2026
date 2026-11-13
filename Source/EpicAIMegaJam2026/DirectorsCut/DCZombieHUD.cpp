// Director's Cut - canvas HUD.

#include "DCZombieHUD.h"
#include "DCBeat.h"
#include "DCMultiCam.h"
#include "DCSceneMarks.h"
#include "DCZombieDirector.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FLinearColor Rose(1.f, 0.55f, 0.7f);
	const FLinearColor Gold(1.f, 0.72f, 0.2f);
	const FLinearColor Good(0.35f, 1.f, 0.45f);
	const FLinearColor Bad(1.f, 0.3f, 0.25f);
	const FLinearColor ZombieGreen(0.55f, 0.9f, 0.3f);
	const FLinearColor Muted(0.7f, 0.74f, 0.78f);
	const FLinearColor Panel(0.f, 0.f, 0.f, 0.6f);
	const FLinearColor Live(0.9f, 0.08f, 0.08f);
	constexpr float GaugeLinger = 1.5f;
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

void ADCZombieHUD::Label(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale, bool bCenter)
{
	UFont* Font = GEngine->GetLargeFont();
	const float FinalScale = Scale * S;
	if (bCenter)
	{
		float W = 0.f, H = 0.f;
		GetTextSize(Text, W, H, Font, FinalScale);
		X -= W * 0.5f;
	}
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, 0.8f), X + 2.f, Y + 2.f, Font, FinalScale);
	DrawText(Text, Color, X, Y, Font, FinalScale);
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
	S = FMath::Max(Canvas->ClipY / 1080.f, 0.6f);
	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.5f;

	if (D->GetPhase() == EDCPhase::Storyboard)
	{
		DrawStoryboardCard(D);
		return;
	}
	if (D->IsReviewing())
	{
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
			Label(TEXT("CLAPPERBOARD - E to call Action!"), P.X, P.Y, FLinearColor::White, 0.7f, true);
		}
	}

	// While shooting, dim everything outside the capture frame (the 70% safe area that scenes must sit in).
	// Drawn first so the HUD panels stay readable on top of it.
	if ((D->GetPhase() == EDCPhase::Countdown || D->GetPhase() == EDCPhase::Filming) && D->IsViewingCamera())
	{
		const float W = Canvas->ClipX;
		const float H = Canvas->ClipY;
		const float Margin = (1.f - DCFrame::CaptureFraction) * 0.5f;
		const float L = W * Margin, R = W * (1.f - Margin), T = H * Margin, B = H * (1.f - Margin);
		const FLinearColor Mask(0.f, 0.f, 0.f, 0.5f);
		DrawRect(Mask, 0.f, 0.f, W, T);
		DrawRect(Mask, 0.f, B, W, H - B);
		DrawRect(Mask, 0.f, T, L, B - T);
		DrawRect(Mask, R, T, W - R, B - T);
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
		DrawZombieAlarm(D);
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
	// Phase 1: the storyboard - what happens, in order. Never when or where.
	DrawRect(FLinearColor(0.02f, 0.02f, 0.03f, 0.92f), 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
	const float CX = Canvas->ClipX * 0.5f;
	float Y = Canvas->ClipY * 0.12f;
	Label(TEXT("1/4 STORYBOARD"), CX, Y, Muted, 0.8f, true);
	Y += 40.f * S;
	Label(D->MovieTitle, CX, Y, Rose, 2.2f, true);
	Y += 90.f * S;
	Label(TEXT("A fantasy tragedy on a working film set. Capture every scene - and keep the crew out of the movie."), CX, Y, FLinearColor::White, 0.8f, true);
	Y += 60.f * S;

	const TArray<FDCMultiCamBeat>& Beats = D->GetBeats();
	const float X = CX - 520.f * S;
	for (int32 i = 0; i < Beats.Num(); ++i)
	{
		const ADCBeat* Beat = Beats[i].Beat;
		Label(FString::Printf(TEXT("%d.  %s%s"), i + 1, *Beat->DisplayName, Beat->bMandatory ? TEXT("   (the studio needs this one)") : TEXT("")),
			X, Y, Beat->bMandatory ? Gold : FLinearColor::White, 0.85f);
		Y += 40.f * S;
	}

	Y += 30.f * S;
	Label(FString::Printf(TEXT("Film crew on camera cost %d points per second."), D->ZombiePenaltyPerSecond), CX, Y, ZombieGreen, 0.8f, true);
	Y += 60.f * S;
	Label(TEXT("Press Enter to start preparing - scrub through the scene with the mouse wheel while you place cameras"), CX, Y, FLinearColor::White, 1.f, true);
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
		FString ToolText = Tool == EDCCamTool::Dolly ? FString::Printf(TEXT("DOLLY (%d rail pts, F = new rail)"), D->GetPendingRailPoints())
			: Tool == EDCCamTool::Crane ? FString::Printf(TEXT("CRANE @%.0fcm"), D->GetPendingHeight())
			: FString::Printf(TEXT("STATIC @%.0fcm"), D->GetPendingHeight());
		Label(FString::Printf(TEXT("Cameras %d   [T] %s"), D->GetCameraCount(), *ToolText),
			Canvas->ClipX - 560.f * S, 22.f * S, FLinearColor(1.f, 0.6f, 0.2f), 0.8f);
	}
	else if (D->GetPhase() == EDCPhase::Filming)
	{
		Label(FString::Printf(TEXT("Crew screen time %.1fs  (-%d)"), D->GetZombieSeconds(), D->GetPenalty()),
			Canvas->ClipX - 460.f * S, 22.f * S, D->GetZombieSeconds() > 0.f ? ZombieGreen : Muted, 0.8f);
	}
}

void ADCZombieHUD::DrawStoryboardPanel(ADCZombieDirector* D)
{
	const TArray<FDCMultiCamBeat>& Beats = D->GetBeats();
	const float X = Canvas->ClipX - 520.f * S;
	float Y = 130.f * S;
	const float RowH = 28.f * S;
	DrawRect(Panel, X - 12.f * S, Y - 10.f * S, 510.f * S, RowH * Beats.Num() + 48.f * S);
	Label(TEXT("STORYBOARD (Tab)"), X, Y, Muted, 0.7f);
	Y += 32.f * S;
	for (int32 i = 0; i < Beats.Num(); ++i)
	{
		const FDCMultiCamBeat& State = Beats[i];
		const TCHAR* Mark = State.Status == EDCShotStatus::Captured ? TEXT("[OK]") : State.Status == EDCShotStatus::Missed ? TEXT("[--]") : TEXT("[  ]");
		const FLinearColor Color = State.Status == EDCShotStatus::Captured ? Good : State.Status == EDCShotStatus::Missed ? Muted
			: State.Beat->bMandatory ? Gold : FLinearColor::White;
		Label(FString::Printf(TEXT("%s %d. %s"), Mark, i + 1, *State.Beat->DisplayName), X, Y, Color, 0.58f);
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

	const float Margin = (1.f - DCFrame::CaptureFraction) * 0.5f;
	const float L = W * Margin, R = W * (1.f - Margin), T = H * Margin, B = H * (1.f - Margin);
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
		FString Rig = Cam->GetRig() == EDCCamRig::Dolly ? FString::Printf(TEXT("   DOLLY x%.2g (Z/X)"), Cam->GetRailSpeedScale())
			: Cam->GetRig() == EDCCamRig::Crane ? FString::Printf(TEXT("   CRANE %.0fcm"), Cam->GetCraneHeight()) : FString();
		Label(FString::Printf(TEXT("CAM %d   %.0fmm%s"), Cam->GetNumber(), 18.f / FMath::Tan(FMath::DegreesToRadians(Cam->GetFOV() * 0.5f)), *Rig),
			L, T - 34.f * S, D->GetPhase() == EDCPhase::Filming ? Bad : FLinearColor::White, 0.8f);
	}
}

void ADCZombieHUD::DrawPlanPreview(ADCZombieDirector* D)
{
	const float CX = Canvas->ClipX * 0.5f;
	float Y = Canvas->ClipY * 0.62f;
	const float Length = FMath::Max(D->TakeDuration, KINDA_SMALL_NUMBER);
	const float Time = D->GetSceneTime();
	Label(FString::Printf(TEXT("SCENE TIMELINE   %.1fs / %.0fs   (Wheel or Left/Right to scrub)"), Time, Length), CX, Y, Gold, 0.75f, true);
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
	bool bZombie = false;
	D->GetPlanPreview(Framed, bZombie);
	Y += 32.f * S;
	if (Framed.Num() > 0)
	{
		Label(TEXT("This camera frames: ") + FString::Join(Framed, TEXT(", ")), CX, Y, Good, 0.75f, true);
	}
	else
	{
		Label(TEXT("No storyboard scene framed at this moment"), CX, Y, Muted, 0.7f, true);
	}
	if (bZombie)
	{
		Y += 30.f * S;
		Label(TEXT("CREW IN VIEW at this moment"), CX, Y, ZombieGreen, 0.85f, true);
	}
}

void ADCZombieHUD::DrawGauges(ADCZombieDirector* D)
{
	const float BarW = 340.f * S;
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
		const FLinearColor Color = bJustCaptured ? Good : State.Beat->bMandatory ? Gold : Rose;
		Label(bJustCaptured ? FString::Printf(TEXT("%s - IN THE CAN"), *State.Beat->DisplayName) : State.Beat->DisplayName,
			Canvas->ClipX * 0.5f, Y, Color, 0.7f, true);
		Y += 26.f * S;
		DrawRect(Panel, X, Y, BarW, BarH);
		DrawRect(Color, X, Y, BarW * Fill, BarH);
		Y += BarH + 12.f * S;
	}
}

void ADCZombieHUD::DrawZombieAlarm(ADCZombieDirector* D)
{
	if (!D->IsZombieInShot())
	{
		return;
	}
	// Pulsing red frame around the live feed.
	const float Pulse = 0.5f + 0.5f * FMath::Sin(GetWorld()->GetTimeSeconds() * 10.f);
	const FLinearColor Edge(1.f, 0.f, 0.f, 0.25f + 0.35f * Pulse);
	const float T = 18.f * S;
	DrawRect(Edge, 0.f, 0.f, Canvas->ClipX, T);
	DrawRect(Edge, 0.f, Canvas->ClipY - T, Canvas->ClipX, T);
	DrawRect(Edge, 0.f, 0.f, T, Canvas->ClipY);
	DrawRect(Edge, Canvas->ClipX - T, 0.f, T, Canvas->ClipY);
	Label(FString::Printf(TEXT("CREW IN SHOT!  -%d/s"), D->ZombiePenaltyPerSecond), Canvas->ClipX * 0.5f, Canvas->ClipY * 0.16f, ZombieGreen, 1.4f, true);
}

void ADCZombieHUD::DrawSwitcher(ADCZombieDirector* D)
{
	// One button per camera; shrinks to fit when there are many.
	const int32 Count = D->IsUnlimited() ? D->GetSlotCount() : D->MaxCameras;
	if (Count == 0)
	{
		return;
	}
	const float Gap = 10.f * S;
	const float BoxW = FMath::Min(110.f * S, (Canvas->ClipX * 0.8f - (Count - 1) * Gap) / Count);
	const float BoxH = 56.f * S;
	float X = Canvas->ClipX * 0.5f - (Count * BoxW + (Count - 1) * Gap) * 0.5f;
	const float Y = Canvas->ClipY - BoxH - 160.f * S;
	const bool bOnAir = D->GetPhase() == EDCPhase::Filming || D->GetPhase() == EDCPhase::Countdown;

	for (int32 i = 0; i < Count; ++i)
	{
		const ADCMultiCam* Cam = D->GetCameraSlot(i);
		const bool bViewing = D->GetViewingSlot() == i;
		FLinearColor Fill = Panel;
		FLinearColor Text = Cam ? FLinearColor::White : Muted;
		FString Status = !Cam ? TEXT("empty")
			: Cam->GetRig() == EDCCamRig::Dolly ? TEXT("dolly")
			: Cam->GetRig() == EDCCamRig::Crane ? TEXT("crane") : TEXT("static");
		if (bViewing)
		{
			Fill = bOnAir ? Live : FLinearColor(0.8f, 0.6f, 0.1f, 0.85f);
			Status = bOnAir ? TEXT("LIVE") : TEXT("aiming");
		}
		DrawRect(Fill, X, Y, BoxW, BoxH);
		DrawLine(X, Y, X + BoxW, Y, Text, 1.f);
		DrawLine(X, Y + BoxH, X + BoxW, Y + BoxH, Text, 1.f);
		DrawLine(X, Y, X, Y + BoxH, Text, 1.f);
		DrawLine(X + BoxW, Y, X + BoxW, Y + BoxH, Text, 1.f);
		Label(FString::Printf(TEXT("%d"), i + 1), X + BoxW * 0.5f, Y + 2.f * S, Text, 1.1f, true);
		Label(Status, X + BoxW * 0.5f, Y + 32.f * S, Text, 0.55f, true);
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
			Lines.Add(TEXT("AIMING: mouse to turn, Shift+Wheel zoom, Wheel scrub the scene, W/S move dolly or crane, Z/X dolly speed"));
			Lines.Add(TEXT("This is where the camera starts every take.   [ / ] other cameras   E or the same number to step back"));
		}
		else
		{
			if (D->IsNearClapper())
			{
				Lines.Add(TEXT("E: hit the clapperboard and call ACTION!"));
			}
			Lines.Add(TEXT("T camera type   LMB place (dolly: rail points, F new rail)   RMB remove   Q/E height   1-9 or [ ] look through   C clear"));
			Lines.Add(TEXT("Wheel or Left/Right scrub the scene   Enter / clapperboard: Action!   Tab storyboard   Shift sprint"));
		}
		break;
	case EDCPhase::Countdown:
	case EDCPhase::Filming:
		Lines.Add(TEXT("1-9 or [ ] cut camera   hold LMB pause recording   Mouse aims   W/S dolly or crane   Z/X dolly speed   Wheel zooms   R restart   P pause"));
		break;
	default:
		break;
	}
	float Y = Canvas->ClipY - (Lines.Num() * 30.f + 20.f) * S;
	for (const FString& Line : Lines)
	{
		Label(Line, Canvas->ClipX * 0.5f, Y, FLinearColor::White, 0.72f, true);
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
	Label(D->WasApproved() ? D->GetCutName() : TEXT("Retake required - the studio's key scene is missing"), CX, Y, D->WasApproved() ? Gold : Bad, 1.f, true);
	Y += 46.f * S;

	for (int32 i = 0; i < Beats.Num(); ++i)
	{
		const FDCMultiCamBeat& State = Beats[i];
		const bool bGot = State.Status == EDCShotStatus::Captured;
		const FString Line = bGot
			? FString::Printf(TEXT("[OK] %d. %s   +%d  (CAM %d)"), i + 1, *State.Beat->DisplayName, State.Beat->Points, State.CapturedBy)
			: FString::Printf(TEXT("[--] %d. %s"), i + 1, *State.Beat->DisplayName);
		Label(Line, X + 40.f * S, Y, bGot ? Good : State.Beat->bMandatory ? Bad : Muted, 0.62f);
		Y += 34.f * S;
	}

	Y += 16.f * S;
	Label(FString::Printf(TEXT("Scenes  +%d      Crew screen time %.1fs  -%d      %d cuts"),
		D->GetBeatPoints(), D->GetZombieSeconds(), D->GetPenalty(), D->GetCutCount()), CX, Y, FLinearColor::White, 0.8f, true);
	Y += 40.f * S;
	Label(FString::Printf(TEXT("FINAL SCORE %d   (best %d)"), D->GetNetScore(), D->GetBestNet()), CX, Y, D->WasApproved() ? Gold : Bad, 1.2f, true);
	Y += 56.f * S;
	Label(D->GetZombieSeconds() <= 0.f && D->WasApproved() ? TEXT("Not a single crew member on screen. The illusion holds.")
		: D->GetZombieSeconds() > 5.f ? TEXT("The crew distracted the audience. Rehearse their routes and try another take.")
		: TEXT("A few crew members slipped into frame. Watch their routes on the next take."), CX, Y, Muted, 0.7f, true);
	Y += 44.f * S;
	Label(TEXT("Space watch the film again   Wheel scrub the film   R / Enter back to preparation (cameras kept)"), CX, Y, FLinearColor::White, 0.72f, true);
}

void ADCZombieHUD::DrawPremiere(ADCZombieDirector* D)
{
	// The finished film: letterboxed, no gameplay HUD.
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const float Bar = H * 0.11f;
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

	// Timeline with red ticks wherever a zombie made it into the film.
	const float X0 = W * 0.2f;
	const float TW = W * 0.6f;
	const float TY = H - Bar * 0.62f;
	const float TH = 6.f * S;
	DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.2f), X0, TY, TW, TH);
	bool bSpoiledNow = false;
	const TArray<FDCTakeSample>& Rec = D->GetRecording();
	for (int32 i = 0; i < Rec.Num(); ++i)
	{
		if (!Rec[i].bSpoiled)
		{
			continue;
		}
		const float T0 = Rec[i].FilmTime;
		const float T1 = Rec.IsValidIndex(i + 1) ? Rec[i + 1].FilmTime : T0;
		DrawRect(Bad, X0 + TW * T0 / Length, TY - 2.f, FMath::Max(TW * (T1 - T0) / Length, 1.f), TH + 4.f);
		bSpoiledNow |= Time >= T0 && Time <= T1;
	}
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
			JustCaptured = FString::Printf(TEXT("IN THE CAN: %s  +%d"), *State.Beat->DisplayName, State.Beat->Points);
		}
	}
	DrawRect(FLinearColor::White, X0 + TW * Time / Length - 1.5f, TY - 6.f, 3.f, TH + 12.f);
	if (!JustCaptured.IsEmpty())
	{
		Label(JustCaptured, W * 0.5f, H - Bar - 50.f * S, Gold, 0.85f, true);
	}

	Label(FString::Printf(TEXT("%02d:%04.1f / %02d:%04.1f"), int32(Time) / 60, FMath::Fmod(Time, 60.f), int32(Length) / 60, FMath::Fmod(Length, 60.f)),
		24.f * S, TY - 10.f * S, Muted, 0.7f);
	Label(TEXT("4/4 REVIEW   Space pause   Wheel scrub   Enter skip to evaluation"), W - 24.f * S - 520.f * S, TY - 10.f * S, Muted, 0.6f);
	Label(D->MovieTitle, W * 0.5f, Bar * 0.35f, Rose, 0.8f, true);
	if (bSpoiledNow)
	{
		Label(TEXT("zombie in shot"), W - 220.f * S, Bar * 0.35f, ZombieGreen, 0.7f);
	}
	if (D->IsPremierePaused() && !D->IsFilmFinished())
	{
		Label(TEXT("PAUSED"), W * 0.5f, H * 0.5f - 20.f * S, FLinearColor::White, 1.4f, true);
	}
}
