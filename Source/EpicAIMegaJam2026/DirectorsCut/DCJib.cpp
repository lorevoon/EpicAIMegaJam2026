// Director's Cut prototype 5 - jib arm.

#include "DCJib.h"
#include "DCVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float TailLength = 130.f;
	/** The camera hangs below the end of the arm on a remote head. */
	constexpr float HeadDrop = 28.f;
}

ADCJib::ADCJib()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	auto MakePart = [this](FName Name, UStaticMesh* Mesh, USceneComponent* Parent)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetStaticMesh(Mesh);
		// Rigs never block movement or the camera's line of sight.
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(false);
		return Part;
	};

	Base = MakePart(TEXT("Base"), CylinderMesh.Object, Root);
	Base->SetRelativeLocation(FVector(0.f, 0.f, 6.f));
	Base->SetRelativeScale3D(FVector(1.1f, 1.1f, 0.12f));

	Post = MakePart(TEXT("Post"), CylinderMesh.Object, Root);
	Post->SetRelativeLocation(FVector(0.f, 0.f, PivotHeight * 0.5f));
	Post->SetRelativeScale3D(FVector(0.14f, 0.14f, PivotHeight / 100.f));

	ArmPivot = CreateDefaultSubobject<USceneComponent>(TEXT("ArmPivot"));
	ArmPivot->SetupAttachment(Root);
	ArmPivot->SetRelativeLocation(FVector(0.f, 0.f, PivotHeight));

	Arm = MakePart(TEXT("Arm"), CubeMesh.Object, ArmPivot);
	Arm->SetRelativeLocation(FVector((ArmLength - TailLength) * 0.5f, 0.f, 0.f));
	Arm->SetRelativeScale3D(FVector((ArmLength + TailLength) / 100.f, 0.1f, 0.12f));

	Counterweight = MakePart(TEXT("Counterweight"), CubeMesh.Object, ArmPivot);
	Counterweight->SetRelativeLocation(FVector(-TailLength, 0.f, -10.f));
	Counterweight->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.4f));

	Head = MakePart(TEXT("Head"), CubeMesh.Object, Root);
	Head->SetRelativeScale3D(FVector(0.3f, 0.16f, 0.16f));

	SetArm(ArmYaw, ArmPitch);
}

void ADCJib::SetArm(float InYaw, float InPitch)
{
	ArmYaw = FRotator::NormalizeAxis(InYaw);
	ArmPitch = FMath::Clamp(InPitch, MinPitch, MaxPitch);
	ArmPivot->SetWorldRotation(FRotator(ArmPitch, ArmYaw, 0.f));
	// The remote head stays level under the end of the arm.
	Head->SetWorldLocationAndRotation(GetCameraLocation(), FRotator(0.f, ArmYaw, 0.f));
}

FVector ADCJib::GetCameraLocation() const
{
	return GetPivotLocation() + FRotator(ArmPitch, ArmYaw, 0.f).Vector() * ArmLength - FVector(0.f, 0.f, HeadDrop);
}

void ADCJib::SetTint(const FLinearColor& Color)
{
	DCVisuals::Paint(Base, Color * 0.5f);
	DCVisuals::Paint(Post, Color);
	DCVisuals::Paint(Arm, Color);
	DCVisuals::Paint(Counterweight, Color * 0.4f);
	DCVisuals::Paint(Head, Color * 0.4f);
}
