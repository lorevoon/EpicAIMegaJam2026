// Director's Cut - the storyboard's reference cut.
//
// Before preparation the player watches a short film of the level shot well: one camera per storyboard beat, found by
// replaying the scene and testing candidate spots with the game's own framing check (IsFramed), so every shot in it
// really would capture its beat. Shots are kept wide, favouring views that show off the scenery. It plays through the same machinery as the review film.

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
#include "Engine/LevelStreaming.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"
#include "String/LexFromString.h"

namespace
{
	/** How finely the scene is replayed when testing shots, and the reference cut's frame rate. */
	constexpr float SolveStep = 0.25f;
	constexpr float FilmRate = 30.f;
	/** How far the scenery check looks; anything this open or more counts as a full view of the landscape. */
	constexpr float SceneryDepth = 6000.f;

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

	FString TagValue(const AActor* Actor, const TCHAR* Key)
	{
		const FString Prefix = FString(Key) + TEXT("=");
		for (const FName Tag : Actor->Tags)
		{
			const FString Text = Tag.ToString();
			if (Text.StartsWith(Prefix)) return Text.Mid(Prefix.Len());
		}
		return FString();
	}

	bool TagFloat(const AActor* Actor, const TCHAR* Key, float& Value)
	{
		return LexTryParseString(Value, *TagValue(Actor, Key)) && FMath::IsFinite(Value);
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

	// Candidate spots: rings around the action, plus the beat's authored camera spot (preferred).
	struct FCandidate { FVector Location; float FOV = ADCMultiCam::DefaultFOV; bool bAuthored = false; float Run = 0.f; float RunStart = 0.f; float Scenery = 0.f; };
	TArray<FCandidate> Candidates;
	if (Beat->bShowCameraSpot)
	{
		const FVector Spot = Beat->GetActorTransform().TransformPosition(Beat->CameraSpot);
		const float SpotFloor = GetWorld()->LineTraceSingleByChannel(Ground, Spot + FVector(0.f, 0.f, 300.f), Spot - FVector(0.f, 0.f, 3000.f), ECC_Visibility, Params)
			? Ground.ImpactPoint.Z : Spot.Z;
		Candidates.Add({ FVector(Spot.X, Spot.Y, SpotFloor + 170.f), ADCMultiCam::DefaultFOV, true });
	}
	// Out to 20 m, at heights the player's rigs reach above the ground right under each spot (a stand at eye level
	// or raised) - measured from that ground, so every reference camera can be copied by a placed one.
	const float RaisedStand = FMath::Min(300.f, MaxStandHeight);
	for (const float Radius : { 500.f, 800.f, 1100.f, 1500.f, 2000.f })
	{
		for (int32 a = 0; a < 16; ++a)
		{
			const float Angle = a * 2.f * PI / 16.f;
			const FVector Above(Middle.X + Radius * FMath::Cos(Angle), Middle.Y + Radius * FMath::Sin(Angle), Middle.Z + 1500.f);
			if (!GetWorld()->LineTraceSingleByChannel(Ground, Above, Above - FVector(0.f, 0.f, 5000.f), ECC_Visibility, Params))
			{
				continue; // no ground to stand a camera on
			}
			for (const float Height : { ADCMultiCam::StaticHeight, RaisedStand })
			{
				Candidates.Add({ Ground.ImpactPoint + FVector(0.f, 0.f, Height) });
			}
		}
	}

	// Drop spots inside the set, then fit a lens to each: wide enough that the action fills only ReferenceActionShare
	// of the frame all window long (and never narrower than ReferenceMinFOV), so the shot shows its surroundings.
	const FCollisionShape Probe = FCollisionShape::MakeSphere(25.f);
	const float MinHalfTan = FMath::Tan(FMath::DegreesToRadians(FMath::Max(ADCMultiCam::MinFOV, ReferenceMinFOV) * 0.5f));
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
				HalfTan = FMath::Max(HalfTan, FMath::Abs(Local.Y / Local.X) / (ReferenceActionShare * Frame.X));
				HalfTan = FMath::Max(HalfTan, FMath::Abs(Local.Z / Local.X) * Aspect / (ReferenceActionShare * Frame.Y));
			}
		}
		if (!bUsable || HalfTan > MaxHalfTan)
		{
			Candidates.RemoveAtSwap(c);
			continue;
		}
		Cand.FOV = FMath::RadiansToDegrees(2.f * FMath::Atan(HalfTan));

		// Scenery: how far the view reaches across the frame (open landscape beats a nearby wall).
		const FRotator View = LookAt(Cand.Location, Middle);
		const FVector Forward = View.Vector();
		const FVector Right = FRotationMatrix(View).GetUnitAxis(EAxis::Y);
		const FVector Up = FRotationMatrix(View).GetUnitAxis(EAxis::Z);
		float Depth = 0.f;
		for (const float U : { -0.8f, 0.f, 0.8f })
		{
			for (const float V : { -0.6f, 0.f, 0.6f })
			{
				const FVector Ray = (Forward + Right * (U * HalfTan * Frame.X) + Up * (V * HalfTan * Frame.Y / Aspect)).GetSafeNormal();
				FHitResult Hit;
				Depth += GetWorld()->LineTraceSingleByChannel(Hit, Cand.Location, Cand.Location + Ray * SceneryDepth, ECC_Visibility, Params)
					? Hit.Distance : SceneryDepth;
			}
		}
		Cand.Scenery = Depth / (9.f * SceneryDepth);
	}

	// Replay the window and test every spot with the real framing check; keep each spot's longest clean stretch.
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
			if (IsFramed(State, Ghost))
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

	// Best: holds the shot (longer is safer, up to a point), then the authored spot, the most scenery, a comfortable distance.
	const FCandidate* Best = nullptr;
	float BestRating = -MAX_flt;
	for (const FCandidate& Cand : Candidates)
	{
		const float Distance = FVector::Dist(Cand.Location, Middle);
		const float Rating = FMath::Min(Cand.Run, Beat->RequiredHold + 2.f) * 10.f + (Cand.bAuthored ? 4.f : 0.f)
			+ Cand.Scenery * 6.f - FMath::Abs(Distance - ReferencePreferredDistance) / 400.f;
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
	ReferenceShotLocations.Reset();
	if (Beats.Num() == 0 || !Ghost || !PlaybackCamera)
	{
		return;
	}

	// The reference film is shown in the real environment. Solve against its
	// buildings and dressing, not the empty green-screen volume they replace.
	// Inset generation can also call this during preparation, so restore the
	// previous environment on every exit without touching the recorded take.
	const ULevelStreaming* Real = UGameplayStatics::GetStreamingLevel(this, RealEnvironmentLevel);
	const bool bWasRealVisible = Real && Real->IsLevelVisible();
	SetReviewEnvironment(true);
	ON_SCOPE_EXIT { SetReviewEnvironment(bWasRealVisible); };

	// Authored cameras let a scene use several angles without adding scoring beats.
	// Levels without these tags retain the original automatic reference solver.
	struct FShot
	{
		int32 Beat = INDEX_NONE;
		FVector Location;
		float FOV = 0.f, CleanFrom = 0.f, Cut = 0.f;
		float EndFOV = 0.f, ZoomStart = 0.f, ZoomEnd = 0.f;
		bool bContinue = false;
	};
	struct FSpeedWindow { float Start = 0.f; float End = 0.f; float Rate = 1.f; };
	TArray<FShot> Shots;
	TArray<FSpeedWindow> SpeedWindows;
	TArray<FVector2D> DialogueWindows;
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		const ACameraActor* Camera = *It;
		if (!Camera->ActorHasTag(TEXT("DCReferenceShot"))) continue;
		FShot Shot;
		const FString BeatId = TagValue(Camera, TEXT("DCBeat"));
		Shot.Beat = Beats.IndexOfByPredicate([&BeatId](const FDCMultiCamBeat& B) { return B.Beat && B.Beat->BeatId == BeatId; });
		if (Shot.Beat == INDEX_NONE || !TagFloat(Camera, TEXT("DCStart"), Shot.Cut)
			|| Shot.Cut < 0.f || Shot.Cut >= TakeDuration)
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid authored reference tags on %s"), *Camera->GetName());
			continue;
		}
		Shot.Location = Camera->GetActorLocation();
		Shot.FOV = Camera->GetCameraComponent()->FieldOfView;
		float TaggedFOV = Shot.FOV;
		if (TagFloat(Camera, TEXT("DCFOV"), TaggedFOV)) Shot.FOV = TaggedFOV;
		Shot.FOV = FMath::Clamp(Shot.FOV, ADCMultiCam::MinFOV, ADCMultiCam::MaxFOV);
		Shot.EndFOV = Shot.FOV;
		TagFloat(Camera, TEXT("DCEndFOV"), Shot.EndFOV);
		Shot.EndFOV = FMath::Clamp(Shot.EndFOV, ADCMultiCam::MinFOV, ADCMultiCam::MaxFOV);
		TagFloat(Camera, TEXT("DCZoomStart"), Shot.ZoomStart);
		TagFloat(Camera, TEXT("DCZoomEnd"), Shot.ZoomEnd);
		Shot.bContinue = Camera->ActorHasTag(TEXT("DCContinueShot"));
		Shots.Add(Shot);
		for (const FName Tag : Camera->Tags)
		{
			const FString Text = Tag.ToString();
			FString Start, End;
			float From = 0.f, To = 0.f;
			if (Text.StartsWith(TEXT("DCDialogue=")) && Text.Mid(11).Split(TEXT(":"), &Start, &End)
				&& LexTryParseString(From, *Start) && LexTryParseString(To, *End)
				&& FMath::IsFinite(From) && FMath::IsFinite(To) && To > From)
				DialogueWindows.Add(FVector2D(From, To));
		}
		FSpeedWindow Speed;
		if (TagFloat(Camera, TEXT("DCSlowStart"), Speed.Start) && TagFloat(Camera, TEXT("DCSlowEnd"), Speed.End)
			&& TagFloat(Camera, TEXT("DCSlowRate"), Speed.Rate) && Speed.Start >= 0.f && Speed.End > Speed.Start
			&& Speed.End <= TakeDuration && Speed.Rate >= .25f && Speed.Rate <= 2.f)
		{
			SpeedWindows.Add(Speed);
		}
	}
	const bool bAuthored = !Shots.IsEmpty();
	if (bAuthored)
	{
		Shots.Sort([](const FShot& A, const FShot& B) { return A.Cut < B.Cut; });
		Shots[0].Cut = 0.f;
		for (const FShot& Shot : Shots)
		{
			UE_LOG(LogTemp, Log, TEXT("Authored reference camera: beat %s at %.2fs, %s, %.1f deg"),
				*Beats[Shot.Beat].Beat->BeatId, Shot.Cut, *Shot.Location.ToCompactString(), Shot.FOV);
		}
	}
	else
	{
		for (int32 i = 0; i < Beats.Num(); ++i)
		{
			FShot Shot;
			Shot.Beat = i;
			if (!FindReferenceShot(Beats[i], Shot.Location, Shot.FOV, Shot.CleanFrom) && Shot.FOV <= 0.f) continue;
			Shots.Add(Shot);
		}
		Shots.Sort([this](const FShot& A, const FShot& B) { return Beats[A.Beat].Beat->GetCaptureStartTime() < Beats[B.Beat].Beat->GetCaptureStartTime(); });
		for (int32 i = 1; i < Shots.Num(); ++i)
		{
			const FShot& Prev = Shots[i - 1];
			FShot& Shot = Shots[i];
			const float Wanted = Beats[Shot.Beat].Beat->GetCaptureStartTime() - ReferenceCutLead;
			const float PrevDone = Prev.CleanFrom + Beats[Prev.Beat].Beat->RequiredHold + SolveStep;
			Shot.Cut = FMath::Clamp(FMath::Max(Wanted, PrevDone), Prev.Cut + .5f, FMath::Max(Shot.CleanFrom, Prev.Cut + .5f));
		}
	}
	if (Shots.IsEmpty())
	{
		EvaluatePerformers(SceneTime);
		return;
	}

	// Shoot it: each camera stays put and pans smoothly with its beat's action, like a stabilized player camera.
	const float Aspect = GetViewAspect();
	const int32 NumFrames = FMath::CeilToInt(TakeDuration * FilmRate) + 1;
	ReferenceCut.Reserve(NumFrames);
	FRotator Aim = FRotator::ZeroRotator;
	int32 ShotIndex = -1;
	float FilmTime = 0.f;
	TArray<float> Rates;
	float ExtraTime = 0.f, CompensableTime = 0.f;
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
		FVector Focus = AnchorCentroid(Beats[Shot.Beat]);
		if (Shot.bContinue && Active > 0)
		{
			const float FollowAlpha = FMath::SmoothStep(Shot.Cut, Shot.Cut + 1.5f, Time);
			Focus = FMath::Lerp(AnchorCentroid(Beats[Shots[Active - 1].Beat]), Focus, FollowAlpha);
		}
		const FRotator Target = LookAt(Shot.Location, Focus);
		Aim = Active != ShotIndex && !Shot.bContinue ? Target : FMath::RInterpTo(Aim, Target, 1.f / FilmRate, 6.f);
		ShotIndex = Active;

		FDCTakeSample& Sample = ReferenceCut.AddDefaulted_GetRef();
		Sample.Time = Time;
		float Rate = 1.f;
		if (n > 0)
		{
			const float StepStart = ReferenceCut[n - 1].Time;
			const float Midpoint = (StepStart + Time) * .5f;
			const bool bDialogue = DialogueWindows.ContainsByPredicate([StepStart, Time](const FVector2D& W)
				{ return StepStart < W.Y && Time > W.X; });
			if (!bDialogue)
			{
				for (const FSpeedWindow& Speed : SpeedWindows)
					if (Midpoint >= Speed.Start && Midpoint < Speed.End) Rate = FMath::Min(Rate, Speed.Rate);
				if (Rate == 1.f) CompensableTime += Time - StepStart;
				else ExtraTime += (Time - StepStart) * (1.f / Rate - 1.f);
			}
			// Zero marks ordinary non-dialogue frames eligible for duration compensation.
			Rates.Add(!bDialogue && Rate == 1.f ? 0.f : Rate);
		}
		else Rates.Add(1.f);
		Sample.FilmTime = FilmTime;
		Sample.Slot = Active;
		Sample.Location = Shot.Location;
		Sample.Rotation = Aim;
		const float ZoomAlpha = Shot.ZoomEnd > Shot.ZoomStart
			? FMath::SmoothStep(Shot.ZoomStart, Shot.ZoomEnd, Time) : 0.f;
		Sample.FilmFOV = DCFrame::CaptureFOV(FMath::Lerp(Shot.FOV, Shot.EndFOV, ZoomAlpha), Aspect);
	}
	// Pay for impact slow motion only in silent action. Speech stays at exactly 1x,
	// so the combined soundtrack never pitches down a scream or spoken line.
	const float CompensationRate = CompensableTime > ExtraTime + KINDA_SMALL_NUMBER
		? CompensableTime / (CompensableTime - ExtraTime) : 1.f;
	for (int32 n = 1; n < ReferenceCut.Num(); ++n)
	{
		const float Rate = Rates[n] == 0.f ? CompensationRate : Rates[n];
		FilmTime += (ReferenceCut[n].Time - ReferenceCut[n - 1].Time) / Rate;
		ReferenceCut[n].FilmTime = FilmTime;
	}
	for (const FShot& Shot : Shots)
	{
		ReferenceShotBeats.Add(Shot.Beat);
		ReferenceShotLocations.Add(Shot.Location);
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
	if (!bReferencePlaying) return nullptr;
	FVector Location; FQuat Rotation; float FOV = 0.f, ScenePoint = 0.f;
	return GetFilmView(ReferenceCut, ReferenceTime, Location, Rotation, FOV, ScenePoint) ? GetReferenceBeatAt(ScenePoint) : nullptr;
}

