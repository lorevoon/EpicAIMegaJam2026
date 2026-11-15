// Director's Cut - multi-camera take director.

#include "DCMultiCamDirector.h"
#include "DCBeat.h"
#include "DCCinemaReplay.h"
#include "DCCrewMember.h"
#include "DCMultiCam.h"
#include "DCMultiCamCharacter.h"
#include "DCPerformer.h"
#include "DCRail.h"
#include "DCSceneMarks.h"
#include "DCPlacement.h"
#include "Algo/BinarySearch.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/RectLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/AmbientSound.h"
#include "Sound/AudioSettings.h"
#include "Sound/SoundWave.h"
#include "Engine/World.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/TextureCube.h"
#include "Engine/LevelStreaming.h"
#include "Engine/LevelStreamingDynamic.h"
#include "HAL/IConsoleManager.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#if WITH_DLSS
#include "DLSSLibrary.h"
#endif

namespace
{
	/**
	 * Picks the render resolution once per run. Where DLSS runs (RTX), render at DLSS Quality's resolution and let it
	 * upscale; anywhere else stay at native resolution, so other GPUs don't lose sharpness to TSR upscaling.
	 */
	void ApplyRenderResolution()
	{
		static bool bApplied = false;
		IConsoleVariable* ScreenPercentage = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));
		if (bApplied || !ScreenPercentage)
		{
			return;
		}
		bApplied = true;
		float Percentage = 100.f;
#if WITH_DLSS
		if (UDLSSLibrary::IsDLSSSupported())
		{
			FVector2D Resolution(1920.f, 1080.f);
			if (GEngine && GEngine->GameViewport)
			{
				GEngine->GameViewport->GetViewportSize(Resolution);
			}
			bool bSupported = false, bFixed = false;
			float Optimal = 0.f, Min = 0.f, Max = 0.f, Sharpness = 0.f;
			UDLSSLibrary::GetDLSSModeInformation(UDLSSMode::Quality, Resolution, bSupported, Optimal, bFixed, Min, Max, Sharpness);
			Percentage = bSupported && Optimal > 0.f ? Optimal : 67.f;
		}
#endif
		// Same priority as DefaultEngine.ini, so a value typed in the console still wins.
		ScreenPercentage->Set(Percentage, ECVF_SetByProjectSetting);
		UE_LOG(LogTemp, Log, TEXT("Render resolution: %.0f%% (%s)"), Percentage, Percentage < 100.f ? TEXT("DLSS Quality") : TEXT("native"));
	}

	constexpr float PlaceRange = 1500.f;
	/** Preparation: the camera to be placed stands this far in front of the player - walk to where it should go. */
	constexpr float PlaceAhead = 160.f;
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
	static ConstructorHelpers::FObjectFinder<USoundWave> TickSound(TEXT("/Game/DirectorsCut/Audio/Gameplay/SW_TimerTick.SW_TimerTick"));
	TimerTickSound = TickSound.Object;
	static ConstructorHelpers::FObjectFinder<USoundWave> ApprovalSound(TEXT("/Game/DirectorsCut/Audio/Gameplay/SW_DirectorContent.SW_DirectorContent"));
	DirectorContentSound = ApprovalSound.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> FrameMPC(TEXT("/Game/DirectorsCut/Materials/MPC_DCFrame.MPC_DCFrame"));
	FrameParams = FrameMPC.Object;
	static ConstructorHelpers::FObjectFinder<UTextureCube> FillCubemap(TEXT("/Engine/MapTemplates/Sky/DaylightAmbientCubemap.DaylightAmbientCubemap"));
	InsetFillCubemap = FillCubemap.Object;

	PrimaryActorTick.bCanEverTick = true;
	// Evaluate after the player has re-aimed the live camera this frame.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Renders the reference cut for the preparation inset; captured by hand only while that inset is showing.
	PlacementCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PlacementCapture"));
 PlacementCapture->SetupAttachment(RootComponent);
 PlacementCapture->bCaptureEveryFrame = false;
 PlacementCapture->bCaptureOnMovement = false;
 PlacementCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
 ReferenceCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("ReferenceInsetCapture"));
	ReferenceCapture->SetupAttachment(RootComponent);
	ReferenceCapture->bCaptureEveryFrame = false;
	ReferenceCapture->bCaptureOnMovement = false;
	ReferenceCapture->bAlwaysPersistRenderingState = true; // keeps exposure history between captures
	ReferenceCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	// This is a small composition guide, not the final film. Avoid paying for a second full-quality
	// Lumen/VSM render in every preparation frame.
	ReferenceCapture->ShowFlags.SetLumenGlobalIllumination(false);
	ReferenceCapture->ShowFlags.SetLumenReflections(false);
	ReferenceCapture->ShowFlags.SetDynamicShadows(false);
	ReferenceCapture->ShowFlags.SetAmbientOcclusion(false);
	ReferenceCapture->ShowFlags.SetScreenSpaceAO(false);
}

