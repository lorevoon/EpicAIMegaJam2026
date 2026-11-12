// Director's Cut prototype 2 - game mode for free rigs and hidden beats.
// Set as the GameMode Override on a level containing DCBeat, DCPerformer, and related actors.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DCFreeGameMode.generated.h"

UCLASS()
class ADCFreeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADCFreeGameMode();

	virtual void StartPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

private:
	UPROPERTY()
	TObjectPtr<AActor> StartSpot;
};
