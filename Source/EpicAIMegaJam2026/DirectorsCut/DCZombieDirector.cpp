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

	// Open on the storyboard, which starts with the reference cut (built once the crew list above is known).
	Phase = EDCPhase::Storyboard;
	bReferenceAutoStart = true;
	Flash(FString(), 0.f);
}

// ---------------------------------------------------------------------------------------------
// Phase flow: storyboard -> preparation (scrub the timeline) -> shooting -> review

void ADCZombieDirector::EnterPreparation()
{
	ResetScene();
	Flash(TEXT("Rehearsal is playing. H pauses/resumes, comma/period change speed. Hold Q/E to scrub (and pause)."), 7.f);
}

void ADCZombieDirector::HandleStartTake()
{
	if (Phase == EDCPhase::Storyboard)
	{
		// Enter skips the reference cut to the storyboard page, then starts preparing.
		if (IsReferencePlaying())
		{
			StopReferenceCut();
		}
		else
		{
			EnterPreparation();
		}
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

// ---------------------------------------------------------------------------------------------
// Crew in shot: only used to keep the reference cut's shots clean

bool ADCZombieDirector::IsZombieVisible(const ADCMultiCam* Cam, FString* OutName) const
{
	if (!Cam)
	{
		return false;
	}
	const FTransform CamXf = Cam->GetCamera()->GetComponentTransform();
	const FVector CamLoc = CamXf.GetLocation();
	const float HalfTan = FMath::Tan(FMath::DegreesToRadians(Cam->GetFOV() * 0.5f));
	const float Aspect = GetViewAspect();
	const FVector2D Frame = DCFrame::CaptureExtent(Aspect);
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
			if (FMath::Abs((Local.Y / Local.X) / HalfTan) > Frame.X
				|| FMath::Abs((Local.Z / Local.X) / (HalfTan / Aspect)) > Frame.Y)
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

void ADCZombieDirector::GetPlanPreview(TArray<FString>& OutFramedBeats) const
{
	OutFramedBeats.Reset();
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
}