const ADCBeat* ADCMultiCamDirector::GetReferenceBeatAt(float Time) const
{
	if (ReferenceCut.Num() == 0)
	{
		return nullptr;
	}
	const int32 Next = Algo::UpperBoundBy(ReferenceCut, Time, &FDCTakeSample::Time);
	const FDCTakeSample& Frame = ReferenceCut[FMath::Clamp(Next - 1, 0, ReferenceCut.Num() - 1)];
	if (!ReferenceShotBeats.IsValidIndex(Frame.Slot))
	{
		return nullptr;
	}
	const ADCBeat* Beat = Beats[ReferenceShotBeats[Frame.Slot]].Beat;
	return Time >= Beat->WindowStart && Time <= Beat->WindowEnd ? Beat : nullptr;
}

float ADCMultiCamDirector::ReferenceFilmTimeAtSceneTime(float Time) const
{
	if (ReferenceCut.IsEmpty()) return 0.f;
	const int32 Next = Algo::UpperBoundBy(ReferenceCut, Time, &FDCTakeSample::Time);
	const FDCTakeSample& A = ReferenceCut[FMath::Clamp(Next - 1, 0, ReferenceCut.Num() - 1)];
	const FDCTakeSample& B = ReferenceCut.IsValidIndex(Next) ? ReferenceCut[Next] : A;
	const float Alpha = B.Time > A.Time ? FMath::Clamp((Time - A.Time) / (B.Time - A.Time), 0.f, 1.f) : 0.f;
	return FMath::Lerp(A.FilmTime, B.FilmTime, Alpha);
}

