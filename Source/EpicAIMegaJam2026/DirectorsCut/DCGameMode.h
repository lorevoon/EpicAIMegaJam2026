// Director's Cut prototype - game mode. Set as the GameMode Override on a level that contains the scene actors.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DCGameMode.generated.h"

UCLASS()
class ADCGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADCGameMode();

	virtual void StartPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

private:
	UPROPERTY()
	TObjectPtr<AActor> StartSpot;
};
