// Director's Cut prototype 3 - canvas HUD.

#include "DCMultiCamHUD.h"
#include "DCBeat.h"
#include "DCMultiCam.h"
#include "DCMultiCamDirector.h"
#include "DCSceneMarks.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FLinearColor Gold(1.f, 0.72f, 0.2f);
	const FLinearColor Cyan(0.3f, 0.85f, 1.f);
	const FLinearColor Good(0.35f, 1.f, 0.45f);
	const FLinearColor Bad(1.f, 0.35f, 0.3f);
	const FLinearColor Muted(0.7f, 0.74f, 0.78f);
	const FLinearColor Panel(0.f, 0.f, 0.f, 0.6f);
	const FLinearColor Live(0.9f, 0.08f, 0.08f);
	constexpr float GaugeLinger = 1.5f;
}

ADCMultiCamDirector* ADCMultiCamHUD::FindDirector()
{
	if (!Director.IsValid())
	{
		for (TActorIterator<ADCMultiCamDirector> It(GetWorld()); It; ++It)
		{
			Director = *It;
			break;
		}
	}
	return Director.Get();
}

void ADCMultiCamHUD::Label(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale, bool bCenter)
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

bool ADCMultiCamHUD::ProjectVisible(const FVector& World, FVector2D& OutScreen) const
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

FLinearColor ADCMultiCamHUD::BeatColor(const FDCMultiCamBeat& State)
{
	switch (State.Status)
	{
	case EDCShotStatus::Captured: return Good;
	case EDCShotStatus::Missed:   return Muted;
	default:                      return State.Beat->bMandatory ? Gold : Cyan;
	}
}

