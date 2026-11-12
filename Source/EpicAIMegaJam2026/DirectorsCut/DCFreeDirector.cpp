// Director's Cut prototype 2 - free-rig, discovery-driven take director.

#include "DCFreeDirector.h"
#include "DCBeat.h"
#include "DCFreeCharacter.h"
#include "DCPerformer.h"
#include "DCRail.h"
#include "DCSceneMarks.h"
#include "DCTakeDirector.h"
#include "DCTripod.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float SafeFrameFraction = 0.70f;
	constexpr float PlaceRange = 1500.f;
	constexpr float FloorDollyHeight = 150.f;
	constexpr float TripodMountRange = 150.f;
	constexpr float RailMountRange = 250.f;
	constexpr float RemoveRange = 200.f;
	constexpr float FaceDot = 0.25f;
	constexpr int32 AllBeatsReward = 500;

	const FLinearColor RailColors[] = {
		FLinearColor(1.f, 0.45f, 0.05f),
		FLinearColor(0.9f, 0.2f, 0.9f),
		FLinearColor(0.5f, 1.f, 0.2f),
		FLinearColor(0.2f, 0.6f, 1.f),
	};
}

ADCFreeDirector::ADCFreeDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	// Evaluate after the player has moved and aimed this frame.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ADCFreeDirector::BeginPlay()
{
	Super::BeginPlay();

	GatherScene();

	FActorSpawnParameters Params;
	Params.Owner = this;
	TripodGhost = GetWorld()->SpawnActor<ADCTripod>(ADCTripod::StaticClass(), FTransform::Identity, Params);
	TripodGhost->SetActorHiddenInGame(true);
	RailGhost = GetWorld()->SpawnActor<ADCRail>(ADCRail::StaticClass(), FTransform::Identity, Params);

	ResetScene();
	Flash(TEXT("Rig the set, then hit the clapperboard (E) or press Enter. The scene won't tell you where to look."), 7.f);
}

void ADCFreeDirector::GatherScene()
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
		FDCBeatState State;
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
	Beats.StableSort([](const FDCBeatState& A, const FDCBeatState& B) { return A.Beat->WindowStart < B.Beat->WindowStart; });

	if (Beats.Num() == 0 && Problems.Num() == 0)
	{
		Problems.Add(TEXT("No DCBeat actors in this level - select a DCFreeDirector and click Build Castle Scene"));
	}
	else if (!bHasMandatory)
	{
		Problems.Add(TEXT("No mandatory beat - tick Mandatory on at least one DCBeat"));
	}
	SetupProblem = FString::Join(Problems, TEXT("\n"));
}

// ---------------------------------------------------------------------------------------------
// Phases

void ADCFreeDirector::Tick(float DeltaSeconds)
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

void ADCFreeDirector::HandleStartTake()
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
	ResetScene();
	RailGhost->SetPreview(false, FVector::ZeroVector, false);
	TripodGhost->SetActorHiddenInGame(true);
	Phase = EDCPhase::Countdown;
	CountdownRemaining = CountdownDuration;
}

void ADCFreeDirector::BeginFilming()
{
	Phase = EDCPhase::Filming;
	SceneTime = 0.f;
	Flash(TEXT("ACTION! The camera is rolling."), 1.5f);
}

void ADCFreeDirector::HandleRestart()
{
	ResetScene();
	Flash(TEXT("Back to preparation. Your rigs and notebook are kept."));
}

void ADCFreeDirector::ResetScene()
{
	Phase = EDCPhase::Preparation;
	SceneTime = 0.f;
	CountdownRemaining = 0.f;
	bApproved = false;
	Score = 0;

	for (FDCBeatState& State : Beats)
	{
		State.Status = EDCShotStatus::Pending;
		State.Hold = 0.f;
		State.LastProgressTime = -100.f;
	}
	for (ADCPerformer* Performer : Performers)
	{
		Performer->Evaluate(0.f);
	}
	if (ADCFreeCharacter* Player = GetPlayer())
	{
		Player->ResetToStart(StartTransform);
	}
}

void ADCFreeDirector::EndTake()
{
	if (Phase != EDCPhase::Filming)
	{
		return; // end-take fires once
	}

	Phase = EDCPhase::Results;
	bApproved = true;
	bool bAll = true;
	Score = 0;
	for (FDCBeatState& State : Beats)
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

	if (ADCFreeCharacter* Player = GetPlayer())
	{
		Player->ReleaseInputs();
	}
	Flash(TEXT("CUT!"), 2.f);
}

