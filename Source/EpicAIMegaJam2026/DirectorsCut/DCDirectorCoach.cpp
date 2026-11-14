// Director's Cut - the film's director, coaching the player through the take.
//
// Each frame of shooting compares the live camera with the reference cut at the same moment (the same comparison as
// the grade: where the camera stands, where it points, how it's zoomed) and voices the single most useful fix, in
// order: cut to the right camera, pan/tilt, zoom - or praise. Lines stay up long enough to read.

#include "DCMultiCamDirector.h"
#include "DCMultiCam.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"

namespace
{
	/** Pan/tilt/zoom differences (degrees) below which the director doesn't bother. */
	constexpr float PanSlack = 9.f;
	constexpr float TiltSlack = 7.f;
	constexpr float ZoomSlack = 5.f;
	/** Past these, he stops being polite. */
	constexpr float PanFar = 30.f;
	constexpr float ZoomFar = 15.f;
}

void ADCMultiCamDirector::Coach(FName Key, EDCDirectorMood Mood, std::initializer_list<const TCHAR*> Variants, int32 Camera, bool bForce)
{
	const float Now = GetWorld()->GetRealTimeSeconds();
	if (!bForce)
	{
		if (Key == CoachKey && Mood == CoachMood)
		{
			return; // already saying it
		}
		if (!CoachLine.IsEmpty() && Now - CoachLineStart < CoachLineSeconds)
		{
			return; // let the current line be read
		}
	}
	const int32 Count = static_cast<int32>(Variants.size());
	FString Line = Count > 0 ? FString(Variants.begin()[CoachLineCount % Count]) : FString();
	Line.ReplaceInline(TEXT("{cam}"), *FString::FromInt(Camera));
	if (Key != CoachKey)
	{
		++CoachLineCount;
	}
	CoachKey = Key;
	CoachMood = Mood;
	CoachLine = MoveTemp(Line);
	CoachLineStart = Now;
}

