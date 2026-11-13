// Director's Cut - a hidden story beat. Players are never told where or when it happens;
// it captures whenever the camera frames its anchors during its window, from any position or angle.
// The actor's own location only matters in the editor (put it near the action so it is easy to find).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCTypes.h"
#include "DCCinemaReplay.h"
#include "DCBeat.generated.h"

class UBillboardComponent;
class UTextRenderComponent;

UCLASS()
class ADCBeat : public AActor
{
	GENERATED_BODY()

public:
	ADCBeat();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, Category = "Beat")
	FString BeatId = TEXT("X01");

	UPROPERTY(EditAnywhere, Category = "Beat")
	FString DisplayName = TEXT("New beat");

	/** Shown on the results screen once captured. */
	UPROPERTY(EditAnywhere, Category = "Beat")
	FString Caption;

	/** Audience emotion during this beat's original scene-time window. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beat|Cinema")
	EDCCinemaMood ReplayMood = EDCCinemaMood::Neutral;

	/** Mandatory beats must be captured for the take to be approved. Their names are in the studio brief. */
	UPROPERTY(EditAnywhere, Category = "Beat")
	bool bMandatory = false;

	UPROPERTY(EditAnywhere, Category = "Beat|Timing", meta = (ClampMin = "0", Units = "s"))
	float WindowStart = 5.f;

	UPROPERTY(EditAnywhere, Category = "Beat|Timing", meta = (ClampMin = "0", Units = "s"))
	float WindowEnd = 10.f;

	/** Earliest scene time at which the scored pose is readable. Zero preserves legacy windows. */
	UPROPERTY(EditAnywhere, Category = "Beat|Timing", meta = (ClampMin = "0", Units = "s"))
	float PoseReadyTime = 0.f;

	float GetCaptureStartTime() const { return FMath::Max(WindowStart, PoseReadyTime); }

	UPROPERTY(EditAnywhere, Category = "Beat|Timing", meta = (ClampMin = "0.1", Units = "s"))
	float RequiredHold = 2.f;

	UPROPERTY(EditAnywhere, Category = "Beat|Framing", meta = (ClampMin = "0", Units = "cm"))
	float MinDistance = 50.f;

	UPROPERTY(EditAnywhere, Category = "Beat|Framing", meta = (ClampMin = "0", Units = "cm"))
	float MaxDistance = 1500.f;

	/** Everything listed here must be inside the safe frame, in range, and unobstructed. */
	UPROPERTY(EditAnywhere, Category = "Beat|Framing")
	TArray<FDCAnchorRef> Anchors;

	UPROPERTY(EditAnywhere, Category = "Beat")
	int32 Points = 250;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> Label;
};
