// Director's Cut prototype 4 - canvas HUD.

#include "DCReplicaHUD.h"
#include "DCReplicaDirector.h"
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
	const FLinearColor RigOrange(1.f, 0.6f, 0.2f);
	const FLinearColor Spray(1.f, 0.3f, 0.8f);
}

ADCReplicaDirector* ADCReplicaHUD::FindDirector()
{
	if (!Director.IsValid())
	{
		for (TActorIterator<ADCReplicaDirector> It(GetWorld()); It; ++It)
		{
			Director = *It;
			break;
		}
	}
	return Director.Get();
}

void ADCReplicaHUD::Label(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale, bool bCenter)
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

bool ADCReplicaHUD::ProjectVisible(const FVector& World, FVector2D& OutScreen) const
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

FLinearColor ADCReplicaHUD::ScoreColor(float Score01)
{
	return Score01 >= 0.75f ? Good : Score01 >= 0.5f ? Gold : Bad;
}

void ADCReplicaHUD::DrawHUD()
{
	Super::DrawHUD();

	ADCReplicaDirector* D = FindDirector();
	if (!D || !Canvas)
	{
		return;
	}
	S = FMath::Max(Canvas->ClipY / 1080.f, 0.6f);
	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.5f;

	if (D->IsShowingFilm())
	{
		DrawFilmOverlay(D);
		return;
	}

	if (D->GetPhase() == EDCPhase::Preparation)
	{
		FVector2D P;
		if (D->GetClapperboard() && ProjectVisible(D->GetClapperboard()->GetActorLocation() + FVector(0.f, 0.f, 170.f), P))
		{
			Label(TEXT("CLAPPERBOARD - E to call Action!"), P.X, P.Y, FLinearColor::White, 0.7f, true);
		}
	}

	DrawHeader(D);
	if (D->bShowNotebook && D->GetPhase() != EDCPhase::Results)
	{
		DrawStoryboard(D);
	}
	DrawRect(FLinearColor::White, CX - 2.f, CY - 2.f, 4.f, 4.f);

	if (D->GetPhase() == EDCPhase::Filming)
	{
		DrawFilming(D);
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

void ADCReplicaHUD::DrawFilmOverlay(ADCReplicaDirector* D)
{
	// Letterbox bars sell "this is the film".
	const float Bar = Canvas->ClipY * 0.1f;
	DrawRect(FLinearColor::Black, 0.f, 0.f, Canvas->ClipX, Bar);
	DrawRect(FLinearColor::Black, 0.f, Canvas->ClipY - Bar, Canvas->ClipX, Bar);

	const bool bTake = D->GetReplay() == EDCReplay::Take;
	Label(bTake ? TEXT("YOUR TAKE") : TEXT("REFERENCE FILM"), 24.f * S, Bar * 0.3f, bTake ? Cyan : Gold, 1.1f);
	Label(FString::Printf(TEXT("%04.1f / %.1f"), D->GetSceneTime(), D->TakeDuration), Canvas->ClipX * 0.5f, Bar * 0.25f, FLinearColor::White, 1.2f, true);

	const int32 Index = D->GetShotIndexAt(D->GetSceneTime());
	if (D->GetShots().IsValidIndex(Index))
	{
		const FDCRefShot& Shot = D->GetShots()[Index];
		FString Title = FString::Printf(TEXT("SHOT %d/%d  -  %s"), Index + 1, D->GetShots().Num(), *Shot.Title);
		if (bTake)
		{
			Title += FString::Printf(TEXT("   (%.0f%% match)"), Shot.GetScore() * 100.f);
		}
		Label(Title, Canvas->ClipX * 0.5f, Canvas->ClipY - Bar * 0.85f, FLinearColor::White, 1.f, true);
	}
	DrawShotStrip(D, Canvas->ClipY - Bar * 0.35f);

	const FString Hint = D->GetPhase() == EDCPhase::Watching
		? TEXT("Study the shots.   R: watch again   Enter: start preparing")
		: TEXT("R or Enter: back to the results");
	Label(Hint, Canvas->ClipX * 0.5f, Canvas->ClipY - Bar * 0.3f + 14.f * S, Muted, 0.6f, true);
}

void ADCReplicaHUD::DrawShotStrip(ADCReplicaDirector* D, float Y)
{
	// Timeline split into the reference shots, with a playhead.
	const float X0 = Canvas->ClipX * 0.2f;
	const float W = Canvas->ClipX * 0.6f;
	const float H = 10.f * S;
	const float Duration = D->TakeDuration;
	const int32 Current = D->GetShotIndexAt(D->GetSceneTime());
	const TArray<FDCRefShot>& Shots = D->GetShots();
	for (int32 i = 0; i < Shots.Num(); ++i)
	{
		const float SX = X0 + W * Shots[i].Start / Duration;
		const float SW = W * (Shots[i].End - Shots[i].Start) / Duration - 3.f;
		DrawRect(i == Current ? Gold : FLinearColor(0.35f, 0.35f, 0.4f), SX, Y, SW, H);
	}
	const float PX = X0 + W * D->GetSceneTime() / Duration;
	DrawRect(FLinearColor::White, PX - 1.5f, Y - 5.f, 3.f, H + 10.f);
}

void ADCReplicaHUD::DrawHeader(ADCReplicaDirector* D)
{
	DrawRect(Panel, 0.f, 0.f, Canvas->ClipX, 70.f * S);

	FString PhaseText;
	FLinearColor PhaseColor = FLinearColor::White;
	switch (D->GetPhase())
	{
	case EDCPhase::Preparation: PhaseText = TEXT("PREPARATION"); PhaseColor = Cyan; break;
	case EDCPhase::Countdown:   PhaseText = TEXT("STAND BY");    PhaseColor = Gold; break;
	case EDCPhase::Filming:     PhaseText = TEXT("ROLLING");     PhaseColor = Bad;  break;
	case EDCPhase::Results:     PhaseText = TEXT("EVALUATION");  PhaseColor = Gold; break;
	default: break;
	}
	Label(PhaseText, 24.f * S, 18.f * S, PhaseColor, 1.2f);
	Label(FString::Printf(TEXT("%04.1f / %.1f"), D->GetSceneTime(), D->TakeDuration), Canvas->ClipX * 0.5f, 10.f * S, FLinearColor::White, 1.6f, true);

	if (D->GetPhase() == EDCPhase::Preparation)
	{
		const EDCRigTool Tool = D->GetTool();
		const float X = Canvas->ClipX - 720.f * S;
		Label(FString::Printf(TEXT("[1] RAIL %d/%d (%d/%d pts)"), D->GetRailsUsed(), D->MaxRails, D->GetActiveRailPoints(), D->MaxPointsPerRail),
			X, 14.f * S, Tool == EDCRigTool::Rail ? FLinearColor::White : RigOrange, 0.65f);
		Label(FString::Printf(TEXT("[2] CRANE %d/%d @%.0fcm"), D->GetTripodCount(), D->MaxTripods, D->GetPendingTripodHeight()),
			X + 250.f * S, 14.f * S, Tool == EDCRigTool::Tripod ? FLinearColor::White : RigOrange, 0.65f);
		Label(FString::Printf(TEXT("[3] SPRAY %d/%d"), D->GetSprayCount(), D->MaxSprayMarks),
			X + 500.f * S, 14.f * S, Tool == EDCRigTool::Spray ? FLinearColor::White : Spray, 0.65f);
		const float Under = Tool == EDCRigTool::Rail ? 0.f : Tool == EDCRigTool::Tripod ? 250.f : 500.f;
		DrawRect(FLinearColor::White, X + Under * S, 42.f * S, 180.f * S, 3.f);
		Label(D->GetBrief(), Canvas->ClipX * 0.5f, 100.f * S, FLinearColor::White, 0.75f, true);
	}
}

void ADCReplicaHUD::DrawStoryboard(ADCReplicaDirector* D)
{
	const TArray<FDCRefShot>& Shots = D->GetShots();
	const float X = Canvas->ClipX - 430.f * S;
	float Y = 130.f * S;
	const float RowH = 30.f * S;
	DrawRect(Panel, X - 12.f * S, Y - 10.f * S, 420.f * S, RowH * Shots.Num() + 48.f * S);
	Label(TEXT("STORYBOARD (Tab)   V rewatch"), X, Y, Muted, 0.7f);
	Y += 32.f * S;

	const int32 Current = D->GetPhase() == EDCPhase::Filming ? D->GetShotIndexAt(D->GetSceneTime()) : INDEX_NONE;
	for (int32 i = 0; i < Shots.Num(); ++i)
	{
		Label(FString::Printf(TEXT("%d. %s  %.0f-%.0fs"), i + 1, *Shots[i].Title, Shots[i].Start, Shots[i].End), X, Y,
			i == Current ? Gold : FLinearColor::White, 0.65f);
		Y += RowH;
	}
}

void ADCReplicaHUD::DrawFilming(ADCReplicaDirector* D)
{
	const TArray<FDCRefShot>& Shots = D->GetShots();
	const int32 Index = D->GetShotIndexAt(D->GetSceneTime());
	if (!Shots.IsValidIndex(Index))
	{
		return;
	}
	const FDCRefShot& Shot = Shots[Index];
	const float CX = Canvas->ClipX * 0.5f;
	float Y = Canvas->ClipY * 0.68f;
	Label(FString::Printf(TEXT("SHOT %d/%d  -  %s"), Index + 1, Shots.Num(), *Shot.Title), CX, Y, Gold, 0.9f, true);
	Y += 32.f * S;
	if (Shots.IsValidIndex(Index + 1))
	{
		const float ToCut = Shots[Index + 1].Start - D->GetSceneTime();
		Label(FString::Printf(TEXT("Next: %s in %.1fs"), *Shots[Index + 1].Title, ToCut), CX, Y, ToCut < 2.f ? Bad : Muted, 0.7f, true);
	}
	else
	{
		Label(FString::Printf(TEXT("Final shot - cut in %.1fs"), D->TakeDuration - D->GetSceneTime()), CX, Y, Muted, 0.7f, true);
	}

	if (D->bShowLiveMatch)
	{
		const float BarW = 300.f * S;
		const float BarH = 12.f * S;
		const float X = CX - BarW * 0.5f;
		Y += 34.f * S;
		DrawRect(Panel, X, Y, BarW, BarH);
		DrawRect(ScoreColor(D->GetLiveMatch()), X, Y, BarW * FMath::Clamp(D->GetLiveMatch(), 0.f, 1.f), BarH);
		Label(FString::Printf(TEXT("match %.0f%%"), D->GetLiveMatch() * 100.f), X + BarW + 12.f * S, Y - 6.f * S, Muted, 0.6f);
	}
}

void ADCReplicaHUD::DrawHelp(ADCReplicaDirector* D)
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
		Lines.Add(TEXT("1 rail   2 crane   3 spray mark   LMB place   RMB remove   F finish rail   Wheel crane height   C clear all"));
		Lines.Add(TEXT("V rewatch reference   E mount rig / clapperboard   Enter: Action!   Tab storyboard   Shift sprint"));
		break;
	case EDCPhase::Filming:
		Lines.Add(TEXT("The camera is rolling - match the reference   E mount/leave rig   W/S travel rail   Wheel crane height   R restart"));
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

void ADCReplicaHUD::DrawResults(ADCReplicaDirector* D)
{
	const TArray<FDCRefShot>& Shots = D->GetShots();
	const float W = 1100.f * S;
	const float X = Canvas->ClipX * 0.5f - W * 0.5f;
	float Y = Canvas->ClipY * 0.14f;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.85f), X, Y, W, (230.f + Shots.Num() * 62.f) * S);

	const float CX = Canvas->ClipX * 0.5f;
	const float Overall = D->GetOverallScore();
	Y += 16.f * S;
	Label(FString::Printf(TEXT("GRADE %s"), *ADCReplicaDirector::GradeFor(Overall)), CX, Y, ScoreColor(Overall), 2.f, true);
	Y += 70.f * S;
	Label(FString::Printf(TEXT("%.0f%% similar to the reference   (session best %.0f%%)"), Overall * 100.f, D->GetBestOverall() * 100.f),
		CX, Y, FLinearColor::White, 0.9f, true);
	Y += 50.f * S;

	for (int32 i = 0; i < Shots.Num(); ++i)
	{
		const FDCRefShot& Shot = Shots[i];
		Label(FString::Printf(TEXT("%d. %s"), i + 1, *Shot.Title), X + 40.f * S, Y, FLinearColor::White, 0.8f);
		Label(FString::Printf(TEXT("%.0f%%"), Shot.GetScore() * 100.f), X + W - 120.f * S, Y, ScoreColor(Shot.GetScore()), 1.f);
		Label(FString::Printf(TEXT("framing %.0f%%   position off %.1f m   angle off %.0f deg   -  %s"),
			Shot.GetFraming() * 100.f, Shot.GetPositionError() / 100.f, Shot.GetAngleError(), *D->GetShotTip(Shot)),
			X + 70.f * S, Y + 28.f * S, Muted, 0.6f);
		Y += 62.f * S;
	}

	Y += 10.f * S;
	Label(TEXT("V watch the reference   T watch your take   R / Enter back to preparation (rigs and marks are kept)"), CX, Y, FLinearColor::White, 0.72f, true);
}
