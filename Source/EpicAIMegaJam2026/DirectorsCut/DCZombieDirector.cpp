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
	Super::Tick(DeltaSeconds);
}

// ---------------------------------------------------------------------------------------------
// Phase flow: storyboard -> preparation (scrub the timeline) -> shooting -> review

void ADCZombieDirector::EnterPreparation()
{
	ResetScene();
	Flash(TEXT("Rehearsal is playing. H pauses/resumes, comma/period change speed. Wheel or arrows scrub and pause."), 7.f);
}

void ADCZombieDirector::HandleStartTake()
{
	if (Phase == EDCPhase::Storyboard)
	{
		EnterPreparation();
		return;
	}
	Super::HandleStartTake();
}

void ADCZombieDirector::HandleRestart()
{
	if (Phase != EDCPhase::Storyboard)
	{
		Super::HandleRestart();
	}
}

void ADCZombieDirector::HandleScrub(float DeltaSeconds)
{
	Super::HandleScrub(DeltaSeconds);
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
			// Only the capture frame is recorded; zombies in the dimmed margin around it don't count.
			if (FMath::Abs((Local.Y / Local.X) / HalfTan) > DCFrame::CaptureFraction
				|| FMath::Abs((Local.Z / Local.X) / (HalfTan / Aspect)) > DCFrame::CaptureFraction)
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
	// A paused recording films nothing, so nothing on screen can ruin it.
	bZombieInShot = !IsRecordingPaused() && IsZombieVisible(GetCameraSlot(ViewingSlot), &Name);
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
		if (SceneTime >= State.Beat->GetCaptureStartTime() && SceneTime <= State.Beat->WindowEnd && IsFramed(State, Cam))
		{
			OutFramedBeats.Add(State.Beat->DisplayName);
		}
	}
	bOutZombie = IsZombieVisible(Cam, nullptr);
}
