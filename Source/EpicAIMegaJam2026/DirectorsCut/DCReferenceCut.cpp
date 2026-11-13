// Director's Cut - the storyboard's reference cut.
//
// Before preparation the player watches a short film of the level shot well: one camera per storyboard beat, found by
// replaying the scene and testing candidate spots with the game's own scoring (IsFramed + the "ruins the shot" check),
// so every shot in it really would capture its beat. It plays through the same machinery as the review film.

#include "DCMultiCamDirector.h"
#include "DCBeat.h"
#include "DCMultiCam.h"
#include "DCMultiCamCharacter.h"
#include "DCPerformer.h"
#include "DCRail.h"
#include "Algo/BinarySearch.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** How finely the scene is replayed when testing shots, and the reference cut's frame rate. */
	constexpr float SolveStep = 0.25f;
	constexpr float FilmRate = 30.f;
	/** Anchors are fitted into this share of the capture frame, leaving breathing room around the action. */
	constexpr float FrameFill = 0.7f;
	/** Distance the solver prefers when several spots work equally well. */
	constexpr float PreferredDistance = 650.f;

	FVector AnchorCentroid(const FDCMultiCamBeat& State)
	{
		FVector Sum = FVector::ZeroVector;
		int32 Count = 0;
		for (const FDCAnchor& A : State.Anchors)
		{
			if (A.Component)
			{
				Sum += A.Component->GetComponentLocation();
				++Count;
			}
		}
		return Count > 0 ? Sum / Count : FVector::ZeroVector;
	}

	FRotator LookAt(const FVector& From, const FVector& To)
	{
		return FRotationMatrix::MakeFromX(To - From).Rotator();
	}
}

void ADCMultiCamDirector::EvaluatePerformers(float Time)
{
	for (ADCPerformer* Performer : Performers)
	{
		Performer->Evaluate(Time);
	}
}

