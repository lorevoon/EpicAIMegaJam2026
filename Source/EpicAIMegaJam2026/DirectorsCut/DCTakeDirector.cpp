// Director's Cut prototype - take director.

#include "DCTakeDirector.h"
#include "DCCharacter.h"
#include "DCPerformer.h"
#include "DCRail.h"
#include "DCSceneMarks.h"
#include "DCShotMarker.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float SafeFrameFraction = 0.70f;
	constexpr float VerticalTolerance = 200.f;
	constexpr float PlaceRange = 800.f;
	constexpr float MountRange = 250.f;
	constexpr int32 AllBonusReward = 250;
	constexpr float StandHeight = 100.f;
}

ADCTakeDirector::ADCTakeDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	// Evaluate after the player has moved and aimed this frame.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

FTransform ADCTakeDirector::FindStartTransform(UWorld* World)
{
	if (World)
	{
		for (TActorIterator<ADCStartMark> It(World); It; ++It)
		{
			return FTransform(FRotator(0.f, It->GetActorRotation().Yaw, 0.f), It->GetActorLocation() + FVector(0.f, 0.f, StandHeight));
		}
	}
	return FTransform(FRotator::ZeroRotator, FVector(-1000.f, 0.f, StandHeight));
}

void ADCTakeDirector::BeginPlay()
{
	Super::BeginPlay();

	GatherScene();

	FActorSpawnParameters Params;
	Params.Owner = this;
	Rail = GetWorld()->SpawnActor<ADCRail>(ADCRail::StaticClass(), FTransform::Identity, Params);
	for (ADCPerformer* Performer : Performers)
	{
		Rail->IgnoredActors.Add(Performer);
	}

	ResetScene();
	Flash(TEXT("Lay a camera rail, then hit the clapperboard (E) or press Enter to call Action!"), 6.f);
}

void ADCTakeDirector::GatherScene()
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

	StartTransform = FindStartTransform(World);

	Shots.Reset();
	TSet<FString> SeenIds;
	bool bHasMandatory = false;
	for (TActorIterator<ADCShotMarker> It(World); It; ++It)
	{
		ADCShotMarker* M = *It;
		FDCShot S;
		S.Marker = M;
		S.Id = M->ShotId;
		S.Name = M->DisplayName;
		S.Caption = M->Caption;
		S.bMandatory = M->bMandatory;
		S.WindowStart = M->WindowStart;
		S.WindowEnd = FMath::Min(M->WindowEnd, TakeDuration);
		S.RequiredHold = M->RequiredHold;
		S.MarkLocation = M->GetActorLocation();
		S.Radius = M->Radius;
		S.MinDistance = M->MinDistance;
		S.MaxDistance = M->MaxDistance;
		S.Points = M->Points;
		for (const FDCAnchorRef& Ref : M->Anchors)
		{
			if (Ref.Performer)
			{
				FDCAnchor A;
				A.Component = Ref.Performer->GetAnchor(Ref.Part);
				A.Owner = Ref.Performer;
				S.Anchors.Add(A);
			}
		}

		// Validate per the GDD data contract; problems are shown on the HUD instead of crashing.
		if (SeenIds.Contains(S.Id))
		{
			Problems.Add(FString::Printf(TEXT("Duplicate shot id %s"), *S.Id));
		}
		SeenIds.Add(S.Id);
		if (S.Anchors.Num() == 0)
		{
			Problems.Add(FString::Printf(TEXT("%s has no anchors (pick a performer in its Anchors list)"), *S.Id));
			continue;
		}
		if (S.WindowStart >= S.WindowEnd || S.RequiredHold > S.WindowEnd - S.WindowStart)
		{
			Problems.Add(FString::Printf(TEXT("%s window is shorter than its hold time"), *S.Id));
			continue;
		}
		if (S.MinDistance >= S.MaxDistance)
		{
			Problems.Add(FString::Printf(TEXT("%s min distance must be below max distance"), *S.Id));
			continue;
		}
		bHasMandatory |= S.bMandatory;
		Shots.Add(S);
	}
	Shots.StableSort([](const FDCShot& A, const FDCShot& B) { return A.WindowStart < B.WindowStart; });

	if (Shots.Num() == 0 && Problems.Num() == 0)
	{
		Problems.Add(TEXT("No shot markers in this level - select a DCTakeDirector and click Build Default Scene, or place DCShotMarker actors"));
	}
	else if (!bHasMandatory)
	{
		Problems.Add(TEXT("No mandatory shot - tick Mandatory on at least one DCShotMarker"));
	}
	if (!Clapperboard)
	{
		Problems.Add(TEXT("No DCClapperboard placed - press Enter to start takes"));
	}
	SetupProblem = FString::Join(Problems, TEXT("\n"));
}

