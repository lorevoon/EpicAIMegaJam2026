// Director's Cut - multi-camera take director.

#include "DCMultiCamDirector.h"
#include "DCBeat.h"
#include "DCMultiCam.h"
#include "DCMultiCamCharacter.h"
#include "DCPerformer.h"
#include "DCRail.h"
#include "DCSceneMarks.h"
#include "Algo/BinarySearch.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float PlaceRange = 1500.f;
	constexpr float PickRadius = 80.f;
	constexpr float FaceDot = 0.25f;
	constexpr float LookDegreesPerUnit = 2.5f;
	constexpr int32 AllBeatsReward = 500;

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
	PrimaryActorTick.bCanEverTick = true;
	// Evaluate after the player has re-aimed the live camera this frame.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
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
	PlaybackCamera->GetCameraComponent()->SetConstraintAspectRatio(false);

	ResetScene();
	Flash(IsUnlimited() ? FString(TEXT("Place as many cameras as you like. T switches between static and dolly."))
		: FString::Printf(TEXT("Place up to %d fixed cameras. Once placed they can't move - only turn."), MaxCameras), 7.f);
}

void ADCMultiCamDirector::GatherScene()
{
	UWorld* World = GetWorld();
	TArray<FString> Problems;

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
	bool bHasMandatory = false;
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
		bHasMandatory |= Beat->bMandatory;
		Beats.Add(State);
	}
	Beats.StableSort([](const FDCMultiCamBeat& A, const FDCMultiCamBeat& B) { return A.Beat->WindowStart < B.Beat->WindowStart; });

	if (Beats.Num() == 0 && Problems.Num() == 0)
	{
		Problems.Add(TEXT("No story beats in this level - place DCBeat actors and assign their capture anchors"));
	}
	else if (!bHasMandatory)
	{
		Problems.Add(TEXT("No mandatory beat - tick Mandatory on at least one DCBeat"));
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

	switch (Phase)
	{
	case EDCPhase::Preparation:
		UpdatePrepPreview();
		break;

	case EDCPhase::Countdown:
		if (!EnsureLiveCamera())
		{
			return;
		}
		CountdownRemaining -= DeltaSeconds;
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
		EvaluateBeats(Prev, SceneTime);
		break;
	}

	case EDCPhase::Review:
		TickPremiere(DeltaSeconds);
		break;

	default:
		break;
	}
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
	Phase = EDCPhase::Preparation;
	SceneTime = 0.f;
	CountdownRemaining = 0.f;
	bApproved = false;
	Score = 0;

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
	bApproved = true;
	bool bAll = true;
	Score = 0;
	for (FDCMultiCamBeat& State : Beats)
	{
		if (State.Status == EDCShotStatus::Pending)
		{
			State.Status = EDCShotStatus::Missed;
		}
		const bool bGot = State.Status == EDCShotStatus::Captured;
		bAll &= bGot;
		if (State.Beat->bMandatory)
		{
			bApproved &= bGot;
		}
		if (bGot)
		{
			Score += State.Beat->Points;
		}
	}
	if (bAll)
	{
		Score += AllBeatsReward;
	}
	if (!bApproved)
	{
		Score = 0;
	}
	else
	{
		BestScore = FMath::Max(BestScore, Score);
	}
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
	Sample.FOV = Lens->FieldOfView;
	Sample.bSpoiled = IsLiveShotSpoiled();
	Recording.Add(Sample);
}

void ADCMultiCamDirector::StartPremiere()
{
	APlayerController* PC = GetPC();
	Phase = EDCPhase::Review;
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
	if (Recording.Num() == 0)
	{
		return;
	}
	// Time is film time; paused stretches of the take are not in the film at all.
	const int32 Next = Algo::UpperBoundBy(Recording, Time, &FDCTakeSample::FilmTime);
	const FDCTakeSample& A = Recording[FMath::Clamp(Next - 1, 0, Recording.Num() - 1)];
	const FDCTakeSample& B = Recording.IsValidIndex(Next) ? Recording[Next] : A;
	// Blend between frames of the same camera; a change of camera or a recording pause is a hard cut.
	const bool bBlend = B.Slot == A.Slot && !B.bJumpCut && B.FilmTime > A.FilmTime;
	const float Alpha = bBlend ? FMath::Clamp((Time - A.FilmTime) / (B.FilmTime - A.FilmTime), 0.f, 1.f) : 0.f;
	const float ScenePoint = bBlend ? FMath::Lerp(A.Time, B.Time, Alpha) : A.Time + FMath::Max(0.f, Time - A.FilmTime);
	for (ADCPerformer* Performer : Performers)
	{
		Performer->Evaluate(ScenePoint);
	}
	PlaybackCamera->SetActorLocationAndRotation(FMath::Lerp(A.Location, B.Location, Alpha),
		FQuat::Slerp(A.Rotation.Quaternion(), B.Rotation.Quaternion(), Alpha));
	// The film is only what was inside the capture frame, so the playback lens shows just that part.
	PlaybackCamera->GetCameraComponent()->SetFieldOfView(DCFrame::CaptureFOV(FMath::Lerp(A.FOV, B.FOV, Alpha)));
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
	if (Phase == EDCPhase::Review)
	{
		SceneTime = FMath::Clamp(SceneTime + DeltaSeconds, 0.f, GetPremiereLength());
		ShowPremiereFrame(SceneTime);
	}
}

