// Director's Cut prototype - one pose in a performer's blocking. Position and facing come from the actor's transform.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCWaypoint.generated.h"

class ADCPerformer;
class UArrowComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class ADCWaypoint : public AActor
{
	GENERATED_BODY()

public:
	ADCWaypoint();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Destroyed() override;

	/** The performer that hits this mark. The performer's own placement is its pose at 0 s. */
	UPROPERTY(EditAnywhere, Category = "Blocking")
	TObjectPtr<ADCPerformer> Performer;

	/** Scene time at which the performer reaches this pose. */
	UPROPERTY(EditAnywhere, Category = "Blocking", meta = (ClampMin = "0", Units = "s"))
	float Time = 1.f;

	/** Head offset toward the facing direction: positive leans in (the bite), negative recoils. */
	UPROPERTY(EditAnywhere, Category = "Blocking", meta = (Units = "cm"))
	float Lean = 0.f;

	/** 0 = prop lowered, 1 = prop raised to face height. */
	UPROPERTY(EditAnywhere, Category = "Blocking", meta = (ClampMin = "0", ClampMax = "1"))
	float PropRaise = 0.f;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Pin;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UArrowComponent> Facing;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> Label;
};