UTextureRenderTarget2D* ADCMultiCamDirector::GetReferenceInset() const
{
	const bool bPhase = Phase == EDCPhase::Preparation || Phase == EDCPhase::Countdown || Phase == EDCPhase::Filming;
	return bPhase && bShowReferenceInset && ReferenceCut.Num() > 0 ? ReferenceInsetTarget.Get() : nullptr;
}

void ADCMultiCamDirector::UpdateReferenceInset(float DeltaSeconds)
{
	constexpr float CaptureInterval = 1.f / 15.f;
	if (!bShowReferenceInset || !ReferenceCapture)
	{
		ReferenceInsetCaptureElapsed = CaptureInterval;
		LastReferenceInsetSceneTime = -TNumericLimits<float>::Max();
		return;
	}
	ReferenceInsetCaptureElapsed += FMath::Max(0.f, DeltaSeconds);
	bool bBuiltNow = false;
	if (!bReferenceCutBuilt)
	{
		BuildReferenceCut(); // e.g. the storyboard's reference cut was switched off or skipped
		bBuiltNow = true;
	}
	// Slow-motion changes film time; the inset must still follow rehearsal scene time.
	FVector Location;
	FQuat Rotation;
	float FOV = 0.f, ScenePoint = 0.f;
	if (!GetFilmView(ReferenceCut, ReferenceFilmTimeAtSceneTime(SceneTime), Location, Rotation, FOV, ScenePoint))
	{
		return;
	}
	const bool bFirstCapture = !ReferenceInsetTarget;
	const bool bSceneChanged = !FMath::IsNearlyEqual(SceneTime, LastReferenceInsetSceneTime, .001f);
	if (!bFirstCapture && !bBuiltNow && (!bSceneChanged || ReferenceInsetCaptureElapsed < CaptureInterval))
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
	ReferenceInsetCaptureElapsed = 0.f;
	LastReferenceInsetSceneTime = SceneTime;
}

