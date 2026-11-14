#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DCTitleGameMode.generated.h"

class SWidget;
class SVerticalBox;

/** Standalone front end; no performers or gameplay are started here. */
UCLASS()
class EPICAIMEGAJAM2026_API ADCTitleGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ADCTitleGameMode();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	void ShowPage(bool bCredits);
	TSharedPtr<SWidget> Menu;
	TSharedPtr<SVerticalBox> Page;
};
