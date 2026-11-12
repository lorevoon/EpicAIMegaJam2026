// Director's Cut prototype 3 - multi-camera take director.

#include "DCMultiCamDirector.h"
#include "DCBeat.h"
#include "DCMultiCam.h"
#include "DCMultiCamCharacter.h"
#include "DCPerformer.h"
#include "DCSceneMarks.h"
#include "DCTakeDirector.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float SafeFrameFraction = 0.70f;
	constexpr float PlaceRange = 1500.f;
	constexpr float PickRadius = 80.f;
	constexpr float FaceDot = 0.25f;
	constexpr float LookDegreesPerUnit = 2.5f;
	constexpr int32 AllBeatsReward = 500;
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
	Cameras.SetNum(MaxCameras);

	FActorSpawnParameters Params;
	Params.Owner = this;
	Ghost = GetWorld()->SpawnActor<ADCMultiCam>(ADCMultiCam::StaticClass(), FTransform::Identity, Params);
	Ghost->SetActorHiddenInGame(true);

	ResetScene();
	Flash(FString::Printf(TEXT("Place up to %d fixed cameras. Once placed they can't move - only turn."), MaxCameras), 7.f);
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

	StartTransform = ADCTakeDirector::FindStartTransform(World);

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
		if (Beat->WindowStart >= Beat->WindowEnd || Beat->RequiredHold > Beat->WindowEnd - Beat->WindowStart)
		{
			Problems.Add(FString::Printf(TEXT("Beat %s window is shorter than its hold time"), *Beat->BeatId));
			continue;
		}
		bHasMandatory |= Beat->bMandatory;
		Beats.Add(State);
	}
	Beats.StableSort([](const FDCMultiCamBeat& A, const FDCMultiCamBeat& B) { return A.Beat->WindowStart < B.Beat->WindowStart; });

	if (Beats.Num() == 0 && Problems.Num() == 0)
	{
		Problems.Add(TEXT("No DCBeat actors in this level - build the castle with a DCFreeDirector, or place DCBeat actors"));
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

	switch (Phase)
	{
	case EDCPhase::Preparation:
		UpdatePrepPreview();
		break;

	case EDCPhase::Countdown:
		CountdownRemaining -= DeltaSeconds;
		if (CountdownRemaining <= 0.f)
		{
			BeginFilming();
		}
		break;

	case EDCPhase::Filming:
	{
		const float Prev = SceneTime;
		SceneTime = FMath::Min(SceneTime + DeltaSeconds, TakeDuration);
		for (ADCPerformer* Performer : Performers)
		{
			Performer->Evaluate(SceneTime);
		}
		EvaluateBeats(Prev, SceneTime);
		break;
	}

	case EDCPhase::Results:
		break;
	}
}

void ADCMultiCamDirector::HandleStartTake()
{
	if (Phase != EDCPhase::Preparation)
	{
		return;
	}
	if (Beats.Num() == 0)
	{
		Flash(TEXT("No valid beats in this level - see the setup warning"));
		return;
	}
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

	// Go live on the lowest-numbered camera.
	SetLabelsVisible(false);
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
	SetLabelsVisible(true);
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

	Phase = EDCPhase::Results;
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
	Flash(TEXT("CUT!"), 2.f);
}

// ---------------------------------------------------------------------------------------------
// Capture: only the live camera records.

void ADCMultiCamDirector::EvaluateBeats(float PrevTime, float NowTime)
{
	const ADCMultiCam* Live = GetCameraSlot(ViewingSlot);

	for (FDCMultiCamBeat& State : Beats)
	{
		if (State.Status != EDCShotStatus::Pending)
		{
			continue;
		}
		const ADCBeat* Beat = State.Beat;
		const float Eligible = FMath::Max(0.f, FMath::Min(NowTime, Beat->WindowEnd) - FMath::Max(PrevTime, Beat->WindowStart));
		if (Live && Eligible > 0.f && IsFramed(State, Live))
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
		if (FMath::Abs(NdcX) > SafeFrameFraction || FMath::Abs(NdcY) > SafeFrameFraction)
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

void ADCMultiCamDirector::SetLabelsVisible(bool bVisible)
{
	for (ADCMultiCam* Cam : Cameras)
	{
		if (Cam)
		{
			Cam->SetLabelVisible(bVisible);
		}
	}
}

void ADCMultiCamDirector::HandleSelectCamera(int32 Number)
{
	const int32 Slot = Number - 1;
	if (Slot < 0 || Slot >= MaxCameras)
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
	if (!Cam || Phase == EDCPhase::Results)
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
	if (ADCMultiCam* Cam = GetCameraSlot(ViewingSlot))
	{
		if (Phase != EDCPhase::Results)
		{
			Cam->SetFOV(Cam->GetFOV() - Delta * 5.f);
			if (Phase == EDCPhase::Preparation)
			{
				Cam->SaveHome();
			}
		}
	}
	else if (Phase == EDCPhase::Preparation)
	{
		PendingHeight = FMath::Clamp(PendingHeight + Delta * 10.f, MinStandHeight, MaxStandHeight);
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
		bPreviewValid = bRoom && GetCameraCount() < MaxCameras;
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
	if (Phase != EDCPhase::Preparation || IsViewingCamera())
	{
		return;
	}
	const int32 Slot = Cameras.Find(nullptr);
	if (Slot == INDEX_NONE)
	{
		Flash(FString::Printf(TEXT("All %d cameras are placed - Right Mouse removes one"), MaxCameras));
		return;
	}
	if (!bPreviewHit || !bPreviewValid)
	{
		Flash(bPreviewHit ? TEXT("No room for a camera there") : TEXT("Aim at a floor, wall, or ceiling within 15 m"));
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	ADCMultiCam* Cam = GetWorld()->SpawnActor<ADCMultiCam>(ADCMultiCam::StaticClass(), FTransform(PreviewPoint), Params);
	Cam->Setup(Slot + 1, PreviewSurface);
	if (ADCMultiCamCharacter* Player = GetPlayer())
	{
		Cam->SetAim(Player->GetControlRotation());
	}
	Cam->SaveHome();
	Cameras[Slot] = Cam;
	Flash(FString::Printf(TEXT("CAM %d placed - press %d to look through it and aim"), Slot + 1, Slot + 1));
}

void ADCMultiCamDirector::HandleSecondaryPressed()
{
	ADCMultiCamCharacter* Player = GetPlayer();
	if (Phase != EDCPhase::Preparation || IsViewingCamera() || !Player)
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
	switch (Phase)
	{
	case EDCPhase::Results:
		HandleRestart();
		break;
	case EDCPhase::Preparation:
		if (IsViewingCamera())
		{
			ViewWalking();
		}
		else if (IsNearClapper())
		{
			HandleStartTake();
		}
		else
		{
			Flash(TEXT("Nothing to interact with here"), 1.5f);
		}
		break;
	default:
		break;
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
