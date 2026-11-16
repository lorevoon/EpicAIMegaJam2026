// Director's Cut - the film's director, coaching the player through the take.
//
// Each frame of shooting compares the live picture with the reference cut's at the same moment (the same comparison as
// the grade: where the characters sit in the frame, how big they are, which side they're seen from) and voices the
// single most useful fix, in order: cut to the camera with the right viewpoint, pan/tilt, size - or praise. Lines stay
// up long enough to read. Size is asked for as "bigger"/"smaller": getting closer and zooming in both do it.

#include "DCMultiCamDirector.h"
#include "DCBeat.h"
#include "DCMultiCam.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"
#include "DCCrewMember.h"
#include "EngineUtils.h"

namespace
{
	/** How far the characters may sit from where the reference frames them (half frame widths) before he speaks up. */
	constexpr float PanSlack = 0.15f;
	constexpr float TiltSlack = 0.1f;
	/** Past this, he stops being polite. */
	constexpr float FrameFar = 0.5f;
	/** Size ratios (live / reference) he lets go, and past which he shouts. */
	constexpr float SizeSlack = 1.3f;
	constexpr float SizeFar = 2.f;
	/** Viewpoint difference (degrees) worth a cut to another camera, and how much better that camera must be. */
	constexpr float CutViewpoint = 35.f;
	constexpr float CutGain = 20.f;

	float AngleBetween(const FVector& A, const FVector& B)
	{
		return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(A, B), -1.f, 1.f)));
	}
}

