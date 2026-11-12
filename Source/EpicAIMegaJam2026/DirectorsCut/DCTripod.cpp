// Director's Cut prototype 2 - tripod.

#include "DCTripod.h"
#include "DCVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float LegSpread = 35.f;
	constexpr float LegHeight = 70.f;
}

ADCTripod::ADCTripod()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	auto MakePart = [this](FName Name, UStaticMesh* Mesh)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Root);
		Part->SetStaticMesh(Mesh);
		// Rigs never block movement or the camera's line of sight.
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(false);
		return Part;
	};

	for (int32 i = 0; i < 3; ++i)
	{
		const float Angle = FMath::DegreesToRadians(120.f * i);
		const FVector Foot(FMath::Cos(Angle) * LegSpread, FMath::Sin(Angle) * LegSpread, 0.f);
		const FVector Top(0.f, 0.f, LegHeight);
		const FVector Dir = Top - Foot;
		UStaticMeshComponent* Leg = MakePart(*FString::Printf(TEXT("Leg%d"), i), CylinderMesh.Object);
		// Cylinder is 100 tall along Z; rotate Z onto the leg direction.
		Leg->SetRelativeLocation((Foot + Top) * 0.5f);
		Leg->SetRelativeRotation(FRotationMatrix::MakeFromZ(Dir.GetSafeNormal()).Rotator());
		Leg->SetRelativeScale3D(FVector(0.04f, 0.04f, Dir.Size() / 100.f));
		Legs.Add(Leg);
	}
	Column = MakePart(TEXT("Column"), CylinderMesh.Object);
	Head = MakePart(TEXT("Head"), CubeMesh.Object);
	Head->SetRelativeScale3D(FVector(0.3f, 0.16f, 0.16f));

	SetHeadHeight(HeadHeight);
}

void ADCTripod::SetHeadHeight(float Height)
{
	HeadHeight = FMath::Clamp(Height, MinHeight, MaxHeight);
	const float ColumnBottom = FMath::Min(LegHeight, HeadHeight - 10.f);
	const float ColumnLen = FMath::Max(HeadHeight - 10.f - ColumnBottom, 1.f);
	Column->SetRelativeLocation(FVector(0.f, 0.f, ColumnBottom + ColumnLen * 0.5f));
	Column->SetRelativeScale3D(FVector(0.05f, 0.05f, ColumnLen / 100.f));
	Head->SetRelativeLocation(FVector(0.f, 0.f, HeadHeight - 4.f));
}

void ADCTripod::SetTint(const FLinearColor& Color)
{
	for (UStaticMeshComponent* Leg : Legs)
	{
		DCVisuals::Paint(Leg, Color);
	}
	DCVisuals::Paint(Column, Color);
	DCVisuals::Paint(Head, Color * 0.4f);
}
