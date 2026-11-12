// Director's Cut - "Love in the Ruins" director.

#include "DCZombieDirector.h"
#include "DCBeat.h"
#include "DCMultiCam.h"
#include "DCPerformer.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

ADCZombieDirector::ADCZombieDirector()
{
	TakeDuration = 60.f;
	// As many cameras as you like, of any kind.
	MaxCameras = 0;
	bAllowMovingRigs = true;
}

void ADCZombieDirector::BeginPlay()
{
	Super::BeginPlay();

	for (ADCPerformer* Performer : Performers)
	{
		if (Performer->bRuinsShot)
		{
			Zombies.Add(Performer);
		}
	}

	// Open on the storyboard.
	Phase = EDCPhase::Storyboard;
	Flash(FString(), 0.f);
}

void ADCZombieDirector::Tick(float DeltaSeconds)
{
	if (Phase == EDCPhase::Storyboard)
	{
		AActor::Tick(DeltaSeconds);
		return;
	}
	if (Phase == EDCPhase::Rehearsal)
	{
		AActor::Tick(DeltaSeconds);
		SceneTime = FMath::Min(SceneTime + DeltaSeconds, TakeDuration);
		for (ADCPerformer* Performer : Performers)
		{
			Performer->Evaluate(SceneTime);
		}
		if (SceneTime >= TakeDuration)
		{
			EnterPlanning();
		}
		return;
	}
	Super::Tick(DeltaSeconds);
}

// ---------------------------------------------------------------------------------------------
// Phase flow: storyboard -> rehearsal -> planning -> shooting

void ADCZombieDirector::StartRehearsal()
{
	ResetScene();
	Phase = EDCPhase::Rehearsal;
	SceneTime = 0.f;
	Flash(TEXT("REHEARSAL - walk the set and watch. Learn where each scene happens... and where the zombies go."), 6.f);
}

void ADCZombieDirector::EnterPlanning()
{
	ResetScene();
	Flash(TEXT("Plan your coverage: place as many cameras as you like (T: static / dolly / crane). Hold Left/Right to scrub the scene."), 7.f);
}

void ADCZombieDirector::HandleStartTake()
{
	switch (Phase)
	{
	case EDCPhase::Storyboard:
		StartRehearsal();
		break;
	case EDCPhase::Rehearsal:
		EnterPlanning(); // Enter skips the rest of the rehearsal
		break;
	default:
		Super::HandleStartTake();
		break;
	}
}

void ADCZombieDirector::HandleInteract()
{
	if (Phase == EDCPhase::Storyboard)
	{
		StartRehearsal();
	}
	else if (Phase != EDCPhase::Rehearsal)
	{
		Super::HandleInteract();
	}
}

void ADCZombieDirector::HandleRestart()
{
	if (Phase == EDCPhase::Rehearsal)
	{
		StartRehearsal();
	}
	else if (Phase != EDCPhase::Storyboard)
	{
		Super::HandleRestart();
	}
}

void ADCZombieDirector::HandleRehearsal()
{
	if (Phase == EDCPhase::Preparation || Phase == EDCPhase::Results)
	{
		StartRehearsal();
	}
}

void ADCZombieDirector::HandleScrub(float DeltaSeconds)
{
	if (Phase != EDCPhase::Preparation)
	{
		return;
	}
	SceneTime = FMath::Clamp(SceneTime + DeltaSeconds, 0.f, TakeDuration);
	for (ADCPerformer* Performer : Performers)
	{
		Performer->Evaluate(SceneTime);
	}
}

void ADCZombieDirector::ResetScene()
{
	Super::ResetScene();
	ZombieSeconds = 0.f;
	bZombieInShot = false;
	ZombieInShotName.Reset();
	NetScore = 0;
}

// ---------------------------------------------------------------------------------------------
// Shooting: zombies on the live camera cost points

float ADCZombieDirector::GetAspect() const
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

bool ADCZombieDirector::IsZombieVisible(const ADCMultiCam* Cam, FString* OutName) const
{
	if (!Cam)
	{
		return false;
	}
	const FTransform CamXf = Cam->GetCamera()->GetComponentTransform();
	const FVector CamLoc = CamXf.GetLocation();
	const float HalfTan = FMath::Tan(FMath::DegreesToRadians(Cam->GetFOV() * 0.5f));
	const float Aspect = GetAspect();
	const FCollisionQueryParams Params = MakeTraceParams(SCENE_QUERY_STAT_NAME_ONLY(DCZombieVisibility));

	for (ADCPerformer* Zombie : Zombies)
	{
		// Any glimpse of head or body anywhere in the full frame counts.
		for (EDCAnchorPart Part : { EDCAnchorPart::Head, EDCAnchorPart::Body })
		{
			const FVector P = Zombie->GetAnchor(Part)->GetComponentLocation();
			const FVector Local = CamXf.InverseTransformPositionNoScale(P);
			if (Local.X <= 1.f || Local.Size() > ZombieSightRange)
			{
				continue;
			}
			if (FMath::Abs((Local.Y / Local.X) / HalfTan) > 1.f || FMath::Abs((Local.Z / Local.X) / (HalfTan / Aspect)) > 1.f)
			{
				continue;
			}
			FHitResult Hit;
			if (!GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, P, ECC_Visibility, Params) || Hit.GetActor() == Zombie)
			{
				if (OutName)
				{
					*OutName = Zombie->DisplayName;
				}
				return true;
			}
		}
	}
	return false;
}

void ADCZombieDirector::EvaluateBeats(float PrevTime, float NowTime)
{
	FString Name;
	bZombieInShot = IsZombieVisible(GetCameraSlot(ViewingSlot), &Name);
	if (bZombieInShot)
	{
		ZombieSeconds += NowTime - PrevTime;
		ZombieInShotName = Name;
	}
	Super::EvaluateBeats(PrevTime, NowTime);
}

int32 ADCZombieDirector::GetBeatPoints() const
{
	int32 Points = 0;
	for (const FDCMultiCamBeat& State : Beats)
	{
		Points += State.Status == EDCShotStatus::Captured ? State.Beat->Points : 0;
	}
	return Points;
}

void ADCZombieDirector::EndTake()
{
	if (Phase != EDCPhase::Filming)
	{
		return;
	}
	Super::EndTake();
	bZombieInShot = false;
	NetScore = bApproved ? FMath::Max(0, Score - GetPenalty()) : 0;
	BestNet = FMath::Max(BestNet, NetScore);
}

void ADCZombieDirector::GetPlanPreview(TArray<FString>& OutFramedBeats, bool& bOutZombie) const
{
	OutFramedBeats.Reset();
	bOutZombie = false;
	const ADCMultiCam* Cam = GetCameraSlot(ViewingSlot);
	if (Phase != EDCPhase::Preparation || !Cam)
	{
		return;
	}
	for (const FDCMultiCamBeat& State : Beats)
	{
		if (SceneTime >= State.Beat->WindowStart && SceneTime <= State.Beat->WindowEnd && IsFramed(State, Cam))
		{
			OutFramedBeats.Add(State.Beat->DisplayName);
		}
	}
	bOutZombie = IsZombieVisible(Cam, nullptr);
}
