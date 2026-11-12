// Director's Cut prototype 4 - "Replica" director.

#include "DCReplicaDirector.h"
#include "DCFreeCharacter.h"
#include "DCPerformer.h"
#include "DCRail.h"
#include "DCRefCamKey.h"
#include "DCSprayMark.h"
#include "DCTripod.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float CameraFOV = 75.f;
	constexpr float FramingTolerance = 0.6f; // screen-space (NDC) distance at which an anchor's placement scores zero
	constexpr float SprayRemoveRange = 150.f;

	/** Projects P into a camera's normalized screen space ([-1, 1] on both axes). False if behind the camera. */
	bool ProjectNdc(const FVector& CamLoc, const FRotator& CamRot, const FVector& P, float HalfTan, float Aspect, FVector2D& OutNdc, float& OutDepth)
	{
		const FVector Local = CamRot.UnrotateVector(P - CamLoc);
		OutDepth = Local.X;
		if (Local.X <= 1.f)
		{
			return false;
		}
		OutNdc = FVector2D((Local.Y / Local.X) / HalfTan, (Local.Z / Local.X) / (HalfTan / Aspect));
		return true;
	}

	bool OnScreen(const FVector2D& Ndc)
	{
		return FMath::Abs(Ndc.X) <= 1.05f && FMath::Abs(Ndc.Y) <= 1.05f;
	}
}

ADCReplicaDirector::ADCReplicaDirector()
{
	TakeDuration = 30.f;
}

void ADCReplicaDirector::BeginPlay()
{
	Super::BeginPlay();

	GatherReference();

	FActorSpawnParameters Params;
	Params.Owner = this;
	FilmCamera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform::Identity, Params);
	FilmCamera->GetCameraComponent()->SetFieldOfView(CameraFOV);
	FilmCamera->GetCameraComponent()->SetConstraintAspectRatio(false);
	SprayGhost = GetWorld()->SpawnActor<ADCSprayMark>(ADCSprayMark::StaticClass(), FTransform::Identity, Params);
	SprayGhost->SetActorHiddenInGame(true);

	// The reference film starts on the first tick, once the player controller has its pawn.
	bPendingIntro = true;
}

void ADCReplicaDirector::GatherReference()
{
	TMap<int32, FDCRefShot> ByNumber;
	for (TActorIterator<ADCRefCamKey> It(GetWorld()); It; ++It)
	{
		ADCRefCamKey* Key = *It;
		FDCRefShot& Shot = ByNumber.FindOrAdd(Key->Shot);
		Shot.Number = Key->Shot;
		Shot.Keys.Add(Key);
		if (Shot.Title.IsEmpty() && !Key->ShotTitle.IsEmpty())
		{
			Shot.Title = Key->ShotTitle;
		}
		if (Shot.Subjects.Num() == 0)
		{
			for (const FDCAnchorRef& Ref : Key->Subjects)
			{
				if (Ref.Performer)
				{
					FDCAnchor A;
					A.Component = Ref.Performer->GetAnchor(Ref.Part);
					A.Owner = Ref.Performer;
					A.bMustSeeFace = Ref.bMustSeeFace;
					Shot.Subjects.Add(A);
				}
			}
		}
	}

	Shots.Reset();
	for (TPair<int32, FDCRefShot>& Pair : ByNumber)
	{
		FDCRefShot& Shot = Pair.Value;
		Shot.Keys.Sort([](const ADCRefCamKey& A, const ADCRefCamKey& B) { return A.Time < B.Time; });
		Shot.Start = Shot.Keys[0]->Time;
		if (Shot.Title.IsEmpty())
		{
			Shot.Title = FString::Printf(TEXT("Shot %d"), Shot.Number);
		}
		Shots.Add(Shot);
	}
	Shots.Sort([](const FDCRefShot& A, const FDCRefShot& B) { return A.Start < B.Start; });
	for (int32 i = 0; i < Shots.Num(); ++i)
	{
		Shots[i].End = Shots.IsValidIndex(i + 1) ? Shots[i + 1].Start : TakeDuration;
	}

	if (Shots.Num() == 0)
	{
		SetupProblem = TEXT("No DCRefCamKey actors - select the DCReplicaDirector and click Build Chapel Scene");
	}
}

