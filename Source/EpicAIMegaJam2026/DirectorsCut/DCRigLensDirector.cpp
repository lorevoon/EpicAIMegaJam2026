// Director's Cut prototype 5 - "Rigs & Lenses" director.

#include "DCRigLensDirector.h"
#include "DCJib.h"
#include "DCRigCharacter.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"

namespace
{
	constexpr float JibMountRange = 220.f;
	constexpr float JibRemoveRange = 250.f;
	constexpr float JibStartPitch = 10.f;
	const FLinearColor JibColor(0.12f, 0.12f, 0.14f);
}

ADCRigLensDirector::ADCRigLensDirector()
{
}

void ADCRigLensDirector::BeginPlay()
{
	Super::BeginPlay();

	FActorSpawnParameters Params;
	Params.Owner = this;
	JibGhost = GetWorld()->SpawnActor<ADCJib>(ADCJib::StaticClass(), FTransform::Identity, Params);
	JibGhost->SetActorHiddenInGame(true);
}

void ADCRigLensDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (JibGhost && (Phase != EDCPhase::Preparation || IsShowingFilm()))
	{
		JibGhost->SetActorHiddenInGame(true);
	}
}

ADCRigCharacter* ADCRigLensDirector::GetRigPlayer() const
{
	return Cast<ADCRigCharacter>(GetPlayer());
}

ADCJib* ADCRigLensDirector::FindNearestJib(const FVector& Location, float Radius) const
{
	ADCJib* Best = nullptr;
	float BestDist = Radius;
	for (ADCJib* Jib : Jibs)
	{
		const float D = FVector::Dist2D(Jib->GetActorLocation(), Location);
		if (D < BestDist)
		{
			BestDist = D;
			Best = Jib;
		}
	}
	return Best;
}

// ---------------------------------------------------------------------------------------------
// Preparation: jib placement

void ADCRigLensDirector::UpdatePrepPreview()
{
	Super::UpdatePrepPreview();

	ADCFreeCharacter* Player = GetPlayer();
	const bool bShow = Tool == EDCRigTool::Jib && bPreviewHit && Player && !Player->IsOnRig();
	JibGhost->SetActorHiddenInGame(!bShow);
	bJibPreviewValid = false;
	if (bShow)
	{
		bJibPreviewValid = PreviewNormal.Z > 0.7f && Jibs.Num() < MaxJibs;
		// The arm points where you are looking.
		JibPreviewYaw = Player->GetCamera()->GetComponentRotation().Yaw;
		JibGhost->SetActorLocation(PreviewSurface);
		JibGhost->SetArm(JibPreviewYaw, JibStartPitch);
		JibGhost->SetTint(bJibPreviewValid ? FLinearColor(0.2f, 1.f, 0.4f) : FLinearColor(1.f, 0.15f, 0.1f));
	}
}

void ADCRigLensDirector::HandlePrimaryPressed()
{
	if (Phase != EDCPhase::Preparation || Tool != EDCRigTool::Jib || IsShowingFilm())
	{
		Super::HandlePrimaryPressed();
		return;
	}
	if (!bPreviewHit)
	{
		Flash(TEXT("Aim at the floor within 15 m"));
		return;
	}
	if (Jibs.Num() >= MaxJibs)
	{
		Flash(FString::Printf(TEXT("Both jibs are out (%d max) - Right Mouse removes one"), MaxJibs));
		return;
	}
	if (!bJibPreviewValid)
	{
		Flash(TEXT("A jib needs flat floor"));
		return;
	}
	FActorSpawnParameters Params;
	Params.Owner = this;
	ADCJib* Jib = GetWorld()->SpawnActor<ADCJib>(ADCJib::StaticClass(), FTransform(PreviewSurface), Params);
	Jib->SetArm(JibPreviewYaw, JibStartPitch);
	Jib->SetTint(JibColor);
	Jibs.Add(Jib);
	Flash(TEXT("Jib placed - walk up to its post and press E to operate it"), 3.f);
}

