// Director's Cut prototype - a placeable shot mark. Its location is the capture spot; its details define the shot.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCTypes.h"
#include "DCShotMarker.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class ADCShotMarker : public AActor
{
	GENERATED_BODY()

public:
	ADCShotMarker();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, Category = "Shot")
	FString ShotId = TEXT("B01");

	UPROPERTY(EditAnywhere, Category = "Shot")
	FString DisplayName = TEXT("New shot");

	/** Shown on the results screen when the shot is captured. */
	UPROPERTY(EditAnywhere, Category = "Shot")
	FString Caption;

	/** Mandatory shots must be captured for the take to be approved. */
	UPROPERTY(EditAnywhere, Category = "Shot")
	bool bMandatory = false;

	UPROPERTY(EditAnywhere, Category = "Shot|Timing", meta = (ClampMin = "0", Units = "s"))
	float WindowStart = 6.f;

	UPROPERTY(EditAnywhere, Category = "Shot|Timing", meta = (ClampMin = "0", Units = "s"))
	float WindowEnd = 12.f;

	/** How long every condition must hold without a break. */
	UPROPERTY(EditAnywhere, Category = "Shot|Timing", meta = (ClampMin = "0.1", Units = "s"))
	float RequiredHold = 1.5f;

	/** Capture area radius around this marker. */
	UPROPERTY(EditAnywhere, Category = "Shot|Framing", meta = (ClampMin = "25", Units = "cm"))
	float Radius = 150.f;

	UPROPERTY(EditAnywhere, Category = "Shot|Framing", meta = (ClampMin = "0", Units = "cm"))
	float MinDistance = 200.f;

	UPROPERTY(EditAnywhere, Category = "Shot|Framing", meta = (ClampMin = "0", Units = "cm"))
	float MaxDistance = 600.f;

	/** Everything listed here must be inside the safe frame, in range, and unobstructed. */
	UPROPERTY(EditAnywhere, Category = "Shot|Framing")
	TArray<FDCAnchorRef> Anchors;

	UPROPERTY(EditAnywhere, Category = "Shot")
	int32 Points = 250;

	/** Recolors the floor disc for the shot's runtime status. */
	void ShowStatus(EDCShotStatus Status);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Disc;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> Label;
};