bool ADCMultiCamDirector::FindReferenceShot(const FDCMultiCamBeat& State, FVector& OutLocation, float& OutFOV, float& OutCleanFrom)
{
	const ADCBeat* Beat = State.Beat;
	const float T0 = Beat->GetCaptureStartTime();
	const float T1 = FMath::Max(Beat->WindowEnd, T0);
	const int32 NumTimes = FMath::FloorToInt((T1 - T0) / SolveStep) + 1;
	const float Aspect = GetViewAspect();
	const FVector2D Frame = DCFrame::CaptureExtent(Aspect);
	FCollisionQueryParams Params = MakeTraceParams(SCENE_QUERY_STAT_NAME_ONLY(DCReferenceCut));
	for (ADCPerformer* Performer : Performers)
	{
		Params.AddIgnoredActor(Performer);
	}

	// Where the action is over the window, anchor by anchor.
	TArray<TArray<FVector>> Points;
	TArray<FVector> Centroids;
	for (int32 k = 0; k < NumTimes; ++k)
	{
		EvaluatePerformers(T0 + k * SolveStep);
		TArray<FVector>& Row = Points.AddDefaulted_GetRef();
		for (const FDCAnchor& A : State.Anchors)
		{
			Row.Add(A.Component->GetComponentLocation());
		}
		Centroids.Add(AnchorCentroid(State));
	}
	const FVector Middle = Centroids[NumTimes / 2];
	FHitResult Ground;
	const float FloorZ = GetWorld()->LineTraceSingleByChannel(Ground, Middle + FVector(0.f, 0.f, 200.f), Middle - FVector(0.f, 0.f, 2000.f), ECC_Visibility, Params)
		? Ground.ImpactPoint.Z : Middle.Z - 100.f;

	// Candidate spots: rings around the action at two lens heights, plus the beat's authored camera spot (preferred).
	struct FCandidate { FVector Location; float FOV = ADCMultiCam::DefaultFOV; bool bAuthored = false; float Run = 0.f; float RunStart = 0.f; };
	TArray<FCandidate> Candidates;
	if (Beat->bShowCameraSpot)
	{
		const FVector Spot = Beat->GetActorTransform().TransformPosition(Beat->CameraSpot);
		const float SpotFloor = GetWorld()->LineTraceSingleByChannel(Ground, Spot + FVector(0.f, 0.f, 300.f), Spot - FVector(0.f, 0.f, 3000.f), ECC_Visibility, Params)
			? Ground.ImpactPoint.Z : Spot.Z;
		Candidates.Add({ FVector(Spot.X, Spot.Y, SpotFloor + 170.f), ADCMultiCam::DefaultFOV, true });
	}
	for (const float Radius : { 350.f, 550.f, 800.f, 1100.f })
	{
		for (int32 a = 0; a < 16; ++a)
		{
			const float Angle = a * 2.f * PI / 16.f;
			for (const float Height : { 150.f, 230.f })
			{
				Candidates.Add({ FVector(Middle.X + Radius * FMath::Cos(Angle), Middle.Y + Radius * FMath::Sin(Angle), FloorZ + Height) });
			}
		}
	}

	// Drop spots inside the set, then fit a lens to each: the tightest that keeps every anchor in frame all window long.
	const FCollisionShape Probe = FCollisionShape::MakeSphere(25.f);
	const float MinHalfTan = FMath::Tan(FMath::DegreesToRadians(ADCMultiCam::MinFOV * 0.5f));
	const float MaxHalfTan = FMath::Tan(FMath::DegreesToRadians(ADCMultiCam::MaxFOV * 0.5f));
	for (int32 c = Candidates.Num() - 1; c >= 0; --c)
	{
		FCandidate& Cand = Candidates[c];
		bool bUsable = !GetWorld()->OverlapAnyTestByChannel(Cand.Location, FQuat::Identity, ECC_Visibility, Probe, Params);
		float HalfTan = MinHalfTan;
		for (int32 k = 0; bUsable && k < NumTimes; ++k)
		{
			const FTransform View(LookAt(Cand.Location, Centroids[k]), Cand.Location);
			for (const FVector& P : Points[k])
			{
				const FVector Local = View.InverseTransformPositionNoScale(P);
				if (Local.X <= 1.f)
				{
					bUsable = false;
					break;
				}
				HalfTan = FMath::Max(HalfTan, FMath::Abs(Local.Y / Local.X) / (FrameFill * Frame.X));
				HalfTan = FMath::Max(HalfTan, FMath::Abs(Local.Z / Local.X) * Aspect / (FrameFill * Frame.Y));
			}
		}
		if (!bUsable || HalfTan > MaxHalfTan)
		{
			Candidates.RemoveAtSwap(c);
			continue;
		}
		Cand.FOV = FMath::RadiansToDegrees(2.f * FMath::Atan(HalfTan));
	}

	// Replay the window and test every spot with the real scoring checks; keep each spot's longest clean stretch.
	TArray<float> Current;
	Current.SetNumZeroed(Candidates.Num());
	for (int32 k = 0; k < NumTimes; ++k)
	{
		EvaluatePerformers(T0 + k * SolveStep);
		for (int32 c = 0; c < Candidates.Num(); ++c)
		{
			FCandidate& Cand = Candidates[c];
			Ghost->SetActorLocation(Cand.Location);
			Ghost->SetAim(LookAt(Cand.Location, Centroids[k]));
			Ghost->SnapFOV(Cand.FOV);
			if (IsFramed(State, Ghost) && !IsShotSpoiledFrom(Ghost))
			{
				Current[c] += SolveStep;
				if (Current[c] > Cand.Run)
				{
					Cand.Run = Current[c];
					Cand.RunStart = T0 + (k + 1) * SolveStep - Current[c];
				}
			}
			else
			{
				Current[c] = 0.f;
			}
		}
	}

	// Best: holds the shot cleanly (longer is safer, up to a point), then the authored spot, then a comfortable distance.
	const FCandidate* Best = nullptr;
	float BestRating = -MAX_flt;
	for (const FCandidate& Cand : Candidates)
	{
		const float Distance = FVector::Dist(Cand.Location, Middle);
		const float Rating = FMath::Min(Cand.Run, Beat->RequiredHold + 2.f) * 10.f + (Cand.bAuthored ? 4.f : 0.f)
			- FMath::Abs(Distance - PreferredDistance) / 200.f;
		if (Rating > BestRating)
		{
			BestRating = Rating;
			Best = &Cand;
		}
	}
	if (!Best)
	{
		return false;
	}
	OutLocation = Best->Location;
	OutFOV = Best->FOV;
	OutCleanFrom = Best->RunStart;
	const bool bCaptures = Best->Run >= Beat->RequiredHold + SolveStep;
	UE_LOG(LogTemp, Log, TEXT("Reference cut: beat %s '%s' from %s, %.0f deg lens, clean %.2fs from %.2fs%s"),
		*Beat->BeatId, *Beat->DisplayName, *Best->Location.ToCompactString(), Best->FOV, Best->Run, Best->RunStart,
		bCaptures ? TEXT("") : TEXT(" - NOT ENOUGH to capture it"));
	return bCaptures;
}

