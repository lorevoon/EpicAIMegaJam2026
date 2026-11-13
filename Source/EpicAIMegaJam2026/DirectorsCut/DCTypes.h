// Director's Cut - shared types.

#pragma once

#include "CoreMinimal.h"
#include "DCTypes.generated.h"

class USceneComponent;
class ADCPerformer;

UENUM()
enum class EDCAnchorPart : uint8
{
	Head,
	Prop,
	Body
};

/** Level-editable reference to a part of a performer that a shot must frame. */
USTRUCT()
struct FDCAnchorRef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Shot")
	TObjectPtr<ADCPerformer> Performer;

	UPROPERTY(EditAnywhere, Category = "Shot")
	EDCAnchorPart Part = EDCAnchorPart::Head;

	/** Only counts when the camera is in front of the performer (sees their face, not the back of their head). */
	UPROPERTY(EditAnywhere, Category = "Shot")
	bool bMustSeeFace = false;
};

UENUM()
enum class EDCPhase : uint8
{
	Preparation,
	Countdown,
	Filming,
	Results,
	/** Reading the storyboard. */
	Storyboard,
	/** Watching the rehearsal on foot. */
	Rehearsal
};

UENUM()
enum class EDCShotStatus : uint8
{
	Pending,
	Captured,
	Missed
};

/** A point the camera must frame; Owner is allowed to be hit by the visibility trace. */
struct FDCAnchor
{
	USceneComponent* Component = nullptr;
	AActor* Owner = nullptr;
	bool bMustSeeFace = false;
};

/** One pose of a performer at a point in scene time. */
struct FDCKey
{
	float Time = 0.f;
	FVector2D Location = FVector2D::ZeroVector;
	float Height = 0.f;
	float Yaw = 0.f;
	/** Head offset toward facing direction in cm (positive leans in, negative recoils). */
	float Lean = 0.f;
	/** 0 = prop lowered, 1 = prop raised to face height. */
	float PropRaise = 0.f;
	/** Whole-body tilt in degrees around the feet: positive falls backward (faint), negative bows forward. */
	float Tilt = 0.f;
};
