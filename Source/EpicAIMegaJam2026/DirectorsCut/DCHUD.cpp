// Director's Cut prototype - canvas HUD.

#include "DCHUD.h"
#include "DCCharacter.h"
#include "DCRail.h"
#include "DCSceneMarks.h"
#include "DCTakeDirector.h"
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
}

ADCTakeDirector* ADCHUD::FindDirector()
{
	if (!Director.IsValid())
	{
		for (TActorIterator<ADCTakeDirector> It(GetWorld()); It; ++It)
		{
			Director = *It;
			break;
		}
	}
	return Director.Get();
}

void ADCHUD::Label(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale, bool bCenter)
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

bool ADCHUD::ProjectVisible(const FVector& World, FVector2D& OutScreen) const
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

FLinearColor ADCHUD::ShotColor(const FDCShot& Shot)
{
	switch (Shot.Status)
	{
	case EDCShotStatus::Captured: return Good;
	case EDCShotStatus::Missed:   return Muted;
	default:                      return Shot.bMandatory ? Gold : Cyan;
	}
}

FString ADCHUD::ShotWindow(const FDCShot& Shot)
{
	return FString::Printf(TEXT("%.0f-%.0fs"), Shot.WindowStart, Shot.WindowEnd);
}

void ADCHUD::DrawHUD()
{
	Super::DrawHUD();

	ADCTakeDirector* D = FindDirector();
	if (!D || !Canvas)
	{
		return;
	}
	ADCCharacter* Player = Cast<ADCCharacter>(GetOwningPawn());
	S = FMath::Max(Canvas->ClipY / 1080.f, 0.6f);

	DrawWorldLabels(D);
	DrawHeader(D, Player);
	DrawTimeline(D);
	if (D->bShowShotList)
	{
		DrawShotList(D);
	}

	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.5f;

	if (D->GetPhase() == EDCPhase::Filming)
	{
		DrawFraming(D);
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
		float Y = 120.f * S;
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
		DrawHelp(D, Player);
	}

	if (UGameplayStatics::IsGamePaused(this))
	{
		DrawRect(Panel, 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
		Label(TEXT("PAUSED - press P to resume"), CX, CY, FLinearColor::White, 1.6f, true);
	}
}

void ADCHUD::DrawHeader(ADCTakeDirector* D, ADCCharacter* Player)
{
	DrawRect(Panel, 0.f, 0.f, Canvas->ClipX, 70.f * S);

	FString PhaseText;
	FLinearColor PhaseColor = FLinearColor::White;
	switch (D->GetPhase())
	{
	case EDCPhase::Preparation: PhaseText = TEXT("PREPARATION"); PhaseColor = Cyan; break;
	case EDCPhase::Countdown:   PhaseText = TEXT("STAND BY");    PhaseColor = Gold; break;
	case EDCPhase::Filming:     PhaseText = TEXT("FILMING");     PhaseColor = Bad;  break;
	case EDCPhase::Results:     PhaseText = TEXT("CUT");         PhaseColor = Gold; break;
	}
	Label(PhaseText, 24.f * S, 18.f * S, PhaseColor, 1.2f);

	Label(FString::Printf(TEXT("%04.1f / 45.0"), D->GetSceneTime()), Canvas->ClipX * 0.5f, 10.f * S, FLinearColor::White, 1.6f, true);

	if (D->GetPhase() == EDCPhase::Filming && Player && Player->IsRecording())
	{
		const bool bBlink = FMath::Fmod(GetWorld()->GetTimeSeconds(), 1.f) < 0.6f;
		Label(bBlink ? TEXT("* REC") : TEXT("  REC"), Canvas->ClipX - 140.f * S, 18.f * S, Bad, 1.2f);
	}
	else if (D->GetPhase() == EDCPhase::Preparation)
	{
		const ADCRail* Rail = D->GetRail();
		Label(FString::Printf(TEXT("Rail %d/%d  %.1f m"), Rail->NumPoints(), ADCRail::MaxPoints, Rail->GetLength() / 100.f),
			Canvas->ClipX - 260.f * S, 18.f * S, FLinearColor(1.f, 0.6f, 0.2f), 1.f);
	}
}

void ADCHUD::DrawTimeline(ADCTakeDirector* D)
{
	const float X0 = Canvas->ClipX * 0.2f;
	const float W = Canvas->ClipX * 0.6f;
	const float Y = 78.f * S;
	const float H = 14.f * S;
	const float Duration = D->TakeDuration;

	DrawRect(Panel, X0 - 4.f, Y - 4.f, W + 8.f, H + 8.f);
	for (const FDCShot& Shot : D->GetShots())
	{
		const float SX = X0 + W * Shot.WindowStart / Duration;
		const float SW = W * (Shot.WindowEnd - Shot.WindowStart) / Duration;
		DrawRect(ShotColor(Shot) * FLinearColor(1.f, 1.f, 1.f, 0.85f), SX, Y, SW, H);
		Label(Shot.Id, SX + SW * 0.5f, Y + H + 2.f, ShotColor(Shot), 0.6f, true);
	}
	const float PX = X0 + W * D->GetSceneTime() / Duration;
	DrawRect(FLinearColor::White, PX - 1.5f, Y - 6.f, 3.f, H + 12.f);
}

void ADCHUD::DrawShotList(ADCTakeDirector* D)
{
	const float X = Canvas->ClipX - 420.f * S;
	float Y = 120.f * S;
	const float RowH = 52.f * S;
	DrawRect(Panel, X - 12.f * S, Y - 10.f * S, 410.f * S, RowH * D->GetShots().Num() + 44.f * S);
	Label(TEXT("SHOT LIST (Tab)"), X, Y, Muted, 0.75f);
	Y += 30.f * S;

	for (const FDCShot& Shot : D->GetShots())
	{
		const TCHAR* Status = Shot.Status == EDCShotStatus::Captured ? TEXT("[OK]") : Shot.Status == EDCShotStatus::Missed ? TEXT("[--]") : TEXT("[  ]");
		Label(FString::Printf(TEXT("%s %s %s"), Status, *Shot.Id, *Shot.Name), X, Y, ShotColor(Shot), 0.8f);
		Label(FString::Printf(TEXT("%s  %s  %d pts  hold %.1fs"), Shot.bMandatory ? TEXT("REQUIRED") : TEXT("bonus"), *ShotWindow(Shot), Shot.Points, Shot.RequiredHold),
			X + 40.f * S, Y + 22.f * S, Muted, 0.6f);
		Y += RowH;
	}
}

void ADCHUD::DrawWorldLabels(ADCTakeDirector* D)
{
	FVector2D P;
	for (const FDCShot& Shot : D->GetShots())
	{
		if (ProjectVisible(Shot.MarkLocation + FVector(0.f, 0.f, 40.f), P))
		{
			Label(FString::Printf(TEXT("%s %s"), *Shot.Id, *Shot.Name), P.X, P.Y, ShotColor(Shot), 0.75f, true);
			Label(FString::Printf(TEXT("%s%s"), *ShotWindow(Shot), Shot.bMandatory ? TEXT("  REQUIRED") : TEXT("")), P.X, P.Y + 22.f * S, Muted, 0.6f, true);
		}
	}
	if (D->GetPhase() == EDCPhase::Preparation)
	{
		if (D->GetClapperboard() && ProjectVisible(D->GetClapperboard()->GetActorLocation() + FVector(0.f, 0.f, 170.f), P))
		{
			Label(TEXT("CLAPPERBOARD - E to call Action!"), P.X, P.Y, FLinearColor::White, 0.7f, true);
		}
		if (ProjectVisible(D->GetStartLocation() - FVector(0.f, 0.f, 80.f), P))
		{
			Label(TEXT("START MARK"), P.X, P.Y, FLinearColor::White, 0.6f, true);
		}
		const TArray<FVector>& Points = D->GetRail()->GetPoints();
		for (int32 i = 0; i < Points.Num(); ++i)
		{
			if (ProjectVisible(Points[i] + FVector(0.f, 0.f, 30.f), P))
			{
				Label(FString::Printf(TEXT("%d"), i + 1), P.X, P.Y, FLinearColor(1.f, 0.6f, 0.2f), 0.6f, true);
			}
		}
	}
}

void ADCHUD::DrawFraming(ADCTakeDirector* D)
{
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const int32 Focus = D->GetFocusShotIndex();
	const FDCShot* Shot = D->GetShots().IsValidIndex(Focus) ? &D->GetShots()[Focus] : nullptr;
	const bool bRolling = Shot && Shot->LastFail == EDCFail::None;
	const FLinearColor BracketColor = bRolling ? Good : FLinearColor(1.f, 1.f, 1.f, 0.6f);

	// 70% safe-frame brackets: every framing anchor must sit inside these.
	const float L = W * 0.15f, R = W * 0.85f, T = H * 0.15f, B = H * 0.85f;
	const float Arm = 40.f * S;
	const float Th = 3.f;
	DrawLine(L, T, L + Arm, T, BracketColor, Th); DrawLine(L, T, L, T + Arm, BracketColor, Th);
	DrawLine(R, T, R - Arm, T, BracketColor, Th); DrawLine(R, T, R, T + Arm, BracketColor, Th);
	DrawLine(L, B, L + Arm, B, BracketColor, Th); DrawLine(L, B, L, B - Arm, BracketColor, Th);
	DrawLine(R, B, R - Arm, B, BracketColor, Th); DrawLine(R, B, R, B - Arm, BracketColor, Th);
	DrawRect(BracketColor, W * 0.5f - 2.f, H * 0.5f - 2.f, 4.f, 4.f);

	if (!Shot)
	{
		Label(TEXT("No more shots this take"), W * 0.5f, B + 10.f * S, Muted, 0.9f, true);
		return;
	}

	const FLinearColor Color = Shot->bMandatory ? Gold : Cyan;
	const bool bOpen = D->GetSceneTime() >= Shot->WindowStart;
	FString Title = FString::Printf(TEXT("%s %s  %s"), *Shot->Id, *Shot->Name, Shot->bMandatory ? TEXT("(REQUIRED)") : TEXT("(bonus)"));
	Label(Title, W * 0.5f, B + 10.f * S, Color, 0.9f, true);

	FString Status = bOpen
		? FString::Printf(TEXT("%s  -  closes in %.1fs"), *ADCTakeDirector::DescribeFail(Shot->LastFail), Shot->WindowEnd - D->GetSceneTime())
		: FString::Printf(TEXT("Opens in %.1fs - get to the %s mark"), Shot->WindowStart - D->GetSceneTime(), *Shot->Id);
	Label(Status, W * 0.5f, B + 40.f * S, bRolling ? Good : FLinearColor::White, 0.8f, true);

	// Hold-progress bar.
	const float BarW = 300.f * S;
	const float BarX = W * 0.5f - BarW * 0.5f;
	const float BarY = H * 0.5f + 30.f * S;
	DrawRect(Panel, BarX, BarY, BarW, 10.f * S);
	DrawRect(Good, BarX, BarY, BarW * FMath::Clamp(Shot->Hold / Shot->RequiredHold, 0.f, 1.f), 10.f * S);
}

void ADCHUD::DrawHelp(ADCTakeDirector* D, ADCCharacter* Player)
{
	TArray<FString> Lines;
	if (Player && Player->IsOnRail())
	{
		Lines.Add(FString::Printf(TEXT("ON DOLLY  %.0f%%  -  W/S push along rail, E dismount"), Player->GetRailProgress() * 100.f));
	}
	else if (D->GetPhase() == EDCPhase::Preparation && D->IsNearClapper())
	{
		Lines.Add(TEXT("E: hit the clapperboard and call ACTION!"));
	}
	else if (D->CanMountRail())
	{
		Lines.Add(TEXT("E: mount the dolly"));
	}

	switch (D->GetPhase())
	{
	case EDCPhase::Preparation:
		Lines.Add(TEXT("LMB place rail point   RMB remove point   C clear rail   E mount dolly (rehearse)"));
		Lines.Add(TEXT("Enter / clapperboard: start take   Tab shot list   P pause   Shift sprint"));
		break;
	case EDCPhase::Filming:
		Lines.Add(TEXT("Hold LMB to record   Shift sprint (no recording)   E mount/dismount dolly   R restart   P pause"));
		break;
	default:
		break;
	}

	float Y = Canvas->ClipY - (Lines.Num() * 30.f + 20.f) * S;
	for (const FString& Line : Lines)
	{
		Label(Line, Canvas->ClipX * 0.5f, Y, FLinearColor::White, 0.75f, true);
		Y += 30.f * S;
	}
}

void ADCHUD::DrawResults(ADCTakeDirector* D)
{
	const float W = 900.f * S;
	const float X = Canvas->ClipX * 0.5f - W * 0.5f;
	float Y = Canvas->ClipY * 0.25f;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.8f), X, Y, W, 520.f * S);

	const float CX = Canvas->ClipX * 0.5f;
	Y += 24.f * S;
	Label(D->GetCutName(), CX, Y, D->WasApproved() ? Gold : Bad, 2.f, true);
	Y += 70.f * S;
	Label(D->WasApproved()
		? FString::Printf(TEXT("Approved by the studio  -  Score %d   (session best %d)"), D->GetScore(), D->GetBestScore())
		: TEXT("The essential shot was missed - no approved score"), CX, Y, FLinearColor::White, 0.9f, true);
	Y += 56.f * S;

	for (const FDCShot& Shot : D->GetShots())
	{
		const bool bGot = Shot.Status == EDCShotStatus::Captured;
		Label(FString::Printf(TEXT("%s %s %s"), bGot ? TEXT("[OK]") : TEXT("[--]"), *Shot.Id, *Shot.Name), X + 40.f * S, Y, ShotColor(Shot), 0.9f);
		const FString Detail = bGot
			? FString::Printf(TEXT("%s  (+%d)"), *Shot.Caption, Shot.Points)
			: Shot.LastFail == EDCFail::WrongTime
				? FString(TEXT("Missed - the take ended before its window"))
				: FString::Printf(TEXT("Missed - last problem: %s"), *ADCTakeDirector::DescribeFail(Shot.LastFail));
		Label(Detail, X + 80.f * S, Y + 28.f * S, Muted, 0.7f);
		Y += 70.f * S;
	}

	Label(TEXT("Press R or Enter to return to preparation (your rail is kept)"), CX, Y + 10.f * S, FLinearColor::White, 0.8f, true);
}
