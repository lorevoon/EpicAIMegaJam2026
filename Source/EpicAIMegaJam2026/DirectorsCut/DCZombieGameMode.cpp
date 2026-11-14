// Director's Cut - game mode.

#include "DCZombieGameMode.h"
#include "DCMultiCamCharacter.h"
#include "DCSceneMarks.h"
#include "DCZombieDirector.h"
#include "DCZombieHUD.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

ADCZombieGameMode::ADCZombieGameMode()
{
	DefaultPawnClass = ADCMultiCamCharacter::StaticClass();
	PlayerControllerClass = APlayerController::StaticClass();
	HUDClass = ADCZombieHUD::StaticClass();
}

void ADCZombieGameMode::StartPlay()
{
	// Set/Real maps are environment layers. When launched on their own, enter
	// their complete act; when streamed, their WorldSettings do not run a game mode.
	const FString Map = UGameplayStatics::GetCurrentLevelName(this, true);
	FName Scene;
	if (Map == TEXT("L_DC_Act2_Set") || Map == TEXT("L_DC_Act2_Real"))
	{
		Scene = TEXT("/Game/DirectorsCut/Levels/L_DC_Act2_BrokenPromise");
	}
	else if (Map == TEXT("L_DC_Act1_Set") || Map == TEXT("L_DC_Act1_Real"))
	{
		Scene = TEXT("/Game/DirectorsCut/Levels/L_DC_Act1_Promise");
	}
	if (!Scene.IsNone())
	{
		UE_LOG(LogTemp, Log, TEXT("Scene test entry: %s -> %s"), *Map, *Scene.ToString());
		UGameplayStatics::OpenLevel(this, Scene);
		return;
	}

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
		StartSpot = GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), ADCStartMark::FindStartTransform(GetWorld()), Params);
	}
	return StartSpot;
}
