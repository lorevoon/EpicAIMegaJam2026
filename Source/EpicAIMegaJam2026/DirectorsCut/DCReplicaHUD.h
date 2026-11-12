// Director's Cut prototype 4 - canvas HUD: reference film overlay, storyboard, live match, evaluation.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DCReplicaHUD.generated.h"

class ADCReplicaDirector;

UCLASS()
class ADCReplicaHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	void DrawFilmOverlay(ADCReplicaDirector* D);
	void DrawHeader(ADCReplicaDirector* D);
	/** Rig tool selector in the header during preparation. */
	virtual void DrawToolStrip(ADCReplicaDirector* D);
	void DrawStoryboard(ADCReplicaDirector* D);
	void DrawFilming(ADCReplicaDirector* D);
	virtual void DrawHelp(ADCReplicaDirector* D);
	void DrawResults(ADCReplicaDirector* D);
	void DrawShotStrip(ADCReplicaDirector* D, float Y);

	void Label(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale = 1.f, bool bCenter = false);
	bool ProjectVisible(const FVector& World, FVector2D& OutScreen) const;
	static FLinearColor ScoreColor(float Score01);

	ADCReplicaDirector* FindDirector();

	TWeakObjectPtr<ADCReplicaDirector> Director;
	float S = 1.f;
};
