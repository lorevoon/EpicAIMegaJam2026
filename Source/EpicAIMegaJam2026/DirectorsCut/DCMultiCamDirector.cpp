// Director's Cut - multi-camera take director.

#include "DCMultiCamDirector.h"
#include "DCBeat.h"
#include "DCCinemaReplay.h"
#include "DCMultiCam.h"
#include "DCMultiCamCharacter.h"
#include "DCPerformer.h"
#include "DCRail.h"
#include "DCSceneMarks.h"
#include "Algo/BinarySearch.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/AudioComponent.h"
#include "Sound/AmbientSound.h"
#include "Sound/AudioSettings.h"
#include "Engine/World.h"
#include "Engine/LevelStreaming.h"
#include "Engine/LevelStreamingDynamic.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float PlaceRange = 1500.f;
	constexpr float PickRadius = 80.f;
	constexpr float FaceDot = 0.25f;
	constexpr float LookDegreesPerUnit = 2.5f;

	/** The dolly camera riding Rail, if one has been created for it yet. */
	ADCMultiCam* FindCameraOnRail(const TArray<TObjectPtr<ADCMultiCam>>& Cameras, const ADCRail* Rail)
	{
		for (ADCMultiCam* Cam : Cameras)
		{
			if (Cam && Rail && Cam->GetRail() == Rail)
			{
				return Cam;
			}
		}
		return nullptr;
	}
}

ADCMultiCamDirector::ADCMultiCamDirector()
{
	static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> FrameMPC(TEXT("/Game/DirectorsCut/Materials/MPC_DCFrame.MPC_DCFrame"));
	FrameParams = FrameMPC.Object;
	PrimaryActorTick.bCanEverTick = true;
	// Evaluate after the player has re-aimed the live camera this frame.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Renders the reference cut for the preparation inset; captured by hand only while that inset is showing.
	ReferenceCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("ReferenceInsetCapture"));
	ReferenceCapture->SetupAttachment(RootComponent);
	ReferenceCapture->bCaptureEveryFrame = false;
	ReferenceCapture->bCaptureOnMovement = false;
	ReferenceCapture->bAlwaysPersistRenderingState = true; // keeps exposure history between captures
	ReferenceCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
}

void ADCMultiCamDirector::BeginPlay()
{
	Super::BeginPlay();

	GatherScene();
	if (!IsUnlimited())
	{
		Cameras.SetNum(MaxCameras);
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Ghost = GetWorld()->SpawnActor<ADCMultiCam>(ADCMultiCam::StaticClass(), FTransform::Identity, Params);
	Ghost->SetActorHiddenInGame(true);
	PlaybackCamera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform::Identity, Params);
	// The film is FilmAspect wide; the engine letterboxes (or pillarboxes) the playback to it on any screen.
	PlaybackCamera->GetCameraComponent()->SetAspectRatio(DCFrame::FilmAspect);
	PlaybackCamera->GetCameraComponent()->SetConstraintAspectRatio(true);

	ResetScene();
	Flash(IsUnlimited() ? FString(TEXT("Place as many cameras as you like. T switches between static and crane."))
		: FString::Printf(TEXT("Place up to %d cameras. WASD adjusts position nearby; mouse aims."), MaxCameras), 7.f);
}

void ADCMultiCamDirector::GatherScene()
{
	UWorld* World = GetWorld();
	TArray<FString> Problems;
	SceneDialogue.Reset();
	for (TActorIterator<AAmbientSound> It(World); It; ++It)
	{
		if (It->ActorHasTag(TEXT("DCSceneDialogue")))
		{
			SceneDialogue = It->GetAudioComponent();
			SceneDialogue->Stop();
			break;
		}
	}

	Performers.Reset();
	for (TActorIterator<ADCPerformer> It(World); It; ++It)
	{
		It->BuildKeys();
		Performers.Add(*It);
	}

	Clapperboard = nullptr;
	for (TActorIterator<ADCClapperboard> It(World); It; ++It)
	{
		Clapperboard = *It;
		break;
	}

	StartTransform = ADCStartMark::FindStartTransform(World);

	Beats.Reset();
	for (TActorIterator<ADCBeat> It(World); It; ++It)
	{
		ADCBeat* Beat = *It;
		FDCMultiCamBeat State;
		State.Beat = Beat;
		for (const FDCAnchorRef& Ref : Beat->Anchors)
		{
			if (Ref.Performer)
			{
				FDCAnchor A;
				A.Component = Ref.Performer->GetAnchor(Ref.Part);
				A.Owner = Ref.Performer;
				A.bMustSeeFace = Ref.bMustSeeFace;
				State.Anchors.Add(A);
			}
		}
		if (State.Anchors.Num() == 0)
		{
			Problems.Add(FString::Printf(TEXT("Beat %s has no anchors"), *Beat->BeatId));
			continue;
		}
		if (Beat->GetCaptureStartTime() >= Beat->WindowEnd || Beat->RequiredHold > Beat->WindowEnd - Beat->GetCaptureStartTime())
		{
			Problems.Add(FString::Printf(TEXT("Beat %s capture window after the scored pose is shorter than its hold time"), *Beat->BeatId));
			continue;
		}
		Beats.Add(State);
	}
	Beats.StableSort([](const FDCMultiCamBeat& A, const FDCMultiCamBeat& B) { return A.Beat->WindowStart < B.Beat->WindowStart; });

	if (Beats.Num() == 0 && Problems.Num() == 0)
	{
		Problems.Add(TEXT("No story beats in this level - place DCBeat actors and assign their capture anchors"));
	}
	SetupProblem = FString::Join(Problems, TEXT("\n"));
}

// ---------------------------------------------------------------------------------------------
// Phases

void ADCMultiCamDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	for (ADCMultiCam* Cam : Cameras)
	{
		if (Cam)
		{
			Cam->UpdateZoom(DeltaSeconds);
			Cam->UpdateStabilizer(DeltaSeconds);
		}
	}

	UpdateCameraSpots();
	UpdateFrameParams();

	switch (Phase)
	{
	case EDCPhase::Preparation:
		if (bRehearsalPlaying && TakeDuration > KINDA_SMALL_NUMBER)
		{
			SceneTime = FMath::Fmod(SceneTime + FMath::Max(0.f, DeltaSeconds) * RehearsalSpeed, TakeDuration);
			for (ADCPerformer* Performer : Performers)
			{
				Performer->Evaluate(SceneTime);
			}
		}
		UpdatePrepPreview();
		UpdateReferenceInset();
		break;

	case EDCPhase::Countdown:
		if (!EnsureLiveCamera())
		{
			return;
		}
		CountdownRemaining -= DeltaSeconds;
		UpdateReferenceInset();
		if (CountdownRemaining <= 0.f)
		{
			BeginFilming();
		}
		break;

	case EDCPhase::Filming:
	{
		if (!EnsureLiveCamera())
		{
			return;
		}
		const float Prev = SceneTime;
		SceneTime = FMath::Min(SceneTime + DeltaSeconds, TakeDuration);
		for (ADCPerformer* Performer : Performers)
		{
			Performer->Evaluate(SceneTime);
		}
		UpdateReferenceInset(); // before EvaluateBeats, which can end the take
		EvaluateBeats(Prev, SceneTime);
		break;
	}

	case EDCPhase::Review:
		TickPremiere(DeltaSeconds);
		break;

	case EDCPhase::Storyboard:
		TickReferenceCut(DeltaSeconds);
		break;

	default:
		break;
	}
	UpdateSceneDialogue(DeltaSeconds);
}

void ADCMultiCamDirector::UpdateSceneDialogue(float DeltaSeconds)
{
	UAudioComponent* Audio = SceneDialogue.Get();
	if (!Audio || !Audio->Sound) return;
	float Position = SceneTime;
	float Rate = 1.f;
	bool bPlaying = false;
	FVector Location;
	FQuat Rotation;
	float FOV;
	if (Phase == EDCPhase::Preparation)
	{
		bPlaying = bRehearsalPlaying;
		Rate = RehearsalSpeed;
	}
	else if (Phase == EDCPhase::Filming)
	{
		// Pausing recording omits footage, but the actors and their speech continue on set.
		bPlaying = SceneTime < TakeDuration;
	}
	else if (Phase == EDCPhase::Storyboard)
	{
		bPlaying = bReferencePlaying && !bReferencePaused
			&& GetFilmView(ReferenceCut, ReferenceTime, Location, Rotation, FOV, Position);
		// The soundtrack must run at the same scene-time rate as the slowed image.
		const int32 Next = Algo::UpperBoundBy(ReferenceCut, ReferenceTime, &FDCTakeSample::FilmTime);
		if (Next > 0 && ReferenceCut.IsValidIndex(Next))
		{
			const FDCTakeSample& A = ReferenceCut[Next - 1];
			const FDCTakeSample& B = ReferenceCut[Next];
			if (B.FilmTime > A.FilmTime) Rate = (B.Time - A.Time) / (B.FilmTime - A.FilmTime);
		}
	}
	else if (Phase == EDCPhase::Review)
	{
		bPlaying = !bPremierePaused && !IsFilmFinished() && !(Cinema && Cinema->IsOpening())
			&& GetFilmView(Recording, SceneTime, Location, Rotation, FOV, Position);
	}
	// Beyond the device pitch range, keep fast rehearsal silent instead of playing out of sync.
	if (!bPlaying || Rate > GetDefault<UAudioSettings>()->GlobalMaxPitchScale
		|| Position < 0.f || Position >= TakeDuration)
	{
		Audio->Stop();
		DialoguePosition = -1.f;
		return;
	}
	// Ignore floating-point slope noise in densely sampled reference timestamps.
	if (FMath::IsNearlyEqual(Rate, DialogueRate, .005f)) Rate = DialogueRate;
	// Seek after a scrub, loop, retry, phase switch, speed change or a cut over omitted footage.
	const float Expected = DialoguePosition + FMath::Max(0.f, DeltaSeconds) * DialogueRate;
	const bool bSeek = DialoguePosition < 0.f || DialoguePhase != Phase
		|| !FMath::IsNearlyEqual(Rate, DialogueRate, .005f)
		|| FMath::Abs(Position - Expected) > .08f;
	Audio->SetPitchMultiplier(Rate);
	if (bSeek || !Audio->IsPlaying())
	{
		Audio->Stop();
		Audio->Play(Position);
	}
	DialoguePosition = Position;
	DialogueRate = Rate;
	DialoguePhase = Phase;
}

bool ADCMultiCamDirector::EnsureLiveCamera()
{
	// There is no handheld shooting: from Action! until the results, the view always belongs to a placed camera.
	APlayerController* PC = GetPC();
	if (!PC)
	{
		return false;
	}
	if (!GetCameraSlot(ViewingSlot))
	{
		const int32 Fallback = Cameras.IndexOfByPredicate([](const TObjectPtr<ADCMultiCam>& Cam) { return Cam != nullptr; });
		if (Fallback == INDEX_NONE)
		{
			HandleRestart();
			Flash(TEXT("Every shot needs a camera - place one before calling Action!"));
			return false;
		}
		ViewCamera(Fallback);
	}
	// Re-assert the live feed if anything handed the view back to the director's own eyes.
	if (PC->GetViewTarget() != Cameras[ViewingSlot])
	{
		PC->SetViewTargetWithBlend(Cameras[ViewingSlot], 0.f);
	}
	return true;
}

void ADCMultiCamDirector::HandleStartTake()
{
	if (Phase == EDCPhase::Review)
	{
		// Enter skips to the evaluation, then goes back to preparation.
		if (IsFilmFinished())
		{
			HandleRestart();
		}
		else
		{
			FinishPremiere();
		}
		return;
	}
	if (Phase != EDCPhase::Preparation)
	{
		return;
	}
	if (Beats.Num() == 0)
	{
		Flash(TEXT("No valid beats in this level - see the setup warning"));
		return;
	}
	FinishPendingRail(true);
	if (GetCameraCount() == 0)
	{
		Flash(TEXT("Place at least one camera first"));
		return;
	}
	ResetScene();
	Ghost->SetActorHiddenInGame(true);
	Phase = EDCPhase::Countdown;
	CountdownRemaining = CountdownDuration;
	CutCount = 0;
	ApplyStabilizer();

	// From Action! until back in preparation, rigs are ghosted inside every shot. Go live on the lowest-numbered camera.
	SetRigsFrameMasked(true);
	for (int32 i = 0; i < Cameras.Num(); ++i)
	{
		if (Cameras[i])
		{
			ViewCamera(i);
			break;
		}
	}
}

void ADCMultiCamDirector::BeginFilming()
{
	Phase = EDCPhase::Filming;
	SceneTime = 0.f;
	Flash(TEXT("ACTION! Cut between cameras with the number keys."), 2.f);
}

