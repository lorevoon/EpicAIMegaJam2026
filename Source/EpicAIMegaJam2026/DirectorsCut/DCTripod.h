// Director's Cut prototype 2 - a camera tripod the player places during preparation. Height is adjustable.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCTripod.generated.h"

class UStaticMeshComponent;

UCLASS()
class ADCTripod : public AActor
{
	GENERATED_BODY()

public:
	ADCTripod();

	static constexpr float MinHeight = 40.f;
	/** Tall enough to double as a camera crane for high and bird's-eye angles. */
	static constexpr float MaxHeight = 800.f;

	void SetHeadHeight(float Height);
	float GetHeadHeight() const { return HeadHeight; }

	/** Where the camera lens sits when mounted. */
	FVector GetCameraLocation() const { return GetActorLocation() + FVector(0.f, 0.f, HeadHeight); }

	/** Tints the tripod (used for the placement ghost). */
	void SetTint(const FLinearColor& Color);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UStaticMeshComponent>> Legs;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Column;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Head;

	float HeadHeight = 160.f;
};