void ADCMultiCamDirector::BuildReferenceCut()
{
	bReferenceCutBuilt = true;
	ReferenceCut.Reset();
	ReferenceShotBeats.Reset();
	if (Beats.Num() == 0 || !Ghost || !PlaybackCamera)
	{
		return;
	}

	// One shot per beat, in story order.
	struct FShot { int32 Beat = INDEX_NONE; FVector Location; float FOV = 0.f; float CleanFrom = 0.f; float Cut = 0.f; };
	TArray<FShot> Shots;
	for (int32 i = 0; i < Beats.Num(); ++i)
	{
		FShot Shot;
		Shot.Beat = i;
		if (!FindReferenceShot(Beats[i], Shot.Location, Shot.FOV, Shot.CleanFrom) && Shot.FOV <= 0.f)
		{
			continue; // no usable spot at all
		}
		Shots.Add(Shot);
	}
	Shots.Sort([this](const FShot& A, const FShot& B) { return Beats[A.Beat].Beat->GetCaptureStartTime() < Beats[B.Beat].Beat->GetCaptureStartTime(); });
	if (Shots.Num() == 0)
	{
		EvaluatePerformers(SceneTime);
		return;
	}

	// Cut to each shot a moment before its beat, but never before the previous shot has held its beat long enough.
	for (int32 i = 1; i < Shots.Num(); ++i)
	{
		const FShot& Prev = Shots[i - 1];
		FShot& Shot = Shots[i];
		const float Wanted = Beats[Shot.Beat].Beat->GetCaptureStartTime() - ReferenceCutLead;
		const float PrevDone = Prev.CleanFrom + Beats[Prev.Beat].Beat->RequiredHold + SolveStep;
		Shot.Cut = FMath::Clamp(FMath::Max(Wanted, PrevDone), Prev.Cut + 0.5f, FMath::Max(Shot.CleanFrom, Prev.Cut + 0.5f));
	}

	// Shoot it: each camera stays put and pans smoothly with its beat's action, like a stabilized player camera.
	const float Aspect = GetViewAspect();
	const int32 NumFrames = FMath::CeilToInt(TakeDuration * FilmRate) + 1;
	ReferenceCut.Reserve(NumFrames);
	FRotator Aim = FRotator::ZeroRotator;
	int32 ShotIndex = -1;
	for (int32 n = 0; n < NumFrames; ++n)
	{
		const float Time = FMath::Min(n / FilmRate, TakeDuration);
		int32 Active = 0;
		while (Shots.IsValidIndex(Active + 1) && Shots[Active + 1].Cut <= Time)
		{
			++Active;
		}
		const FShot& Shot = Shots[Active];
		EvaluatePerformers(Time);
		const FRotator Target = LookAt(Shot.Location, AnchorCentroid(Beats[Shot.Beat]));
		Aim = Active != ShotIndex ? Target : FMath::RInterpTo(Aim, Target, 1.f / FilmRate, 6.f);
		ShotIndex = Active;

		FDCTakeSample& Sample = ReferenceCut.AddDefaulted_GetRef();
		Sample.Time = Time;
		Sample.FilmTime = Time;
		Sample.Slot = Active;
		Sample.Location = Shot.Location;
		Sample.Rotation = Aim;
		Sample.FilmFOV = DCFrame::CaptureFOV(Shot.FOV, Aspect);
	}
	for (const FShot& Shot : Shots)
	{
		ReferenceShotBeats.Add(Shot.Beat);
	}
	EvaluatePerformers(SceneTime);
}

void ADCMultiCamDirector::StartReferenceCut()
{
	APlayerController* PC = GetPC();
	if (!bShowReferenceCut || Phase != EDCPhase::Storyboard || !PC)
	{
		return;
	}
	if (!bReferenceCutBuilt)
	{
		BuildReferenceCut();
	}
	if (ReferenceCut.Num() == 0)
	{
		return;
	}
	// Shown like the finished film: real dressing, no rigs, letterboxed playback camera.
	SetReviewEnvironment(true);
	SetRigsHidden(true);
	bReferencePlaying = true;
	bReferencePaused = false;
	ReferenceTime = 0.f;
	ShowFilmFrame(ReferenceCut, ReferenceTime);
	PC->bAutoManageActiveCameraTarget = false;
	PC->SetViewTargetWithBlend(PlaybackCamera, 0.f);
}