// ---------------------------------------------------------------------------------------------
// Phases

void ADCTakeDirector::Tick(float DeltaSeconds)
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
		EvaluateShots(Prev, SceneTime);
		break;
	}

	case EDCPhase::Results:
		break;
	}
}

void ADCTakeDirector::HandleStartTake()
{
	if (Phase != EDCPhase::Preparation)
	{
		return;
	}
	if (Shots.Num() == 0)
	{
		Flash(TEXT("No valid shots in this level - see the setup warning"));
		return;
	}
	ResetScene();
	Rail->SetPreview(false, FVector::ZeroVector, false);
	Phase = EDCPhase::Countdown;
	CountdownRemaining = CountdownDuration;
}

void ADCTakeDirector::BeginFilming()
{
	Phase = EDCPhase::Filming;
	SceneTime = 0.f;
	Flash(TEXT("ACTION!"), 1.5f);
}

void ADCTakeDirector::HandleRestart()
{
	ResetScene();
	Flash(TEXT("Back to preparation. Your rail is preserved."));
}

void ADCTakeDirector::ResetScene()
{
	Phase = EDCPhase::Preparation;
	SceneTime = 0.f;
	CountdownRemaining = 0.f;
	bApproved = false;
	Score = 0;

	for (FDCShot& Shot : Shots)
	{
		Shot.ResetRuntime();
	}
	for (ADCPerformer* Performer : Performers)
	{
		Performer->Evaluate(0.f);
	}
	RefreshMarkerColors();

	if (ADCCharacter* Player = GetPlayer())
	{
		Player->ResetToStart(StartTransform);
	}
}

void ADCTakeDirector::EndTake()
{
	if (Phase != EDCPhase::Filming)
	{
		return; // end-take fires once
	}

	Phase = EDCPhase::Results;
	bApproved = true;
	int32 Bonus = 0;
	int32 BonusTotal = 0;
	Score = 0;
	for (FDCShot& Shot : Shots)
	{
		if (Shot.Status == EDCShotStatus::Pending)
		{
			Shot.Status = EDCShotStatus::Missed;
		}
		if (Shot.bMandatory)
		{
			bApproved &= Shot.Status == EDCShotStatus::Captured;
		}
		else
		{
			++BonusTotal;
		}
		if (Shot.Status == EDCShotStatus::Captured)
		{
			Score += Shot.Points;
			Bonus += Shot.bMandatory ? 0 : 1;
		}
	}
	if (Bonus == BonusTotal && BonusTotal > 0)
	{
		Score += AllBonusReward;
	}
	if (!bApproved)
	{
		Score = 0;
	}
	else
	{
		BestScore = FMath::Max(BestScore, Score);
	}
	RefreshMarkerColors();

	if (ADCCharacter* Player = GetPlayer())
	{
		Player->ReleaseInputs();
	}
	Flash(TEXT("CUT!"), 2.f);
}

// ---------------------------------------------------------------------------------------------
// Capture contract (GDD section 04)

void ADCTakeDirector::EvaluateShots(float PrevTime, float NowTime)
{
	for (FDCShot& Shot : Shots)
	{
		if (Shot.Status != EDCShotStatus::Pending)
		{
			continue;
		}

		const float Eligible = FMath::Max(0.f, FMath::Min(NowTime, Shot.WindowEnd) - FMath::Max(PrevTime, Shot.WindowStart));
		const bool bInWindow = NowTime >= Shot.WindowStart && PrevTime <= Shot.WindowEnd;
		Shot.LastFail = CheckShot(Shot, bInWindow);

		if (Shot.LastFail == EDCFail::None && Eligible > 0.f)
		{
			Shot.Hold += Eligible;
		}
		else
		{
			Shot.Hold = 0.f;
		}

		if (Shot.Hold >= Shot.RequiredHold)
		{
			Shot.Status = EDCShotStatus::Captured;
			Shot.CapturedAt = NowTime;
			Flash(FString::Printf(TEXT("Captured %s: %s"), *Shot.Id, *Shot.Name));
			RefreshMarkerColors();
		}
	}

	for (FDCShot& Shot : Shots)
	{
		if (Shot.Status == EDCShotStatus::Pending && NowTime > Shot.WindowEnd)
		{
			Shot.Status = EDCShotStatus::Missed;
			RefreshMarkerColors();
			if (Shot.bMandatory)
			{
				EndTake();
				return;
			}
		}
	}

	if (NowTime >= TakeDuration)
	{
		EndTake();
	}
}

