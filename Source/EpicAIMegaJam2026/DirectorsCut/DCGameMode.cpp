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
	// Use the director placed in the level, or add one; either way it reads the scene from the level.
	bool bHasDirector = false;
	for (TActorIterator<ADCTakeDirector> It(GetWorld()); It; ++It)
	{
		bHasDirector = true;
	}
	if (!bHasDirector)
	{
		GetWorld()->SpawnActor<ADCTakeDirector>(ADCTakeDirector::StaticClass(), FTransform::Identity);
	}

	Super::StartPlay();
}

AActor* ADCGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// The take always starts from the level's DCStartMark.
	if (!StartSpot)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		StartSpot = GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), ADCTakeDirector::FindStartTransform(GetWorld()), Params);
	}
	return StartSpot;
}