void ADCMultiCamDirector::StopReferenceCut()
{
	if (!bReferencePlaying)
	{
		return;
	}
	bReferencePlaying = false;
	bReferencePaused = false;
	SetReviewEnvironment(false);
	SetRigsHidden(false);
	SceneTime = 0.f;
	EvaluatePerformers(SceneTime);
	ViewWalking();
}

void ADCMultiCamDirector::TickReferenceCut(float DeltaSeconds)
{
	if (bReferenceAutoStart)
	{
		bReferenceAutoStart = false;
		StartReferenceCut();
		return;
	}
	if (!bReferencePlaying || bReferencePaused)
	{
		return;
	}
	ReferenceTime += DeltaSeconds;
	if (ReferenceTime >= GetReferenceLength())
	{
		StopReferenceCut(); // over to the storyboard page
		return;
	}
	ShowFilmFrame(ReferenceCut, ReferenceTime);
}

void ADCMultiCamDirector::ScrubReferenceCut(float DeltaSeconds)
{
	if (bReferencePlaying)
	{
		ReferenceTime = FMath::Clamp(ReferenceTime + DeltaSeconds, 0.f, GetReferenceLength());
		ShowFilmFrame(ReferenceCut, ReferenceTime);
	}
}

void ADCMultiCamDirector::ToggleReferenceCut()
{
	if (bReferencePlaying)
	{
		bReferencePaused = !bReferencePaused;
	}
	else
	{
		StartReferenceCut(); // watch it again from the storyboard page
	}
}

const ADCBeat* ADCMultiCamDirector::GetReferenceBeat() const
{
	return bReferencePlaying ? GetReferenceBeatAt(ReferenceTime) : nullptr;
}

const ADCBeat* ADCMultiCamDirector::GetReferenceBeatAt(float Time) const
{
	if (ReferenceCut.Num() == 0)
	{
		return nullptr;
	}
	const int32 Next = Algo::UpperBoundBy(ReferenceCut, Time, &FDCTakeSample::FilmTime);
	const FDCTakeSample& Frame = ReferenceCut[FMath::Clamp(Next - 1, 0, ReferenceCut.Num() - 1)];
	if (!ReferenceShotBeats.IsValidIndex(Frame.Slot))
	{
		return nullptr;
	}
	const ADCBeat* Beat = Beats[ReferenceShotBeats[Frame.Slot]].Beat;
	return Time >= Beat->WindowStart && Time <= Beat->WindowEnd ? Beat : nullptr;
}

UTextureRenderTarget2D* ADCMultiCamDirector::GetReferenceInset() const
{
	return Phase == EDCPhase::Preparation && bShowReferenceInset && ReferenceCut.Num() > 0 ? ReferenceInsetTarget.Get() : nullptr;
}

void ADCMultiCamDirector::UpdateReferenceInset()
{
	if (!bShowReferenceInset || !ReferenceCapture)
	{
		return;
	}
	if (!bReferenceCutBuilt)
	{
		BuildReferenceCut(); // e.g. the storyboard's reference cut was switched off or skipped
	}
	// The reference cut has no recording pauses, so its film time is the rehearsal's scene time.
	FVector Location;
	FQuat Rotation;
	float FOV = 0.f, ScenePoint = 0.f;
	if (!GetFilmView(ReferenceCut, SceneTime, Location, Rotation, FOV, ScenePoint))
	{
		return;
	}
	if (!ReferenceInsetTarget)
	{
		ReferenceInsetTarget = NewObject<UTextureRenderTarget2D>(this);
		ReferenceInsetTarget->InitCustomFormat(640, FMath::RoundToInt(640.f / DCFrame::FilmAspect), PF_B8G8R8A8, false);
		ReferenceCapture->TextureTarget = ReferenceInsetTarget;
	}

	// Only the scene: no player rigs, placement ghost, director on foot, or hint circles.
	ReferenceCapture->HiddenActors.Reset();
	ReferenceCapture->HiddenActors.Add(Ghost);
	ReferenceCapture->HiddenActors.Add(GetPlayer());
	for (ADCMultiCam* Cam : Cameras)
	{
		ReferenceCapture->HiddenActors.Add(Cam);
	}
	for (TActorIterator<ADCRail> It(GetWorld()); It; ++It)
	{
		ReferenceCapture->HiddenActors.Add(*It);
	}
	for (const FDCMultiCamBeat& State : Beats)
	{
		ReferenceCapture->HiddenActors.Add(State.Beat);
	}

	ReferenceCapture->SetWorldLocationAndRotation(Location, Rotation);
	ReferenceCapture->FOVAngle = FOV;
	ReferenceCapture->CaptureScene();
}