void ADCMultiCamDirector::HandleRestart()
{
	ResetScene();
	Flash(TEXT("Back to preparation. Cameras and notebook are kept."));
}

void ADCMultiCamDirector::ResetScene()
{
	if (SceneDialogue.IsValid()) SceneDialogue->Stop();
	DialoguePosition = -1.f;
	bRehearsalPlaying = true;
	RehearsalSpeed = 1.f;
	if (Cinema) { Cinema->EndScreening(); Cinema->ClearMissionOutcomes(); }
	ReplayMissionOutcomes.Reset();
	SetReviewEnvironment(false);
	Phase = EDCPhase::Preparation;
	SceneTime = 0.f;
	CountdownRemaining = 0.f;
	Match = FDCMatch();

	for (FDCMultiCamBeat& State : Beats)
	{
		State.Status = EDCShotStatus::Pending;
		State.Hold = 0.f;
		State.LastProgressTime = -100.f;
		State.CapturedBy = 0;
	}
	for (ADCPerformer* Performer : Performers)
	{
		Performer->Evaluate(0.f);
	}
	// Every take starts from the aim and zoom chosen during preparation.
	for (ADCMultiCam* Cam : Cameras)
	{
		if (Cam)
		{
			Cam->RestoreHome();
		}
	}
	ViewWalking();
	SetRigsFrameMasked(false);
	SetRigsHidden(false);
	ApplyStabilizer();
	Recording.Reset();
	bPremierePaused = false;
	bRecordingPauseHeld = false;
	PausedSeconds = 0.f;
	bJumpCutPending = false;
	if (ADCMultiCamCharacter* Player = GetPlayer())
	{
		Player->ResetToStart(StartTransform);
	}
}

void ADCMultiCamDirector::EndTake()
{
	if (Phase != EDCPhase::Filming)
	{
		return; // end-take fires once
	}

	Phase = EDCPhase::Review;
	for (FDCMultiCamBeat& State : Beats)
	{
		if (State.Status == EDCShotStatus::Pending)
		{
			State.Status = EDCShotStatus::Missed;
		}
	}
	// The grade: how closely the film matches the reference cut.
	ComputeMatch();
	BestMatch = FMath::Max(BestMatch, Match.Overall);
	StartPremiere();
}

// ---------------------------------------------------------------------------------------------
// Recording and premiere: the take is re-shot from the recorded camera poses (the scene is driven
// purely by the scene clock, so replaying the clock replays the performance exactly).

void ADCMultiCamDirector::RecordSample(float DeltaSeconds)
{
	if (IsRecordingPaused())
	{
		// Paused footage never makes it into the film; the next frame recorded is a jump cut.
		PausedSeconds += DeltaSeconds;
		bJumpCutPending = true;
		return;
	}
	const ADCMultiCam* Live = GetCameraSlot(ViewingSlot);
	if (!Live)
	{
		return;
	}
	const UCameraComponent* Lens = Live->GetCamera();
	FDCTakeSample Sample;
	Sample.Time = SceneTime;
	Sample.FilmTime = SceneTime - PausedSeconds;
	Sample.bJumpCut = bJumpCutPending;
	bJumpCutPending = false;
	Sample.Slot = ViewingSlot;
	Sample.Location = Lens->GetComponentLocation();
	Sample.Rotation = Lens->GetComponentRotation();
	Sample.FilmFOV = DCFrame::CaptureFOV(Lens->FieldOfView, GetViewAspect());
	Recording.Add(Sample);
}

void ADCMultiCamDirector::StartPremiere()
{
	APlayerController* PC = GetPC();
	Phase = EDCPhase::Review;
	SetReviewEnvironment(true);
	SetRigsHidden(true);
	if (!Cinema)
	{
		if (!CinemaLevel.IsNull())
		{
			bool bLoaded = false;
			ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(this, CinemaLevel, FVector::ZeroVector, FRotator::ZeroRotator, bLoaded);
			GetWorld()->FlushLevelStreaming(EFlushLevelStreamingType::Full);
			for (TActorIterator<ADCCinemaReplay> It(GetWorld()); It; ++It) { Cinema = *It; break; }
		}
		// Legacy/custom maps can still premiere if their presentation asset is absent.
		if (!Cinema) Cinema = GetWorld()->SpawnActor<ADCCinemaReplay>();
	}
	Cinema->BeginScreening(this);
	for (const auto& Outcome : ReplayMissionOutcomes) Cinema->SetMissionOutcome(Outcome.Key, Outcome.Value);
	if (Recording.Num() == 0 || !PC)
	{
		FinishPremiere(); // nothing was filmed: straight to the evaluation
		return;
	}
	bPremierePaused = false;
	SceneTime = 0.f;
	SetRigsHidden(true);
	ShowPremiereFrame(0.f);
	PC->SetViewTargetWithBlend(PlaybackCamera, 0.f);
}

void ADCMultiCamDirector::TickPremiere(float DeltaSeconds)
{
	if (Cinema && Cinema->IsOpening()) return;
	if (bPremierePaused)
	{
		return;
	}
	SceneTime = FMath::Min(SceneTime + DeltaSeconds, GetPremiereLength());
	ShowPremiereFrame(SceneTime);
	if (SceneTime >= GetPremiereLength())
	{
		FinishPremiere();
	}
}

void ADCMultiCamDirector::ShowPremiereFrame(float Time)
{
	ShowFilmFrame(Recording, Time);
	if (Cinema && Recording.Num() > 0) Cinema->ProjectFrame(PlaybackCamera);
}

bool ADCMultiCamDirector::GetFilmView(const TArray<FDCTakeSample>& Film, float Time, FVector& OutLocation, FQuat& OutRotation, float& OutFOV, float& OutScenePoint) const
{
	if (Film.Num() == 0)
	{
		return false;
	}
	// Time is film time; paused stretches of the take are not in the film at all.
	const int32 Next = Algo::UpperBoundBy(Film, Time, &FDCTakeSample::FilmTime);
	const FDCTakeSample& A = Film[FMath::Clamp(Next - 1, 0, Film.Num() - 1)];
	const FDCTakeSample& B = Film.IsValidIndex(Next) ? Film[Next] : A;
	// Blend between frames of the same camera; a change of camera or a recording pause is a hard cut.
	const bool bBlend = B.Slot == A.Slot && !B.bJumpCut && B.FilmTime > A.FilmTime;
	const float Alpha = bBlend ? FMath::Clamp((Time - A.FilmTime) / (B.FilmTime - A.FilmTime), 0.f, 1.f) : 0.f;
	// A camera cut is spatial, not temporal. Continue the scene-time interpolation
	// across it so a slowed reference shot does not briefly return to normal speed.
	const bool bTimeBlend = !B.bJumpCut && B.FilmTime > A.FilmTime;
	const float TimeAlpha = bTimeBlend ? FMath::Clamp((Time - A.FilmTime) / (B.FilmTime - A.FilmTime), 0.f, 1.f) : 0.f;
	OutScenePoint = bTimeBlend ? FMath::Lerp(A.Time, B.Time, TimeAlpha) : A.Time + FMath::Max(0.f, Time - A.FilmTime);
	OutLocation = FMath::Lerp(A.Location, B.Location, Alpha);
	OutRotation = FQuat::Slerp(A.Rotation.Quaternion(), B.Rotation.Quaternion(), Alpha);
	// The film is only what was inside the capture frame, so the lens shows just that part.
	OutFOV = FMath::Lerp(A.FilmFOV, B.FilmFOV, Alpha);
	return true;
}

