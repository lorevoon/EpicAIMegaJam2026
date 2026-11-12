// Director's Cut prototype - performer waypoint.

#include "DCWaypoint.h"
#include "DCPerformer.h"
#include "DCVisuals.h"
#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ADCWaypoint::ADCWaypoint()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Pin = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pin"));
	Pin->SetupAttachment(Root);
	Pin->SetStaticMesh(SphereMesh.Object);
	Pin->SetRelativeLocation(FVector(0.f, 0.f, 10.f));
	Pin->SetRelativeScale3D(FVector(0.2f));
	Pin->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Pin->SetHiddenInGame(true);
	Pin->SetCastShadow(false);

	Facing = CreateDefaultSubobject<UArrowComponent>(TEXT("Facing"));
	Facing->SetupAttachment(Root);
	Facing->SetRelativeLocation(FVector(0.f, 0.f, 10.f));
	Facing->ArrowSize = 0.8f;

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 30.f));
	DCVisuals::SetupLabel(Label, FColor::White, 20.f);
}

void ADCWaypoint::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const FLinearColor Color = Performer ? Performer->BodyColor * 0.5f + FLinearColor(0.5f, 0.5f, 0.5f) : FLinearColor::Red;
	DCVisuals::Paint(Pin, Color);
	Facing->SetArrowColor(Color);

	FString Text = FString::Printf(TEXT("%s @ %.1fs"), Performer ? *Performer->DisplayName : TEXT("(no performer!)"), Time);
	if (!FMath::IsNearlyZero(Lean))
	{
		Text += FString::Printf(TEXT(" lean %.0f"), Lean);
	}
	if (PropRaise > 0.f)
	{
		Text += FString::Printf(TEXT(" prop %.0f%%"), PropRaise * 100.f);
	}
	Label->SetText(FText::FromString(Text));

	if (Performer)
	{
		Performer->RefreshPath();
	}
}

void ADCWaypoint::Destroyed()
{
	ADCPerformer* Owned = Performer;
	Performer = nullptr;
	if (Owned)
	{
		Owned->RefreshPath();
	}
	Super::Destroyed();
}
