// Director's Cut - a placeholder actor that plays back its blocking on the scene clock.
// Its placement in the level is its pose at 0 s; ADCWaypoint actors that point at it add the later poses.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCTypes.h"
#include "DCPerformer.generated.h"

class UStaticMeshComponent;
class USplineComponent;
class USkeletalMeshComponent;
class USkeletalMesh;
class UAnimSequence;

/** A clip evaluated from the director clock, including backwards scrubbing. */
USTRUCT(BlueprintType)
struct FDCAnimationCue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float StartTime = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimSequence> Animation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float PlayRate = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float StartOffset = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bLoop = true;
};

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

	/** Optional carried prop, placed at its destination when the scene clock reaches SetDownTime. */
	UPROPERTY(EditAnywhere, Category = "Performer|Prop")
	TObjectPtr<UStaticMesh> CarriedMesh;
	/** Scene-clock props (arrows) use the same deterministic route as the cast. */
	UPROPERTY(EditAnywhere, Category = "Performer|Prop")
	bool bPropOnly = false;
	UPROPERTY(EditAnywhere, Category = "Performer|Prop")
	FName CarryBone = NAME_None;
	UPROPERTY(EditAnywhere, Category = "Performer|Prop")
	float PropVisibleFrom = 0.f;
	UPROPERTY(EditAnywhere, Category = "Performer|Prop")
	float PropVisibleUntil = 100000.f;
	UPROPERTY(EditAnywhere, Category = "Performer|Prop")
	FTransform CarryTransform = FTransform::Identity;
	UPROPERTY(EditAnywhere, Category = "Performer|Prop")
	float SetDownTime = -1.f;
	/** Seconds spent lowering the prop before release. Zero preserves an instant placement. */
	UPROPERTY(EditAnywhere, Category = "Performer|Prop", meta = (ClampMin = "0.0"))
	float SetDownBlendDuration = 0.f;
	UPROPERTY(EditAnywhere, Category = "Performer|Prop")
	FTransform SetDownTransform = FTransform::Identity;

	/** Optional imported character. When unset, the original blockout remains visible. */
	UPROPERTY(EditAnywhere, Category = "Performer|Skeletal")
	TObjectPtr<USkeletalMesh> CharacterMesh;

	UPROPERTY(EditAnywhere, Category = "Performer|Skeletal")
	FTransform MeshOffset = FTransform::Identity;

	UPROPERTY(EditAnywhere, Category = "Performer|Skeletal")
	TArray<FDCAnimationCue> AnimationCues;

	UPROPERTY(EditAnywhere, Category = "Performer|Skeletal")
	FName HeadBone = TEXT("Head");

	UPROPERTY(EditAnywhere, Category = "Performer|Skeletal")
	FName BodyBone = TEXT("Spine");

	/** Enable waypoint Z values for sloped or elevated sets; legacy scenes retain their floor height. */
	UPROPERTY(EditAnywhere, Category = "Performer")
	bool bUseWaypointHeight = false;

	/** Constant-speed travel between authored keys, useful with in-place walk cycles. */
	UPROPERTY(EditAnywhere, Category = "Performer")
	bool bLinearMovement = false;

	/** Collects this performer's waypoints into keyframes. Call once before Evaluate. */
	void BuildKeys();

	/** Poses the performer deterministically for the given scene time. */
	UFUNCTION(BlueprintCallable, Category = "Performer")
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

	UPROPERTY(VisibleAnywhere, Category = "Performer|Skeletal")
	TObjectPtr<USkeletalMeshComponent> Character;

	TArray<FDCKey> Keys;
	bool bKeysBuilt = false;
};
