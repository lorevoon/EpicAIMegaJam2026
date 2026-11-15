// Director's Cut - "Love in the Ruins" director.

#include "DCZombieDirector.h"
#include "DCBeat.h"
#include "DCMultiCam.h"
#include "DCPerformer.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

ADCZombieDirector::ADCZombieDirector()
{
	TakeDuration = 60.f;
	// As many cameras as you like, of any kind.
	MaxCameras = 0;

	for (const TCHAR* Name : { TEXT("Default"), TEXT("Almost"), TEXT("Disgust"), TEXT("Joy"), TEXT("ThatsIt"),
		TEXT("Recruit"), TEXT("Vision"), TEXT("Camera"), TEXT("Action"), TEXT("Explaining"), TEXT("Explaining2") })
	{
		ConstructorHelpers::FObjectFinder<UTexture2D> Finder(*FString::Printf(TEXT("/Game/DirectorsCut/UI/Director/Director_%s.Director_%s"), Name, Name));
		if (Finder.Succeeded())
		{
			Faces.Add(Finder.Object);
		}
	}
}

void ADCZombieDirector::BeginPlay()
{
	Super::BeginPlay();

	LoadActScript();

	// Open out of black: the film's first act on the main menu, later acts straight on their intro.
	Phase = EDCPhase::Reference;
	Flash(FString(), 0.f);
	FadeInFromBlack();
	if (bShowMainMenu && ActIndex <= 0)
	{
		bInMainMenu = true;
	}
	else
	{
		PlayIntro();
	}
}

void ADCZombieDirector::StartFromMenu()
{
	if (!bInMainMenu || FadeDirection > 0)
	{
		return;
	}
	FadeThrough([this]()
	{
		bInMainMenu = false;
		PlayIntro();
	});
}

void ADCZombieDirector::PlayIntro()
{
	// The director recruits the player, then shows them the reference cut.
	if (bShowCutscenes && IntroCutscene.Num() > 0)
	{
		PlayCutscene(IntroCutscene, [this]()
		{
			StartReferenceCut();
			if (!IsReferencePlaying())
			{
				OnReferenceCutFinished(); // no reference film to show
			}
		});
	}
	else
	{
		bReferenceAutoStart = true; // the player controller may not be ready until the next tick
	}
}

// ---------------------------------------------------------------------------------------------
// Phase flow: (main menu) -> intro cutscene -> reference film -> briefing cutscene -> preparation
// -> action cutscene -> shooting -> review -> outro cutscene -> the next act (DCActs.cpp)

void ADCZombieDirector::OnReferenceCutFinished()
{
	FadeThrough([this]()
	{
		StopReferenceCut(); // into preparation, with the briefing over it
		PlayCutscene(BriefingCutscene, [this]() { StartSetOverview(); });
	});
}

void ADCZombieDirector::HandleStartTake()
{
 if (IsSetOverview() || IsReferenceFocused()) return;
	if (bInMainMenu)
	{
		StartFromMenu(); // Enter works as well as clicking START
		return;
	}
	if (HandleCutsceneInput())
	{
		return;
	}
	if (Phase == EDCPhase::Reference)
	{
		return; // the reference film can't be skipped: it plays through, then the briefing follows
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