void ADCMultiCamDirector::BeginPlay()
{
	Super::BeginPlay();
	ApplyRenderResolution();

 // Both inset cameras must use the same lighting pipeline and temporal state.
 PlacementCapture->ShowFlags = ReferenceCapture->ShowFlags;
 PlacementCapture->PostProcessSettings = ReferenceCapture->PostProcessSettings;
 PlacementCapture->PostProcessBlendWeight = ReferenceCapture->PostProcessBlendWeight;
 PlacementCapture->bAlwaysPersistRenderingState = ReferenceCapture->bAlwaysPersistRenderingState;
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

	// The studio's big rect lights put the waxy highlights on the actors. The real environment has none, so the film
	// camera carries a soft light of its own there, up and to one side like a key light, that touches only the actors.
	FilmKeyLight = NewObject<URectLightComponent>(PlaybackCamera, TEXT("FilmKeyLight"));
	FilmKeyLight->SetupAttachment(PlaybackCamera->GetRootComponent());
	FilmKeyLight->SetRelativeLocationAndRotation(FVector(0.f, 150.f, 120.f), FRotator(-12.f, -10.f, 0.f));
	FilmKeyLight->SetIntensityUnits(ELightUnits::Candelas);
	FilmKeyLight->SetIntensity(FilmKeyLightIntensity);
	FilmKeyLight->SetSourceWidth(300.f);
	FilmKeyLight->SetSourceHeight(200.f);
	FilmKeyLight->SetAttenuationRadius(5000.f);
	FilmKeyLight->SetCastShadows(false);
	FilmKeyLight->SetLightingChannels(false, true, false);
	FilmKeyLight->SetVisibility(false);
	FilmKeyLight->RegisterComponent();

	ResetScene();
	Flash(IsUnlimited() ? FString(TEXT("Place as many cameras as you like. Walk to a spot - the camera stands just in front of you."))
		: FString::Printf(TEXT("Place up to %d cameras. Walk to a spot - the camera stands just in front of you."), MaxCameras), 7.f);
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
	TickFade(DeltaSeconds);
 if (OverviewRemaining > 0.f)
 {
  OverviewRemaining -= DeltaSeconds;
  if (OverviewRemaining <= 0.f)
   if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) PC->SetViewTargetWithBlend(PC->GetPawn(), 0.f);
  return;
 }
 if (Phase != EDCPhase::Preparation) bReferenceFocused = false;

	const APlayerController* ViewPC = GetPC();
	const bool bHasView = ViewPC && ViewPC->PlayerCameraManager;
	const FVector ViewLocation = bHasView ? ViewPC->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
	for (ADCMultiCam* Cam : Cameras)
	{
		if (Cam)
		{
			Cam->UpdateZoom(DeltaSeconds);
			Cam->UpdateStabilizer(DeltaSeconds);
			if (bHasView)
			{
				Cam->FaceLabel(ViewLocation); // "CAM n" labels always face the player
			}
		}
	}

	UpdateCameraSpots();
	UpdateFrameParams();
	UpdateFocus(DeltaSeconds);

	switch (Phase)
	{
	case EDCPhase::Preparation:
		if (IsInCutscene())
		{
			break; // the set holds still while he talks
		}
		if (bRehearsalPlaying && TakeDuration > KINDA_SMALL_NUMBER)
		{
			SceneTime = FMath::Fmod(SceneTime + FMath::Max(0.f, DeltaSeconds) , TakeDuration);
			for (ADCPerformer* Performer : Performers)
			{
				Performer->Evaluate(SceneTime);
			}
		}
		UpdatePrepPreview();
		UpdateReferenceInset(DeltaSeconds);
		UpdateCoveredScenes();
		break;

	case EDCPhase::Countdown:
		if (!EnsureLiveCamera())
		{
			return;
		}
		if (!IsInCutscene())
		{
			const int32 Number = FMath::CeilToInt(CountdownRemaining);
			if (Number > 0 && Number != LastCountdownSoundNumber)
			{
				LastCountdownSoundNumber = Number;
				UGameplayStatics::PlaySound2D(this, TimerTickSound);
			}
			CountdownRemaining -= DeltaSeconds; // holds while the filming tutorial explains
		}
		UpdateReferenceInset(DeltaSeconds);
		if (CountdownRemaining <= 0.f)
		{
			BeginFilming();
		}
		break;

	case EDCPhase::Filming:
	{
		if (IsWrappingUp())
		{
			TickWrapUp(DeltaSeconds); // the scene holds on its last frame while he wraps up
			break;
		}
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
		UpdateReferenceInset(DeltaSeconds); // before EvaluateBeats, which can end the take
		UpdateCoach();
		EvaluateBeats(Prev, SceneTime);
		break;
	}

	case EDCPhase::Review:
		TickPremiere(DeltaSeconds);
		break;

	case EDCPhase::Reference:
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
		bPlaying = false;
		Rate = 1.f;
	}
	else if (Phase == EDCPhase::Filming)
	{
		// Pausing recording omits footage, but the actors and their speech continue on set.
		bPlaying = SceneTime < TakeDuration;
	}
	else if (Phase == EDCPhase::Reference)
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
	if (!bPlaying || IsInCutscene() || Rate > GetDefault<UAudioSettings>()->GlobalMaxPitchScale
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
	if (HandleCutsceneInput())
	{
		return;
	}
	if (Phase == EDCPhase::Preparation && IsViewingCamera())
	{
		return; // Left-click confirms; Enter must not install a camera or start filming from this view.
	}
	if (Phase == EDCPhase::Review)
	{
		// Enter skips to the evaluation, then moves on from it.
		if (IsFilmFinished())
		{
			OnEvaluationFinished();
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
	PlayCutscene(ActionCutscene, [this]()
	{
		StartCountdown();
		// The first take: on set behind the live camera, he explains the controls before anything rolls.
		if (bShowCutscenes && !bFilmingTutorialShown && FilmingTutorial.Num() > 0)
		{
			bFilmingTutorialShown = true;
			PlayCutscene(FilmingTutorial, [this]() { RollCountdown(); }, true);
		}
	});
}

void ADCMultiCamDirector::StartCountdown()
{
	ResetScene();
	Ghost->SetActorHiddenInGame(true);
	Phase = EDCPhase::Countdown;
	CutCount = 0;
	ApplyStabilizer();
	RollCountdown();

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

void ADCMultiCamDirector::RollCountdown()
{
	CountdownRemaining = CountdownDuration;
	LastCountdownSoundNumber = INDEX_NONE;
	Coach(TEXT("Countdown"), EDCDirectorMood::Default, { TEXT("Places, everyone! And you - shoot it like the reference cut."),
		TEXT("Quiet on set! Remember the reference. Every angle."), TEXT("Rolling soon. Don't make me regret hiring you.") }, 0, true);
}

void ADCMultiCamDirector::BeginFilming()
{
	Phase = EDCPhase::Filming;
	SceneTime = 0.f;
}

void ADCMultiCamDirector::OnEvaluationFinished()
{
	HandleRestart();
}

void ADCMultiCamDirector::HandleRestart()
{
	ResetScene();
	Flash(TEXT("Back to preparation. Cameras are kept."));
}

void ADCMultiCamDirector::ResetScene()
{
 bReferenceFocused = false;
 OverviewRemaining = 0.f;
	if (IsPlacementPending()) HandleCameraCancel();
	if (SceneDialogue.IsValid()) SceneDialogue->Stop();
	DialoguePosition = -1.f;
	bRehearsalPlaying = true;
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
	LiveLikeness = -1.f;
	CoachLine.Reset();
	CoachKey = NAME_None;
	CoachMood = EDCDirectorMood::Default;
	CoachPortrait = nullptr;
	WrapUpLine = INDEX_NONE;
	bPremierePaused = false;
	if (ADCMultiCamCharacter* Player = GetPlayer())
	{
		Player->ResetToStart(StartTransform);
	}
}

void ADCMultiCamDirector::EndTake()
{
	if (Phase != EDCPhase::Filming || IsWrappingUp())
	{
		return; // end-take fires once
	}
	for (FDCMultiCamBeat& State : Beats)
	{
		if (State.Status == EDCShotStatus::Pending)
		{
			State.Status = EDCShotStatus::Missed;
		}
	}
	// Not straight to the theater: the director calls cut and wraps up in his corner first.
	LiveLikeness = -1.f; // the shot is no longer being judged: no vignette
	if (bShowCutscenes && WrapUpLines.Num() > 0)
	{
		SayWrapUpLine(0);
	}
	else
	{
		FinishTake();
	}
}

void ADCMultiCamDirector::SayWrapUpLine(int32 Index)
{
	WrapUpLine = Index;
	WrapUpElapsed = 0.f;
	CoachPortrait = WrapUpLines[Index].Portrait;
	CoachKey = NAME_None;
	CoachMood = Index == 0 ? EDCDirectorMood::Joy : EDCDirectorMood::Default;
	CoachLine = WrapUpLines[Index].Line;
	CoachLineStart = GetWorld()->GetRealTimeSeconds();
}

void ADCMultiCamDirector::TickWrapUp(float DeltaSeconds)
{
	WrapUpElapsed += FMath::Max(DeltaSeconds, 0.f);
	if (WrapUpElapsed < WrapUpLineSeconds || FadeDirection != 0)
	{
		return;
	}
	if (WrapUpLines.IsValidIndex(WrapUpLine + 1))
	{
		SayWrapUpLine(WrapUpLine + 1);
		return;
	}
	// Last line said: fade through black into the theater (the grading runs while the screen is black).
	FadeThrough([this]()
	{
		WrapUpLine = INDEX_NONE;
		CoachPortrait = nullptr;
		FinishTake();
	});
}

void ADCMultiCamDirector::FinishTake()
{
	Phase = EDCPhase::Review;
	// The grade: how closely the film matches the reference cut.
	ComputeMatch();
	BestMatch = FMath::Max(BestMatch, Match.Overall);
	StartPremiere();
}

// ---------------------------------------------------------------------------------------------
// Recording and premiere: the take is re-shot from the recorded camera poses (the scene is driven
// purely by the scene clock, so replaying the clock replays the performance exactly).

void ADCMultiCamDirector::RecordSample()
{
	const ADCMultiCam* Live = GetCameraSlot(ViewingSlot);
	if (!Live)
	{
		return;
	}
	const UCameraComponent* Lens = Live->GetCamera();
	FDCTakeSample Sample;
	Sample.Time = SceneTime;
	Sample.FilmTime = SceneTime;
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
	// A cut or a scrub jumps the camera: focus lands on the new shot at once instead of racking over from the last one.
	const bool bCut = FVector::DistSquared(PlaybackCamera->GetActorLocation(), Location) > FMath::Square(100.f)
		|| PlaybackCamera->GetActorQuat().AngularDistance(Rotation) > FMath::DegreesToRadians(10.f);
	PlaybackCamera->SetActorLocationAndRotation(Location, Rotation);
	PlaybackCamera->GetCameraComponent()->SetFieldOfView(FOV);
	PullFocus(PlaybackCamera->GetCameraComponent(), GetWorld()->GetDeltaSeconds(), bCut);
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
	else if (Phase == EDCPhase::Reference)
	{
		ScrubReferenceCut(DeltaSeconds);
	}
}

void ADCMultiCamDirector::HandleTransportToggle()
{
	if (Phase == EDCPhase::Preparation)
	{
		HandleRehearsalToggle();
		return;
	}
	if (Phase == EDCPhase::Reference)
	{
		ToggleReferenceCut(); // Pause/resume the reference film.
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

namespace
{
	/** Hardware ray tracing (and Lumen's use of it) on or off at runtime; only touches settings that change. */
	void SetHardwareRayTracing(bool bOn)
	{
		const int32 Want = bOn ? 1 : 0;
		for (const TCHAR* Name : { TEXT("r.RayTracing.Enable"), TEXT("r.Lumen.HardwareRayTracing") })
		{
			IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name);
			if (Var && Var->GetInt() != Want)
			{
				Var->Set(Want, ECVF_SetByCode);
			}
		}
	}
}

void ADCMultiCamDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetHardwareRayTracing(true);
	Super::EndPlay(EndPlayReason);
}

void ADCMultiCamDirector::SetReviewEnvironment(bool bReview)
{
	if (StudioEnvironmentLevel.IsNone() || RealEnvironmentLevel.IsNone())
	{
		return; // Other acts retain their existing presentation.
	}
	SetHardwareRayTracing(!bReview);
	if (FilmKeyLight)
	{
		FilmKeyLight->SetVisibility(bReview && FilmKeyLightIntensity > 0.f);
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
	RecordSample();

	const ADCMultiCam* Live = GetCameraSlot(ViewingSlot);

	for (FDCMultiCamBeat& State : Beats)
	{
		if (State.Status != EDCShotStatus::Pending)
		{
			continue;
		}
		const ADCBeat* Beat = State.Beat;
		const float Eligible = FMath::Max(0.f, FMath::Min(NowTime, Beat->WindowEnd) - FMath::Max(PrevTime, Beat->GetCaptureStartTime()));
		if (Live && Eligible > 0.f && IsFramed(State, Live))
		{
			State.Hold += Eligible;
			State.bSpotted = true;
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
			State.CapturedFilmTime = SceneTime;
			PlayDirectorContent();
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
	if (IsPlacementPending()) return;
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
	else if (Phase == EDCPhase::Review || Phase == EDCPhase::Reference)
	{
		HandleScrub(Delta * WheelScrubSeconds);
	}
}

void ADCMultiCamDirector::HandlePlacementHeight(float Notches)
{
	if (Phase == EDCPhase::Preparation && !IsViewingCamera())
	{
		PendingHeight = FMath::Clamp(PendingHeight + Notches * StandHeightStep, FMath::Max(MinStandHeight, ADCMultiCam::MinStaticHeight), FMath::Max(MaxStandHeight, ADCMultiCam::MinStaticHeight));
	}
}

// ---------------------------------------------------------------------------------------------
// Preparation: placing cameras

void ADCMultiCamDirector::StartSetOverview()
{
 if (!PlaybackCamera) return;
 // Enter at the player's position; the HUD supplies the clapperboard transition.
 ViewWalking();
 OverviewRemaining = 2.4f;
 if (SceneDialogue.IsValid()) SceneDialogue->Stop();
 DialoguePosition = -1.f;
 if (Ghost) Ghost->SetActorHiddenInGame(true);
}

FVector ADCMultiCamDirector::GetPlacementJointLocation() const
{
	return Ghost ? Ghost->GetStandJoint() : PreviewSurface;
}

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
	FCollisionQueryParams Params = MakeTraceParams(SCENE_QUERY_STAT_NAME_ONLY(DCMultiCamPreview));
	for (ADCPerformer* Performer : Performers)
	{
		Params.AddIgnoredActor(Performer);
	}

	// The camera to be placed stands on the ground a step in front of the player, facing the way they face: walking
	// takes it where it should go, and looking around only turns it (looking up or down doesn't move it). Scrolling
	// still sets its lens height.
	const float NeckHeight = Ghost->GetPlacementAimHeight(ADCMultiCam::StaticHeight);
	FHitResult Ground;
	const bool bGround = DCPlacement::TraceGround(GetWorld(), Ground, CamLoc,
		CamLoc - FVector(0.f, 0.f, PlaceRange), Params);
	const float FloorZ = bGround ? Ground.ImpactPoint.Z : Player->GetActorLocation().Z - 96.f;
	const FVector Facing = FRotator(0.f, Player->GetControlRotation().Yaw, 0.f).Vector();
	const FVector Spot = Player->GetActorLocation() + Facing * PlaceAhead;
	const FVector Probe(Spot.X, Spot.Y, FMath::Max(CamLoc.Z, FloorZ + MaxStandHeight) + 50.f);
 FHitResult Hit;
 if (DCPlacement::TraceGround(GetWorld(), Hit, Probe, Probe - FVector(0.f, 0.f, PlaceRange), Params))
	{
		bPreviewHit = true;
		PreviewSurface = Hit.ImpactPoint;
		// Floors get a stand at the chosen height; walls and ceilings get a short bracket.
		PreviewPoint = Hit.ImpactNormal.Z > 0.7f ? Hit.ImpactPoint + FVector(0.f, 0.f, PendingHeight) : Hit.ImpactPoint + Hit.ImpactNormal * WallStandoff;
		FHitResult Block;
		// Test actual branches/trunk geometry, not a simplified hull enclosing the whole canopy.
		Params.bTraceComplex = true;
		const bool bRoom = !GetWorld()->LineTraceSingleByChannel(Block, Hit.ImpactPoint + Hit.ImpactNormal * 2.f, PreviewPoint, ECC_Visibility, Params);
		FHitResult Occluder;
  const bool bVisible = !GetWorld()->LineTraceSingleByChannel(Occluder, CamLoc,
   Hit.ImpactPoint + FVector(0.f, 0.f, NeckHeight), ECC_Visibility, Params);
  bPreviewValid = bRoom && bVisible && Hit.ImpactNormal.Z > 0.7f && HasCameraBudget();
	}

	Ghost->SetActorHiddenInGame(!bPreviewHit);
	if (bPreviewHit)
	{
		Ghost->SetActorLocation(PreviewPoint);
		Ghost->Setup(0, PreviewSurface);
		Ghost->SetAim(FRotator(0.f, Player->GetControlRotation().Yaw, 0.f));
		Ghost->SetTint(bPreviewValid ? FLinearColor(0.2f, 1.f, 0.4f) : FLinearColor(1.f, 0.15f, 0.1f));
  PlacementCaptureElapsed += GetWorld()->GetDeltaSeconds();
  if (PlacementCaptureElapsed >= 1.f / 15.f)
  {
   if (!PlacementTarget)
   {
    PlacementTarget = NewObject<UTextureRenderTarget2D>(this);
    PlacementTarget->InitCustomFormat(640, FMath::RoundToInt(640.f / DCFrame::FilmAspect), PF_B8G8R8A8, false);
    PlacementCapture->TextureTarget = PlacementTarget;
   }
   PlacementCapture->HiddenActors.Reset();
   PlacementCapture->HiddenActors.Add(Ghost);
   PlacementCapture->HiddenActors.Add(Player);
   for (ADCMultiCam* Cam : Cameras) PlacementCapture->HiddenActors.Add(Cam);
   for (const FDCMultiCamBeat& State : Beats) PlacementCapture->HiddenActors.Add(State.Beat);
   PlacementCapture->SetWorldLocationAndRotation(Ghost->GetCamera()->GetComponentLocation(), Ghost->GetAim());
   PlacementCapture->FOVAngle = DCFrame::CaptureFOV(Ghost->GetFOV(), DCFrame::ScreenAspect);
   CaptureInset(PlacementCapture);
   PlacementCaptureElapsed = 0.f;
  }
	}
}

void ADCMultiCamDirector::HandlePrimaryPressed()
{
 if (IsSetOverview() || IsReferenceFocused()) return;
	if (HandleCutsceneInput())
	{
		return;
	}
	if (Phase != EDCPhase::Preparation)
	{
		return;
	}
	if (IsViewingCamera())
	{
		if (ADCMultiCam* Cam = GetCameraSlot(ViewingSlot)) Cam->SaveHome();
		PendingPlacementSlot = INDEX_NONE;
		ViewWalking();
		UpdateCoveredScenes(); // Confirm the picture while rehearsal is still on the frame the player matched.
		Flash(TEXT(""));
		return; // This click only confirms: never place a second camera on the same press.
	}
	if (!PendingRail && !HasCameraBudget())
	{
		Flash(FString::Printf(TEXT("All %d cameras are placed - Right Mouse removes one"), MaxCameras));
		return;
	}
	if (!bPreviewHit || !bPreviewValid)
	{
		Flash(!bPreviewHit ? TEXT("Walk onto clear ground to stand a camera there") : TEXT("No room for a camera there"));
		return;
	}

	const int32 Slot = FindFreeSlot();
	ADCMultiCam* Cam = SpawnCamera(PreviewPoint);
	Cam->Setup(Slot + 1, PreviewSurface);
	Cam->SetAim(FRotator(0.f, Cam->GetAim().Yaw, 0.f));
	Cam->SaveHome();
	Cameras[Slot] = Cam;
	PendingPlacementSlot = Slot;
	if (Ghost) Ghost->SetActorHiddenInGame(true);
	ViewCamera(Slot);
	Flash(TEXT(""));
}

void ADCMultiCamDirector::HandleCameraCancel()
{
	if (Phase != EDCPhase::Preparation) return;
	const int32 Pending = PendingPlacementSlot;
	PendingPlacementSlot = INDEX_NONE;
	ViewWalking();
	if (ADCMultiCam* Cam = GetCameraSlot(Pending))
	{
		Cameras[Pending] = nullptr;
		Cam->Destroy();
	}
	Flash(TEXT(""));
}

int32 ADCMultiCamDirector::GetNearbyCameraSlot() const
{
	const ADCMultiCamCharacter* Player = GetPlayer();
	if (Phase != EDCPhase::Preparation || IsViewingCamera() || !Player) return INDEX_NONE;
	// Only a camera the player can see: one behind them or off screen can't be opened (and shows no F prompt).
	const APlayerController* PC = GetPC();
	int32 ViewW = 0, ViewH = 0;
	if (PC)
	{
		PC->GetViewportSize(ViewW, ViewH);
	}
	auto OnScreen = [&](const ADCMultiCam* Cam)
	{
		FVector2D Screen;
		if (!PC || ViewW <= 0 || ViewH <= 0)
		{
			return true; // no screen to check against (automation tests)
		}
		return PC->ProjectWorldLocationToScreen(Cam->GetStandJoint(), Screen)
			&& Screen.X >= 0.f && Screen.Y >= 0.f && Screen.X <= ViewW && Screen.Y <= ViewH;
	};
	int32 Best = INDEX_NONE;
	float BestDistance = FMath::Square(250.f);
	for (int32 Slot = 0; Slot < Cameras.Num(); ++Slot)
	{
		const ADCMultiCam* Cam = GetCameraSlot(Slot);
		if (!Cam || Slot == PendingPlacementSlot || !OnScreen(Cam)) continue;
		const float Distance = FVector::DistSquared(Player->GetActorLocation(), Cam->GetInteractionPoint() + FVector(0.f, 0.f, 90.f));
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = Slot;
		}
	}
	return Best;
}

void ADCMultiCamDirector::HandleCameraInteract()
{
	const int32 Slot = GetNearbyCameraSlot();
	if (Slot != INDEX_NONE) ViewCamera(Slot);
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

float ADCMultiCamDirector::GetViewAspect() const
{
	return DCFrame::ScreenAspect; // every gameplay camera keeps this shape, whatever the window's
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
	if (IsPlacementPending()) return;
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
		HandleCameraCancel(); // Cancel previews; exit installed cameras.
		return;
	}
	if (Phase != EDCPhase::Preparation || !Player)
	{
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
	if (IsPlacementPending()) HandleCameraCancel();
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
	return Count - (IsPlacementPending() ? 1 : 0);
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
	const TCHAR* const CutNames[] = { TEXT("Director's Cut"), TEXT("Final Cut"), TEXT("Rough Cut"), TEXT("Assembly Cut"), TEXT("Back to rehearsal") };
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

// ---------------------------------------------------------------------------------------------
// Clay look: a shallow, miniature-style depth of field with automatic focus pulling.
// The rest of the look (grade, bloom, AO, bounce light) lives on the PPV_ClayLook post-process volume.

namespace
{
	TAutoConsoleVariable<float> CVarClayFStop(
		TEXT("dc.ClayLook.FStop"),
		2.f,
		TEXT("Aperture of the clay look's depth of field: lower blurs the foreground and background more, so the set reads ")
		TEXT("as a tabletop miniature. 0 turns the depth of field off."));

	constexpr float FocusTraceRange = 50000.f;
	constexpr float FocusPullSpeed = 6.f;

	UCameraComponent* GetLens(AActor* Actor)
	{
		if (const ADCMultiCam* Cam = Cast<ADCMultiCam>(Actor))
		{
			return Cam->GetCamera();
		}
		return Actor ? Actor->FindComponentByClass<UCameraComponent>() : nullptr;
	}
}

void ADCMultiCamDirector::UpdateFocus(float DeltaSeconds)
{
	// The films (reference cut, review, cinema) pull focus as each frame is posed, in ShowFilmFrame.
	const APlayerController* PC = GetPC();
	AActor* ViewTarget = PC && PC->PlayerCameraManager ? PC->PlayerCameraManager->GetViewTarget() : nullptr;
	if (ViewTarget && ViewTarget != PlaybackCamera)
	{
		PullFocus(GetLens(ViewTarget), DeltaSeconds, false);
	}
}

void ADCMultiCamDirector::PullFocus(UCameraComponent* Lens, float DeltaSeconds, bool bSnap) const
{
	if (!Lens)
	{
		return;
	}
	const float FStop = CVarClayFStop.GetValueOnGameThread();
	FPostProcessSettings& PP = Lens->PostProcessSettings;
	PP.bOverride_DepthOfFieldFstop = FStop > 0.f;
	PP.bOverride_DepthOfFieldFocalDistance = FStop > 0.f;
	if (FStop <= 0.f)
	{
		return;
	}
	PP.DepthOfFieldFstop = FStop;

	const APlayerController* PC = GetPC();
	FCollisionQueryParams Query(SCENE_QUERY_STAT(DCFocus), false);
	Query.AddIgnoredActor(Lens->GetOwner());
	Query.AddIgnoredActor(PC ? PC->GetPawn() : nullptr);
	Query.AddIgnoredActor(Ghost);
	for (ADCMultiCam* Cam : Cameras)
	{
		Query.AddIgnoredActor(Cam); // the camera rigs on set never steal focus from the actors
	}

	// Like a focus puller, keep the people sharp: focus on the visible performer or crew member nearest the middle of
	// the frame, so the set behind them falls out of focus. With nobody in shot, focus on whatever is in the middle.
	TArray<TPair<const AActor*, FVector>, TInlineAllocator<32>> Heads;
	for (const ADCPerformer* Performer : Performers)
	{
		if (const USceneComponent* Head = Performer && !Performer->IsHidden() ? Performer->GetAnchor(EDCAnchorPart::Head) : nullptr)
		{
			Heads.Emplace(Performer, Head->GetComponentLocation());
		}
	}
	for (TActorIterator<ADCCrewMember> It(GetWorld()); It; ++It)
	{
		if (!It->IsHidden())
		{
			const USkeletalMeshComponent* Body = It->Character;
			Heads.Emplace(*It, Body && Body->DoesSocketExist(TEXT("Head")) ? Body->GetSocketLocation(TEXT("Head"))
				: It->GetActorLocation() + FVector(0.f, 0.f, It->HeadHeight));
		}
	}
	const FVector Start = Lens->GetComponentLocation();
	const FTransform LensToWorld(Lens->GetComponentRotation(), Start);
	const float TanHalfWidth = FMath::Tan(FMath::DegreesToRadians(Lens->FieldOfView * .5f));
	const float TanHalfHeight = TanHalfWidth / DCFrame::ScreenAspect;
	float Target = -1.f;
	float BestOffCentre = TNumericLimits<float>::Max();
	for (const TPair<const AActor*, FVector>& Person : Heads)
	{
		const FVector Local = LensToWorld.InverseTransformPosition(Person.Value);
		if (Local.X < 50.f || FMath::Abs(Local.Y) > Local.X * TanHalfWidth || FMath::Abs(Local.Z) > Local.X * TanHalfHeight)
		{
			continue; // behind the lens or out of frame
		}
		const float OffCentre = FMath::Square(Local.Y / (Local.X * TanHalfWidth)) + FMath::Square(Local.Z / (Local.X * TanHalfHeight));
		if (OffCentre >= BestOffCentre)
		{
			continue;
		}
		FCollisionQueryParams SightQuery = Query;
		SightQuery.AddIgnoredActor(Person.Key);
		FHitResult Blocker;
		if (!GetWorld()->LineTraceSingleByChannel(Blocker, Start, Person.Value, ECC_Visibility, SightQuery))
		{
			BestOffCentre = OffCentre;
			Target = Local.X; // focal distance is measured along the lens axis
		}
	}
	if (Target < 0.f)
	{
		FHitResult Hit;
		Target = GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Lens->GetForwardVector() * FocusTraceRange, ECC_Visibility, Query)
			? FMath::Max(Hit.Distance, 50.f) : FocusTraceRange;
	}
	PP.DepthOfFieldFocalDistance = bSnap || PP.DepthOfFieldFocalDistance <= 0.f ? Target
		: FMath::FInterpTo(PP.DepthOfFieldFocalDistance, Target, DeltaSeconds, FocusPullSpeed);
}