void ADCMultiCamDirector::ShowFilmFrame(const TArray<FDCTakeSample>& Film, float Time)
{
	FVector Location;
	FQuat Rotation;
	float FOV = 0.f, ScenePoint = 0.f;
	if (!GetFilmView(Film, Time, Location, Rotation, FOV, ScenePoint))
	{
		return;
	}
	EvaluatePerformers(ScenePoint);
	PlaybackCamera->SetActorLocationAndRotation(Location, Rotation);
	PlaybackCamera->GetCameraComponent()->SetFieldOfView(FOV);
}

void ADCMultiCamDirector::SetReplayMissionOutcome(FName BeatId, bool bSucceeded)
{
	ReplayMissionOutcomes.Add(BeatId, bSucceeded);
	if (Cinema) Cinema->SetMissionOutcome(BeatId, bSucceeded);
}

void ADCMultiCamDirector::FinishPremiere()
{
	// The evaluation sits over the film's final frame; rigs stay hidden until back in preparation.
	SceneTime = GetPremiereLength();
	ShowPremiereFrame(SceneTime);
	bPremierePaused = true;
}

void ADCMultiCamDirector::HandleScrub(float DeltaSeconds)
{
	if (Phase == EDCPhase::Preparation)
	{
		bRehearsalPlaying = false;
		SceneTime = FMath::Clamp(SceneTime + DeltaSeconds, 0.f, TakeDuration);
		for (ADCPerformer* Performer : Performers) Performer->Evaluate(SceneTime);
		return;
	}
	if (Phase == EDCPhase::Review)
	{
		SceneTime = FMath::Clamp(SceneTime + DeltaSeconds, 0.f, GetPremiereLength());
		ShowPremiereFrame(SceneTime);
	}
	else if (Phase == EDCPhase::Storyboard)
	{
		ScrubReferenceCut(DeltaSeconds);
	}
}

void ADCMultiCamDirector::HandleTransportToggle()
{
	if (Phase == EDCPhase::Storyboard)
	{
		ToggleReferenceCut(); // pause/resume, or watch it again from the storyboard page
		return;
	}
	if (Phase != EDCPhase::Review)
	{
		return;
	}
	if (IsFilmFinished())
	{
		SceneTime = 0.f; // watch the film again
		if (Cinema) Cinema->BeginScreening(this);
		ShowPremiereFrame(SceneTime);
		bPremierePaused = false;
	}
	else
	{
		bPremierePaused = !bPremierePaused;
	}
}

void ADCMultiCamDirector::HandleRehearsalToggle()
{
	if (Phase == EDCPhase::Preparation)
	{
		bRehearsalPlaying = !bRehearsalPlaying;
	}
}

void ADCMultiCamDirector::HandleRehearsalSpeed(int32 Direction)
{
	if (Phase == EDCPhase::Preparation && Direction != 0)
	{
		RehearsalSpeed = FMath::Clamp(RehearsalSpeed * (Direction > 0 ? 2.f : 0.5f), 0.5f, 4.f);
	}
}

void ADCMultiCamDirector::SetRigsHidden(bool bHide)
{
	// Include unfinished rails as well as rails attached to a placed camera.
	for (TActorIterator<ADCRail> It(GetWorld()); It; ++It)
	{
		It->SetActorHiddenInGame(bHide);
	}
	for (ADCMultiCam* Cam : Cameras)
	{
		if (Cam)
		{
			Cam->SetRigHidden(bHide);
		}
	}
}

void ADCMultiCamDirector::SetReviewEnvironment(bool bReview)
{
	if (StudioEnvironmentLevel.IsNone() || RealEnvironmentLevel.IsNone())
	{
		return; // Other acts retain their existing presentation.
	}
	ULevelStreaming* Studio = UGameplayStatics::GetStreamingLevel(this, StudioEnvironmentLevel);
	ULevelStreaming* Real = UGameplayStatics::GetStreamingLevel(this, RealEnvironmentLevel);
	if (!Studio || !Real)
	{
		UE_LOG(LogTemp, Error, TEXT("Director replay requires both environment sublevels: %s / %s"),
			*StudioEnvironmentLevel.ToString(), *RealEnvironmentLevel.ToString());
		return;
	}
	// Load both during preparation, then change only visibility. Never travel to a new world:
	// that would discard the recording, player cameras, beat references and deterministic clock.
	Studio->SetShouldBeLoaded(true);
	Real->SetShouldBeLoaded(true);
	Studio->SetShouldBeVisible(!bReview);
	Real->SetShouldBeVisible(bReview);
	GetWorld()->FlushLevelStreaming(EFlushLevelStreamingType::Full);
	if (bReview)
	{
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(TEXT("DC_SetOnly")))
			{
				SetOnlyHiddenStates.FindOrAdd(*It, It->IsHidden());
				It->SetActorHiddenInGame(true);
			}
		}
	}
	else
	{
		for (const auto& Entry : SetOnlyHiddenStates)
		{
			if (AActor* Actor = Entry.Key.Get())
			{
				Actor->SetActorHiddenInGame(Entry.Value);
			}
		}
		SetOnlyHiddenStates.Reset();
	}
}

// ---------------------------------------------------------------------------------------------
// Capture: only the live camera records.

