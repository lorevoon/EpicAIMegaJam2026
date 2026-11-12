// Director's Cut prototype 4 - game mode.

#include "DCReplicaGameMode.h"
#include "DCFreeCharacter.h"
#include "DCReplicaDirector.h"
#include "DCReplicaHUD.h"
#include "DCTakeDirector.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"

ADCReplicaGameMode::ADCReplicaGameMode()
{
	DefaultPawnClass = ADCFreeCharacter::StaticClass();
	PlayerControllerClass = APlayerController::StaticClass();
	HUDClass = ADCReplicaHUD::StaticClass();
}

void ADCReplicaGameMode::StartPlay()
{
	// Use the director placed in the level, or add one; either way it reads the scene from the level.
	bool bHasDirector = false;
	for (TActorIterator<ADCReplicaDirector> It(GetWorld()); It; ++It)
	{
		bHasDirector = true;
	}
	if (!bHasDirector)
	{
		GetWorld()->SpawnActor<ADCReplicaDirector>(ADCReplicaDirector::StaticClass(), FTransform::Identity);
	}

	Super::StartPlay();
}

AActor* ADCReplicaGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (!StartSpot)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		StartSpot = GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), ADCTakeDirector::FindStartTransform(GetWorld()), Params);
	}
	return StartSpot;
}
