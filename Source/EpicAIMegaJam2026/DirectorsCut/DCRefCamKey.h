// Director's Cut prototype 4 - one key of the reference ("perfect") film's camera.
// The actor's transform is the camera pose at Time. Keys of the same Shot are blended; a new Shot is a hard cut.
// Invisible in game so players have to work the positions out from the film itself.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCTypes.h"
#include "DCRefCamKey.generated.h"

class UArrowComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class ADCRefCamKey : public AActor
{
	GENERATED_BODY()

public:
	ADCRefCamKey();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Shots play in increasing order; the first key of a shot starts it (a hard cut). */
	UPROPERTY(EditAnywhere, Category = "Reference Film", meta = (ClampMin = "1"))
	int32 Shot = 1;

	UPROPERTY(EditAnywhere, Category = "Reference Film", meta = (ClampMin = "0", Units = "s"))
	float Time = 0.f;

	/** Horizontal field of view of the reference camera at this key (75 = a 24 mm lens on a full-frame sensor). */
	UPROPERTY(EditAnywhere, Category = "Reference Film", meta = (ClampMin = "5", ClampMax = "170", Units = "deg"))
	float FieldOfView = 75.f;

	/** Shown during the reference film and on the results screen. Only needed on one key per shot. */
	UPROPERTY(EditAnywhere, Category = "Reference Film")
	FString ShotTitle;

	/** What the shot is about. Framing similarity compares where these sit on screen. Only needed on one key per shot. */
	UPROPERTY(EditAnywhere, Category = "Reference Film")
	TArray<FDCAnchorRef> Subjects;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UArrowComponent> Facing;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> Label;
};
