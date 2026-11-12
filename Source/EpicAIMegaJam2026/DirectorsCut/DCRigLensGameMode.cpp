// Director's Cut prototype 5 - game mode.

#include "DCRigLensGameMode.h"
#include "DCRigCharacter.h"
#include "DCRigLensDirector.h"
#include "DCRigLensHUD.h"
#include "EngineUtils.h"
#include "Engine/World.h"

ADCRigLensGameMode::ADCRigLensGameMode()
{
	DefaultPawnClass = ADCRigCharacter::StaticClass();
	HUDClass = ADCRigLensHUD::StaticClass();
}

void ADCRigLensGameMode::StartPlay()
{
	// A level copied from prototype 4 may still hold a plain Replica director: replace it, keeping its settings.
	ADCReplicaDirector* Old = nullptr;
	bool bHasRigLens = false;
	for (TActorIterator<ADCReplicaDirector> It(GetWorld()); It; ++It)
	{
		if (It->IsA<ADCRigLensDirector>())
		{
			bHasRigLens = true;
		}
		else
		{
			Old = *It;
		}
	}
	if (!bHasRigLens)
	{
		const FTransform Transform = Old ? Old->GetActorTransform() : FTransform::Identity;
		ADCRigLensDirector* New = GetWorld()->SpawnActor<ADCRigLensDirector>(ADCRigLensDirector::StaticClass(), Transform);
		if (Old)
		{
			New->TakeDuration = Old->TakeDuration;
			New->CountdownDuration = Old->CountdownDuration;
			New->MaxRails = Old->MaxRails;
			New->MaxPointsPerRail = Old->MaxPointsPerRail;
			New->MaxTripods = Old->MaxTripods;
			New->RailStandoff = Old->RailStandoff;
			New->SampleRate = Old->SampleRate;
			New->PositionTolerance = Old->PositionTolerance;
			New->AngleTolerance = Old->AngleTolerance;
			New->FramingWeight = Old->FramingWeight;
			New->bShowLiveMatch = Old->bShowLiveMatch;
			New->MaxSprayMarks = Old->MaxSprayMarks;
			Old->Destroy();
		}
	}

	Super::StartPlay();
}
