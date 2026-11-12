// Director's Cut - fixed camera.

#include "DCMultiCam.h"
#include "DCRail.h"
#include "DCVisuals.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ADCMultiCam::ADCMultiCam()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Head = CreateDefaultSubobject<USceneComponent>(TEXT("Head"));
	Head->SetupAttachment(Root);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Head);
	Camera->SetFieldOfView(DefaultFOV);
	Camera->PostProcessSettings.bOverride_MotionBlurAmount = true;
	Camera->PostProcessSettings.MotionBlurAmount = 0.f;

	auto MakePart = [this](FName Name, UStaticMesh* Mesh, USceneComponent* Parent)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetStaticMesh(Mesh);
		// Cameras never block movement or another camera's line of sight, and are invisible from their own view.
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(false);
		Part->SetOwnerNoSee(true);
		return Part;
	};

	Body = MakePart(TEXT("Body"), CubeMesh.Object, Head);
	Body->SetRelativeLocation(FVector(-18.f, 0.f, 0.f));
	Body->SetRelativeScale3D(FVector(0.36f, 0.18f, 0.24f));

	Lens = MakePart(TEXT("Lens"), CylinderMesh.Object, Head);
	Lens->SetRelativeLocation(FVector(4.f, 0.f, 0.f));
	Lens->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	Lens->SetRelativeScale3D(FVector(0.12f, 0.12f, 0.14f));

	Tally = MakePart(TEXT("Tally"), SphereMesh.Object, Head);
	Tally->SetRelativeLocation(FVector(-8.f, 0.f, 15.f));
	Tally->SetRelativeScale3D(FVector(0.06f));

	Mount = MakePart(TEXT("Mount"), CubeMesh.Object, Root);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 35.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextBottom);
	Label->SetWorldSize(26.f);
	Label->SetTextRenderColor(FColor::White);
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Label->SetCastShadow(false);
	Label->SetOwnerNoSee(true);
}

void ADCMultiCam::Setup(int32 InNumber, const FVector& Surface)
{
	Number = InNumber;
	Rig = EDCCamRig::Static;
	SetNumberLabel();
	UpdateMount(Surface);
	SetTint(FLinearColor(0.08f, 0.08f, 0.09f));
	SetLive(false);
}

void ADCMultiCam::SetupDolly(int32 InNumber, ADCRail* InRail)
{
	Number = InNumber;
	Rig = EDCCamRig::Dolly;
	Rail = InRail;
	RailDistance = HomeRailDistance = 0.f;
	SetNumberLabel();
	Mount->SetVisibility(false);
	ApplyRigPosition();
	SetTint(FLinearColor(0.1f, 0.06f, 0.02f));
	SetLive(false);
}

void ADCMultiCam::SetupCrane(int32 InNumber, const FVector& Floor, float Height)
{
	Number = InNumber;
	Rig = EDCCamRig::Crane;
	CraneBase = Floor;
	CraneHeight = HomeCraneHeight = FMath::Clamp(Height, CraneMinHeight, CraneMaxHeight);
	SetNumberLabel();
	ApplyRigPosition();
	SetTint(FLinearColor(0.02f, 0.06f, 0.1f));
	SetLive(false);
}

void ADCMultiCam::SetNumberLabel()
{
	const TCHAR* Kind = Rig == EDCCamRig::Dolly ? TEXT(" DOLLY") : Rig == EDCCamRig::Crane ? TEXT(" CRANE") : TEXT("");
	Label->SetText(FText::FromString(Number > 0 ? FString::Printf(TEXT("CAM %d%s"), Number, Kind) : FString()));
}

void ADCMultiCam::UpdateMount(const FVector& Surface)
{
	// Mount: a stand down to the floor or a bracket back to the wall/ceiling it is bolted to.
	const FVector ToSurface = Surface - GetActorLocation();
	const float Length = ToSurface.Size();
	Mount->SetVisibility(Length > 1.f);
	if (Length > 1.f)
	{
		Mount->SetWorldLocationAndRotation(GetActorLocation() + ToSurface * 0.5f, FRotationMatrix::MakeFromX(ToSurface).Rotator());
		Mount->SetWorldScale3D(FVector(Length / 100.f, 0.05f, 0.05f));
	}
}

void ADCMultiCam::ApplyRigPosition()
{
	if (Rig == EDCCamRig::Dolly && Rail)
	{
		SetActorLocation(Rail->GetFloorLocationAtDistance(RailDistance));
	}
	else if (Rig == EDCCamRig::Crane)
	{
		SetActorLocation(CraneBase + FVector(0.f, 0.f, CraneHeight));
		UpdateMount(CraneBase);
	}
}

void ADCMultiCam::MoveRig(float Axis, float DeltaSeconds)
{
	if (Rig == EDCCamRig::Dolly && Rail)
	{
		RailDistance = FMath::Clamp(RailDistance + Axis * DollySpeed * DeltaSeconds, 0.f, Rail->GetLength());
	}
	else if (Rig == EDCCamRig::Crane)
	{
		CraneHeight = FMath::Clamp(CraneHeight + Axis * CraneSpeed * DeltaSeconds, CraneMinHeight, CraneMaxHeight);
	}
	ApplyRigPosition();
}

void ADCMultiCam::Destroyed()
{
	if (Rail)
	{
		Rail->Destroy();
		Rail = nullptr;
	}
	Super::Destroyed();
}

float ADCMultiCam::GetRailProgress() const
{
	return Rail && Rail->GetLength() > 0.f ? RailDistance / Rail->GetLength() : 0.f;
}

void ADCMultiCam::SetAim(const FRotator& NewAim)
{
	Aim = FRotator(FMath::Clamp(NewAim.Pitch, -85.f, 85.f), FRotator::NormalizeAxis(NewAim.Yaw), 0.f);
	Head->SetWorldRotation(Aim);
}

float ADCMultiCam::GetFOV() const
{
	return Camera->FieldOfView;
}

void ADCMultiCam::SetFOV(float NewFOV)
{
	Camera->SetFieldOfView(FMath::Clamp(NewFOV, MinFOV, MaxFOV));
}

void ADCMultiCam::SaveHome()
{
	HomeAim = Aim;
	HomeFOV = GetFOV();
	HomeRailDistance = RailDistance;
	HomeCraneHeight = CraneHeight;
}

void ADCMultiCam::RestoreHome()
{
	SetAim(HomeAim);
	SetFOV(HomeFOV);
	RailDistance = HomeRailDistance;
	CraneHeight = HomeCraneHeight;
	ApplyRigPosition();
}

void ADCMultiCam::SetLive(bool bLive)
{
	DCVisuals::Paint(Tally, bLive ? FLinearColor(1.f, 0.05f, 0.05f) : FLinearColor(0.15f, 0.02f, 0.02f));
}

void ADCMultiCam::SetLabelVisible(bool bVisible)
{
	Label->SetVisibility(bVisible);
}

void ADCMultiCam::SetTint(const FLinearColor& Color)
{
	DCVisuals::Paint(Body, Color);
	DCVisuals::Paint(Lens, Color * 0.5f);
	DCVisuals::Paint(Mount, Color * 1.5f);
}
