// Director's Cut prototype - game mode. Set as the GameMode Override on an empty level and press Play.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DCGameMode.generated.h"

class ADCTakeDirector;

UCLASS()
class ADCGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADCGameMode();

	virtual void StartPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	/** Set false when the level already has its own lights and sky. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	bool bSpawnLighting = true;

private:
	UPROPERTY()
	TObjectPtr<AActor> StartSpot;
};
