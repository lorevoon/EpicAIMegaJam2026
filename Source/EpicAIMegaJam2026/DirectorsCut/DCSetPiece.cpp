// Director's Cut prototype - blockout set piece.

#include "DCSetPiece.h"
#include "DCVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ADCSetPiece::ADCSetPiece()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetStaticMesh(CubeMesh.Object);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RootComponent = Mesh;
}

void ADCSetPiece::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (bApplyColor)
	{
		DCVisuals::Paint(Mesh, Color);
	}
}
