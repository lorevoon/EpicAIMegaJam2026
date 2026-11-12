// Director's Cut prototype 3 - game mode.

#include "DCMultiCamGameMode.h"
#include "DCMultiCamCharacter.h"
#include "DCMultiCamDirector.h"
#include "DCMultiCamHUD.h"
#include "DCTakeDirector.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"

ADCMultiCamGameMode::ADCMultiCamGameMode()
{
	DefaultPawnClass = ADCMultiCamCharacter::StaticClass();
	PlayerControllerClass = APlayerController::StaticClass();
	HUDClass = ADCMultiCamHUD::StaticClass();
}

void ADCMultiCamGameMode::StartPlay()
{
	// Use the director placed in the level, or add one; either way it reads the scene from the level.
	bool bHasDirector = false;
	for (TActorIterator<ADCMultiCamDirector> It(GetWorld()); It; ++It)
	{
		bHasDirector = true;
	}
	if (!bHasDirector)
	{
		GetWorld()->SpawnActor<ADCMultiCamDirector>(ADCMultiCamDirector::StaticClass(), FTransform::Identity);
	}

	Super::StartPlay();
}

AActor* ADCMultiCamGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (!StartSpot)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		StartSpot = GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), ADCTakeDirector::FindStartTransform(GetWorld()), Params);
	}
	return StartSpot;
}
