// Director's Cut prototype 2 - canvas HUD.

#include "DCFreeHUD.h"
#include "DCBeat.h"
#include "DCFreeDirector.h"
#include "DCSceneMarks.h"
#include "DCTripod.h"
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
	const FLinearColor RigOrange(1.f, 0.6f, 0.2f);
	constexpr float GaugeLinger = 1.5f;
}

ADCFreeDirector* ADCFreeHUD::FindDirector()
{
	if (!Director.IsValid())
	{
		for (TActorIterator<ADCFreeDirector> It(GetWorld()); It; ++It)
		{
			Director = *It;
			break;
		}
	}
	return Director.Get();
}

void ADCFreeHUD::Label(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale, bool bCenter)
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

bool ADCFreeHUD::ProjectVisible(const FVector& World, FVector2D& OutScreen) const
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

FLinearColor ADCFreeHUD::BeatColor(const FDCBeatState& State)
{
	switch (State.Status)
	{
	case EDCShotStatus::Captured: return Good;
	case EDCShotStatus::Missed:   return Muted;
	default:                      return State.Beat->bMandatory ? Gold : Cyan;
	}
}

void ADCFreeHUD::DrawHUD()
{
	Super::DrawHUD();

	ADCFreeDirector* D = FindDirector();
	if (!D || !Canvas)
	{
		return;
	}
	S = FMath::Max(Canvas->ClipY / 1080.f, 0.6f);
	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.5f;

	if (D->GetPhase() == EDCPhase::Preparation)
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

	if (D->GetPhase() == EDCPhase::Filming)
	{
		DrawFraming(D);
		DrawGauges(D);
	}
	else
	{
		DrawRect(FLinearColor::White, CX - 2.f, CY - 2.f, 4.f, 4.f);
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
		DrawHelp(D);
	}

	if (UGameplayStatics::IsGamePaused(this))
	{
		DrawRect(Panel, 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
		Label(TEXT("PAUSED - press P to resume"), CX, CY, FLinearColor::White, 1.6f, true);
	}
}

void ADCFreeHUD::DrawHeader(ADCFreeDirector* D)
{
	DrawRect(Panel, 0.f, 0.f, Canvas->ClipX, 70.f * S);

	FString PhaseText;
	FLinearColor PhaseColor = FLinearColor::White;
	switch (D->GetPhase())
	{
	case EDCPhase::Preparation: PhaseText = TEXT("PREPARATION"); PhaseColor = Cyan; break;
	case EDCPhase::Countdown:   PhaseText = TEXT("STAND BY");    PhaseColor = Gold; break;
	case EDCPhase::Filming:     PhaseText = TEXT("ROLLING");     PhaseColor = Bad;  break;
	case EDCPhase::Results:     PhaseText = TEXT("CUT");         PhaseColor = Gold; break;
	}
	Label(PhaseText, 24.f * S, 18.f * S, PhaseColor, 1.2f);
	Label(FString::Printf(TEXT("%04.1f / %.1f"), D->GetSceneTime(), D->TakeDuration), Canvas->ClipX * 0.5f, 10.f * S, FLinearColor::White, 1.6f, true);

	if (D->GetPhase() == EDCPhase::Filming)
	{
		const bool bBlink = FMath::Fmod(GetWorld()->GetTimeSeconds(), 1.f) < 0.6f;
		Label(bBlink ? TEXT("* REC") : TEXT("  REC"), Canvas->ClipX - 140.f * S, 18.f * S, Bad, 1.2f);
	}
	else if (D->GetPhase() == EDCPhase::Preparation)
	{
		const bool bRail = D->GetTool() == EDCRigTool::Rail;
		const FString Rigs = FString::Printf(TEXT("[1] RAIL %d/%d (%d/%d pts)    [2] TRIPOD %d/%d  @%.0fcm"),
			D->GetRailsUsed(), D->MaxRails, D->GetActiveRailPoints(), D->MaxPointsPerRail,
			D->GetTripodCount(), D->MaxTripods, D->GetPendingTripodHeight());
		Label(Rigs, Canvas->ClipX - 640.f * S, 22.f * S, RigOrange, 0.75f);
		Label(bRail ? TEXT("^ rail tool") : TEXT("                                     ^ tripod tool"), Canvas->ClipX - 640.f * S, 46.f * S, FLinearColor::White, 0.55f);
	}

	if (D->GetPhase() == EDCPhase::Preparation)
	{
		Label(D->GetBrief(), Canvas->ClipX * 0.5f, 100.f * S, FLinearColor::White, 0.75f, true);
	}
}

void ADCFreeHUD::DrawTimeline(ADCFreeDirector* D)
{
	const float X0 = Canvas->ClipX * 0.2f;
	const float W = Canvas->ClipX * 0.6f;
	const float Y = 76.f * S;
	const float H = 12.f * S;
	const float Duration = D->TakeDuration;

	DrawRect(Panel, X0 - 4.f, Y - 4.f, W + 8.f, H + 8.f);
	// Only beats the player has discovered appear on the timeline.
	for (const FDCBeatState& State : D->GetBeats())
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

void ADCFreeHUD::DrawNotebook(ADCFreeDirector* D)
{
	const TArray<FDCBeatState>& Beats = D->GetBeats();
	const float X = Canvas->ClipX - 430.f * S;
	float Y = 130.f * S;
	const float RowH = 28.f * S;
	DrawRect(Panel, X - 12.f * S, Y - 10.f * S, 420.f * S, RowH * Beats.Num() + 48.f * S);
	Label(FString::Printf(TEXT("NOTEBOOK (Tab)   discovered %d/%d"), D->GetSpottedCount(), Beats.Num()), X, Y, Muted, 0.7f);
	Y += 32.f * S;

	for (const FDCBeatState& State : Beats)
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

void ADCFreeHUD::DrawFraming(ADCFreeDirector* D)
{
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const FLinearColor BracketColor = D->IsAnyBeatRolling() ? Good : FLinearColor(1.f, 1.f, 1.f, 0.45f);

	// 70% safe-frame brackets: subjects must sit inside these to count.
	const float L = W * 0.15f, R = W * 0.85f, T = H * 0.15f, B = H * 0.85f;
	const float Arm = 40.f * S;
	const float Th = 3.f;
	DrawLine(L, T, L + Arm, T, BracketColor, Th); DrawLine(L, T, L, T + Arm, BracketColor, Th);
	DrawLine(R, T, R - Arm, T, BracketColor, Th); DrawLine(R, T, R, T + Arm, BracketColor, Th);
	DrawLine(L, B, L + Arm, B, BracketColor, Th); DrawLine(L, B, L, B - Arm, BracketColor, Th);
	DrawLine(R, B, R - Arm, B, BracketColor, Th); DrawLine(R, B, R, B - Arm, BracketColor, Th);
	DrawRect(BracketColor, W * 0.5f - 2.f, H * 0.5f - 2.f, 4.f, 4.f);
}

void ADCFreeHUD::DrawGauges(ADCFreeDirector* D)
{
	// A gauge appears only while a beat is actually being filmed (or just finished).
	const float BarW = 340.f * S;
	const float BarH = 12.f * S;
	float Y = Canvas->ClipY * 0.62f;
	const float X = Canvas->ClipX * 0.5f - BarW * 0.5f;

	for (const FDCBeatState& State : D->GetBeats())
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

void ADCFreeHUD::DrawHelp(ADCFreeDirector* D)
{
	TArray<FString> Lines;
	const FString Hint = D->GetInteractHint();
	if (!Hint.IsEmpty())
	{
		Lines.Add(Hint);
	}
	switch (D->GetPhase())
	{
	case EDCPhase::Preparation:
		Lines.Add(TEXT("1 rail tool   2 tripod tool   LMB place   RMB remove   F finish rail   C clear rigs   Wheel tripod height"));
		Lines.Add(TEXT("E mount rig / clapperboard   Enter: Action!   Tab notebook   Shift sprint   P pause"));
		break;
	case EDCPhase::Filming:
		Lines.Add(TEXT("The camera is always rolling   E mount/leave rig   W/S travel rail   Wheel tripod height   R restart   P pause"));
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

void ADCFreeHUD::DrawResults(ADCFreeDirector* D)
{
	const TArray<FDCBeatState>& Beats = D->GetBeats();
	const float W = 1000.f * S;
	const float X = Canvas->ClipX * 0.5f - W * 0.5f;
	float Y = Canvas->ClipY * 0.14f;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.85f), X, Y, W, (190.f + Beats.Num() * 50.f) * S);

	const float CX = Canvas->ClipX * 0.5f;
	Y += 20.f * S;
	Label(D->GetCutName(), CX, Y, D->WasApproved() ? Gold : Bad, 1.8f, true);
	Y += 64.f * S;
	Label(D->WasApproved()
		? FString::Printf(TEXT("Approved  -  Score %d   (session best %d)   discovered %d/%d"), D->GetScore(), D->GetBestScore(), D->GetSpottedCount(), Beats.Num())
		: FString::Printf(TEXT("The studio's required footage is missing - no approved score   (discovered %d/%d)"), D->GetSpottedCount(), Beats.Num()),
		CX, Y, FLinearColor::White, 0.85f, true);
	Y += 46.f * S;

	for (const FDCBeatState& State : Beats)
	{
		const ADCBeat* Beat = State.Beat;
		const bool bGot = State.Status == EDCShotStatus::Captured;
		if (bGot)
		{
			Label(FString::Printf(TEXT("[OK] %s  (+%d)"), *Beat->DisplayName, Beat->Points), X + 40.f * S, Y, Good, 0.8f);
			Label(Beat->Caption, X + 80.f * S, Y + 24.f * S, Muted, 0.6f);
		}
		else if (State.bSpotted || Beat->bMandatory)
		{
			Label(FString::Printf(TEXT("[--] %s"), *Beat->DisplayName), X + 40.f * S, Y, Beat->bMandatory ? Bad : Muted, 0.8f);
			Label(State.bSpotted ? FString::Printf(TEXT("Glimpsed but not held long enough - it plays %.0f-%.0fs"), Beat->WindowStart, Beat->WindowEnd)
				: FString(TEXT("Never found - explore the set")), X + 80.f * S, Y + 24.f * S, Muted, 0.6f);
		}
		else
		{
			Label(TEXT("[??] Something else happened somewhere..."), X + 40.f * S, Y, Muted, 0.8f);
		}
		Y += 50.f * S;
	}

	Label(TEXT("Press R or Enter to return to preparation (rigs and notebook are kept)"), CX, Y + 10.f * S, FLinearColor::White, 0.75f, true);
}
