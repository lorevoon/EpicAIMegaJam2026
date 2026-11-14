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

	// Open directly on the reference film.
	Phase = EDCPhase::Reference;
	bReferenceAutoStart = true;
	Flash(FString(), 0.f);
}

// ---------------------------------------------------------------------------------------------
// Phase flow: reference film -> preparation -> shooting -> review

void ADCZombieDirector::HandleStartTake()
{
	if (Phase == EDCPhase::Reference)
	{
		StopReferenceCut(); // Skipping the film enters preparation directly.
		return;
	}
	Super::HandleStartTake();
}

void ADCZombieDirector::HandleRestart()
{
	if (Phase != EDCPhase::Reference)
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