int32 ADCReplicaDirector::GetShotIndexAt(float Time) const
{
	int32 Index = INDEX_NONE;
	for (int32 i = 0; i < Shots.Num(); ++i)
	{
		if (Shots[i].Start <= Time + KINDA_SMALL_NUMBER)
		{
			Index = i;
		}
	}
	return Index;
}

bool ADCReplicaDirector::GetReferencePose(float Time, FVector& OutLocation, FRotator& OutRotation) const
{
	const int32 Index = GetShotIndexAt(Time);
	if (Index == INDEX_NONE)
	{
		return false;
	}
	// Keys within a shot blend; the shot boundary itself is a hard cut.
	const TArray<ADCRefCamKey*>& Keys = Shots[Index].Keys;
	const ADCRefCamKey* A = Keys[0];
	const ADCRefCamKey* B = Keys[0];
	for (int32 i = 0; i < Keys.Num(); ++i)
	{
		if (Keys[i]->Time <= Time)
		{
			A = Keys[i];
			B = Keys.IsValidIndex(i + 1) ? Keys[i + 1] : Keys[i];
		}
	}
	const float Span = B->Time - A->Time;
	const float Alpha = Span > KINDA_SMALL_NUMBER ? FMath::Clamp((Time - A->Time) / Span, 0.f, 1.f) : 0.f;
	OutLocation = FMath::Lerp(A->GetActorLocation(), B->GetActorLocation(), Alpha);
	OutRotation = FQuat::Slerp(A->GetActorQuat(), B->GetActorQuat(), Alpha).Rotator();
	return true;
}

bool ADCReplicaDirector::GetTakePose(float Time, FVector& OutLocation, FRotator& OutRotation) const
{
	if (Recording.Num() == 0)
	{
		return false;
	}
	int32 i = 0;
	while (i + 1 < Recording.Num() && Recording[i + 1].Time <= Time)
	{
		++i;
	}
	const FDCCamSample& A = Recording[i];
	const FDCCamSample& B = Recording[FMath::Min(i + 1, Recording.Num() - 1)];
	const float Span = B.Time - A.Time;
	const float Alpha = Span > KINDA_SMALL_NUMBER ? FMath::Clamp((Time - A.Time) / Span, 0.f, 1.f) : 0.f;
	OutLocation = FMath::Lerp(A.Location, B.Location, Alpha);
	OutRotation = FQuat::Slerp(A.Rotation.Quaternion(), B.Rotation.Quaternion(), Alpha).Rotator();
	return true;
}

// ---------------------------------------------------------------------------------------------
// Film playback (the reference during Watching, either film as a replay during Results)

void ADCReplicaDirector::Tick(float DeltaSeconds)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (bPendingIntro && PC && GetPlayer())
	{
		bPendingIntro = false;
		if (Shots.Num() > 0)
		{
			StartFilm(EDCReplay::Reference);
		}
	}

	if (IsShowingFilm())
	{
		AActor::Tick(DeltaSeconds);
		TickFilm(DeltaSeconds);
		return;
	}
	Super::Tick(DeltaSeconds);
}

void ADCReplicaDirector::StartFilm(EDCReplay Mode)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || (Mode == EDCReplay::Take && !HasRecording()))
	{
		return;
	}
	const bool bAlreadyShowing = IsShowingFilm();

	if (Phase == EDCPhase::Results)
	{
		Replay = Mode;
	}
	else
	{
		ResetScene();
		Phase = EDCPhase::Watching;
		Replay = EDCReplay::None;
	}

	RailGhost->SetPreview(false, FVector::ZeroVector, false);
	TripodGhost->SetActorHiddenInGame(true);
	SprayGhost->SetActorHiddenInGame(true);

	SceneTime = 0.f;
	for (ADCPerformer* Performer : Performers)
	{
		Performer->Evaluate(0.f);
	}
	TickFilm(0.f);
	PC->SetViewTargetWithBlend(FilmCamera, 0.f);
	if (!bAlreadyShowing)
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
	}
}

