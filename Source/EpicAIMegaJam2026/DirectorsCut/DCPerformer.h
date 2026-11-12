// Director's Cut prototype - a placeholder actor that plays back its blocking on the scene clock.
// Its placement in the level is its pose at 0 s; ADCWaypoint actors that point at it add the later poses.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCTypes.h"
#include "DCPerformer.generated.h"

class UStaticMeshComponent;
class USplineComponent;

UCLASS()
class ADCPerformer : public AActor
{
	GENERATED_BODY()

public:
	ADCPerformer();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, Category = "Performer")
	FString DisplayName = TEXT("Performer");

	UPROPERTY(EditAnywhere, Category = "Performer")
	FLinearColor BodyColor = FLinearColor(0.8f, 0.8f, 0.8f);

	UPROPERTY(EditAnywhere, Category = "Performer")
	FLinearColor HeadColor = FLinearColor(0.9f, 0.72f, 0.6f);

	UPROPERTY(EditAnywhere, Category = "Performer")
	bool bHasCape = false;

	/** A hand-held prop (the letter). Shots can frame it with the Prop anchor part. */
	UPROPERTY(EditAnywhere, Category = "Performer")
	bool bHasProp = false;

	/** Anything with this ticked (zombies) ruins the shot whenever the live camera can see it. */
	UPROPERTY(EditAnywhere, Category = "Performer")
	bool bRuinsShot = false;

	/** Collects this performer's waypoints into keyframes. Call once before Evaluate. */
	void BuildKeys();

	/** Poses the performer deterministically for the given scene time. */
	void Evaluate(float SceneTime);

	USceneComponent* GetAnchor(EDCAnchorPart Part) const;

	/** Redraws the editor path through the waypoints. */
	void RefreshPath();

private:
	UStaticMeshComponent* MakePart(FName Name, UStaticMesh* Mesh, USceneComponent* Parent);
	TArray<FDCKey> CollectKeys() const;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Cape;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Head;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Nose;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Prop;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USplineComponent> Path;

	TArray<FDCKey> Keys;
	bool bKeysBuilt = false;
};