void ADCMultiCamDirector::EvaluateBeats(float PrevTime, float NowTime)
{
	// Record what went out live this frame (before the take can end below).
	RecordSample(NowTime - PrevTime);

	const ADCMultiCam* Live = GetCameraSlot(ViewingSlot);

	for (FDCMultiCamBeat& State : Beats)
	{
		if (State.Status != EDCShotStatus::Pending)
		{
			continue;
		}
		const ADCBeat* Beat = State.Beat;
		const float Eligible = FMath::Max(0.f, FMath::Min(NowTime, Beat->WindowEnd) - FMath::Max(PrevTime, Beat->GetCaptureStartTime()));
		if (Live && Eligible > 0.f && !IsRecordingPaused() && IsFramed(State, Live))
		{
			State.Hold += Eligible;
			State.LastProgressTime = NowTime;
			if (!State.bSpotted)
			{
				State.bSpotted = true;
				Flash(FString::Printf(TEXT("New discovery: %s"), *Beat->DisplayName));
			}
		}
		else
		{
			// Cutting away breaks the hold, just like losing the framing.
			State.Hold = 0.f;
		}

		if (State.Hold >= Beat->RequiredHold)
		{
			State.Status = EDCShotStatus::Captured;
			State.bEverCaptured = true;
			State.CapturedBy = Live ? Live->GetNumber() : 0;
			State.CapturedFilmTime = SceneTime - PausedSeconds;
			Flash(FString::Printf(TEXT("Captured on CAM %d: %s"), State.CapturedBy, *Beat->DisplayName));
		}
		else if (NowTime > Beat->WindowEnd)
		{
			State.Status = EDCShotStatus::Missed;
		}
	}

	if (NowTime >= TakeDuration)
	{
		EndTake();
	}
}

bool ADCMultiCamDirector::IsFramed(const FDCMultiCamBeat& State, const ADCMultiCam* Cam) const
{
	const FTransform CamXf = Cam->GetCamera()->GetComponentTransform();
	const FVector CamLoc = CamXf.GetLocation();
	const float HalfTan = FMath::Tan(FMath::DegreesToRadians(Cam->GetFOV() * 0.5f));
	const float Aspect = GetViewAspect();
	const FVector2D Frame = DCFrame::CaptureExtent(Aspect);
	// Zooming in extends reach in proportion to the magnification relative to the default lens.
	const float Reach = bZoomExtendsRange ? FMath::Tan(FMath::DegreesToRadians(ADCMultiCam::DefaultFOV * 0.5f)) / HalfTan : 1.f;
	const ADCBeat* Beat = State.Beat;

	for (const FDCAnchor& A : State.Anchors)
	{
		const FVector P = A.Component->GetComponentLocation();
		// Project with the live camera's own lens (horizontal FOV), independent of the view-target blend.
		const FVector Local = CamXf.InverseTransformPositionNoScale(P);
		if (Local.X <= 1.f)
		{
			return false;
		}
		const float NdcX = (Local.Y / Local.X) / HalfTan;
		const float NdcY = (Local.Z / Local.X) / (HalfTan / Aspect);
		if (FMath::Abs(NdcX) > Frame.X || FMath::Abs(NdcY) > Frame.Y)
		{
			return false;
		}
		const float Dist = Local.Size();
		if (Dist < Beat->MinDistance || Dist > Beat->MaxDistance * FMath::Max(Reach, 1.f))
		{
			return false;
		}
		if (A.bMustSeeFace && FVector::DotProduct(A.Owner->GetActorForwardVector(), (CamLoc - P).GetSafeNormal()) < FaceDot)
		{
			return false;
		}
	}

	const FCollisionQueryParams Params = MakeTraceParams(SCENE_QUERY_STAT_NAME_ONLY(DCMultiCamVisibility));
	for (const FDCAnchor& A : State.Anchors)
	{
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, A.Component->GetComponentLocation(), ECC_Visibility, Params) && Hit.GetActor() != A.Owner)
		{
			return false;
		}
	}
	return true;
}

FCollisionQueryParams ADCMultiCamDirector::MakeTraceParams(const FName& Tag) const
{
	FCollisionQueryParams Params(Tag, false, GetPlayer());
	for (ADCMultiCam* Cam : Cameras)
	{
		if (Cam)
		{
			Params.AddIgnoredActor(Cam);
		}
	}
	Params.AddIgnoredActor(Ghost);
	return Params;
}

// ---------------------------------------------------------------------------------------------
// Camera views

void ADCMultiCamDirector::ViewCamera(int32 Slot)
{
	ADCMultiCam* Cam = GetCameraSlot(Slot);
	APlayerController* PC = GetPC();
	if (!Cam || !PC)
	{
		return;
	}
	for (ADCMultiCam* Other : Cameras)
	{
		if (Other)
		{
			Other->SetLive(Other == Cam);
		}
	}
	ViewingSlot = Slot;
	// The director decides what the player sees; don't let possession or respawns reset it to the walking view.
	PC->bAutoManageActiveCameraTarget = false;
	PC->SetViewTargetWithBlend(Cam, 0.f);
}

void ADCMultiCamDirector::ViewWalking()
{
	for (ADCMultiCam* Cam : Cameras)
	{
		if (Cam)
		{
			Cam->SetLive(false);
		}
	}
	ViewingSlot = INDEX_NONE;
	APlayerController* PC = GetPC();
	if (PC && PC->GetPawn())
	{
		PC->SetViewTargetWithBlend(PC->GetPawn(), 0.f);
	}
}

void ADCMultiCamDirector::SetRigsFrameMasked(bool bMasked)
{
	for (ADCMultiCam* Cam : Cameras)
	{
		if (Cam)
		{
			Cam->SetFrameMasked(bMasked);
		}
	}
}

void ADCMultiCamDirector::HandleSelectCamera(int32 Number)
{
	const int32 Slot = Number - 1;
	if (Slot < 0 || Slot >= (IsUnlimited() ? Cameras.Num() : MaxCameras))
	{
		return;
	}
	ADCMultiCam* Cam = GetCameraSlot(Slot);

	switch (Phase)
	{
	case EDCPhase::Preparation:
		if (!Cam)
		{
			Flash(FString::Printf(TEXT("CAM %d isn't placed yet"), Number), 1.5f);
		}
		else if (ViewingSlot == Slot)
		{
			ViewWalking();
		}
		else
		{
			ViewCamera(Slot);
		}
		break;

	case EDCPhase::Countdown:
	case EDCPhase::Filming:
		if (Cam && ViewingSlot != Slot)
		{
			ViewCamera(Slot);
			CutCount += Phase == EDCPhase::Filming ? 1 : 0;
		}
		break;

	default:
		break;
	}
}

