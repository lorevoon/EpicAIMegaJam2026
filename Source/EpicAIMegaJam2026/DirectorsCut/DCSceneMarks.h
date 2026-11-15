// Director's Cut - the player's start mark and the clapperboard that starts a take.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCSceneMarks.generated.h"

class UArrowComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Where the camera operator is returned to at the start of every take. The arrow is the starting view direction. */
UCLASS()
class ADCStartMark : public AActor
{
	GENERATED_BODY()

public:
	ADCStartMark();
	virtual void BeginPlay() override;

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Where the player stands at the start of every take: the level's start mark, or a default spot. */
	static FTransform FindStartTransform(UWorld* World);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Disc;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UArrowComponent> Facing;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> Label;
};

/** Press E near this during preparation to call "Action!". */
UCLASS()
class ADCClapperboard : public AActor
{
	GENERATED_BODY()

public:
	ADCClapperboard();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, Category = "Clapperboard", meta = (ClampMin = "50", Units = "cm"))
	float InteractRange = 300.f;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Stand;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Board;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Clapper;
};
