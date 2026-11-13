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
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class ADCBeat : public AActor
{
	GENERATED_BODY()

public:
	ADCBeat();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

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


	/** Shows a glowing circle on the ground where a camera gets this scene well, while its window is on the timeline. */
	UPROPERTY(EditAnywhere, Category = "Beat|Camera Spot")
	bool bShowCameraSpot = false;

	/** Where the hint circle goes, relative to this actor (drag the diamond in the viewport). Snapped to the ground below. */
	UPROPERTY(EditAnywhere, Category = "Beat|Camera Spot", meta = (MakeEditWidget = true, EditCondition = "bShowCameraSpot"))
	FVector CameraSpot = FVector(-400.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, Category = "Beat|Camera Spot", meta = (ClampMin = "20", Units = "cm", EditCondition = "bShowCameraSpot"))
	float CameraSpotRadius = 70.f;

	/** 0 hides the circle; the director sets this from the phase and the timeline. */
	void SetCameraSpotOpacity(float Opacity);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> SpotMarker;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> SpotMaterial;

	float SpotOpacity = -1.f;

	/** Puts the circle on the ground under CameraSpot. */
	void PlaceSpotMarker();
};
