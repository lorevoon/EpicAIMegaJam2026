// Director's Cut - the film's acts, and what the director says between their phases.
//
// Each act is its own level with its own director. An act runs: (intro) -> reference cut -> briefing -> preparation
// -> action -> shooting -> evaluation -> outro, then on to the next act's level. The first act opens on the main
// menu and the director's recruiting speech; the last act's outro wraps the film and goes back to the menu.

#include "DCZombieDirector.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** The acts, in order. */
	const TCHAR* const ActLevels[] = {
		TEXT("/Game/DirectorsCut/Levels/L_DC_Act1_Promise"),
		TEXT("/Game/DirectorsCut/Levels/L_DC_Act2_BrokenPromise"),
	};
	constexpr int32 NumActs = UE_ARRAY_COUNT(ActLevels);

	/** A line by face name and text, before the face is looked up. */
	struct FScriptLine
	{
		const TCHAR* Face;
		const TCHAR* Line;
		bool bHighlightShootingUI = false;
		bool bShowCameraControls = false;
	};
}

UTexture2D* ADCZombieDirector::Face(const TCHAR* Name) const
{
	const FString Wanted = FString(TEXT("Director_")) + Name;
	for (UTexture2D* Texture : Faces)
	{
		if (Texture && Texture->GetName() == Wanted)
		{
			return Texture;
		}
	}
	return nullptr;
}

void ADCZombieDirector::LoadActScript()
{
	const FString Level = UGameplayStatics::GetCurrentLevelName(this, true);
	ActIndex = INDEX_NONE;
	for (int32 i = 0; i < NumActs; ++i)
	{
		if (FPackageName::GetShortName(ActLevels[i]) == Level)
		{
			ActIndex = i;
		}
	}

	// Lines set on the director in the level win; empty cutscenes get the act's own.
	auto Fill = [this](TArray<FDCCutsceneLine>& Scene, std::initializer_list<FScriptLine> Lines)
	{
		if (Scene.Num() > 0)
		{
			return;
		}
		for (const FScriptLine& Line : Lines)
		{
			Scene.Add({ Face(Line.Face), Line.Line, Line.bHighlightShootingUI, Line.bShowCameraControls });
		}
	};

	// Every act: when the take ends he calls cut, then sends everyone to the premiere.
	Fill(WrapUpLines, {
		{ TEXT("Recruit"), TEXT("Aaand that's cut! Great work, everybody!") },
		{ TEXT("Explaining2"), TEXT("Let's now watch the audience feedback.") } });

	switch (ActIndex)
	{
	case 0: // Act 1 - The Promise
		Fill(IntroCutscene, {
			{ TEXT("Recruit"), TEXT("I have a vision for a masterpiece film... And I recruit YOU as my cinematographer!!") },
			{ TEXT("Vision"), TEXT("Now, let me show you the vision in my head.") } });
		Fill(BriefingCutscene, {
			{ TEXT("Almost"), TEXT("Now your job is to make my vision a reality. Make it as similar as possible!!") },
			{ TEXT("Camera"), TEXT("Let's start placing the cameras!") } });
		Fill(ActionCutscene, {
			{ TEXT("Action"), TEXT("It's time to actually film now!!") },
			{ TEXT("Action"), TEXT("We will do this in ONE TAKE. Good luck!!") } });
		Fill(FilmingTutorial, {
			{ TEXT("Explaining"), TEXT("Now, you must [move the camera] and [zoom in & out] to match the reference cut."), false, true },
			{ TEXT("Explaining2"), TEXT("Don't forget to switch the cameras at the right moments by pressing the [number keys]!"), true },
			{ TEXT("Recruit"), TEXT("Break a leg!") } });
		Fill(OutroCutscene, {
			{ TEXT("Joy"), TEXT("Yes! We finished Act 1! I call that a success!!") },
			{ TEXT("Vision"), TEXT("Now time for Act 2. This will have DRAMA! Let me show you!") } });
		break;

	case 1: // Act 2 - The Broken Promise: straight from Act 1's outro into its reference cut.
		Fill(BriefingCutscene, {
			{ TEXT("Camera"), TEXT("You know the drill! Place the cameras now!") } });
		Fill(ActionCutscene, {
			{ TEXT("Action"), TEXT("Quiet on set! This is the scene that breaks everyone's heart.") },
			{ TEXT("Action"), TEXT("Still ONE TAKE. Make the audience CRY!!") } });
		Fill(OutroCutscene, {
			{ TEXT("Joy"), TEXT("CUT!! That's a wrap on the whole film!!") },
			{ TEXT("ThatsIt"), TEXT("You turned my vision into a real movie. You're a true cinematographer!") },
			{ TEXT("Vision"), TEXT("Now go and rest. The world premiere awaits!") } });
		// Sounds on the scene clock, unless set on the director in the level. Each one-shot starts just before its
		// action, so its hit lands on it: the sword leaves its sheath ~0.3 s in, the arrow's twang ~0.2 s in.
		if (SceneSounds.Num() == 0)
		{
			SceneSounds = {
				{ VillagePanicSound, 0.f, .45f, 1.5f, 3.f },  // villagers flee the raid (19 s)
				{ SwordDrawSound, 11.8f, .9f },               // Romeo draws his sword (11.8-13.5)
				{ ArrowShotSound, 17.6f, .8f },               // Juliet's first arrow (released 17.8)
				{ ArrowShotSound, 20.0f, .8f } };             // and her second (released 20.2)
		}
		break;

	default: // a level outside the film: just its own cutscenes, if any
		break;
	}
}

void ADCZombieDirector::OnEvaluationFinished()
{
	if (ActIndex == INDEX_NONE)
	{
		Super::OnEvaluationFinished(); // not part of the film: another take
		return;
	}
	if (bShowCutscenes && OutroCutscene.Num() > 0)
	{
		PlayCutscene(OutroCutscene, [this]() { GoToNextAct(); });
	}
	else
	{
		FadeThrough([this]() { GoToNextAct(); });
	}
}

void ADCZombieDirector::GoToNextAct()
{
	// Opened while the screen is black; the next act's director fades in from black.
	UGameplayStatics::OpenLevel(this, FName(ActLevels[(ActIndex + 1) % NumActs]));
}
