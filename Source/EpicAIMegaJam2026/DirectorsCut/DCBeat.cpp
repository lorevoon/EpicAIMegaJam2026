// Director's Cut - hidden story beat.

#include "DCBeat.h"
#include "DCPerformer.h"
#include "DCVisuals.h"
#include "Components/TextRenderComponent.h"

ADCBeat::ADCBeat()
{
	PrimaryActorTick.bCanEverTick = false;
	SetHidden(true);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	DCVisuals::SetupLabel(Label, FColor::White, 26.f);
}

void ADCBeat::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	TArray<FString> Subjects;
	for (const FDCAnchorRef& Ref : Anchors)
	{
		const TCHAR* Part = Ref.Part == EDCAnchorPart::Prop ? TEXT("prop") : Ref.Part == EDCAnchorPart::Body ? TEXT("body") : TEXT("head");
		Subjects.Add(FString::Printf(TEXT("%s %s%s"), Ref.Performer ? *Ref.Performer->DisplayName : TEXT("(none)"), Part, Ref.bMustSeeFace ? TEXT(" (face)") : TEXT("")));
	}
	Label->SetText(FText::FromString(FString::Printf(TEXT("BEAT %s %s%s\n%.0f-%.0fs: %s"),
		*BeatId, *DisplayName, bMandatory ? TEXT(" [REQUIRED]") : TEXT(""), WindowStart, WindowEnd, *FString::Join(Subjects, TEXT(" + ")))));
	Label->SetTextRenderColor(bMandatory ? FColor(255, 170, 30) : FColor(80, 220, 255));
}
