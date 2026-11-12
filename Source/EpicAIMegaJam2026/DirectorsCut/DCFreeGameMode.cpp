// Director's Cut prototype 2 - game mode.

#include "DCFreeGameMode.h"
#include "DCFreeCharacter.h"
#include "DCFreeDirector.h"
#include "DCFreeHUD.h"
#include "DCTakeDirector.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"

ADCFreeGameMode::ADCFreeGameMode()
{
	DefaultPawnClass = ADCFreeCharacter::StaticClass();
	PlayerControllerClass = APlayerController::StaticClass();
	HUDClass = ADCFreeHUD::StaticClass();
}

void ADCFreeGameMode::StartPlay()
{
	// Use the director placed in the level, or add one; either way it reads the scene from the level.
	bool bHasDirector = false;
	for (TActorIterator<ADCFreeDirector> It(GetWorld()); It; ++It)
	{
		bHasDirector = true;
	}
	if (!bHasDirector)
	{
		GetWorld()->SpawnActor<ADCFreeDirector>(ADCFreeDirector::StaticClass(), FTransform::Identity);
	}

	Super::StartPlay();
}

AActor* ADCFreeGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (!StartSpot)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		StartSpot = GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), ADCTakeDirector::FindStartTransform(GetWorld()), Params);
	}
	return StartSpot;
}
