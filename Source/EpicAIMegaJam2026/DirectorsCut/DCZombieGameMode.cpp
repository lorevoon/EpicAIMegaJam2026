// Director's Cut prototype 5 - game mode.

#include "DCZombieGameMode.h"
#include "DCMultiCamCharacter.h"
#include "DCTakeDirector.h"
#include "DCZombieDirector.h"
#include "DCZombieHUD.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"

ADCZombieGameMode::ADCZombieGameMode()
{
	DefaultPawnClass = ADCMultiCamCharacter::StaticClass();
	PlayerControllerClass = APlayerController::StaticClass();
	HUDClass = ADCZombieHUD::StaticClass();
}

void ADCZombieGameMode::StartPlay()
{
	// Use the director placed in the level, or add one; either way it reads the scene from the level.
	bool bHasDirector = false;
	for (TActorIterator<ADCZombieDirector> It(GetWorld()); It; ++It)
	{
		bHasDirector = true;
	}
	if (!bHasDirector)
	{
		GetWorld()->SpawnActor<ADCZombieDirector>(ADCZombieDirector::StaticClass(), FTransform::Identity);
	}

	Super::StartPlay();
}

AActor* ADCZombieGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (!StartSpot)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		StartSpot = GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), ADCTakeDirector::FindStartTransform(GetWorld()), Params);
	}
	return StartSpot;
}
