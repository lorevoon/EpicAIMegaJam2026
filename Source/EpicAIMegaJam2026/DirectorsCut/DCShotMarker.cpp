// Director's Cut prototype - shot marker.

#include "DCShotMarker.h"
#include "DCVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FLinearColor MandatoryColor(1.f, 0.65f, 0.1f);
	const FLinearColor BonusColor(0.1f, 0.75f, 1.f);
	const FLinearColor CapturedColor(0.2f, 1.f, 0.3f);
	const FLinearColor MissedColor(0.25f, 0.25f, 0.25f);
}

ADCShotMarker::ADCShotMarker()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Disc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Disc"));
	Disc->SetupAttachment(Root);
	Disc->SetStaticMesh(CylinderMesh.Object);
	Disc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Disc->SetCastShadow(false);
	Disc->SetRelativeLocation(FVector(0.f, 0.f, 1.f));

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	DCVisuals::SetupLabel(Label, FColor::White, 28.f);
}

void ADCShotMarker::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Keep the disc at the true radius no matter how the actor is scaled.
	const FVector Scale = GetActorScale3D().GetAbs().ComponentMax(FVector(KINDA_SMALL_NUMBER));
	Disc->SetRelativeScale3D(FVector(Radius / 50.f / Scale.X, Radius / 50.f / Scale.Y, 0.02f / Scale.Z));
	ShowStatus(EDCShotStatus::Pending);

	Label->SetText(FText::FromString(FString::Printf(TEXT("%s %s\n%.0f-%.0fs %s"),
		*ShotId, *DisplayName, WindowStart, WindowEnd, bMandatory ? TEXT("REQUIRED") : TEXT("bonus"))));
	Label->SetTextRenderColor((bMandatory ? MandatoryColor : BonusColor).ToFColor(true));
}

void ADCShotMarker::ShowStatus(EDCShotStatus Status)
{
	FLinearColor Color = bMandatory ? MandatoryColor : BonusColor;
	if (Status == EDCShotStatus::Captured)
	{
		Color = CapturedColor;
	}
	else if (Status == EDCShotStatus::Missed)
	{
		Color = MissedColor;
	}
	DCVisuals::Paint(Disc, Color);
}