void ADCMultiCamDirector::UpdateCoach()
{
	if (!bShowDirectorCoach)
	{
		return;
	}
	if (IsRecordingPaused())
	{
		Coach(TEXT("Paused"), EDCDirectorMood::Almost, { TEXT("We're not rolling! Let go and keep filming."), TEXT("Paused?! The actors are still acting!") });
		return;
	}
	const ADCMultiCam* Live = GetCameraSlot(ViewingSlot);
	FVector RefLocation;
	FQuat RefQuat;
	float RefFOV = 0.f, ScenePoint = 0.f;
	if (!Live || !GetFilmView(ReferenceCut, SceneTime, RefLocation, RefQuat, RefFOV, ScenePoint))
	{
		return;
	}

	// Where the reference films from: is the live camera there, or is another of the player's cameras?
	auto SpotError = [&RefLocation](const ADCMultiCam* Cam) { return FVector::Dist2D(Cam->GetActorLocation(), RefLocation); };
	const float LiveError = SpotError(Live);
	const ADCMultiCam* Nearest = Live;
	for (const ADCMultiCam* Cam : Cameras)
	{
		if (Cam && SpotError(Cam) < SpotError(Nearest))
		{
			Nearest = Cam;
		}
	}

	// How the live lens differs from the reference: pan, tilt and zoom (on the film frame).
	const UCameraComponent* Lens = Live->GetCamera();
	const FRotator LiveRot = Lens->GetComponentRotation();
	const FRotator RefRot = RefQuat.Rotator();
	const float Pan = FRotator::NormalizeAxis(RefRot.Yaw - LiveRot.Yaw);
	const float Tilt = FRotator::NormalizeAxis(RefRot.Pitch - LiveRot.Pitch);
	const float Zoom = DCFrame::CaptureFOV(Lens->FieldOfView, GetViewAspect()) - RefFOV;

	// How close the shot is overall, the same way the grade measures it - that's his mood.
	auto Closeness = [](float Error, float Tolerance) { return 1.f - FMath::Square(FMath::Clamp(Error / Tolerance, 0.f, 1.f)); };
	const float Angle = FMath::RadiansToDegrees(Lens->GetComponentQuat().AngularDistance(RefQuat));
	const float Likeness = (Closeness(LiveError, MatchPositionTolerance) + Closeness(Angle, MatchAngleTolerance)
		+ Closeness(FMath::Abs(Zoom), MatchZoomTolerance)) / 3.f;
	const EDCDirectorMood Mood = Likeness >= 0.85f ? EDCDirectorMood::Joy : Likeness >= 0.6f ? EDCDirectorMood::Almost : EDCDirectorMood::Disgust;

	// 1. Wrong camera: another of the player's cameras stands much closer to where the reference films from.
	if (Nearest != Live && LiveError > ReferenceSpotRadius && LiveError - SpotError(Nearest) > ReferenceSpotRadius * 0.5f)
	{
		Coach(*FString::Printf(TEXT("Cut%d"), Nearest->GetNumber()), EDCDirectorMood::Disgust,
			{ TEXT("Wrong camera! Cut to CAM {cam}!"), TEXT("Why are we on this camera?! CAM {cam}, now!"), TEXT("CAM {cam} has the shot. CUT!") },
			Nearest->GetNumber());
		return;
	}
	// 2. Pointing the wrong way: pan first, then tilt.
	if (FMath::Abs(Pan) > PanSlack || FMath::Abs(Tilt) > TiltSlack)
	{
		const bool bPan = FMath::Abs(Pan) / PanSlack >= FMath::Abs(Tilt) / TiltSlack;
		const bool bFar = bPan ? FMath::Abs(Pan) > PanFar : FMath::Abs(Tilt) > PanFar;
		if (bPan && Pan < 0.f)
		{
			Coach(bFar ? TEXT("PanLeftFar") : TEXT("PanLeft"), Mood, bFar
				? std::initializer_list<const TCHAR*>{ TEXT("Pan LEFT! The story's over there!"), TEXT("Left! LEFT! You're filming nothing!") }
				: std::initializer_list<const TCHAR*>{ TEXT("Ease left a touch..."), TEXT("A little to the left. My left.") });
		}
		else if (bPan)
		{
			Coach(bFar ? TEXT("PanRightFar") : TEXT("PanRight"), Mood, bFar
				? std::initializer_list<const TCHAR*>{ TEXT("Pan RIGHT! They're over there!"), TEXT("Right! RIGHT! Where are you looking?!") }
				: std::initializer_list<const TCHAR*>{ TEXT("Nudge it right..."), TEXT("A hair to the right, darling.") });
		}
		else if (Tilt > 0.f)
		{
			Coach(TEXT("TiltUp"), Mood, { TEXT("Tilt up! I'm paying for those faces!"), TEXT("Up, up! We're filming feet!") });
		}
		else
		{
			Coach(TEXT("TiltDown"), Mood, { TEXT("Tilt down! We're filming the sky!"), TEXT("Lower! Bring it down to the actors!") });
		}
		return;
	}
	// 3. Framed the right way but at the wrong size.
	if (Zoom > ZoomSlack)
	{
		Coach(Zoom > ZoomFar ? TEXT("ZoomInFar") : TEXT("ZoomIn"), Mood, Zoom > ZoomFar
			? std::initializer_list<const TCHAR*>{ TEXT("Zoom IN! I want to see their eyes!"), TEXT("Push in! They're specks!") }
			: std::initializer_list<const TCHAR*>{ TEXT("Push in a little..."), TEXT("Tighter. Just a touch.") });
		return;
	}
	if (Zoom < -ZoomSlack)
	{
		Coach(Zoom < -ZoomFar ? TEXT("ZoomOutFar") : TEXT("ZoomOut"), Mood, Zoom < -ZoomFar
			? std::initializer_list<const TCHAR*>{ TEXT("Zoom OUT! Give me the whole scene!"), TEXT("Too tight! Pull back, pull back!") }
			: std::initializer_list<const TCHAR*>{ TEXT("Ease out a bit..."), TEXT("Wider. Let it breathe.") });
		return;
	}
	// 4. Nailed it.
	if (Mood == EDCDirectorMood::Joy)
	{
		Coach(TEXT("Perfect"), EDCDirectorMood::Joy, { TEXT("YES! Hold it right there!"), TEXT("Beautiful! That's the shot!"), TEXT("Magnificent. Don't touch anything.") });
	}
	else
	{
		// Pointing and zoomed right, but from the wrong place - and none of the cameras is closer.
		Coach(TEXT("Spot"), Mood, { TEXT("Good framing... from the wrong spot. Next take, move a camera."), TEXT("Nice angle. Wrong place. We'll fix it next take.") });
	}
}
