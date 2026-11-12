// Director's Cut prototype 4 - game mode for "Replica": reproduce a reference film.
// Set as the GameMode Override on a level containing DCRefCamKey, DCPerformer, and related actors.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DCReplicaGameMode.generated.h"

UCLASS()
class ADCReplicaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADCReplicaGameMode();

	virtual void StartPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

private:
	UPROPERTY()
	TObjectPtr<AActor> StartSpot;
};