void ADCRigLensDirector::HandleSecondaryPressed()
{
	if (Phase != EDCPhase::Preparation || Tool != EDCRigTool::Jib || IsShowingFilm())
	{
		Super::HandleSecondaryPressed();
		return;
	}
	ADCJib* Jib = bPreviewHit ? FindNearestJib(PreviewSurface, JibRemoveRange) : nullptr;
	if (!Jib)
	{
		Flash(TEXT("Aim at a jib's base to remove it"), 1.5f);
		return;
	}
	Jibs.Remove(Jib);
	Jib->Destroy();
}

void ADCRigLensDirector::HandleClearRigs()
{
	const bool bPrep = Phase == EDCPhase::Preparation;
	Super::HandleClearRigs();
	if (bPrep)
	{
		for (ADCJib* Jib : Jibs)
		{
			Jib->Destroy();
		}
		Jibs.Reset();
	}
}

void ADCRigLensDirector::HandleInteract()
{
	ADCRigCharacter* Player = GetRigPlayer();
	const bool bCanMount = Player && !Player->IsOnRig() && !IsShowingFilm()
		&& (Phase == EDCPhase::Filming || (Phase == EDCPhase::Preparation && !IsNearClapper()));
	if (bCanMount)
	{
		if (ADCJib* Jib = FindNearestJib(Player->GetActorLocation(), JibMountRange))
		{
			Player->MountJib(Jib);
			return;
		}
	}
	Super::HandleInteract();
}

// ---------------------------------------------------------------------------------------------
// HUD text

FString ADCRigLensDirector::GetInteractHint() const
{
	const ADCRigCharacter* Player = GetRigPlayer();
	if (Player && Phase != EDCPhase::Countdown && Phase != EDCPhase::Results)
	{
		if (Player->GetMountedJib())
		{
			return TEXT("ON JIB - W/S boom up/down, A/D swing. It's heavy: start moves early and ease off early.   E step off");
		}
		if (Player->GetMountedRail())
		{
			return TEXT("ON DOLLY - W/S push, let go to coast, Space brakes.   E step off");
		}
		if (Player->GetMountedTripod())
		{
			return TEXT("ON TRIPOD - fluid head: pans are smooth but slow.   Wheel height (Shift+Wheel on the zoom)   E step off");
		}
		if (!(Phase == EDCPhase::Preparation && IsNearClapper()) && FindNearestJib(Player->GetActorLocation(), JibMountRange))
		{
			return TEXT("E: operate the jib");
		}
	}
	return Super::GetInteractHint();
}

FString ADCRigLensDirector::GetBrief() const
{
	return FString::Printf(TEXT("Reproduce the reference film: %d shots in %.0f s. Pick your rigs and glass - the reference was shot on a 24mm."),
		GetShots().Num(), TakeDuration);
}

FString ADCRigLensDirector::GetShotTip(const FDCRefShot& Shot) const
{
	if (Shot.Samples > 0)
	{
		const float Mine = ADCRigCharacter::FovToFocal(Shot.GetFov());
		const float Ref = ADCRigCharacter::FovToFocal(Shot.GetRefFov());
		const bool bLensDiffers = FMath::Abs(Mine - Ref) / Ref > 0.25f;
		if (bLensDiffers && Shot.GetFraming() >= 0.6f && Shot.GetPositionError() > 300.f)
		{
			return FString::Printf(TEXT("Framed with a %.0fmm from %.1f m away - the reference's %.0fmm up close has a different perspective"),
				Mine, Shot.GetPositionError() / 100.f, Ref);
		}
		if (bLensDiffers && Shot.GetFraming() < 0.6f)
		{
			return FString::Printf(TEXT("Lens mismatch: you shot this on ~%.0fmm, the reference on ~%.0fmm"), Mine, Ref);
		}
	}
	return Super::GetShotTip(Shot);
}
