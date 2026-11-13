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

	// Open on the storyboard, which starts with the reference cut.
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