void ADCMultiCamDirector::ComputeMatch()
{
	Match = FDCMatch();
	if (!bReferenceCutBuilt)
	{
		BuildReferenceCut();
	}
	if (ReferenceCut.Num() == 0 || TakeDuration <= 0.f)
	{
		return;
	}
	// Compare the film with the reference cut moment by moment, in scene time, on what each one shows of the scene.
	constexpr float Step = 0.1f;
	// A gap longer than this in the recording (a recording pause) means the moment isn't in the film.
	constexpr float MaxGap = 0.25f;
	const int32 NumMoments = FMath::FloorToInt(TakeDuration / Step) + 1;
	double Framing = 0.0, Size = 0.0, Viewpoint = 0.0, Overall = 0.0;
	TArray<FVector> Points;
	for (int32 i = 0; i < NumMoments; ++i)
	{
		const float Time = i * Step;
		FVector RefLocation;
		FQuat RefRotation;
		float RefFOV = 0.f, ScenePoint = 0.f;
		const int32 Next = Algo::UpperBoundBy(Recording, Time, &FDCTakeSample::Time);
		if (Next == 0 || Time - Recording[Next - 1].Time > MaxGap
			|| !GetFilmView(ReferenceCut, ReferenceFilmTimeAtSceneTime(Time), RefLocation, RefRotation, RefFOV, ScenePoint))
		{
			continue;
		}
		const FDCTakeSample& Shot = Recording[Next - 1];
		EvaluatePerformers(Time);
		GetSubjectPoints(Time, Points);
		const FDCMatch Likeness = CompareFrames(ReadFrame(Points, Shot.Location, Shot.Rotation.Quaternion(), Shot.FilmFOV),
			ReadFrame(Points, RefLocation, RefRotation, RefFOV));
		Framing += Likeness.Framing;
		Size += Likeness.Size;
		Viewpoint += Likeness.Viewpoint;
		Overall += Likeness.Overall;
	}
	EvaluatePerformers(SceneTime);
	Match.Framing = float(Framing / NumMoments);
	Match.Size = float(Size / NumMoments);
	Match.Viewpoint = float(Viewpoint / NumMoments);
	Match.Overall = float(Overall / NumMoments);
}

