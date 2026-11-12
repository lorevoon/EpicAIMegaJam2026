// Director's Cut - dolly rail placement, validation, and visuals.

#include "DCRail.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float SampleStep = 40.f;
	constexpr float TraceHeight = 30.f;
	constexpr float MinPointSpacing = 100.f;
	constexpr float RailGauge = 25.f;
}

ADCRail::ADCRail()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = Cube.Object;
	CylinderMesh = Cylinder.Object;
	BaseMaterial = Material.Object;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	Spline->SetupAttachment(RootComponent);
	Spline->ClearSplinePoints();

	Ghost = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ghost"));
	Ghost->SetupAttachment(RootComponent);
	Ghost->SetStaticMesh(CylinderMesh);
	Ghost->SetRelativeScale3D(FVector(0.25f, 0.25f, 0.6f));
	Ghost->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ghost->SetCastShadow(false);
	Ghost->SetVisibility(false);
}

static UMaterialInstanceDynamic* MakeColor(UMaterialInterface* Base, UObject* Outer, const FLinearColor& Color)
{
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, Outer);
	MID->SetVectorParameterValue(TEXT("Color"), Color);
	return MID;
}

bool ADCRail::TryAddPoint(const FVector& Point, FString& OutReason, TOptional<FVector> Surface)
{
	if (Points.Num() >= MaxPoints)
	{
		OutReason = FString::Printf(TEXT("Rail is at the %d point limit"), MaxPoints);
		return false;
	}
	if (Points.Num() > 0 && FVector::Dist(Points.Last(), Point) < MinPointSpacing)
	{
		OutReason = TEXT("Too close to the previous rail point");
		return false;
	}

	Points.Add(Point);
	SurfacePoints.Add(Surface.Get(Point));
	RebuildSpline(Points);
	if (!IsPathClear(OutReason))
	{
		Points.Pop();
		SurfacePoints.Pop();
		RebuildSpline(Points);
		return false;
	}

	RebuildVisuals();
	return true;
}

bool ADCRail::RemovePointNear(const FVector& Near, float Radius)
{
	if (Points.Num() == 0)
	{
		return false;
	}

	int32 Best = Points.Num() - 1;
	float BestDist = Radius;
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		const float D = FVector::Dist(Points[i], Near);
		if (D < BestDist)
		{
			BestDist = D;
			Best = i;
		}
	}

	const FVector Removed = Points[Best];
	const FVector RemovedSurface = SurfacePoints[Best];
	Points.RemoveAt(Best);
	SurfacePoints.RemoveAt(Best);
	RebuildSpline(Points);
	FString Unused;
	if (!IsPathClear(Unused))
	{
		// Removing a middle point would reroute the curve through scenery, so keep it.
		Points.Insert(Removed, Best);
		SurfacePoints.Insert(RemovedSurface, Best);
		RebuildSpline(Points);
		return false;
	}
	RebuildVisuals();
	return true;
}

void ADCRail::Clear()
{
	Points.Reset();
	SurfacePoints.Reset();
	RebuildSpline(Points);
	RebuildVisuals();
}

void ADCRail::SetPreview(bool bVisible, const FVector& Location, bool bValid, float ZOffset)
{
	Ghost->SetVisibility(bVisible);
	if (!bVisible)
	{
		return;
	}
	if (!GhostMaterial)
	{
		GhostMaterial = MakeColor(BaseMaterial, this, FLinearColor::White);
		Ghost->SetMaterial(0, GhostMaterial);
	}
	GhostMaterial->SetVectorParameterValue(TEXT("Color"), bValid ? FLinearColor(0.2f, 1.f, 0.4f) : FLinearColor(1.f, 0.15f, 0.1f));
	Ghost->SetWorldLocation(Location + FVector(0.f, 0.f, ZOffset));
}

float ADCRail::GetLength() const
{
	return IsUsable() ? Spline->GetSplineLength() : 0.f;
}

FVector ADCRail::GetFloorLocationAtDistance(float Distance) const
{
	return Spline->GetLocationAtDistanceAlongSpline(FMath::Clamp(Distance, 0.f, GetLength()), ESplineCoordinateSpace::World);
}

float ADCRail::FindClosestDistance(const FVector& Location, float& OutGap) const
{
	if (!IsUsable())
	{
		OutGap = TNumericLimits<float>::Max();
		return 0.f;
	}
	const float Key = Spline->FindInputKeyClosestToWorldLocation(Location);
	const float Distance = Spline->GetDistanceAlongSplineAtSplineInputKey(Key);
	const FVector OnRail = GetFloorLocationAtDistance(Distance);
	OutGap = bFloorRail ? FVector::Dist2D(OnRail, Location) : FVector::Dist(OnRail, Location);
	return Distance;
}

