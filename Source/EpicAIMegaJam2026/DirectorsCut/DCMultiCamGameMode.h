// Director's Cut prototype 3 - game mode for fixed multi-camera takes.
// Set as the GameMode Override on a level containing DCBeat, DCPerformer, and related actors.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DCMultiCamGameMode.generated.h"

UCLASS()
class ADCMultiCamGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADCMultiCamGameMode();

	virtual void StartPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

private:
	UPROPERTY()
	TObjectPtr<AActor> StartSpot;
};
