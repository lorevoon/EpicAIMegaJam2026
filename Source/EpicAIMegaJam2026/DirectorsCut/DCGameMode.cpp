// Director's Cut prototype - game mode.

#include "DCGameMode.h"
#include "DCCharacter.h"
#include "DCHUD.h"
#include "DCTakeDirector.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"

ADCGameMode::ADCGameMode()
{
	DefaultPawnClass = ADCCharacter::StaticClass();
	PlayerControllerClass = APlayerController::StaticClass();
	HUDClass = ADCHUD::StaticClass();
}

void ADCGameMode::StartPlay()
{
	// Reuse a director placed in the level, otherwise build the whole set at runtime.
	bool bHasDirector = false;
	for (TActorIterator<ADCTakeDirector> It(GetWorld()); It; ++It)
	{
		bHasDirector = true;
	}
	if (!bHasDirector)
	{
		FActorSpawnParameters Params;
		Params.bDeferConstruction = true;
		ADCTakeDirector* Director = GetWorld()->SpawnActor<ADCTakeDirector>(ADCTakeDirector::StaticClass(), FTransform::Identity, Params);
		Director->bSpawnLighting = bSpawnLighting;
		Director->FinishSpawning(FTransform::Identity);
	}

	Super::StartPlay();
}

AActor* ADCGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// The take always starts from the director's fixed start mark.
	if (!StartSpot)
	{
		const FTransform Start = GetDefault<ADCTakeDirector>()->GetStartTransform();
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		StartSpot = GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), Start, Params);
	}
	return StartSpot;
}