void ADCMultiCamDirector::HandleTransportToggle()
{
	if (Phase != EDCPhase::Review)
	{
		return;
	}
	if (IsFilmFinished())
	{
		SceneTime = 0.f; // watch the film again
		ShowPremiereFrame(SceneTime);
		bPremierePaused = false;
	}
	else
	{
		bPremierePaused = !bPremierePaused;
	}
}

void ADCMultiCamDirector::SetRigsHidden(bool bHide)
{
	for (ADCMultiCam* Cam : Cameras)
	{
		if (Cam)
		{
			Cam->SetRigHidden(bHide);
		}
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
			Flash(FString::Printf(TEXT("Captured on CAM %d: %s (+%d)"), State.CapturedBy, *Beat->DisplayName, Beat->Points));
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
	float Aspect = 16.f / 9.f;
	if (APlayerController* PC = GetPC())
	{
		int32 ViewW = 0, ViewH = 0;
		PC->GetViewportSize(ViewW, ViewH);
		if (ViewW > 0 && ViewH > 0)
		{
			Aspect = float(ViewW) / float(ViewH);
		}
	}
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
		if (FMath::Abs(NdcX) > DCFrame::CaptureFraction || FMath::Abs(NdcY) > DCFrame::CaptureFraction)
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

void ADCMultiCamDirector::HandleWheel(float Delta, bool bZoom)
{
	ADCMultiCam* Cam = GetCameraSlot(ViewingSlot);
	const bool bShooting = Phase == EDCPhase::Countdown || Phase == EDCPhase::Filming;
	if (Cam && (bShooting || (Phase == EDCPhase::Preparation && bZoom)))
	{
		Cam->SetFOV(Cam->GetTargetFOV() - Delta * 5.f);
		if (Phase == EDCPhase::Preparation)
		{
			Cam->SaveHome();
		}
	}
	else if (Phase == EDCPhase::Preparation || Phase == EDCPhase::Review)
	{
		HandleScrub(Delta * WheelScrubSeconds);
	}
}

void ADCMultiCamDirector::HandlePlacementHeight(float AxisTimesDelta)
{
	if (Phase == EDCPhase::Preparation && !IsViewingCamera())
	{
		PendingHeight = FMath::Clamp(PendingHeight + AxisTimesDelta * StandHeightSpeed, MinStandHeight, MaxStandHeight);
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
		bPreviewValid = bRoom && (HasCameraBudget() || PendingRail);
	}

	Ghost->SetActorHiddenInGame(!bPreviewHit);
	if (bPreviewHit)
	{
		Ghost->SetActorLocation(PreviewPoint);
		Ghost->Setup(0, PreviewSurface);
		Ghost->SetAim(Player->GetControlRotation());
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
		Flash(!bPreviewHit ? TEXT("Aim at a floor, wall, or ceiling within 15 m") : TEXT("No room for a camera there"));
		return;
	}

	if (Tool == EDCCamTool::Dolly)
	{
		// Lay the next rail point; F mounts the dolly camera on the finished rail.
		if (!PendingRail)
		{
			FActorSpawnParameters Params;
			Params.Owner = this;
			PendingRail = GetWorld()->SpawnActor<ADCRail>(ADCRail::StaticClass(), FTransform::Identity, Params);
			PendingRail->bFloorRail = false;
			for (ADCPerformer* Performer : Performers)
			{
				PendingRail->IgnoredActors.Add(Performer);
			}
		}
		FString Reason;
		if (!PendingRail->TryAddPoint(PreviewPoint, Reason, PreviewSurface))
		{
			Flash(Reason);
			return;
		}
		// The dolly camera exists as soon as its rail does (two points), so it is in the switcher straight away.
		if (ADCMultiCam* Dolly = FindCameraOnRail(Cameras, PendingRail))
		{
			Dolly->MoveRig(0.f, 0.f);
			Dolly->SaveHome();
		}
		else if (PendingRail->IsUsable())
		{
			const int32 Slot = FindFreeSlot();
			Dolly = SpawnCamera(PendingRail->GetFloorLocationAtDistance(0.f));
			Dolly->SetupDolly(Slot + 1, PendingRail);
			Dolly->SaveHome();
			Cameras[Slot] = Dolly;
			Flash(FString::Printf(TEXT("Dolly CAM %d added. Keep clicking to extend its rail; F when done."), Slot + 1), 3.f);
		}
		else
		{
			Flash(TEXT("Click a second point to lay the dolly rail"), 2.f);
		}
		return;
	}

	const int32 Slot = FindFreeSlot();
	ADCMultiCam* Cam = SpawnCamera(PreviewPoint);
	Cam->Setup(Slot + 1, PreviewSurface);
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
	if (!bAllowMovingRigs)
	{
		Flash(TEXT("Only static cameras on this production"), 1.5f);
		return;
	}
	FinishPendingRail(true);
	Tool = Tool == EDCCamTool::Static ? EDCCamTool::Dolly : EDCCamTool::Static;
	Flash(Tool == EDCCamTool::Static ? TEXT("STATIC camera: stand, wall, or ceiling")
		: TEXT("DOLLY camera: click rail points on any surface; the camera appears at the second point. F starts a new rail."), 2.5f);
}

void ADCMultiCamDirector::HandleCycleStabilizer()
{
	if (Phase == EDCPhase::Storyboard || IsReviewing())
	{
		return;
	}
	Stabilizer = Stabilizer == EDCStabilizer::Off ? EDCStabilizer::Light
		: Stabilizer == EDCStabilizer::Light ? EDCStabilizer::Heavy : EDCStabilizer::Off;
	ApplyStabilizer();
	const TCHAR* Level = Stabilizer == EDCStabilizer::Off ? TEXT("OFF") : Stabilizer == EDCStabilizer::Light ? TEXT("LIGHT") : TEXT("HEAVY");
	Flash(Phase == EDCPhase::Preparation ? FString::Printf(TEXT("Stabilizer %s (used while shooting)"), Level)
		: FString::Printf(TEXT("Stabilizer %s"), Level), 1.5f);
}

void ADCMultiCamDirector::ApplyStabilizer()
{
	const bool bOn = (Phase == EDCPhase::Countdown || Phase == EDCPhase::Filming) && Stabilizer != EDCStabilizer::Off;
	const bool bHeavy = Stabilizer == EDCStabilizer::Heavy;
	const float Aim = bOn ? (bHeavy ? HeavyAimSmoothing : LightAimSmoothing) : 0.f;
	const float Dolly = bOn ? (bHeavy ? HeavyDollyEase : LightDollyEase) : 0.f;
	for (ADCMultiCam* Cam : Cameras)
	{
		if (Cam)
		{
			Cam->SetStabilizer(Aim, Dolly);
		}
	}
}

void ADCMultiCamDirector::HandleRigMove(float Axis, float DeltaSeconds)
{
	ADCMultiCam* Cam = GetCameraSlot(ViewingSlot);
	if (!Cam || IsReviewing() || FMath::IsNearlyZero(Axis))
	{
		return;
	}
	Cam->MoveRig(Axis, DeltaSeconds);
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
	if (Phase != EDCPhase::Preparation || IsViewingCamera() || !Player)
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

void ADCMultiCamDirector::HandleInteract()
{
	// E only steps back out of a camera. It doubles as "raise camera" while placing, so it never
	// advances the game - Enter alone moves from one phase to the next.
	if (Phase == EDCPhase::Preparation && IsViewingCamera())
	{
		ViewWalking();
	}
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

FString ADCMultiCamDirector::GetBrief() const
{
	TArray<FString> Required;
	for (const FDCMultiCamBeat& State : Beats)
	{
		if (State.Beat->bMandatory)
		{
			Required.Add(FString::Printf(TEXT("\"%s\""), *State.Beat->DisplayName));
		}
	}
	return FString::Printf(TEXT("The studio needs %s. %d more beats are hidden. Only the live camera records."),
		*FString::Join(Required, TEXT(", ")), Beats.Num() - Required.Num());
}

FString ADCMultiCamDirector::GetCutName() const
{
	if (!bApproved)
	{
		return TEXT("Retake required");
	}
	int32 Bonus = 0, BonusTotal = 0;
	for (const FDCMultiCamBeat& State : Beats)
	{
		if (!State.Beat->bMandatory)
		{
			++BonusTotal;
			Bonus += State.Status == EDCShotStatus::Captured ? 1 : 0;
		}
	}
	if (Bonus == 0)
	{
		return TEXT("Studio Cut");
	}
	return Bonus == BonusTotal ? TEXT("Director's Cut") : FString::Printf(TEXT("Extended Cut (%d/%d beats)"), Bonus, BonusTotal);
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