EDCFail ADCTakeDirector::CheckShot(const FDCShot& Shot, bool bInWindow) const
{
	if (!bInWindow)
	{
		return EDCFail::WrongTime;
	}

	ADCCharacter* Player = GetPlayer();
	APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
	if (!PC)
	{
		return EDCFail::WrongPosition;
	}

	const FVector Feet = Player->GetActorLocation() - FVector(0.f, 0.f, Player->GetSimpleCollisionHalfHeight());
	if (FVector::Dist2D(Feet, Shot.MarkLocation) > Shot.Radius || FMath::Abs(Feet.Z - Shot.MarkLocation.Z) > VerticalTolerance)
	{
		return EDCFail::WrongPosition;
	}
	if (!Player->IsRecording())
	{
		return EDCFail::NotRecording;
	}
	if (Player->IsSprinting())
	{
		return EDCFail::Sprinting;
	}

	const FVector CamLoc = Player->GetCamera()->GetComponentLocation();
	const FVector CamFwd = Player->GetCamera()->GetForwardVector();
	int32 ViewW = 0, ViewH = 0;
	PC->GetViewportSize(ViewW, ViewH);

	for (const FDCAnchor& A : Shot.Anchors)
	{
		const FVector P = A.Component->GetComponentLocation();
		const FVector ToP = P - CamLoc;
		FVector2D Screen;
		if (FVector::DotProduct(ToP, CamFwd) <= 0.f || !PC->ProjectWorldLocationToScreen(P, Screen, true))
		{
			return EDCFail::OutOfFrame;
		}
		const float HalfSafeW = ViewW * SafeFrameFraction * 0.5f;
		const float HalfSafeH = ViewH * SafeFrameFraction * 0.5f;
		if (FMath::Abs(Screen.X - ViewW * 0.5f) > HalfSafeW || FMath::Abs(Screen.Y - ViewH * 0.5f) > HalfSafeH)
		{
			return EDCFail::OutOfFrame;
		}
		const float Dist = ToP.Size();
		if (Dist < Shot.MinDistance)
		{
			return EDCFail::TooClose;
		}
		if (Dist > Shot.MaxDistance)
		{
			return EDCFail::TooFar;
		}
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(DCShotVisibility), false, Player);
	Params.AddIgnoredActor(Rail);
	for (const FDCAnchor& A : Shot.Anchors)
	{
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, A.Component->GetComponentLocation(), ECC_Visibility, Params) && Hit.GetActor() != A.Owner)
		{
			return EDCFail::Blocked;
		}
	}

	return EDCFail::None;
}

// ---------------------------------------------------------------------------------------------
// Preparation: rail editing

void ADCTakeDirector::UpdatePrepPreview()
{
	ADCCharacter* Player = GetPlayer();
	bPreviewHit = false;
	bPreviewValid = false;
	if (!Player || Player->IsOnRail())
	{
		Rail->SetPreview(false, FVector::ZeroVector, false);
		return;
	}

	const FVector CamLoc = Player->GetCamera()->GetComponentLocation();
	const FVector End = CamLoc + Player->GetCamera()->GetForwardVector() * PlaceRange;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DCRailPreview), false, Player);
	Params.AddIgnoredActor(Rail);
	for (ADCPerformer* Performer : Performers)
	{
		Params.AddIgnoredActor(Performer);
	}

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, End, ECC_Visibility, Params))
	{
		bPreviewHit = true;
		PreviewPoint = Hit.ImpactPoint;
		bPreviewValid = Hit.ImpactNormal.Z > 0.7f && Rail->NumPoints() < ADCRail::MaxPoints;
	}
	Rail->SetPreview(bPreviewHit, PreviewPoint, bPreviewValid);
}

void ADCTakeDirector::HandlePrimaryPressed()
{
	if (Phase != EDCPhase::Preparation)
	{
		return;
	}
	if (!bPreviewHit)
	{
		Flash(TEXT("Aim at the floor within 8 m to place a rail point"));
		return;
	}
	if (!bPreviewValid)
	{
		Flash(Rail->NumPoints() >= ADCRail::MaxPoints ? TEXT("Rail point limit reached") : TEXT("Rail points go on the floor"));
		return;
	}
	FString Reason;
	if (!Rail->TryAddPoint(PreviewPoint, Reason))
	{
		Flash(Reason);
	}
}

void ADCTakeDirector::HandleSecondaryPressed()
{
	if (Phase != EDCPhase::Preparation)
	{
		return;
	}
	if (!Rail->RemovePointNear(bPreviewHit ? PreviewPoint : FVector(TNumericLimits<float>::Max()), 150.f))
	{
		Flash(Rail->NumPoints() == 0 ? TEXT("No rail points to remove") : TEXT("Removing that point would route the rail through scenery"));
	}
}