void ADCMultiCamDirector::UpdateCoveredScenes()
{
	if (!bReferenceCutBuilt)
	{
		BuildReferenceCut();
	}
	TArray<bool> Now;
	Now.SetNumZeroed(Beats.Num());
	for (int32 Shot = 0; Shot < ReferenceShotBeats.Num(); ++Shot)
	{
		for (const ADCMultiCam* Cam : Cameras)
		{
			const FVector Offset = Cam ? Cam->GetActorLocation() - ReferenceShotLocations[Shot] : FVector::ZeroVector;
			if (Cam && Offset.Size2D() <= ReferenceSpotRadius && FMath::Abs(Offset.Z) <= ReferenceSpotHeightTolerance)
			{
				Now[ReferenceShotBeats[Shot]] = true;
				break;
			}
		}
	}
	// Each scene ticked off gets its moment: the storyboard animates it and the director pops up to praise it.
	if (SceneCoveredTime.Num() != Now.Num())
	{
		SceneCoveredTime.Init(-1.f, Now.Num());
	}
	for (int32 i = 0; i < Now.Num(); ++i)
	{
		if (Now[i] && !(SceneCovered.IsValidIndex(i) && SceneCovered[i]))
		{
			SceneCoveredTime[i] = GetWorld()->GetRealTimeSeconds();
			Cheer(i);
		}
	}
	SceneCovered = MoveTemp(Now);
}
