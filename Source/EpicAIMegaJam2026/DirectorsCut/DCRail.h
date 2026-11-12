// Director's Cut - the camera dolly rail the player lays down during preparation.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCRail.generated.h"

class USplineComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS()
class ADCRail : public AActor
{
	GENERATED_BODY()

public:
	ADCRail();

	static constexpr int32 MaxPoints = 24;

	/**
	 * Appends a point. Returns false (with a reason) if the new segment would pass through scenery.
	 * Surface is where the rail is bolted on (defaults to the point itself for floor rails).
	 */
	bool TryAddPoint(const FVector& Point, FString& OutReason, TOptional<FVector> Surface = {});

	/** Removes the point nearest Near (within Radius), or the last point when none is close. */
	bool RemovePointNear(const FVector& Near, float Radius);

	void Clear();

	/** Fades the rail inside the capture frame while shooting (see DCVisuals::RigMaterialPath). */
	void SetFrameMasked(bool bMasked);

	/** Ghost post that previews where the next point would go. */
	void SetPreview(bool bVisible, const FVector& Location, bool bValid, float ZOffset = 30.f);

	int32 NumPoints() const { return Points.Num(); }
	const TArray<FVector>& GetPoints() const { return Points; }
	bool IsUsable() const { return Points.Num() >= 2; }
	float GetLength() const;
	FVector GetFloorLocationAtDistance(float Distance) const;

	/** Distance along the rail closest to Location, and how far Location is from the rail. */
	float FindClosestDistance(const FVector& Location, float& OutGap) const;

	/** Actors that rail placement traces should ignore (performers move, so their start pose proves nothing). */
	TArray<TWeakObjectPtr<AActor>> IgnoredActors;

	/**
	 * Floor rails sit on the ground and carry a walking-height camera.
	 * Free rails (false) run through the camera positions themselves and can be bolted to walls or ceilings.
	 */
	bool bFloorRail = true;

	/** Color of the rail's point posts, so several rails can be told apart. */
	FLinearColor PostColor = FLinearColor(1.f, 0.45f, 0.05f);

private:
	void RebuildSpline(const TArray<FVector>& InPoints) const;
	bool IsPathClear(FString& OutReason) const;
	void RebuildVisuals();
	UStaticMeshComponent* AddPiece(UStaticMesh* Mesh, const FTransform& Transform, UMaterialInterface* Material);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USplineComponent> Spline;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Ghost;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Pieces;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> RailMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> TieMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> PostMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> GhostMaterial;

	TArray<FVector> Points;
	TArray<FVector> SurfacePoints;
};
