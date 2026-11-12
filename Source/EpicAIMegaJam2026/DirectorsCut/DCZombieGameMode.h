// Director's Cut - game mode for "Love in the Ruins".
// Set as the GameMode Override on a level containing DCBeat, DCPerformer (some with Ruins Shot), and related actors.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DCZombieGameMode.generated.h"

UCLASS()
class ADCZombieGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADCZombieGameMode();

	virtual void StartPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

private:
	UPROPERTY()
	TObjectPtr<AActor> StartSpot;
};