void ADCReplicaDirector::StopFilm()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (Phase == EDCPhase::Watching)
	{
		ResetScene();
		Flash(TEXT("Now rig the set to reproduce that film. V rewatches it any time."), 6.f);
	}
	else
	{
		Replay = EDCReplay::None;
		SceneTime = TakeDuration;
	}
	if (PC)
	{
		PC->ResetIgnoreInputFlags();
		if (PC->GetPawn())
		{
			PC->SetViewTargetWithBlend(PC->GetPawn(), 0.f);
		}
	}
}

void ADCReplicaDirector::TickFilm(float DeltaSeconds)
{
	SceneTime = FMath::Min(SceneTime + DeltaSeconds, TakeDuration);
	for (ADCPerformer* Performer : Performers)
	{
		Performer->Evaluate(SceneTime);
	}

	FVector Loc;
	FRotator Rot;
	const bool bPose = Replay == EDCReplay::Take ? GetTakePose(SceneTime, Loc, Rot) : GetReferencePose(SceneTime, Loc, Rot);
	if (bPose)
	{
		FilmCamera->SetActorLocationAndRotation(Loc, Rot);
	}

	if (SceneTime >= TakeDuration)
	{
		StopFilm();
	}
}

// ---------------------------------------------------------------------------------------------
// Recording and evaluation

float ADCReplicaDirector::GetAspect() const
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		int32 W = 0, H = 0;
		PC->GetViewportSize(W, H);
		if (W > 0 && H > 0)
		{
			return float(W) / float(H);
		}
	}
	return 16.f / 9.f;
}

float ADCReplicaDirector::ScoreSample(const FDCRefShot& Shot, const FVector& RefLoc, const FRotator& RefRot, const FVector& Loc, const FRotator& Rot,
	float& OutFraming, float& OutPositionError, float& OutAngleError) const
{
	OutPositionError = FVector::Dist(RefLoc, Loc);
	OutAngleError = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(RefRot.Vector(), Rot.Vector()), -1.f, 1.f)));
	const float PositionScore = FMath::Clamp(1.f - OutPositionError / PositionTolerance, 0.f, 1.f);
	const float AngleScore = FMath::Clamp(1.f - OutAngleError / AngleTolerance, 0.f, 1.f);

	// Framing: where each subject sits on screen and how big it appears, compared to the reference frame.
	const float HalfTan = FMath::Tan(FMath::DegreesToRadians(CameraFOV * 0.5f));
	const float Aspect = GetAspect();
	float Sum = 0.f;
	int32 Count = 0;
	for (const FDCAnchor& A : Shot.Subjects)
	{
		const FVector P = A.Component->GetComponentLocation();
		FVector2D RefNdc, Ndc;
		float RefDepth = 0.f, Depth = 0.f;
		if (!ProjectNdc(RefLoc, RefRot, P, HalfTan, Aspect, RefNdc, RefDepth) || !OnScreen(RefNdc))
		{
			continue; // not in the reference frame at this moment, so it doesn't count
		}
		++Count;
		if (!ProjectNdc(Loc, Rot, P, HalfTan, Aspect, Ndc, Depth) || !OnScreen(Ndc))
		{
			continue; // in the reference but out of the player's frame: scores zero
		}
		const float Placement = FMath::Clamp(1.f - (RefNdc - Ndc).Size() / FramingTolerance, 0.f, 1.f);
		const float SizeMatch = FMath::Min(RefDepth, Depth) / FMath::Max(RefDepth, Depth);
		Sum += 0.7f * Placement + 0.3f * SizeMatch;
	}
	OutFraming = Count > 0 ? Sum / Count : AngleScore;

	return FramingWeight * OutFraming + (1.f - FramingWeight) * 0.5f * (AngleScore + PositionScore);
}

