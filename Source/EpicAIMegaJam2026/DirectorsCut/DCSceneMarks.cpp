// Director's Cut prototype - start mark and clapperboard.

#include "DCSceneMarks.h"
#include "DCVisuals.h"
#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ADCStartMark::ADCStartMark()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Disc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Disc"));
	Disc->SetupAttachment(Root);
	Disc->SetStaticMesh(CylinderMesh.Object);
	Disc->SetRelativeLocation(FVector(0.f, 0.f, 1.f));
	Disc->SetRelativeScale3D(FVector(1.2f, 1.2f, 0.02f));
	Disc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Disc->SetCastShadow(false);

	Facing = CreateDefaultSubobject<UArrowComponent>(TEXT("Facing"));
	Facing->SetupAttachment(Root);
	Facing->SetRelativeLocation(FVector(0.f, 0.f, 20.f));
	Facing->SetArrowColor(FLinearColor::White);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	Label->SetText(FText::FromString(TEXT("START MARK")));
	DCVisuals::SetupLabel(Label, FColor::White, 28.f);
}

void ADCStartMark::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	DCVisuals::Paint(Disc, FLinearColor(0.9f, 0.9f, 0.9f));
}

ADCClapperboard::ADCClapperboard()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	auto MakePart = [this](FName Name, const FVector& Location, const FVector& Size)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Root);
		Part->SetStaticMesh(CubeMesh.Object);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeScale3D(Size / 100.f);
		Part->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		return Part;
	};
	Stand = MakePart(TEXT("Stand"), FVector(0, 0, 50), FVector(30, 30, 100));
	Board = MakePart(TEXT("Board"), FVector(0, 0, 118), FVector(8, 60, 36));
	Clapper = MakePart(TEXT("Clapper"), FVector(0, 0, 140), FVector(8, 60, 8));
}

void ADCClapperboard::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	DCVisuals::Paint(Stand, FLinearColor(0.15f, 0.14f, 0.17f));
	DCVisuals::Paint(Board, FLinearColor(0.02f, 0.02f, 0.02f));
	DCVisuals::Paint(Clapper, FLinearColor(0.9f, 0.9f, 0.9f));
}
