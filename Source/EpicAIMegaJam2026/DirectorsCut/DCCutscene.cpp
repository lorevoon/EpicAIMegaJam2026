// Director's Cut - the director's cutscenes between phases, and the fades through black that join them.
//
// A cutscene is a list of lines (a portrait and what he says). Each line types itself out; Enter or a click finishes
// it, then moves to the next. After the last line the screen fades to black, the next phase is set up while it's
// black, and the screen fades back in on it.

#include "DCMultiCamDirector.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void ADCMultiCamDirector::PlayCutscene(const TArray<FDCCutsceneLine>& Lines, TFunction<void()> OnDone, bool bOverScene)
{
	if (!bShowCutscenes || Lines.Num() == 0)
	{
		OnDone();
		return;
	}
	Cutscene = &Lines;
	CutsceneLine = 0;
	CutsceneLineStart = GetWorld()->GetRealTimeSeconds();
	CutsceneDone = MoveTemp(OnDone);
	bCutsceneOverScene = bOverScene;
}

bool ADCMultiCamDirector::HandleCutsceneInput()
{
	if (!IsInputBlocked())
	{
		return false;
	}
	if (!Cutscene || FadeDirection != 0)
	{
		return true; // mid-fade: wait for it
	}
	const float Now = GetWorld()->GetRealTimeSeconds();
	const float TypedFor = (*Cutscene)[CutsceneLine].Line.Len() / CutsceneTypeRate;
	if (Now - CutsceneLineStart < TypedFor)
	{
		CutsceneLineStart = Now - TypedFor; // show the whole line first
		return true;
	}
	if (CutsceneLine + 1 < Cutscene->Num())
	{
		++CutsceneLine;
		CutsceneLineStart = Now;
		return true;
	}
	// Last line said: leave the cutscene - straight away over the scene, else once the screen has faded to black.
	auto Leave = [this]()
	{
		Cutscene = nullptr;
		bCutsceneOverScene = false;
		TFunction<void()> Done = MoveTemp(CutsceneDone);
		CutsceneDone = nullptr;
		if (Done)
		{
			Done();
		}
	};
	if (bCutsceneOverScene)
	{
		Leave();
	}
	else
	{
		FadeThrough(Leave);
	}
	return true;
}

void ADCMultiCamDirector::FadeThrough(TFunction<void()> AtBlack)
{
	FadeAction = MoveTemp(AtBlack);
	FadeDirection = 1;
}

void ADCMultiCamDirector::FadeInFromBlack()
{
	FadeAlpha = 1.f;
	FadeDirection = -1;
}

void ADCMultiCamDirector::TickFade(float DeltaSeconds)
{
	if (FadeDirection == 0)
	{
		return;
	}
	FadeAlpha = FMath::Clamp(FadeAlpha + FadeDirection * FMath::Max(DeltaSeconds, 0.f) / PhaseFadeSeconds, 0.f, 1.f);
	if (FadeDirection > 0 && FadeAlpha >= 1.f)
	{
		FadeDirection = -1;
		// The action may start another fade or cutscene, so take it out first.
		TFunction<void()> Action = MoveTemp(FadeAction);
		FadeAction = nullptr;
		if (Action)
		{
			Action();
		}
	}
	else if (FadeDirection < 0 && FadeAlpha <= 0.f)
	{
		FadeDirection = 0;
	}
}
