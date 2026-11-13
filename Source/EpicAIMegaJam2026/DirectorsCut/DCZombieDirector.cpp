// Director's Cut - "Love in the Ruins" director.

#include "DCZombieDirector.h"
#include "DCBeat.h"
#include "DCMultiCam.h"
#include "DCPerformer.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
	const float RehearsalSpeeds[] = { 0.5f, 1.f, 2.f, 4.f };
}

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
	// The rehearsal runs inside preparation: the scene plays while the player walks the set and rigs it.
	if (Phase == EDCPhase::Preparation && bRehearsalPlaying)
	{
		SceneTime = FMath::Min(SceneTime + DeltaSeconds * GetRehearsalSpeed(), TakeDuration);
		bRehearsalPlaying = SceneTime < TakeDuration;
		for (ADCPerformer* Performer : Performers)
		{
			Performer->Evaluate(SceneTime);
		}
	}
	Super::Tick(DeltaSeconds);
}

// ---------------------------------------------------------------------------------------------
// Phase flow: storyboard -> preparation (with rehearsal) -> shooting -> evaluation

void ADCZombieDirector::EnterPreparation(bool bPlayRehearsal)
{
	ResetScene();
	bRehearsalPlaying = bPlayRehearsal;
	if (bPlayRehearsal)
	{
		Flash(TEXT("The rehearsal is playing. Watch where each scene happens - and where the film crew go - while you place cameras."), 7.f);
	}
}

void ADCZombieDirector::HandleStartTake()
{
	if (Phase == EDCPhase::Storyboard)
	{
		EnterPreparation(true);
		return;
	}
	Super::HandleStartTake();
}

void ADCZombieDirector::HandleInteract()
{
	if (Phase == EDCPhase::Storyboard)
	{
		EnterPreparation(true);
		return;
	}
	Super::HandleInteract();
}

void ADCZombieDirector::HandleRestart()
{
	if (Phase != EDCPhase::Storyboard)
	{
		Super::HandleRestart();
	}
}

void ADCZombieDirector::HandleRehearsal()
{
	// V: watch the rehearsal again from the top.
	if (Phase == EDCPhase::Preparation || Phase == EDCPhase::Results)
	{
		EnterPreparation(true);
	}
}

float ADCZombieDirector::GetRehearsalSpeed() const
{
	return RehearsalSpeeds[RehearsalSpeedIndex];
}

void ADCZombieDirector::HandleTransportToggle()
{
	if (Phase != EDCPhase::Preparation)
	{
		Super::HandleTransportToggle();
		return;
	}
	if (!bRehearsalPlaying && SceneTime >= TakeDuration)
	{
		SceneTime = 0.f; // play again from the top
	}
	bRehearsalPlaying = !bRehearsalPlaying;
}

void ADCZombieDirector::HandleTransportSpeed(int32 Direction)
{
	if (Phase == EDCPhase::Preparation)
	{
		RehearsalSpeedIndex = FMath::Clamp(RehearsalSpeedIndex + Direction, 0, int32(UE_ARRAY_COUNT(RehearsalSpeeds)) - 1);
	}
}

void ADCZombieDirector::HandleScrub(float DeltaSeconds)
{
	if (Phase != EDCPhase::Preparation)
	{
		Super::HandleScrub(DeltaSeconds);
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
	bRehearsalPlaying = false;
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
