// Director's Cut - shared types.

#pragma once

#include "CoreMinimal.h"
#include "DCTypes.generated.h"

class USceneComponent;
class ADCPerformer;

namespace DCFrame
{
	/**
	 * Aspect ratio of the film. The screen of the field monitor drawn around the live frame is 912 x 430 px, so the
	 * recording, the review playback, and its letterbox are all 912:430 (about 2.12:1).
	 */
	constexpr float FilmAspect = 912.f / 430.f;

	/** The capture frame is the largest FilmAspect rectangle within this centred share of the view's width and height. */
	constexpr float SafeFraction = 0.70f;

	/**
	 * Half-size of the capture frame in normalized view coordinates (1 = the view's edge) for a view of the given
	 * aspect. Scenes must sit inside it, zombies only count inside it, the HUD dims everything outside it, rigs
	 * fade inside it (M_DCRig reads it from MPC_DCFrame), and the review plays back exactly this part.
	 */
	inline FVector2D CaptureExtent(float ViewAspect)
	{
		const float X = FMath::Min(SafeFraction, SafeFraction * FilmAspect / ViewAspect);
		return FVector2D(X, X * ViewAspect / FilmAspect);
	}

	/** Horizontal FOV that shows exactly the capture frame of a camera with the given horizontal FOV. */
	inline float CaptureFOV(float FullFOVDegrees, float ViewAspect)
	{
		const float HalfTan = CaptureExtent(ViewAspect).X * FMath::Tan(FMath::DegreesToRadians(FullFOVDegrees * 0.5f));
		return FMath::RadiansToDegrees(2.f * FMath::Atan(HalfTan));
	}
}

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
	/** 1. Watching the reference film. */
	Reference,
	/** 2. Placing cameras and scrubbing the scene's timeline to test them. */
	Preparation,
	/** 3. Shooting: the count-in, then the take itself. */
	Countdown,
	Filming,
	/** 4. The recorded take played back as a film, with its evaluation. */
	Review
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