void ADCTakeDirector::HandleClearRail()
{
	if (Phase == EDCPhase::Preparation)
	{
		Rail->Clear();
		Flash(TEXT("Rail cleared"));
	}
}

bool ADCTakeDirector::IsNearClapper() const
{
	ADCCharacter* Player = GetPlayer();
	return Player && Clapperboard && FVector::Dist2D(Player->GetActorLocation(), Clapperboard->GetActorLocation()) < Clapperboard->InteractRange;
}

bool ADCTakeDirector::CanMountRail() const
{
	ADCCharacter* Player = GetPlayer();
	if (!Player || Player->IsOnRail() || !Rail->IsUsable() || (Phase != EDCPhase::Preparation && Phase != EDCPhase::Filming))
	{
		return false;
	}
	float Gap = 0.f;
	Rail->FindClosestDistance(Player->GetActorLocation(), Gap);
	return Gap < MountRange;
}

void ADCTakeDirector::HandleInteract()
{
	ADCCharacter* Player = GetPlayer();
	if (!Player)
	{
		return;
	}

	if (Phase == EDCPhase::Results)
	{
		HandleRestart();
	}
	else if (Player->IsOnRail())
	{
		Player->DismountRail();
	}
	else if (Phase == EDCPhase::Preparation && IsNearClapper())
	{
		HandleStartTake();
	}
	else if (CanMountRail())
	{
		float Gap = 0.f;
		Player->MountRail(Rail, Rail->FindClosestDistance(Player->GetActorLocation(), Gap));
	}
	else
	{
		Flash(TEXT("Nothing to interact with here"), 1.5f);
	}
}

// ---------------------------------------------------------------------------------------------
// Helpers

int32 ADCTakeDirector::GetFocusShotIndex() const
{
	// The pending shot whose window is open, otherwise the next one to open.
	int32 Next = INDEX_NONE;
	for (int32 i = 0; i < Shots.Num(); ++i)
	{
		const FDCShot& Shot = Shots[i];
		if (Shot.Status != EDCShotStatus::Pending)
		{
			continue;
		}
		if (SceneTime >= Shot.WindowStart && SceneTime <= Shot.WindowEnd)
		{
			return i;
		}
		if (Shot.WindowStart > SceneTime && (Next == INDEX_NONE || Shot.WindowStart < Shots[Next].WindowStart))
		{
			Next = i;
		}
	}
	return Next;
}

int32 ADCTakeDirector::GetCapturedBonusCount() const
{
	int32 Count = 0;
	for (const FDCShot& Shot : Shots)
	{
		Count += (!Shot.bMandatory && Shot.Status == EDCShotStatus::Captured) ? 1 : 0;
	}
	return Count;
}

FString ADCTakeDirector::GetCutName() const
{
	if (!bApproved)
	{
		return TEXT("Retake required");
	}
	const int32 Bonus = GetCapturedBonusCount();
	int32 BonusTotal = 0;
	for (const FDCShot& Shot : Shots)
	{
		BonusTotal += Shot.bMandatory ? 0 : 1;
	}
	if (Bonus == 0)
	{
		return TEXT("Studio Cut");
	}
	return Bonus == BonusTotal ? TEXT("Director's Cut") : TEXT("Extended Cut");
}

FString ADCTakeDirector::GetMessage() const
{
	return GetWorld()->GetTimeSeconds() < MessageExpireTime ? Message : FString();
}

void ADCTakeDirector::Flash(const FString& Text, float Seconds)
{
	Message = Text;
	MessageExpireTime = GetWorld()->GetTimeSeconds() + Seconds;
}

void ADCTakeDirector::RefreshMarkerColors()
{
	for (const FDCShot& Shot : Shots)
	{
		if (Shot.Marker)
		{
			Shot.Marker->ShowStatus(Shot.Status);
		}
	}
}

ADCCharacter* ADCTakeDirector::GetPlayer() const
{
	return Cast<ADCCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
}

FString ADCTakeDirector::DescribeFail(EDCFail Fail)
{
	switch (Fail)
	{
	case EDCFail::None:          return TEXT("Rolling - hold it!");
	case EDCFail::WrongTime:     return TEXT("Not yet - wait for the window");
	case EDCFail::WrongPosition: return TEXT("Get to the shot mark");
	case EDCFail::NotRecording:  return TEXT("Hold Left Mouse to record");
	case EDCFail::Sprinting:     return TEXT("Can't record while sprinting");
	case EDCFail::OutOfFrame:    return TEXT("Frame the subject");
	case EDCFail::TooClose:      return TEXT("Too close - back up");
	case EDCFail::TooFar:        return TEXT("Too far - move closer");
	case EDCFail::Blocked:       return TEXT("View is blocked");
	}
	return FString();
}