void ADCMultiCamHUD::DrawHUD()
{
	Super::DrawHUD();

	ADCMultiCamDirector* D = FindDirector();
	if (!D || !Canvas)
	{
		return;
	}
	S = FMath::Max(Canvas->ClipY / 1080.f, 0.6f);
	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.5f;

	if (D->GetPhase() == EDCPhase::Preparation && !D->IsViewingCamera())
	{
		FVector2D P;
		if (D->GetClapperboard() && ProjectVisible(D->GetClapperboard()->GetActorLocation() + FVector(0.f, 0.f, 170.f), P))
		{
			Label(TEXT("CLAPPERBOARD - E to call Action!"), P.X, P.Y, FLinearColor::White, 0.7f, true);
		}
	}

	DrawHeader(D);
	DrawTimeline(D);
	if (D->bShowNotebook)
	{
		DrawNotebook(D);
	}

	if (D->IsViewingCamera())
	{
		DrawFraming(D);
	}
	else
	{
		DrawRect(FLinearColor::White, CX - 2.f, CY - 2.f, 4.f, 4.f);
	}
	if (D->GetPhase() == EDCPhase::Filming)
	{
		DrawGauges(D);
	}

	if (D->GetPhase() == EDCPhase::Countdown)
	{
		const int32 N = FMath::CeilToInt(D->GetCountdownRemaining());
		Label(FString::Printf(TEXT("%d"), FMath::Max(N, 1)), CX, CY - 160.f * S, FLinearColor::White, 4.f, true);
		Label(TEXT("Places, everyone..."), CX, CY - 40.f * S, Muted, 1.f, true);
	}

	const FString Message = D->GetMessage();
	if (!Message.IsEmpty())
	{
		Label(Message, CX, Canvas->ClipY * 0.22f, FLinearColor::White, 1.1f, true);
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

	if (D->GetPhase() == EDCPhase::Results)
	{
		DrawResults(D);
	}
	else
	{
		DrawSwitcher(D);
		DrawHelp(D);
	}

	if (UGameplayStatics::IsGamePaused(this))
	{
		DrawRect(Panel, 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
		Label(TEXT("PAUSED - press P to resume"), CX, CY, FLinearColor::White, 1.6f, true);
	}
}

void ADCMultiCamHUD::DrawHeader(ADCMultiCamDirector* D)
{
	DrawRect(Panel, 0.f, 0.f, Canvas->ClipX, 70.f * S);

	FString PhaseText;
	FLinearColor PhaseColor = FLinearColor::White;
	switch (D->GetPhase())
	{
	case EDCPhase::Preparation: PhaseText = TEXT("PREPARATION"); PhaseColor = Cyan; break;
	case EDCPhase::Countdown:   PhaseText = TEXT("STAND BY");    PhaseColor = Gold; break;
	case EDCPhase::Filming:     PhaseText = TEXT("ON AIR");      PhaseColor = Bad;  break;
	case EDCPhase::Results:     PhaseText = TEXT("CUT");         PhaseColor = Gold; break;
	}
	Label(PhaseText, 24.f * S, 18.f * S, PhaseColor, 1.2f);
	Label(FString::Printf(TEXT("%04.1f / %.1f"), D->GetSceneTime(), D->TakeDuration), Canvas->ClipX * 0.5f, 10.f * S, FLinearColor::White, 1.6f, true);

	const ADCMultiCam* Viewing = D->GetCameraSlot(D->GetViewingSlot());
	if (D->GetPhase() == EDCPhase::Filming && Viewing)
	{
		Label(FString::Printf(TEXT("* LIVE  CAM %d"), Viewing->GetNumber()), Canvas->ClipX - 260.f * S, 18.f * S, Bad, 1.1f);
	}
	else if (D->GetPhase() == EDCPhase::Preparation)
	{
		Label(FString::Printf(TEXT("Cameras %d/%d   stand height %.0fcm"), D->GetCameraCount(), D->MaxCameras, D->GetPendingHeight()),
			Canvas->ClipX - 460.f * S, 22.f * S, FLinearColor(1.f, 0.6f, 0.2f), 0.8f);
		Label(D->GetBrief(), Canvas->ClipX * 0.5f, 100.f * S, FLinearColor::White, 0.75f, true);
	}
}

void ADCMultiCamHUD::DrawTimeline(ADCMultiCamDirector* D)
{
	const float X0 = Canvas->ClipX * 0.2f;
	const float W = Canvas->ClipX * 0.6f;
	const float Y = 76.f * S;
	const float H = 12.f * S;
	const float Duration = D->TakeDuration;

	DrawRect(Panel, X0 - 4.f, Y - 4.f, W + 8.f, H + 8.f);
	// Only beats the player has discovered appear on the timeline.
	for (const FDCMultiCamBeat& State : D->GetBeats())
	{
		if (!State.bSpotted)
		{
			continue;
		}
		const float SX = X0 + W * State.Beat->WindowStart / Duration;
		const float SW = W * (State.Beat->WindowEnd - State.Beat->WindowStart) / Duration;
		DrawRect(BeatColor(State) * FLinearColor(1.f, 1.f, 1.f, 0.6f), SX, Y, SW, H);
	}
	const float PX = X0 + W * D->GetSceneTime() / Duration;
	DrawRect(FLinearColor::White, PX - 1.5f, Y - 5.f, 3.f, H + 10.f);
}

void ADCMultiCamHUD::DrawNotebook(ADCMultiCamDirector* D)
{
	const TArray<FDCMultiCamBeat>& Beats = D->GetBeats();
	const float X = Canvas->ClipX - 430.f * S;
	float Y = 130.f * S;
	const float RowH = 28.f * S;
	DrawRect(Panel, X - 12.f * S, Y - 10.f * S, 420.f * S, RowH * Beats.Num() + 48.f * S);
	Label(FString::Printf(TEXT("NOTEBOOK (Tab)   discovered %d/%d"), D->GetSpottedCount(), Beats.Num()), X, Y, Muted, 0.7f);
	Y += 32.f * S;

	for (const FDCMultiCamBeat& State : Beats)
	{
		const ADCBeat* Beat = State.Beat;
		const TCHAR* Status = State.Status == EDCShotStatus::Captured ? TEXT("[OK]") : State.Status == EDCShotStatus::Missed ? TEXT("[--]") : TEXT("[  ]");
		FString Line;
		FLinearColor Color = BeatColor(State);
		if (State.bSpotted)
		{
			Line = FString::Printf(TEXT("%s %s  %.0f-%.0fs%s"), Status, *Beat->DisplayName, Beat->WindowStart, Beat->WindowEnd, State.bEverCaptured ? TEXT("  *") : TEXT(""));
		}
		else if (Beat->bMandatory)
		{
			Line = FString::Printf(TEXT("%s %s  - when? where?"), Status, *Beat->DisplayName);
		}
		else
		{
			Line = FString::Printf(TEXT("%s ???"), Status);
			Color = Muted;
		}
		Label(Line, X, Y, Color, 0.65f);
		Y += RowH;
	}
}

void ADCMultiCamHUD::DrawFraming(ADCMultiCamDirector* D)
{
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const bool bRolling = D->GetPhase() == EDCPhase::Filming && D->IsLiveFeedRolling();
	const FLinearColor BracketColor = bRolling ? Good : FLinearColor(1.f, 1.f, 1.f, 0.45f);

	// 70% safe-frame brackets: subjects must sit inside these to count.
	const float L = W * 0.15f, R = W * 0.85f, T = H * 0.15f, B = H * 0.85f;
	const float Arm = 40.f * S;
	const float Th = 3.f;
	DrawLine(L, T, L + Arm, T, BracketColor, Th); DrawLine(L, T, L, T + Arm, BracketColor, Th);
	DrawLine(R, T, R - Arm, T, BracketColor, Th); DrawLine(R, T, R, T + Arm, BracketColor, Th);
	DrawLine(L, B, L + Arm, B, BracketColor, Th); DrawLine(L, B, L, B - Arm, BracketColor, Th);
	DrawLine(R, B, R - Arm, B, BracketColor, Th); DrawLine(R, B, R, B - Arm, BracketColor, Th);
	DrawRect(BracketColor, W * 0.5f - 2.f, H * 0.5f - 2.f, 4.f, 4.f);

	if (const ADCMultiCam* Cam = D->GetCameraSlot(D->GetViewingSlot()))
	{
		Label(FString::Printf(TEXT("CAM %d   %.0fmm"), Cam->GetNumber(), 18.f / FMath::Tan(FMath::DegreesToRadians(Cam->GetFOV() * 0.5f))),
			L, T - 34.f * S, D->GetPhase() == EDCPhase::Filming ? Bad : FLinearColor::White, 0.8f);
	}
}

void ADCMultiCamHUD::DrawGauges(ADCMultiCamDirector* D)
{
	// A gauge appears only while the live feed is actually filming a beat (or just finished one).
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
		Label(bJustCaptured ? FString::Printf(TEXT("%s - IN THE CAN"), *State.Beat->DisplayName) : State.Beat->DisplayName,
			Canvas->ClipX * 0.5f, Y, bJustCaptured ? Good : BeatColor(State), 0.75f, true);
		Y += 26.f * S;
		DrawRect(Panel, X, Y, BarW, BarH);
		DrawRect(bJustCaptured ? Good : BeatColor(State), X, Y, BarW * Fill, BarH);
		Y += BarH + 12.f * S;
	}
}

void ADCMultiCamHUD::DrawSwitcher(ADCMultiCamDirector* D)
{
	// The vision mixer: one button per camera slot.
	const int32 Count = D->MaxCameras;
	const float BoxW = 110.f * S;
	const float BoxH = 56.f * S;
	const float Gap = 12.f * S;
	float X = Canvas->ClipX * 0.5f - (Count * BoxW + (Count - 1) * Gap) * 0.5f;
	const float Y = Canvas->ClipY - BoxH - 110.f * S;
	const bool bOnAir = D->GetPhase() == EDCPhase::Filming || D->GetPhase() == EDCPhase::Countdown;

	for (int32 i = 0; i < Count; ++i)
	{
		const ADCMultiCam* Cam = D->GetCameraSlot(i);
		const bool bViewing = D->GetViewingSlot() == i;
		FLinearColor Fill = Panel;
		FLinearColor Text = Muted;
		FString Status = TEXT("empty");
		if (Cam)
		{
			Text = FLinearColor::White;
			Status = TEXT("ready");
		}
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

void ADCMultiCamHUD::DrawHelp(ADCMultiCamDirector* D)
{
	TArray<FString> Lines;
	switch (D->GetPhase())
	{
	case EDCPhase::Preparation:
		if (D->IsViewingCamera())
		{
			Lines.Add(TEXT("AIMING: mouse to turn, wheel to zoom - this is where the camera starts every take. E or the same number to step back."));
		}
		else
		{
			if (D->IsNearClapper())
			{
				Lines.Add(TEXT("E: hit the clapperboard and call ACTION!"));
			}
			Lines.Add(TEXT("LMB place camera (faces where you look)   RMB remove   Wheel stand height   1-4 look through / aim   C clear"));
			Lines.Add(TEXT("Enter / clapperboard: Action!   Tab notebook   Shift sprint   P pause"));
		}
		break;
	case EDCPhase::Countdown:
	case EDCPhase::Filming:
		Lines.Add(TEXT("1-4 cut to camera   Mouse aims the live camera   Wheel zooms   R restart   P pause"));
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

void ADCMultiCamHUD::DrawResults(ADCMultiCamDirector* D)
{
	const TArray<FDCMultiCamBeat>& Beats = D->GetBeats();
	const float W = 1000.f * S;
	const float X = Canvas->ClipX * 0.5f - W * 0.5f;
	float Y = Canvas->ClipY * 0.14f;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.85f), X, Y, W, (190.f + Beats.Num() * 50.f) * S);

	const float CX = Canvas->ClipX * 0.5f;
	Y += 20.f * S;
	Label(D->GetCutName(), CX, Y, D->WasApproved() ? Gold : Bad, 1.8f, true);
	Y += 64.f * S;
	Label(D->WasApproved()
		? FString::Printf(TEXT("Approved  -  Score %d   (session best %d)   %d cuts   discovered %d/%d"), D->GetScore(), D->GetBestScore(), D->GetCutCount(), D->GetSpottedCount(), Beats.Num())
		: FString::Printf(TEXT("The studio's required footage is missing - no approved score   (%d cuts, discovered %d/%d)"), D->GetCutCount(), D->GetSpottedCount(), Beats.Num()),
		CX, Y, FLinearColor::White, 0.85f, true);
	Y += 46.f * S;

	for (const FDCMultiCamBeat& State : Beats)
	{
		const ADCBeat* Beat = State.Beat;
		if (State.Status == EDCShotStatus::Captured)
		{
			Label(FString::Printf(TEXT("[OK] %s  (+%d, CAM %d)"), *Beat->DisplayName, Beat->Points, State.CapturedBy), X + 40.f * S, Y, Good, 0.8f);
			Label(Beat->Caption, X + 80.f * S, Y + 24.f * S, Muted, 0.6f);
		}
		else if (State.bSpotted || Beat->bMandatory)
		{
			Label(FString::Printf(TEXT("[--] %s"), *Beat->DisplayName), X + 40.f * S, Y, Beat->bMandatory ? Bad : Muted, 0.8f);
			Label(State.bSpotted ? FString::Printf(TEXT("Glimpsed but not held long enough - it plays %.0f-%.0fs"), Beat->WindowStart, Beat->WindowEnd)
				: FString(TEXT("Never found - no camera was on it")), X + 80.f * S, Y + 24.f * S, Muted, 0.6f);
		}
		else
		{
			Label(TEXT("[??] Something else happened somewhere..."), X + 40.f * S, Y, Muted, 0.8f);
		}
		Y += 50.f * S;
	}

	Label(TEXT("Press R or Enter to return to preparation (cameras and notebook are kept)"), CX, Y + 10.f * S, FLinearColor::White, 0.75f, true);
}