void ADCMultiCamDirector::PlayDirectorContent()
{
	// He hums as each cutscene line appears; let one finish before the next.
	if (DirectorContentSound && (!IsValid(DirectorContentAudio) || !DirectorContentAudio->IsPlaying()))
	{
		DirectorContentAudio = UGameplayStatics::SpawnSound2D(this, DirectorContentSound);
	}
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

void ADCMultiCamDirector::Cheer(int32 BeatIndex)
{
	for (TActorIterator<ADCCrewMember> It(GetWorld()); It; ++It)
	{
		if (It->bDirector && !It->IsHidden()) It->Celebrate();
	}
	static const TCHAR* const Lines[] = {
		TEXT("THAT'S IT! That's my spot for scene {n}!"),
		TEXT("Yes, YES! \"{scene}\" is going to look gorgeous!"),
		TEXT("Now THAT is a camera position! Scene {n}, sorted."),
		TEXT("Bravo! I can already see scene {n} on the big screen!"),
		TEXT("Magnificent! Scene {n} has its camera!"),
	};
	const float Now = GetWorld()->GetRealTimeSeconds();
	// Already up from the last one? Then he just keeps talking instead of popping up again.
	if (Now - CheerLineStart >= CheerSeconds)
	{
		CheerUpSince = Now;
	}
	FString Line = Lines[CheerCount % UE_ARRAY_COUNT(Lines)];
	Line.ReplaceInline(TEXT("{n}"), *FString::FromInt(BeatIndex + 1));
	Line.ReplaceInline(TEXT("{scene}"), Beats.IsValidIndex(BeatIndex) && Beats[BeatIndex].Beat ? *Beats[BeatIndex].Beat->DisplayName : TEXT(""));
	CheerLine = MoveTemp(Line);
	CheerLineStart = Now;
	++CheerCount;
}

void ADCMultiCamDirector::UpdateCoach()
{
	LiveLikeness = -1.f;
	const ADCMultiCam* Live = GetCameraSlot(ViewingSlot);
	FVector RefLocation;
	FQuat RefQuat;
	float RefFOV = 0.f, ScenePoint = 0.f;
	TArray<FVector> Points;
	GetSubjectPoints(SceneTime, Points);
	if (!Live || Points.Num() == 0 || !GetFilmView(ReferenceCut, ReferenceFilmTimeAtSceneTime(SceneTime), RefLocation, RefQuat, RefFOV, ScenePoint))
	{
		return;
	}
	const UCameraComponent* Lens = Live->GetCamera();
	const FDCFrameReading Ref = ReadFrame(Points, RefLocation, RefQuat, RefFOV);
	const FDCFrameReading Shot = ReadFrame(Points, Lens->GetComponentLocation(), Lens->GetComponentQuat(),
		DCFrame::CaptureFOV(Lens->FieldOfView, GetViewAspect()));

	// How alike the pictures are, the same way the grade measures it - that's his mood (and the frame's vignette).
	LiveLikeness = CompareFrames(Shot, Ref).Overall;
	if (!bShowDirectorCoach)
	{
		return;
	}
	const EDCDirectorMood Mood = LiveLikeness >= PerfectLikeness ? EDCDirectorMood::Joy
		: LiveLikeness >= 0.6f ? EDCDirectorMood::Almost : EDCDirectorMood::Disgust;

	// 1. Wrong camera: another of the player's cameras sees the characters from much nearer the reference's side.
	const float LiveView = AngleBetween(Shot.ViewDir, Ref.ViewDir);
	const ADCMultiCam* Best = Live;
	float BestView = LiveView;
	for (const ADCMultiCam* Cam : Cameras)
	{
		if (Cam && Cam != Live)
		{
			const float View = AngleBetween(ReadFrame(Points, Cam->GetCamera()->GetComponentLocation(), FQuat::Identity, 90.f).ViewDir, Ref.ViewDir);
			if (View < BestView)
			{
				Best = Cam;
				BestView = View;
			}
		}
	}
	if (Best != Live && LiveView > CutViewpoint && LiveView - BestView > CutGain)
	{
		Coach(*FString::Printf(TEXT("Cut%d"), Best->GetNumber()), EDCDirectorMood::Disgust,
			{ TEXT("Wrong camera! Cut to CAM {cam}!"), TEXT("Why are we on this camera?! CAM {cam}, now!"), TEXT("CAM {cam} has the shot. CUT!") },
			Best->GetNumber());
		return;
	}

	// 2. The characters sit in the wrong part of the frame: pan or tilt them to where the reference has them.
	// Off-screen (or behind the camera), go by the direction they're in instead.
	FVector2D Offset = Shot.Center - Ref.Center;
	if (!Shot.bVisible)
	{
		FVector Centroid = FVector::ZeroVector;
		for (const FVector& P : Points)
		{
			Centroid += P / Points.Num();
		}
		const FVector Local = Lens->GetComponentQuat().UnrotateVector(Centroid - Lens->GetComponentLocation());
		Offset = FVector2D(Local.Y, Local.Z).GetSafeNormal() * (FrameFar + 1.f);
	}
	if (FMath::Abs(Offset.X) > PanSlack || FMath::Abs(Offset.Y) > TiltSlack)
	{
		const bool bPan = FMath::Abs(Offset.X) / PanSlack >= FMath::Abs(Offset.Y) / TiltSlack;
		const bool bFar = (bPan ? FMath::Abs(Offset.X) : FMath::Abs(Offset.Y)) > FrameFar;
		if (bPan && Offset.X < 0.f)
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
		else if (Offset.Y > 0.f)
		{
			Coach(TEXT("TiltUp"), Mood, { TEXT("Tilt up! I'm paying for those faces!"), TEXT("Up, up! We're filming feet!") });
		}
		else
		{
			Coach(TEXT("TiltDown"), Mood, { TEXT("Tilt down! We're filming the sky!"), TEXT("Lower! Bring it down to the actors!") });
		}
		return;
	}

	// 3. Framed right but at the wrong size - closer or zoom, whichever the player likes.
	const float Ratio = Shot.Size / FMath::Max(Ref.Size, KINDA_SMALL_NUMBER);
	if (Ratio < 1.f / SizeSlack)
	{
		const bool bFar = Ratio < 1.f / SizeFar;
		Coach(bFar ? TEXT("BiggerFar") : TEXT("Bigger"), Mood, bFar
			? std::initializer_list<const TCHAR*>{ TEXT("They're specks! Get closer or zoom in!"), TEXT("Bigger! I want to see their eyes!") }
			: std::initializer_list<const TCHAR*>{ TEXT("A little tighter..."), TEXT("Bring them in a touch. Closer, or zoom.") });
		return;
	}
	if (Ratio > SizeSlack)
	{
		const bool bFar = Ratio > SizeFar;
		Coach(bFar ? TEXT("SmallerFar") : TEXT("Smaller"), Mood, bFar
			? std::initializer_list<const TCHAR*>{ TEXT("Too tight! Pull back or zoom out!"), TEXT("Give me the whole scene! Wider!") }
			: std::initializer_list<const TCHAR*>{ TEXT("Ease out a bit..."), TEXT("Wider. Let it breathe.") });
		return;
	}

	// 4. Nailed it - or the picture is right but seen from the wrong side, and no camera has a better view.
	if (Mood == EDCDirectorMood::Joy)
	{
		Coach(TEXT("Perfect"), EDCDirectorMood::Joy, { TEXT("YES! Hold it right there!"), TEXT("Beautiful! That's the shot!"), TEXT("Magnificent. Don't touch anything.") });
	}
	else if (Shot.ViewDir.Z - Ref.ViewDir.Z > 0.25f)
	{
		Coach(TEXT("High"), Mood, { TEXT("Nice framing... from way up there. Next take, lower a camera."), TEXT("Too high! This isn't a bird documentary.") });
	}
	else if (Ref.ViewDir.Z - Shot.ViewDir.Z > 0.25f)
	{
		Coach(TEXT("Low"), Mood, { TEXT("Nice framing... from down in the dirt. Next take, raise a camera."), TEXT("Too low! Get me some height next take.") });
	}
	else
	{
		Coach(TEXT("Side"), Mood, { TEXT("Good framing... from the wrong side. Next take, move a camera."), TEXT("Nice shot. Wrong angle. We'll fix it next take.") });
	}
}