bool ADCMultiCamDirector::HandleLook(float DeltaYaw, float DeltaPitch)
{
	ADCMultiCam* Cam = GetCameraSlot(ViewingSlot);
	if (!Cam || IsReviewing())
	{
		return Cam != nullptr;
	}
	// Turn slower when zoomed in so fine framing stays controllable.
	const float Scale = LookDegreesPerUnit * Cam->GetFOV() / ADCMultiCam::DefaultFOV;
	Cam->AddAim(DeltaYaw * Scale, DeltaPitch * Scale);
	if (Phase == EDCPhase::Preparation)
	{
		Cam->SaveHome();
	}
	return true;
}

void ADCMultiCamDirector::HandleWheel(float Delta)
{
	ADCMultiCam* Cam = GetCameraSlot(ViewingSlot);
	const bool bShooting = Phase == EDCPhase::Countdown || Phase == EDCPhase::Filming;
	if (Cam && (bShooting || Phase == EDCPhase::Preparation))
	{
		Cam->SetFOV(Cam->GetTargetFOV() - Delta * 5.f);
		if (Phase == EDCPhase::Preparation)
		{
			Cam->SaveHome();
		}
	}
	else if (Phase == EDCPhase::Preparation)
	{
		HandlePlacementHeight(Delta);
	}
	else if (Phase == EDCPhase::Review || Phase == EDCPhase::Storyboard)
	{
		HandleScrub(Delta * WheelScrubSeconds);
	}
}

void ADCMultiCamDirector::HandlePlacementHeight(float Notches)
{
	if (Phase == EDCPhase::Preparation && !IsViewingCamera())
	{
		PendingHeight = FMath::Clamp(PendingHeight + Notches * StandHeightStep, Tool == EDCCamTool::Crane ? 450.f : MinStandHeight, Tool == EDCCamTool::Crane ? 1200.f : MaxStandHeight);
	}
}

// ---------------------------------------------------------------------------------------------
// Preparation: placing cameras

void ADCMultiCamDirector::UpdatePrepPreview()
{
	ADCMultiCamCharacter* Player = GetPlayer();
	bPreviewHit = false;
	bPreviewValid = false;
	if (!Player || IsViewingCamera())
	{
		Ghost->SetActorHiddenInGame(true);
		return;
	}

	const FVector CamLoc = Player->GetCamera()->GetComponentLocation();
	const FVector End = CamLoc + Player->GetCamera()->GetForwardVector() * PlaceRange;
	FCollisionQueryParams Params = MakeTraceParams(SCENE_QUERY_STAT_NAME_ONLY(DCMultiCamPreview));
	for (ADCPerformer* Performer : Performers)
	{
		Params.AddIgnoredActor(Performer);
	}

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, End, ECC_Visibility, Params))
	{
		bPreviewHit = true;
		PreviewSurface = Hit.ImpactPoint;
		// Floors get a stand at the chosen height; walls and ceilings get a short bracket.
		PreviewPoint = Hit.ImpactNormal.Z > 0.7f ? Hit.ImpactPoint + FVector(0.f, 0.f, PendingHeight) : Hit.ImpactPoint + Hit.ImpactNormal * WallStandoff;
		FHitResult Block;
		const bool bRoom = !GetWorld()->LineTraceSingleByChannel(Block, Hit.ImpactPoint + Hit.ImpactNormal * 2.f, PreviewPoint, ECC_Visibility, Params);
		bPreviewValid = bRoom && Hit.ImpactNormal.Z > 0.7f && HasCameraBudget();
	}

	Ghost->SetActorHiddenInGame(!bPreviewHit);
	if (bPreviewHit)
	{
		Ghost->SetActorLocation(PreviewPoint);
		Ghost->Setup(0, PreviewSurface, Tool == EDCCamTool::Crane ? EDCCamRig::Crane : EDCCamRig::Static);
		Ghost->SetAim(FRotator(Tool == EDCCamTool::Crane ? -60.f : 0.f, Player->GetControlRotation().Yaw, 0.f));
		Ghost->SetTint(bPreviewValid ? FLinearColor(0.2f, 1.f, 0.4f) : FLinearColor(1.f, 0.15f, 0.1f));
	}
}

void ADCMultiCamDirector::HandlePrimaryPressed()
{
	if (Phase == EDCPhase::Countdown || Phase == EDCPhase::Filming)
	{
		// Holding left click during the take pauses the recording until it's released.
		bRecordingPauseHeld = true;
		return;
	}
	if (Phase != EDCPhase::Preparation || IsViewingCamera())
	{
		return;
	}
	if (!PendingRail && !HasCameraBudget())
	{
		Flash(FString::Printf(TEXT("All %d cameras are placed - Right Mouse removes one"), MaxCameras));
		return;
	}
	if (!bPreviewHit || !bPreviewValid)
	{
		Flash(!bPreviewHit ? TEXT("Aim at clear ground within 15 m") : TEXT("No room for a camera there"));
		return;
	}

	const int32 Slot = FindFreeSlot();
	ADCMultiCam* Cam = SpawnCamera(PreviewPoint);
	Cam->Setup(Slot + 1, PreviewSurface, Tool == EDCCamTool::Crane ? EDCCamRig::Crane : EDCCamRig::Static);
	Cam->SetAim(FRotator(Tool == EDCCamTool::Crane ? -60.f : 0.f, Cam->GetAim().Yaw, 0.f));
	Cam->SaveHome();
	Cameras[Slot] = Cam;
	Flash(FString::Printf(TEXT("CAM %d placed - look through it with %s to aim"), Slot + 1,
		Slot < 9 ? *FString::Printf(TEXT("%d"), Slot + 1) : TEXT("[ / ]")));
}

void ADCMultiCamDirector::HandlePrimaryReleased()
{
	bRecordingPauseHeld = false;
}

int32 ADCMultiCamDirector::FindFreeSlot()
{
	int32 Slot = Cameras.Find(nullptr);
	if (Slot == INDEX_NONE && IsUnlimited())
	{
		Slot = Cameras.Add(nullptr);
	}
	return Slot;
}

int32 ADCMultiCamDirector::GetPlacementSlot() const
{
	if (Phase != EDCPhase::Preparation || IsViewingCamera())
	{
		return INDEX_NONE;
	}
	if (const ADCMultiCam* Dolly = FindCameraOnRail(Cameras, PendingRail))
	{
		return Cameras.IndexOfByKey(Dolly);
	}
	if (!HasCameraBudget())
	{
		return INDEX_NONE;
	}
	const int32 Free = Cameras.Find(nullptr);
	return Free != INDEX_NONE ? Free : Cameras.Num();
}

