// Director's Cut - hidden story beat.

#include "DCBeat.h"
#include "DCPlacement.h"
#include "DCPerformer.h"
#include "DCVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ADCBeat::ADCBeat()
{
	PrimaryActorTick.bCanEverTick = false;
	SetHidden(true);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	DCVisuals::SetupLabel(Label, FColor::White, 26.f);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SpotMat(TEXT("/Game/DirectorsCut/Materials/M_DCCameraSpot.M_DCCameraSpot"));
	SpotMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpotMarker"));
	SpotMarker->SetupAttachment(Root);
	SpotMarker->SetStaticMesh(PlaneMesh.Object);
	if (SpotMat.Succeeded())
	{
		SpotMarker->SetMaterial(0, SpotMat.Object);
	}
	SpotMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpotMarker->SetCastShadow(false);
	SpotMarker->SetVisibility(false);
}

void ADCBeat::BeginPlay()
{
	Super::BeginPlay();
	SpotMarker->SetVisibility(false);
	if (bShowCameraSpot)
	{
		// The beat stays a hidden editor marker except for its hint circle.
		Label->SetHiddenInGame(true);
		SetActorHiddenInGame(false);
		PlaceSpotMarker();
		SpotMaterial = SpotMarker->CreateDynamicMaterialInstance(0);
	}
}

void ADCBeat::PlaceSpotMarker()
{
	const FVector Spot = GetActorTransform().TransformPosition(CameraSpot);
	FVector Ground = Spot;
	FHitResult Hit;
	FCollisionQueryParams Params(TEXT("DCCameraSpot"), false, this);
	if (DCPlacement::TraceGround(GetWorld(), Hit, Spot + FVector(0.f, 0.f, 300.f), Spot - FVector(0.f, 0.f, 3000.f), Params))
	{
		Ground = Hit.ImpactPoint;
	}
	// The engine plane is 100 cm across and faces up; lift it a hair to avoid z-fighting with the floor.
	SpotMarker->SetWorldLocationAndRotation(Ground + FVector(0.f, 0.f, 2.f), FRotator::ZeroRotator);
	SpotMarker->SetWorldScale3D(FVector(CameraSpotRadius / 50.f));
}

void ADCBeat::SetCameraSpotOpacity(float Opacity)
{
	if (!bShowCameraSpot || FMath::IsNearlyEqual(Opacity, SpotOpacity, 0.005f))
	{
		return;
	}
	SpotOpacity = Opacity;
	SpotMarker->SetVisibility(Opacity > 0.005f);
	if (SpotMaterial)
	{
		SpotMaterial->SetScalarParameterValue(TEXT("Opacity"), Opacity);
	}
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
	Label->SetText(FText::FromString(FString::Printf(TEXT("BEAT %s %s\n%.0f-%.0fs: %s"),
		*BeatId, *DisplayName, WindowStart, WindowEnd, *FString::Join(Subjects, TEXT(" + ")))));
	Label->SetTextRenderColor(FColor(80, 220, 255));

	// In the editor the circle is always shown (when enabled) so it can be placed; in game the director drives it.
	SpotMarker->SetVisibility(bShowCameraSpot);
	if (bShowCameraSpot)
	{
		PlaceSpotMarker();
	}
}
