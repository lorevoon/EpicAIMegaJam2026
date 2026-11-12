// Director's Cut prototype 5 - HUD.

#include "DCRigLensHUD.h"
#include "DCRigCharacter.h"
#include "DCRigLensDirector.h"
#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FLinearColor Gold(1.f, 0.72f, 0.2f);
	const FLinearColor Good(0.35f, 1.f, 0.45f);
	const FLinearColor Bad(1.f, 0.35f, 0.3f);
	const FLinearColor Muted(0.7f, 0.74f, 0.78f);
	const FLinearColor Panel(0.f, 0.f, 0.f, 0.6f);
	const FLinearColor RigOrange(1.f, 0.6f, 0.2f);
	const FLinearColor SprayPink(1.f, 0.3f, 0.8f);
	const FLinearColor RecRed(0.95f, 0.1f, 0.08f);

	FString ShortLensName(const FDCLens& Lens)
	{
		return Lens.IsZoom() ? TEXT("ZOOM") : FString::Printf(TEXT("%.0f"), Lens.MinFocal);
	}
}

void ADCRigLensHUD::DrawHUD()
{
	Super::DrawHUD();

	ADCReplicaDirector* D = FindDirector();
	if (!D || !Canvas || UGameplayStatics::IsGamePaused(this))
	{
		return;
	}
	if (D->IsShowingFilm())
	{
		DrawSkipButton(D);
		return;
	}
	ADCRigCharacter* P = PlayerOwner ? Cast<ADCRigCharacter>(PlayerOwner->GetPawn()) : nullptr;
	if (!P)
	{
		return;
	}

	if (D->GetPhase() == EDCPhase::Filming)
	{
		DrawViewfinder(D);
		if (P->IsViewObstructed())
		{
			DrawLensChange(P);
		}
	}
	if (D->GetPhase() != EDCPhase::Results)
	{
		DrawCameraPanel(P);
	}
}

void ADCRigLensHUD::DrawSkipButton(ADCReplicaDirector* D)
{
	// Top-right, inside the upper letterbox bar (the lower bar holds the shot strip).
	const bool bWatching = D->GetPhase() == EDCPhase::Watching;
	const FString Text = bWatching ? TEXT("SKIP  >>   Space / Enter") : TEXT("BACK TO RESULTS   Space / Enter");
	const float W = 330.f * S;
	const float H = 44.f * S;
	const float X = Canvas->ClipX - W - 28.f * S;
	const float Y = Canvas->ClipY * 0.1f * 0.5f - H * 0.5f;
	DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.9f), X - 2.f, Y - 2.f, W + 4.f, H + 4.f);
	DrawRect(FLinearColor(0.05f, 0.05f, 0.06f, 1.f), X, Y, W, H);
	Label(Text, X + W * 0.5f, Y + 8.f * S, FLinearColor::White, 0.8f, true);
}

void ADCRigLensHUD::DrawViewfinder(ADCReplicaDirector* D)
{
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;

	// Rule-of-thirds guides.
	const FLinearColor Guide(1.f, 1.f, 1.f, 0.12f);
	for (int32 i = 1; i <= 2; ++i)
	{
		DrawRect(Guide, W * i / 3.f, 0.f, 1.f, H);
		DrawRect(Guide, 0.f, H * i / 3.f, W, 1.f);
	}

	// Frame-line corners.
	const float Inset = 0.04f;
	const float L = 60.f * S;
	const float T = 3.f;
	const float X0 = W * Inset, X1 = W * (1.f - Inset);
	const float Y0 = H * Inset + 70.f * S, Y1 = H * (1.f - Inset);
	const FLinearColor Frame(1.f, 1.f, 1.f, 0.7f);
	DrawRect(Frame, X0, Y0, L, T);      DrawRect(Frame, X0, Y0, T, L);
	DrawRect(Frame, X1 - L, Y0, L, T);  DrawRect(Frame, X1 - T, Y0, T, L);
	DrawRect(Frame, X0, Y1 - T, L, T);  DrawRect(Frame, X0, Y1 - L, T, L);
	DrawRect(Frame, X1 - L, Y1 - T, L, T); DrawRect(Frame, X1 - T, Y1 - L, T, L);

	// Blinking tally.
	if (FMath::Fmod(GetWorld()->GetTimeSeconds(), 1.f) < 0.6f)
	{
		DrawRect(RecRed, X0 + 16.f * S, Y0 + 18.f * S, 16.f * S, 16.f * S);
	}
	Label(TEXT("REC"), X0 + 42.f * S, Y0 + 12.f * S, RecRed, 0.8f);
}

void ADCRigLensHUD::DrawLensChange(ADCRigCharacter* P)
{
	DrawRect(FLinearColor::Black, 0.f, 70.f * S, Canvas->ClipX, Canvas->ClipY - 70.f * S);
	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.5f;
	Label(TEXT("LENS CHANGE"), CX, CY - 40.f * S, Gold, 1.6f, true);
	Label(FString::Printf(TEXT("Nothing usable is being recorded  -  %.1f s"), P->GetLensChangeRemaining()), CX, CY + 20.f * S, Muted, 0.8f, true);
}

void ADCRigLensHUD::DrawMeter(float X, float Y, float W, float Value01, const FLinearColor& Color, bool bSigned)
{
	const float H = 8.f * S;
	DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.15f), X, Y, W, H);
	if (bSigned)
	{
		const float Mid = X + W * 0.5f;
		const float Fill = W * 0.5f * FMath::Clamp(FMath::Abs(Value01), 0.f, 1.f);
		DrawRect(Color, Value01 >= 0.f ? Mid : Mid - Fill, Y, Fill, H);
		DrawRect(FLinearColor::White, Mid - 1.f, Y - 3.f, 2.f, H + 6.f);
	}
	else
	{
		DrawRect(Color, X, Y, W * FMath::Clamp(Value01, 0.f, 1.f), H);
	}
}

