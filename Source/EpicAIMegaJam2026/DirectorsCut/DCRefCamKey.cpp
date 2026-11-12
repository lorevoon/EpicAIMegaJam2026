// Director's Cut prototype 4 - reference film camera key.

#include "DCRefCamKey.h"
#include "DCVisuals.h"
#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ADCRefCamKey::ADCRefCamKey()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetStaticMesh(CubeMesh.Object);
	Body->SetRelativeLocation(FVector(-15.f, 0.f, 0.f));
	Body->SetRelativeScale3D(FVector(0.3f, 0.15f, 0.2f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetCastShadow(false);
	Body->SetHiddenInGame(true);

	Facing = CreateDefaultSubobject<UArrowComponent>(TEXT("Facing"));
	Facing->SetupAttachment(Root);
	Facing->ArrowSize = 1.2f;

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 25.f));
	Label->SetUsingAbsoluteRotation(true);
	DCVisuals::SetupLabel(Label, FColor(255, 200, 60), 20.f);
}

void ADCRefCamKey::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const FLinearColor Color = FLinearColor::MakeFromHSV8(uint8((Shot * 53) % 255), 200, 255);
	DCVisuals::Paint(Body, Color);
	Facing->SetArrowColor(Color);
	Label->SetText(FText::FromString(FString::Printf(TEXT("SHOT %d @ %.1fs%s%s"), Shot, Time,
		ShotTitle.IsEmpty() ? TEXT("") : TEXT("\n"), *ShotTitle)));
	Label->SetTextRenderColor(Color.ToFColor(true));
}