bool ADCMultiCamDirector::HasCameraBudget() const
{
	return IsUnlimited() || GetCameraCount() < MaxCameras;
}

ADCMultiCam* ADCMultiCamDirector::SpawnCamera(const FVector& Location)
{
	FActorSpawnParameters Params;
	Params.Owner = this;
	ADCMultiCam* Cam = GetWorld()->SpawnActor<ADCMultiCam>(ADCMultiCam::StaticClass(), FTransform(Location), Params);
	if (ADCMultiCamCharacter* Player = GetPlayer())
	{
		Cam->SetAim(Player->GetControlRotation());
	}
	return Cam;
}

void ADCMultiCamDirector::FinishPendingRail(bool bQuiet)
{
	if (!PendingRail)
	{
		if (!bQuiet)
		{
			Flash(TEXT("Lay rail points with the dolly tool first (T to switch tools)"), 2.f);
		}
		return;
	}
	const ADCMultiCam* Dolly = FindCameraOnRail(Cameras, PendingRail);
	if (!Dolly)
	{
		// A single point is not a rail yet.
		if (bQuiet)
		{
			PendingRail->Destroy();
			PendingRail = nullptr;
		}
		else
		{
			Flash(TEXT("A dolly rail needs at least two points"), 2.f);
		}
		return;
	}
	PendingRail = nullptr;
	if (!bQuiet)
	{
		Flash(FString::Printf(TEXT("Dolly CAM %d done - look through it and use W/S to set its start. Next click starts a new rail."), Dolly->GetNumber()), 3.f);
	}
}

void ADCMultiCamDirector::HandleFinishRail()
{
	if (Phase == EDCPhase::Preparation)
	{
		FinishPendingRail(false);
	}
}

int32 ADCMultiCamDirector::GetPendingRailPoints() const
{
	return PendingRail ? PendingRail->NumPoints() : 0;
}

void ADCMultiCamDirector::HandleCycleTool()
{
	if (Phase != EDCPhase::Preparation)
	{
		return;
	}
	FinishPendingRail(true);
	Tool = Tool == EDCCamTool::Static ? EDCCamTool::Crane : EDCCamTool::Static;
	PendingHeight = Tool == EDCCamTool::Crane ? ADCMultiCam::CraneHeight : ADCMultiCam::StaticHeight;
	Flash(Tool == EDCCamTool::Crane ? TEXT("CRANE: elevated bird's-eye camera; click ground to place")
		: TEXT("STATIC: eye-level camera; click ground to place"), 2.5f);
}