void ADCRail::RebuildSpline(const TArray<FVector>& InPoints) const
{
	Spline->ClearSplinePoints(false);
	for (const FVector& P : InPoints)
	{
		Spline->AddSplinePoint(P, ESplineCoordinateSpace::World, false);
	}
	Spline->UpdateSpline();
}

bool ADCRail::IsPathClear(FString& OutReason) const
{
	if (!IsUsable())
	{
		return true;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(DCRailPath), false, this);
	for (const TWeakObjectPtr<AActor>& Ignored : IgnoredActors)
	{
		Params.AddIgnoredActor(Ignored.Get());
	}

	const FVector Lift(0.f, 0.f, bFloorRail ? TraceHeight : 0.f);
	const float Length = Spline->GetSplineLength();
	FVector Prev = GetFloorLocationAtDistance(0.f);
	for (float D = SampleStep; D < Length + SampleStep; D += SampleStep)
	{
		const FVector Next = GetFloorLocationAtDistance(D);
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Prev + Lift, Next + Lift, ECC_Visibility, Params))
		{
			OutReason = TEXT("Rail would pass through scenery");
			return false;
		}
		Prev = Next;
	}
	return true;
}

UStaticMeshComponent* ADCRail::AddPiece(UStaticMesh* Mesh, const FTransform& Transform, UMaterialInterface* Material)
{
	UStaticMeshComponent* Piece = NewObject<UStaticMeshComponent>(this);
	Piece->SetStaticMesh(Mesh);
	Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Piece->SetCastShadow(false);
	Piece->SetupAttachment(RootComponent);
	Piece->RegisterComponent();
	Piece->SetWorldTransform(Transform);
	Piece->SetMaterial(0, Material);
	Pieces.Add(Piece);
	return Piece;
}

void ADCRail::RebuildVisuals()
{
	for (UStaticMeshComponent* Piece : Pieces)
	{
		if (Piece)
		{
			Piece->DestroyComponent();
		}
	}
	Pieces.Reset();

	if (!RailMaterial)
	{
		RailMaterial = MakeColor(BaseMaterial, this, FLinearColor(0.75f, 0.75f, 0.8f));
		TieMaterial = MakeColor(BaseMaterial, this, FLinearColor(0.25f, 0.15f, 0.08f));
		PostMaterial = MakeColor(BaseMaterial, this, PostColor);
	}

	for (int32 i = 0; i < Points.Num(); ++i)
	{
		const FVector& P = Points[i];
		if (bFloorRail)
		{
			AddPiece(CylinderMesh, FTransform(FRotator::ZeroRotator, P + FVector(0, 0, 6), FVector(0.22f, 0.22f, 0.12f)), PostMaterial);
			continue;
		}
		// Free rails: a knob at the camera position plus a standoff bracket back to the surface it is bolted to.
		AddPiece(CylinderMesh, FTransform(FRotator::ZeroRotator, P, FVector(0.16f, 0.16f, 0.16f)), PostMaterial);
		const FVector Bracket = SurfacePoints[i] - P;
		if (Bracket.Size() > 1.f)
		{
			AddPiece(CubeMesh, FTransform(Bracket.Rotation(), P + Bracket * 0.5f, FVector(Bracket.Size() / 100.f, 0.05f, 0.05f)), TieMaterial);
		}
	}

	if (!IsUsable())
	{
		return;
	}

	const float Length = Spline->GetSplineLength();
	int32 Step = 0;
	for (float D = 0.f; D < Length; D += SampleStep, ++Step)
	{
		const FVector A = GetFloorLocationAtDistance(D);
		const FVector B = GetFloorLocationAtDistance(FMath::Min(D + SampleStep, Length));
		const FVector Dir = (B - A).GetSafeNormal();
		if (Dir.IsNearlyZero())
		{
			continue;
		}
		const FRotator Rot = Dir.Rotation();
		const FVector Right = FRotationMatrix(Rot).GetUnitAxis(EAxis::Y);
		const FVector Mid = (A + B) * 0.5f + FVector(0, 0, bFloorRail ? 4.f : 0.f);
		const float SegScale = (B - A).Size() / 100.f + 0.02f;

		AddPiece(CubeMesh, FTransform(Rot, Mid + Right * RailGauge, FVector(SegScale, 0.04f, 0.04f)), RailMaterial);
		AddPiece(CubeMesh, FTransform(Rot, Mid - Right * RailGauge, FVector(SegScale, 0.04f, 0.04f)), RailMaterial);
		if (bFloorRail && Step % 2 == 0)
		{
			AddPiece(CubeMesh, FTransform(Rot, A + FVector(0, 0, 1), FVector(0.08f, 0.7f, 0.02f)), TieMaterial);
		}
	}
}