// ---------------------------------------------------------------------------------------------
// Hidden-beat capture: no marks, no record button - framing the right subjects at the right time is enough.

void ADCFreeDirector::EvaluateBeats(float PrevTime, float NowTime)
{
	for (FDCBeatState& State : Beats)
	{
		if (State.Status != EDCShotStatus::Pending)
		{
			continue;
		}
		const ADCBeat* Beat = State.Beat;
		const float Eligible = FMath::Max(0.f, FMath::Min(NowTime, Beat->WindowEnd) - FMath::Max(PrevTime, Beat->WindowStart));
		if (Eligible > 0.f && IsFramed(State))
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
			State.Hold = 0.f;
		}

		if (State.Hold >= Beat->RequiredHold)
		{
			State.Status = EDCShotStatus::Captured;
			State.bEverCaptured = true;
			Flash(FString::Printf(TEXT("Captured: %s (+%d)"), *Beat->DisplayName, Beat->Points));
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

bool ADCFreeDirector::IsFramed(const FDCBeatState& State) const
{
	ADCFreeCharacter* Player = GetPlayer();
	APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
	if (!PC)
	{
		return false;
	}

	const FVector CamLoc = Player->GetCamera()->GetComponentLocation();
	const FVector CamFwd = Player->GetCamera()->GetForwardVector();
	int32 ViewW = 0, ViewH = 0;
	PC->GetViewportSize(ViewW, ViewH);
	const ADCBeat* Beat = State.Beat;

	for (const FDCAnchor& A : State.Anchors)
	{
		const FVector P = A.Component->GetComponentLocation();
		const FVector ToP = P - CamLoc;
		FVector2D Screen;
		if (FVector::DotProduct(ToP, CamFwd) <= 0.f || !PC->ProjectWorldLocationToScreen(P, Screen, true))
		{
			return false;
		}
		if (FMath::Abs(Screen.X - ViewW * 0.5f) > ViewW * SafeFrameFraction * 0.5f || FMath::Abs(Screen.Y - ViewH * 0.5f) > ViewH * SafeFrameFraction * 0.5f)
		{
			return false;
		}
		const float Dist = ToP.Size();
		if (Dist < Beat->MinDistance || Dist > Beat->MaxDistance)
		{
			return false;
		}
		if (A.bMustSeeFace && FVector::DotProduct(A.Owner->GetActorForwardVector(), -ToP.GetSafeNormal()) < FaceDot)
		{
			return false;
		}
	}

	const FCollisionQueryParams Params = MakeTraceParams(SCENE_QUERY_STAT_NAME_ONLY(DCBeatVisibility));
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

FCollisionQueryParams ADCFreeDirector::MakeTraceParams(const FName& Tag) const
{
	FCollisionQueryParams Params(Tag, false, GetPlayer());
	for (ADCRail* Rail : Rails)
	{
		Params.AddIgnoredActor(Rail);
	}
	for (ADCTripod* Tripod : Tripods)
	{
		Params.AddIgnoredActor(Tripod);
	}
	Params.AddIgnoredActor(RailGhost);
	Params.AddIgnoredActor(TripodGhost);
	return Params;
}

// ---------------------------------------------------------------------------------------------
// Preparation: rigging

void ADCFreeDirector::UpdatePrepPreview()
{
	ADCFreeCharacter* Player = GetPlayer();
	bPreviewHit = false;
	bPreviewValid = false;
	if (!Player || Player->IsOnRig())
	{
		RailGhost->SetPreview(false, FVector::ZeroVector, false);
		TripodGhost->SetActorHiddenInGame(true);
		return;
	}

	const FVector CamLoc = Player->GetCamera()->GetComponentLocation();
	const FVector End = CamLoc + Player->GetCamera()->GetForwardVector() * PlaceRange;
	FCollisionQueryParams Params = MakeTraceParams(SCENE_QUERY_STAT_NAME_ONLY(DCRigPreview));
	for (ADCPerformer* Performer : Performers)
	{
		Params.AddIgnoredActor(Performer);
	}

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, End, ECC_Visibility, Params))
	{
		bPreviewHit = true;
		PreviewSurface = Hit.ImpactPoint;
		const bool bFloor = Hit.ImpactNormal.Z > 0.7f;
		if (Tool == EDCRigTool::Rail)
		{
			// Floor rails carry the camera at dolly height; wall and ceiling rails hold it just off the surface.
			PreviewPoint = bFloor ? Hit.ImpactPoint + FVector(0.f, 0.f, FloorDollyHeight) : Hit.ImpactPoint + Hit.ImpactNormal * RailStandoff;
			FHitResult Block;
			const bool bRoom = !GetWorld()->LineTraceSingleByChannel(Block, Hit.ImpactPoint + Hit.ImpactNormal * 2.f, PreviewPoint, ECC_Visibility, Params);
			const ADCRail* Active = Rails.IsValidIndex(ActiveRail) ? Rails[ActiveRail].Get() : nullptr;
			const bool bBudget = Active ? Active->NumPoints() < MaxPointsPerRail : GetRailsUsed() < MaxRails;
			bPreviewValid = bRoom && bBudget;
		}
		else
		{
			PreviewPoint = Hit.ImpactPoint;
			bPreviewValid = bFloor && Tripods.Num() < MaxTripods;
		}
	}

	if (Tool == EDCRigTool::Rail)
	{
		TripodGhost->SetActorHiddenInGame(true);
		RailGhost->SetPreview(bPreviewHit, PreviewPoint, bPreviewValid, 0.f);
	}
	else
	{
		RailGhost->SetPreview(false, FVector::ZeroVector, false);
		TripodGhost->SetActorHiddenInGame(!bPreviewHit);
		TripodGhost->SetActorLocation(PreviewPoint);
		TripodGhost->SetHeadHeight(PendingTripodHeight);
		TripodGhost->SetTint(bPreviewValid ? FLinearColor(0.2f, 1.f, 0.4f) : FLinearColor(1.f, 0.15f, 0.1f));
	}
}

ADCRail* ADCFreeDirector::SpawnRail()
{
	FActorSpawnParameters Params;
	Params.Owner = this;
	ADCRail* Rail = GetWorld()->SpawnActor<ADCRail>(ADCRail::StaticClass(), FTransform::Identity, Params);
	Rail->bFloorRail = false;
	Rail->PostColor = RailColors[Rails.Num() % UE_ARRAY_COUNT(RailColors)];
	for (ADCPerformer* Performer : Performers)
	{
		Rail->IgnoredActors.Add(Performer);
	}
	Rails.Add(Rail);
	return Rail;
}

ADCRail* ADCFreeDirector::GetOrStartActiveRail()
{
	if (Rails.IsValidIndex(ActiveRail))
	{
		return Rails[ActiveRail];
	}
	for (int32 i = 0; i < Rails.Num(); ++i)
	{
		if (Rails[i]->NumPoints() == 0)
		{
			ActiveRail = i;
			return Rails[i];
		}
	}
	if (Rails.Num() < MaxRails)
	{
		SpawnRail();
		ActiveRail = Rails.Num() - 1;
		return Rails[ActiveRail];
	}
	return nullptr;
}

void ADCFreeDirector::HandlePrimaryPressed()
{
	if (Phase != EDCPhase::Preparation)
	{
		return;
	}
	if (!bPreviewHit)
	{
		Flash(TEXT("Aim at a surface within 15 m"));
		return;
	}

	if (Tool == EDCRigTool::Tripod)
	{
		if (Tripods.Num() >= MaxTripods)
		{
			Flash(FString::Printf(TEXT("All %d tripods are placed - remove one with Right Mouse"), MaxTripods));
			return;
		}
		if (!bPreviewValid)
		{
			Flash(TEXT("Tripods stand on the floor"));
			return;
		}
		FActorSpawnParameters Params;
		Params.Owner = this;
		ADCTripod* Tripod = GetWorld()->SpawnActor<ADCTripod>(ADCTripod::StaticClass(), FTransform(PreviewPoint), Params);
		Tripod->SetHeadHeight(PendingTripodHeight);
		Tripod->SetTint(FLinearColor(0.15f, 0.15f, 0.18f));
		Tripods.Add(Tripod);
		return;
	}

	ADCRail* Rail = GetOrStartActiveRail();
	if (!Rail)
	{
		Flash(FString::Printf(TEXT("All %d rails are in use - clear one with Right Mouse or C"), MaxRails));
		return;
	}
	if (Rail->NumPoints() >= MaxPointsPerRail)
	{
		Flash(TEXT("This rail is full - press F to start another"));
		return;
	}
	if (!bPreviewValid)
	{
		Flash(TEXT("No room for the camera there"));
		return;
	}
	FString Reason;
	if (!Rail->TryAddPoint(PreviewPoint, Reason, PreviewSurface))
	{
		Flash(Reason);
	}
}

void ADCFreeDirector::HandleSecondaryPressed()
{
	if (Phase != EDCPhase::Preparation || !bPreviewHit)
	{
		return;
	}

	if (Tool == EDCRigTool::Tripod)
	{
		if (ADCTripod* Tripod = FindNearestTripod(PreviewSurface, RemoveRange))
		{
			Tripods.Remove(Tripod);
			Tripod->Destroy();
		}
		return;
	}

	// Remove the closest point on any rail.
	ADCRail* Best = nullptr;
	float BestDist = RemoveRange;
	for (ADCRail* Rail : Rails)
	{
		for (const FVector& P : Rail->GetPoints())
		{
			const float D = FMath::Min(FVector::Dist(P, PreviewPoint), FVector::Dist(P, PreviewSurface));
			if (D < BestDist)
			{
				BestDist = D;
				Best = Rail;
			}
		}
	}
	if (!Best)
	{
		Flash(TEXT("Aim at a rail point to remove it"));
		return;
	}
	if (!Best->RemovePointNear(PreviewPoint, RemoveRange * 2.f))
	{
		Flash(TEXT("Removing that point would route the rail through scenery"));
	}
}

void ADCFreeDirector::HandleFinishRail()
{
	if (Phase != EDCPhase::Preparation)
	{
		return;
	}
	ActiveRail = INDEX_NONE;
	Flash(GetRailsUsed() < MaxRails ? TEXT("Rail finished - your next point starts a new rail") : TEXT("Rail finished - that was your last rail"));
}

void ADCFreeDirector::HandleClearRigs()
{
	if (Phase != EDCPhase::Preparation)
	{
		return;
	}
	for (ADCRail* Rail : Rails)
	{
		Rail->Clear();
	}
	for (ADCTripod* Tripod : Tripods)
	{
		Tripod->Destroy();
	}
	Tripods.Reset();
	ActiveRail = INDEX_NONE;
	Flash(TEXT("All rigs cleared"));
}

void ADCFreeDirector::HandleSelectTool(EDCRigTool NewTool)
{
	if (Phase == EDCPhase::Preparation)
	{
		Tool = NewTool;
	}
}

void ADCFreeDirector::HandleHeightInput(float Delta)
{
	ADCFreeCharacter* Player = GetPlayer();
	if (ADCTripod* Mounted = Player ? Player->GetMountedTripod() : nullptr)
	{
		Mounted->SetHeadHeight(Mounted->GetHeadHeight() + Delta * 10.f);
		PendingTripodHeight = Mounted->GetHeadHeight();
	}
	else if (Phase == EDCPhase::Preparation)
	{
		PendingTripodHeight = FMath::Clamp(PendingTripodHeight + Delta * 10.f, ADCTripod::MinHeight, ADCTripod::MaxHeight);
	}
}

ADCTripod* ADCFreeDirector::FindNearestTripod(const FVector& Location, float Radius) const
{
	ADCTripod* Best = nullptr;
	float BestDist = Radius;
	for (ADCTripod* Tripod : Tripods)
	{
		const float D = FVector::Dist2D(Tripod->GetActorLocation(), Location);
		if (D < BestDist)
		{
			BestDist = D;
			Best = Tripod;
		}
	}
	return Best;
}

ADCRail* ADCFreeDirector::FindNearestRail(const FVector& Location, float Radius, float& OutDistance) const
{
	ADCRail* Best = nullptr;
	float BestGap = Radius;
	for (ADCRail* Rail : Rails)
	{
		if (!Rail->IsUsable())
		{
			continue;
		}
		float Gap = 0.f;
		const float Distance = Rail->FindClosestDistance(Location, Gap);
		if (Gap < BestGap)
		{
			BestGap = Gap;
			Best = Rail;
			OutDistance = Distance;
		}
	}
	return Best;
}

bool ADCFreeDirector::IsNearClapper() const
{
	ADCFreeCharacter* Player = GetPlayer();
	return Player && Clapperboard && FVector::Dist2D(Player->GetActorLocation(), Clapperboard->GetActorLocation()) < Clapperboard->InteractRange;
}

FString ADCFreeDirector::GetInteractHint() const
{
	ADCFreeCharacter* Player = GetPlayer();
	if (!Player || Phase == EDCPhase::Countdown || Phase == EDCPhase::Results)
	{
		return FString();
	}
	if (Player->GetMountedTripod())
	{
		return TEXT("ON TRIPOD - mouse wheel raises/lowers the camera, E to step off");
	}
	if (Player->IsOnRig())
	{
		return TEXT("ON RAIL - W/S to travel, E to step off");
	}
	if (Phase == EDCPhase::Preparation && IsNearClapper())
	{
		return TEXT("E: hit the clapperboard and call ACTION!");
	}
	if (FindNearestTripod(Player->GetActorLocation(), TripodMountRange))
	{
		return TEXT("E: mount the tripod");
	}
	float Unused = 0.f;
	if (FindNearestRail(Player->GetCamera()->GetComponentLocation(), RailMountRange, Unused))
	{
		return TEXT("E: mount the rail");
	}
	return FString();
}

void ADCFreeDirector::HandleInteract()
{
	ADCFreeCharacter* Player = GetPlayer();
	if (!Player)
	{
		return;
	}
	if (Phase == EDCPhase::Results)
	{
		HandleRestart();
		return;
	}
	if (Phase == EDCPhase::Countdown)
	{
		return;
	}
	if (Player->IsOnRig())
	{
		Player->Dismount();
		return;
	}
	if (Phase == EDCPhase::Preparation && IsNearClapper())
	{
		HandleStartTake();
		return;
	}
	if (ADCTripod* Tripod = FindNearestTripod(Player->GetActorLocation(), TripodMountRange))
	{
		Player->MountTripod(Tripod);
		return;
	}
	float Distance = 0.f;
	if (ADCRail* Rail = FindNearestRail(Player->GetCamera()->GetComponentLocation(), RailMountRange, Distance))
	{
		Player->MountRail(Rail, Distance);
		return;
	}
	Flash(TEXT("Nothing to interact with here"), 1.5f);
}

// ---------------------------------------------------------------------------------------------
// Helpers

int32 ADCFreeDirector::GetRailsUsed() const
{
	int32 Count = 0;
	for (const ADCRail* Rail : Rails)
	{
		Count += Rail->NumPoints() > 0 ? 1 : 0;
	}
	return Count;
}

int32 ADCFreeDirector::GetActiveRailPoints() const
{
	return Rails.IsValidIndex(ActiveRail) ? Rails[ActiveRail]->NumPoints() : 0;
}

int32 ADCFreeDirector::GetSpottedCount() const
{
	int32 Count = 0;
	for (const FDCBeatState& State : Beats)
	{
		Count += State.bSpotted ? 1 : 0;
	}
	return Count;
}

bool ADCFreeDirector::IsAnyBeatRolling() const
{
	for (const FDCBeatState& State : Beats)
	{
		if (State.Status == EDCShotStatus::Pending && State.Hold > 0.f)
		{
			return true;
		}
	}
	return false;
}

FString ADCFreeDirector::GetBrief() const
{
	TArray<FString> Required;
	for (const FDCBeatState& State : Beats)
	{
		if (State.Beat->bMandatory)
		{
			Required.Add(FString::Printf(TEXT("\"%s\""), *State.Beat->DisplayName));
		}
	}
	return FString::Printf(TEXT("The studio needs %s. %d more beats are hidden in this scene."),
		*FString::Join(Required, TEXT(", ")), Beats.Num() - Required.Num());
}

FString ADCFreeDirector::GetCutName() const
{
	if (!bApproved)
	{
		return TEXT("Retake required");
	}
	int32 Bonus = 0, BonusTotal = 0;
	for (const FDCBeatState& State : Beats)
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

FString ADCFreeDirector::GetMessage() const
{
	return GetWorld()->GetTimeSeconds() < MessageExpireTime ? Message : FString();
}

void ADCFreeDirector::Flash(const FString& Text, float Seconds)
{
	Message = Text;
	MessageExpireTime = GetWorld()->GetTimeSeconds() + Seconds;
}

ADCFreeCharacter* ADCFreeDirector::GetPlayer() const
{
	return Cast<ADCFreeCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
}