float ADCMultiCamDirector::GetViewAspect() const
{
	if (APlayerController* PC = GetPC())
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

void ADCMultiCamDirector::UpdateFrameParams()
{
	const float Aspect = GetViewAspect();
	if (!FrameParams || Aspect == FrameParamsAspect)
	{
		return;
	}
	if (UMaterialParameterCollectionInstance* Params = GetWorld()->GetParameterCollectionInstance(FrameParams))
	{
		// Viewport UV of the frame: min corner in RG, max corner in BA.
		const FVector2D Half = DCFrame::CaptureExtent(Aspect) * 0.5f;
		Params->SetVectorParameterValue(TEXT("CaptureRect"), FLinearColor(0.5f - Half.X, 0.5f - Half.Y, 0.5f + Half.X, 0.5f + Half.Y));
		FrameParamsAspect = Aspect;
	}
}

void ADCMultiCamDirector::UpdateCameraSpots()
{
	constexpr float FadeSeconds = 0.4f;
	const bool bShooting = Phase == EDCPhase::Countdown || Phase == EDCPhase::Filming;
	const float PhaseOpacity = Phase == EDCPhase::Preparation ? 1.f : bShooting ? ShootingCameraSpotOpacity : 0.f;
	for (const FDCMultiCamBeat& State : Beats)
	{
		// Only scenes still to get: a captured or missed beat's circle goes away mid-take.
		float Opacity = 0.f;
		if (PhaseOpacity > 0.f && State.Status == EDCShotStatus::Pending)
		{
			const float FadeIn = (SceneTime - (State.Beat->WindowStart - CameraSpotLeadTime)) / FadeSeconds;
			const float FadeOut = (State.Beat->WindowEnd - SceneTime) / FadeSeconds;
			Opacity = PhaseOpacity * FMath::Clamp(FadeIn, 0.f, 1.f) * FMath::Clamp(FadeOut, 0.f, 1.f);
		}
		State.Beat->SetCameraSpotOpacity(Opacity);
	}
}

void ADCMultiCamDirector::ApplyStabilizer()
{
	// Full stabilization while shooting; aiming in preparation stays instant.
	const bool bOn = Phase == EDCPhase::Countdown || Phase == EDCPhase::Filming;
	const float Aim = bOn ? StabilizerAimSmoothing : 0.f;
	const float Dolly = bOn ? StabilizerDollyEase : 0.f;
	for (ADCMultiCam* Cam : Cameras)
	{
		if (Cam)
		{
			Cam->SetStabilizer(Aim, Dolly);
		}
	}
}

void ADCMultiCamDirector::HandleRigMove(FVector2D Axis, float DeltaSeconds)
{
	ADCMultiCam* Cam = GetCameraSlot(ViewingSlot);
	if (!Cam || IsReviewing() || Axis.IsNearlyZero())
	{
		return;
	}
	Cam->MoveLocal(Axis, DeltaSeconds);
	if (Phase == EDCPhase::Preparation)
	{
		Cam->SaveHome();
	}
}

void ADCMultiCamDirector::HandleRailSpeed(int32 Direction)
{
	ADCMultiCam* Cam = GetCameraSlot(ViewingSlot);
	if (!Cam || Cam->GetRig() != EDCCamRig::Dolly || IsReviewing())
	{
		return;
	}
	Cam->AdjustRailSpeed(Direction);
	Flash(FString::Printf(TEXT("Dolly speed x%.2g"), Cam->GetRailSpeedScale()), 1.f);
}

void ADCMultiCamDirector::HandleCycleCamera(int32 Direction)
{
	if (Phase != EDCPhase::Preparation && Phase != EDCPhase::Countdown && Phase != EDCPhase::Filming)
	{
		return;
	}
	TArray<int32> Placed;
	for (int32 i = 0; i < Cameras.Num(); ++i)
	{
		if (Cameras[i])
		{
			Placed.Add(i);
		}
	}
	if (Placed.Num() == 0)
	{
		return;
	}
	const int32 Current = Placed.Find(ViewingSlot);
	const int32 Next = Current == INDEX_NONE ? (Direction > 0 ? 0 : Placed.Num() - 1) : (Current + Direction + Placed.Num()) % Placed.Num();
	if (Placed[Next] != ViewingSlot)
	{
		ViewCamera(Placed[Next]);
		CutCount += Phase == EDCPhase::Filming ? 1 : 0;
	}
}

void ADCMultiCamDirector::HandleSecondaryPressed()
{
	ADCMultiCamCharacter* Player = GetPlayer();
	if (Phase == EDCPhase::Preparation && IsViewingCamera())
	{
		ViewWalking(); // step back out of the camera
		return;
	}
	if (Phase != EDCPhase::Preparation || !Player)
	{
		return;
	}
	// While laying a dolly rail, Right Mouse takes back the last point.
	if (Tool == EDCCamTool::Dolly && PendingRail)
	{
		ADCMultiCam* Dolly = FindCameraOnRail(Cameras, PendingRail);
		if (Dolly && PendingRail->NumPoints() <= 2)
		{
			// Going below two points removes the dolly camera (and its rail) entirely.
			const int32 Slot = Cameras.Find(Dolly);
			Dolly->Destroy();
			Cameras[Slot] = nullptr;
			PendingRail = nullptr;
			Flash(FString::Printf(TEXT("Dolly CAM %d removed"), Slot + 1), 1.5f);
			return;
		}
		PendingRail->RemovePointNear(FVector(TNumericLimits<float>::Max()), 1.f);
		if (Dolly)
		{
			Dolly->MoveRig(0.f, 0.f);
			Dolly->SaveHome();
		}
		else if (PendingRail->NumPoints() == 0)
		{
			PendingRail->Destroy();
			PendingRail = nullptr;
		}
		return;
	}
	// Pick the camera closest to the aim ray (cameras have no collision, so trace by geometry).
	const FVector Origin = Player->GetCamera()->GetComponentLocation();
	const FVector Dir = Player->GetCamera()->GetForwardVector();
	int32 Best = INDEX_NONE;
	float BestDist = PickRadius;
	for (int32 i = 0; i < Cameras.Num(); ++i)
	{
		if (!Cameras[i])
		{
			continue;
		}
		const FVector P = Cameras[i]->GetActorLocation();
		const float T = FMath::Clamp(FVector::DotProduct(P - Origin, Dir), 0.f, PlaceRange);
		const float D = FVector::Dist(Origin + Dir * T, P);
		if (D < BestDist)
		{
			BestDist = D;
			Best = i;
		}
	}
	if (Best == INDEX_NONE)
	{
		Flash(TEXT("Aim at a camera to remove it"), 1.5f);
		return;
	}
	if (PendingRail && Cameras[Best]->GetRail() == PendingRail)
	{
		PendingRail = nullptr; // the camera takes its rail with it
	}
	Cameras[Best]->Destroy();
	Cameras[Best] = nullptr;
	Flash(FString::Printf(TEXT("CAM %d removed"), Best + 1), 1.5f);
}

void ADCMultiCamDirector::HandleClearCameras()
{
	if (Phase != EDCPhase::Preparation)
	{
		return;
	}
	ViewWalking();
	for (TObjectPtr<ADCMultiCam>& Cam : Cameras)
	{
		if (Cam)
		{
			Cam->Destroy();
			Cam = nullptr;
		}
	}
	if (IsUnlimited())
	{
		Cameras.Reset();
	}
	if (PendingRail)
	{
		PendingRail->Destroy();
		PendingRail = nullptr;
	}
	Flash(TEXT("All cameras removed"));
}

bool ADCMultiCamDirector::IsNearClapper() const
{
	ADCMultiCamCharacter* Player = GetPlayer();
	return Player && Clapperboard && !IsViewingCamera()
		&& FVector::Dist2D(Player->GetActorLocation(), Clapperboard->GetActorLocation()) < Clapperboard->InteractRange;
}

// ---------------------------------------------------------------------------------------------
// Helpers

int32 ADCMultiCamDirector::GetCameraCount() const
{
	int32 Count = 0;
	for (const ADCMultiCam* Cam : Cameras)
	{
		Count += Cam ? 1 : 0;
	}
	return Count;
}

int32 ADCMultiCamDirector::GetSpottedCount() const
{
	int32 Count = 0;
	for (const FDCMultiCamBeat& State : Beats)
	{
		Count += State.bSpotted ? 1 : 0;
	}
	return Count;
}

bool ADCMultiCamDirector::IsLiveFeedRolling() const
{
	for (const FDCMultiCamBeat& State : Beats)
	{
		if (State.Status == EDCShotStatus::Pending && State.Hold > 0.f)
		{
			return true;
		}
	}
	return false;
}

namespace
{
	/** Minimum match for S, A, B and C; anything lower is a D. */
	constexpr float GradeThresholds[] = { 0.85f, 0.7f, 0.55f, 0.4f };
	const TCHAR* const GradeLetters[] = { TEXT("S"), TEXT("A"), TEXT("B"), TEXT("C"), TEXT("D") };
	const TCHAR* const CutNames[] = { TEXT("Director's Cut"), TEXT("Final Cut"), TEXT("Rough Cut"), TEXT("Assembly Cut"), TEXT("Back to the storyboard") };
}

int32 ADCMultiCamDirector::GetGradeRank() const
{
	int32 Rank = 0;
	while (Rank < UE_ARRAY_COUNT(GradeThresholds) && Match.Overall < GradeThresholds[Rank])
	{
		++Rank;
	}
	return Rank;
}

FString ADCMultiCamDirector::GetGrade() const
{
	return GradeLetters[GetGradeRank()];
}

FString ADCMultiCamDirector::GetCutName() const
{
	return CutNames[GetGradeRank()];
}

FString ADCMultiCamDirector::GetMessage() const
{
	return GetWorld()->GetTimeSeconds() < MessageExpireTime ? Message : FString();
}

void ADCMultiCamDirector::Flash(const FString& Text, float Seconds)
{
	Message = Text;
	MessageExpireTime = GetWorld()->GetTimeSeconds() + Seconds;
}

ADCMultiCamCharacter* ADCMultiCamDirector::GetPlayer() const
{
	return Cast<ADCMultiCamCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
}

APlayerController* ADCMultiCamDirector::GetPC() const
{
	return UGameplayStatics::GetPlayerController(this, 0);
}