void ADCReplicaDirector::EvaluateBeats(float PrevTime, float NowTime)
{
	ADCFreeCharacter* Player = GetPlayer();
	if (Player)
	{
		const FVector Loc = Player->GetCamera()->GetComponentLocation();
		const FRotator Rot = Player->GetCamera()->GetComponentRotation();
		const float Step = 1.f / SampleRate;
		while (NextSampleTime <= NowTime + KINDA_SMALL_NUMBER)
		{
			FDCCamSample Sample;
			Sample.Time = NextSampleTime;
			Sample.Location = Loc;
			Sample.Rotation = Rot;
			Recording.Add(Sample);

			const int32 Index = GetShotIndexAt(NextSampleTime);
			FVector RefLoc;
			FRotator RefRot;
			if (Index != INDEX_NONE && GetReferencePose(NextSampleTime, RefLoc, RefRot))
			{
				FDCRefShot& Shot = Shots[Index];
				float Framing = 0.f, PosErr = 0.f, AngErr = 0.f;
				const float Score01 = ScoreSample(Shot, RefLoc, RefRot, Loc, Rot, Framing, PosErr, AngErr);
				++Shot.Samples;
				Shot.SumScore += Score01;
				Shot.SumFraming += Framing;
				Shot.SumPositionError += PosErr;
				Shot.SumAngleError += AngErr;
				LiveMatch = FMath::Lerp(LiveMatch, Score01, 0.25f);
			}
			NextSampleTime += Step;
		}
	}

	if (NowTime >= TakeDuration)
	{
		EndTake();
	}
}

void ADCReplicaDirector::EndTake()
{
	if (Phase != EDCPhase::Filming)
	{
		return; // end-take fires once
	}
	Phase = EDCPhase::Results;

	float Sum = 0.f;
	int32 Samples = 0;
	for (const FDCRefShot& Shot : Shots)
	{
		Sum += Shot.SumScore;
		Samples += Shot.Samples;
	}
	OverallScore = Samples > 0 ? Sum / Samples : 0.f;
	BestOverall = FMath::Max(BestOverall, OverallScore);
	Score = FMath::RoundToInt(OverallScore * 100.f);
	bApproved = OverallScore >= 0.4f;

	if (ADCFreeCharacter* Player = GetPlayer())
	{
		Player->ReleaseInputs();
	}
	Flash(TEXT("CUT! Let's see how close you got."), 2.5f);
}

void ADCReplicaDirector::ResetScene()
{
	Super::ResetScene();
	Recording.Reset();
	for (FDCRefShot& Shot : Shots)
	{
		Shot.ResetEvaluation();
	}
	NextSampleTime = 0.f;
	LiveMatch = 0.f;
	Replay = EDCReplay::None;
}

FString ADCReplicaDirector::GradeFor(float Score01)
{
	if (Score01 >= 0.9f) return TEXT("S");
	if (Score01 >= 0.75f) return TEXT("A");
	if (Score01 >= 0.6f) return TEXT("B");
	if (Score01 >= 0.4f) return TEXT("C");
	return TEXT("D");
}

FString ADCReplicaDirector::GetShotTip(const FDCRefShot& Shot) const
{
	if (Shot.Samples == 0)
	{
		return TEXT("Not filmed");
	}
	if (Shot.GetPositionError() > 300.f)
	{
		return FString::Printf(TEXT("Camera was %.1f m from where the reference stood"), Shot.GetPositionError() / 100.f);
	}
	if (Shot.GetAngleError() > 20.f)
	{
		return FString::Printf(TEXT("Camera pointed %.0f degrees away from the reference"), Shot.GetAngleError());
	}
	if (Shot.GetFraming() < 0.6f)
	{
		return TEXT("Right area, but the subjects weren't framed like the reference");
	}
	return Shot.GetScore() >= 0.85f ? TEXT("Nearly identical - beautiful") : TEXT("Close - refine height and timing");
}

FString ADCReplicaDirector::GetBrief() const
{
	return FString::Printf(TEXT("Reproduce the reference film: %d shots in %.0f seconds. V to rewatch it."), Shots.Num(), TakeDuration);
}

// ---------------------------------------------------------------------------------------------
// Commands

