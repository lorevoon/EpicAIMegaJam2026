// Director's Cut prototype - shared types.

#pragma once

#include "CoreMinimal.h"
#include "DCTypes.generated.h"

class USceneComponent;
class ADCPerformer;
class ADCShotMarker;

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
	/** Prototype 4: watching the reference film before preparing. */
	Watching
};

/** Why a shot is not currently capturing, in GDD priority order. */
UENUM()
enum class EDCFail : uint8
{
	None,
	WrongTime,
	WrongPosition,
	NotRecording,
	Sprinting,
	OutOfFrame,
	TooClose,
	TooFar,
	Blocked
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

/** Shot definition copied from a placed ADCShotMarker, plus its runtime state (kept separate per the GDD's reset contract). */
struct FDCShot
{
	// Definition
	ADCShotMarker* Marker = nullptr;
	FString Id;
	FString Name;
	FString Caption;
	bool bMandatory = false;
	float WindowStart = 0.f;
	float WindowEnd = 0.f;
	float RequiredHold = 1.5f;
	FVector MarkLocation = FVector::ZeroVector;
	float Radius = 150.f;
	float MinDistance = 200.f;
	float MaxDistance = 600.f;
	int32 Points = 250;
	TArray<FDCAnchor> Anchors;

	// Runtime
	EDCShotStatus Status = EDCShotStatus::Pending;
	float Hold = 0.f;
	EDCFail LastFail = EDCFail::WrongTime;
	float CapturedAt = -1.f;

	void ResetRuntime()
	{
		Status = EDCShotStatus::Pending;
		Hold = 0.f;
		LastFail = EDCFail::WrongTime;
		CapturedAt = -1.f;
	}
};

/** One pose of a performer at a point in scene time. */
struct FDCKey
{
	float Time = 0.f;
	FVector2D Location = FVector2D::ZeroVector;
	float Yaw = 0.f;
	/** Head offset toward facing direction in cm (positive leans in, negative recoils). */
	float Lean = 0.f;
	/** 0 = prop lowered, 1 = prop raised to face height. */
	float PropRaise = 0.f;
	/** Whole-body tilt in degrees around the feet: positive falls backward (faint), negative bows forward. */
	float Tilt = 0.f;
};
