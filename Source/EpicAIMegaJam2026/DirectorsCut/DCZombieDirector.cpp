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
}

void ADCZombieDirector::BeginPlay()
{
	Super::BeginPlay();

	// Open on the director, out of black: he recruits the player, then shows them the reference cut.
	Phase = EDCPhase::Storyboard;
	Flash(FString(), 0.f);
	if (bShowCutscenes && IntroCutscene.Num() > 0)
	{
		FadeInFromBlack();
		PlayCutscene(IntroCutscene, [this]() { StartReferenceCut(); });
	}
	else
	{
		bReferenceAutoStart = true; // the player controller isn't ready until the first tick
	}
}

// ---------------------------------------------------------------------------------------------
// Phase flow: intro cutscene -> reference cut -> briefing cutscene -> preparation (scrub the timeline)
// -> action cutscene -> shooting -> review

void ADCZombieDirector::OnReferenceCutFinished()
{
	FadeThrough([this]()
	{
		StopReferenceCut();
		BriefAndPrepare();
	});
}

void ADCZombieDirector::BriefAndPrepare()
{
	PlayCutscene(BriefingCutscene, [this]() { EnterPreparation(); });
}

void ADCZombieDirector::EnterPreparation()
{
	ResetScene();
	Flash(TEXT("Rehearsal is playing. H pauses/resumes, comma/period change speed. Hold Q/E to scrub (and pause)."), 7.f);
}

void ADCZombieDirector::HandleStartTake()
{
	if (HandleCutsceneInput())
	{
		return;
	}
	if (Phase == EDCPhase::Storyboard)
	{
		// Enter skips the reference cut (or leaves the storyboard page) for the briefing.
		if (IsReferencePlaying())
		{
			OnReferenceCutFinished();
		}
		else
		{
			FadeThrough([this]() { BriefAndPrepare(); });
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