void ADCReplicaDirector::HandleStartTake()
{
	if (IsShowingFilm())
	{
		StopFilm(); // Enter skips the film
		return;
	}
	Super::HandleStartTake();
}

void ADCReplicaDirector::HandleRestart()
{
	if (Phase == EDCPhase::Watching)
	{
		StartFilm(EDCReplay::Reference); // R replays the reference from the top
		return;
	}
	if (Replay != EDCReplay::None)
	{
		StopFilm();
		return;
	}
	Super::HandleRestart();
}

void ADCReplicaDirector::HandleWatchReference()
{
	if (Phase == EDCPhase::Preparation || (Phase == EDCPhase::Results && Replay == EDCReplay::None))
	{
		StartFilm(EDCReplay::Reference);
	}
}

void ADCReplicaDirector::HandleWatchTake()
{
	if (Phase == EDCPhase::Results && Replay == EDCReplay::None)
	{
		StartFilm(EDCReplay::Take);
	}
}

void ADCReplicaDirector::HandleInteract()
{
	if (!IsShowingFilm())
	{
		Super::HandleInteract();
	}
}

void ADCReplicaDirector::UpdatePrepPreview()
{
	Super::UpdatePrepPreview();

	ADCFreeCharacter* Player = GetPlayer();
	const bool bShow = Tool == EDCRigTool::Spray && bPreviewHit && Player && !Player->IsOnRig();
	SprayGhost->SetActorHiddenInGame(!bShow);
	if (bShow)
	{
		SprayGhost->Setup(0, PreviewSurface, PreviewNormal);
		SprayGhost->SetTint(SprayMarks.Num() < MaxSprayMarks ? FLinearColor(0.2f, 1.f, 0.4f) : FLinearColor(1.f, 0.15f, 0.1f));
	}
}

void ADCReplicaDirector::HandlePrimaryPressed()
{
	if (Phase != EDCPhase::Preparation || Tool != EDCRigTool::Spray)
	{
		Super::HandlePrimaryPressed();
		return;
	}
	if (!bPreviewHit)
	{
		Flash(TEXT("Aim at a surface within 15 m"));
		return;
	}
	if (SprayMarks.Num() >= MaxSprayMarks)
	{
		Flash(FString::Printf(TEXT("Out of paint - %d marks max. Right Mouse removes one."), MaxSprayMarks));
		return;
	}
	int32 Number = 1;
	for (const ADCSprayMark* Mark : SprayMarks)
	{
		Number = FMath::Max(Number, Mark->GetNumber() + 1);
	}
	FActorSpawnParameters Params;
	Params.Owner = this;
	ADCSprayMark* Mark = GetWorld()->SpawnActor<ADCSprayMark>(ADCSprayMark::StaticClass(), FTransform::Identity, Params);
	Mark->Setup(Number, PreviewSurface, PreviewNormal);
	SprayMarks.Add(Mark);
}

void ADCReplicaDirector::HandleSecondaryPressed()
{
	if (Phase != EDCPhase::Preparation || Tool != EDCRigTool::Spray)
	{
		Super::HandleSecondaryPressed();
		return;
	}
	ADCSprayMark* Best = nullptr;
	float BestDist = SprayRemoveRange;
	for (ADCSprayMark* Mark : SprayMarks)
	{
		const float D = FVector::Dist(Mark->GetActorLocation(), PreviewSurface);
		if (bPreviewHit && D < BestDist)
		{
			BestDist = D;
			Best = Mark;
		}
	}
	if (!Best)
	{
		Flash(TEXT("Aim at a spray mark to scrub it off"), 1.5f);
		return;
	}
	SprayMarks.Remove(Best);
	Best->Destroy();
}

void ADCReplicaDirector::HandleClearRigs()
{
	const bool bPrep = Phase == EDCPhase::Preparation;
	Super::HandleClearRigs();
	if (bPrep)
	{
		for (ADCSprayMark* Mark : SprayMarks)
		{
			Mark->Destroy();
		}
		SprayMarks.Reset();
	}
}
