// Director's Cut - spray mark.

#include "DCSprayMark.h"
#include "DCVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ADCSprayMark::ADCSprayMark()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// Root's Z axis is the surface normal.
	Splat = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Splat"));
	Splat->SetupAttachment(Root);
	Splat->SetStaticMesh(CylinderMesh.Object);
	Splat->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.01f));
	Splat->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Splat->SetCastShadow(false);

	// Text faces along the normal so it reads from in front of the surface.
	Digits = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Digits"));
	Digits->SetupAttachment(Root);
	Digits->SetRelativeLocation(FVector(0.f, 0.f, 2.f));
	Digits->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	Digits->SetHorizontalAlignment(EHTA_Center);
	Digits->SetVerticalAlignment(EVRTA_TextCenter);
	Digits->SetWorldSize(50.f);
	Digits->SetTextRenderColor(FColor(20, 20, 20));
	Digits->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Digits->SetCastShadow(false);
}

void ADCSprayMark::Setup(int32 InNumber, const FVector& Surface, const FVector& Normal)
{
	Number = InNumber;
	SetActorLocationAndRotation(Surface + Normal * 1.f, FRotationMatrix::MakeFromZ(Normal).Rotator());
	Digits->SetText(FText::FromString(Number > 0 ? FString::FromInt(Number) : FString()));
	SetTint(FLinearColor(1.f, 0.2f, 0.75f));
}

void ADCSprayMark::SetTint(const FLinearColor& Color)
{
	DCVisuals::Paint(Splat, Color);
}