void ADCRigLensHUD::DrawCameraPanel(ADCRigCharacter* P)
{
	const float X = 24.f * S;
	float Y = Canvas->ClipY * 0.58f;
	const float PanelW = 400.f * S;
	const bool bDolly = P->GetMountedRail() != nullptr;
	DrawRect(Panel, X - 12.f * S, Y - 10.f * S, PanelW, (bDolly ? 228.f : 196.f) * S);

	Label(P->GetRigName(), X, Y, Gold, 0.85f);
	Y += 34.f * S;

	const FDCLens& Lens = P->GetLens();
	const FString LensText = Lens.IsZoom()
		? FString::Printf(TEXT("%s @ %.0fmm   f/%.1f"), *Lens.Name, P->GetFocalLength(), Lens.FStop)
		: FString::Printf(TEXT("%s   f/%.1f"), *Lens.Name, Lens.FStop);
	Label(LensText, X, Y, FLinearColor::White, 0.7f);
	Y += 28.f * S;
	Label(FString::Printf(TEXT("focus %.1f m    view %.0f deg"), P->GetFocusDistance() / 100.f, P->GetCameraFOV()), X, Y, Muted, 0.62f);
	Y += 30.f * S;

	// Lens kit, current one highlighted.
	float LX = X;
	for (int32 i = 0; i < P->GetLensCount(); ++i)
	{
		const bool bCurrent = i == P->GetLensIndex();
		const FString Text = ShortLensName(P->GetLensAt(i));
		Label(Text, LX, Y, bCurrent ? FLinearColor::White : FLinearColor(0.45f, 0.47f, 0.5f), bCurrent ? 0.75f : 0.62f);
		if (bCurrent)
		{
			DrawRect(FLinearColor::White, LX, Y + 26.f * S, (Lens.IsZoom() ? 54.f : 26.f) * S, 2.f);
		}
		LX += (P->GetLensAt(i).IsZoom() ? 70.f : 48.f) * S;
	}
	Label(TEXT("Q/Z"), X + PanelW - 70.f * S, Y, Muted, 0.6f);
	Y += 38.f * S;

	// Steadiness: how much the image is shaking right now.
	const float Steady = 1.f - P->GetShake01();
	Label(TEXT("steady"), X, Y - 8.f * S, Muted, 0.6f);
	DrawMeter(X + 90.f * S, Y, 260.f * S, Steady, Steady > 0.75f ? Good : Steady > 0.45f ? Gold : Bad);
	Y += 30.f * S;

	if (bDolly)
	{
		Label(TEXT("dolly"), X, Y - 8.f * S, Muted, 0.6f);
		DrawMeter(X + 90.f * S, Y, 260.f * S, P->GetDollySpeed01(), RigOrange, true);
	}
}

void ADCRigLensHUD::DrawToolStrip(ADCReplicaDirector* D)
{
	const ADCRigLensDirector* R = Cast<ADCRigLensDirector>(D);
	const EDCRigTool Tool = D->GetTool();
	const float Step = 225.f * S;
	const float X = Canvas->ClipX - 4.f * Step - 20.f * S;
	const float Y = 14.f * S;

	struct FEntry { EDCRigTool Tool; FString Text; FLinearColor Color; };
	const FEntry Entries[] = {
		{ EDCRigTool::Rail, FString::Printf(TEXT("[1] TRACK %d/%d (%d pts)"), D->GetRailsUsed(), D->MaxRails, D->GetActiveRailPoints()), RigOrange },
		{ EDCRigTool::Tripod, FString::Printf(TEXT("[2] TRIPOD %d/%d @%.0f"), D->GetTripodCount(), D->MaxTripods, D->GetPendingTripodHeight()), RigOrange },
		{ EDCRigTool::Spray, FString::Printf(TEXT("[3] SPRAY %d/%d"), D->GetSprayCount(), D->MaxSprayMarks), SprayPink },
		{ EDCRigTool::Jib, FString::Printf(TEXT("[4] JIB %d/%d"), R ? R->GetJibCount() : 0, R ? R->MaxJibs : 0), RigOrange },
	};
	for (int32 i = 0; i < UE_ARRAY_COUNT(Entries); ++i)
	{
		const bool bSelected = Entries[i].Tool == Tool;
		Label(Entries[i].Text, X + i * Step, Y, bSelected ? FLinearColor::White : Entries[i].Color, 0.62f);
		if (bSelected)
		{
			DrawRect(FLinearColor::White, X + i * Step, 42.f * S, 190.f * S, 3.f);
		}
	}
}

void ADCRigLensHUD::DrawHelp(ADCReplicaDirector* D)
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
		Lines.Add(TEXT("1 dolly track   2 tripod   3 spray mark   4 jib   LMB place   RMB remove   F finish track   Wheel tripod height   C clear all"));
		Lines.Add(TEXT("Q/Z swap lens   Wheel zooms the zoom lens   G handheld/Steadicam   V rewatch   E mount rig / clapperboard   Enter: Action!"));
		break;
	case EDCPhase::Filming:
		Lines.Add(TEXT("Rolling - match the reference   E mount/leave rig   G handheld/Steadicam   Q/Z swap lens (1.2 s blind)   Wheel zoom   R restart"));
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
