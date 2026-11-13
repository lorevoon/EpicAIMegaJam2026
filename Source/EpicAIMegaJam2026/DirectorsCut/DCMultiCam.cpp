// Director's Cut - fixed camera.

#include "DCMultiCam.h"
#include "DCRail.h"
#include "DCVisuals.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const float RailSpeedScales[] = { 0.25f, 0.5f, 1.f, 2.f };

	/** How far above its rail a dolly camera's lens sits. */
	constexpr float DollyLift = 100.f;
}

ADCMultiCam::ADCMultiCam()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RigMat(DCVisuals::RigMaterialPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	RigMaterial = RigMat.Succeeded() ? RigMat.Object : BasicMat.Object;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Head = CreateDefaultSubobject<USceneComponent>(TEXT("Head"));
	Head->SetupAttachment(Root);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Head);
	Camera->SetFieldOfView(DefaultFOV);
	// FOV is the horizontal angle across the whole screen, on any screen shape. The engine's default (Maintain Y FOV)
	// would rescale it by the window's aspect, and then the live view would no longer match the capture-frame math
	// (DCFrame), the zombie and beat checks, or the review playback.
	Camera->bOverrideAspectRatioAxisConstraint = true;
	Camera->SetAspectRatioAxisConstraint(EAspectRatioAxisConstraint::AspectRatio_MaintainXFOV);
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
	Boom = MakePart(TEXT("CraneBoom"), CubeMesh.Object, Root);
	Boom->SetVisibility(false);

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

void ADCMultiCam::Setup(int32 InNumber, const FVector& Surface, EDCCamRig InRig)
{
	Number = InNumber;
	Rig = InRig;
	PlacementCenter = HomeLocation = GetActorLocation();
	MountSurface = Surface;
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
	ApplyRigPosition();
	SetTint(FLinearColor(0.1f, 0.06f, 0.02f));
	SetLive(false);
}

void ADCMultiCam::SetNumberLabel()
{
	const TCHAR* Kind = Rig == EDCCamRig::Crane ? TEXT(" CRANE") : Rig == EDCCamRig::Dolly ? TEXT(" DOLLY") : TEXT(" STATIC");
	Label->SetText(FText::FromString(Number > 0 ? FString::Printf(TEXT("CAM %d%s"), Number, Kind) : FString()));
}

void ADCMultiCam::UpdateMount(const FVector& Surface)
{
	const bool bCrane = Rig == EDCCamRig::Crane;
	const FVector Base = bCrane ? MountSurface - FVector(250.f, 0.f, 0.f) : Surface;
	const FVector Top = bCrane ? FVector(Base.X, Base.Y, PlacementCenter.Z) : GetActorLocation();
	auto Beam = [](UStaticMeshComponent* Part, const FVector& A, const FVector& B, float Width)
	{
		const FVector Delta = B - A;
		Part->SetVisibility(Delta.Size() > 1.f);
		Part->SetWorldLocationAndRotation((A+B)*.5f, Delta.Rotation());
		Part->SetWorldScale3D(FVector(Delta.Size()/100.f, Width, Width));
	};
	Beam(Mount, Base, Top, bCrane ? .14f : .05f);
	Boom->SetVisibility(bCrane);
	if (bCrane) Beam(Boom, Top, GetActorLocation(), .10f);
}

void ADCMultiCam::ApplyRigPosition()
{
	if (Rig == EDCCamRig::Dolly && Rail)
	{
		// Ride above the rail on a short post, so the rail sits below the lens instead of running through the shot.
		const FVector OnRail = Rail->GetFloorLocationAtDistance(RailDistance);
		SetActorLocation(OnRail + FVector(0.f, 0.f, DollyLift));
		UpdateMount(OnRail);
	}
}

void ADCMultiCam::MoveRig(float Axis, float DeltaSeconds)
{
	if (DollyEase > 0.f)
	{
		RigInput = Axis; // eased in UpdateStabilizer
		return;
	}
	if (Rig == EDCCamRig::Dolly && Rail)
	{
		RailDistance = FMath::Clamp(RailDistance + Axis * DollySpeed * GetRailSpeedScale() * DeltaSeconds, 0.f, Rail->GetLength());
		ApplyRigPosition();
	}
}

void ADCMultiCam::MoveLocal(const FVector2D& Axis, float DeltaSeconds)
{
	if (Rig == EDCCamRig::Dolly) { MoveRig(Axis.Y, DeltaSeconds); return; }
	const FVector2D Input = Axis.GetClampedToMaxSize(1.f);
	const FRotator Facing(0.f, Aim.Yaw, 0.f);
	const FVector Delta = (Facing.Vector()*Input.Y + FRotationMatrix(Facing).GetUnitAxis(EAxis::Y)*Input.X)
		* (Rig == EDCCamRig::Crane ? 180.f : 120.f) * FMath::Max(0.f, DeltaSeconds);
	FVector Offset = GetActorLocation() + Delta - PlacementCenter;
	Offset.Z = 0.f;
	const FVector Target = PlacementCenter + Offset.GetClampedToMaxSize(GetMovementRadius());
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DCFreeCameraMove), false, this);
	if (GetOwner()) Params.AddIgnoredActor(GetOwner());
	for (TActorIterator<ADCMultiCam> It(GetWorld()); It; ++It) Params.AddIgnoredActor(*It);
	FHitResult Hit;
	if (!GetWorld()->SweepSingleByChannel(Hit, GetActorLocation(), Target, FQuat::Identity,
		ECC_Visibility, FCollisionShape::MakeSphere(15.f), Params))
	{
		SetActorLocation(Target);
		UpdateMount(MountSurface);
	}
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

void ADCMultiCam::SetAimTarget(const FRotator& NewAim)
{
	Aim = FRotator(FMath::Clamp(NewAim.Pitch, -90.f, 85.f), FRotator::NormalizeAxis(NewAim.Yaw), 0.f);
}

void ADCMultiCam::SetAim(const FRotator& NewAim)
{
	SetAimTarget(NewAim);
	Head->SetWorldRotation(Aim);
}

void ADCMultiCam::AddAim(float DeltaYaw, float DeltaPitch)
{
	SetAimTarget(Aim + FRotator(DeltaPitch, DeltaYaw, 0.f));
	if (AimSmoothing <= 0.f)
	{
		Head->SetWorldRotation(Aim);
	}
}

void ADCMultiCam::SetStabilizer(float InAimSmoothing, float InDollyEase)
{
	AimSmoothing = InAimSmoothing;
	DollyEase = InDollyEase;
	if (AimSmoothing <= 0.f)
	{
		Head->SetWorldRotation(Aim);
	}
	if (DollyEase <= 0.f)
	{
		RailVelocity = 0.f;
		RigInput = 0.f;
	}
}

void ADCMultiCam::UpdateStabilizer(float DeltaSeconds)
{
	// Head: glide toward the aim (shortest way round) instead of snapping, like a fluid head on a tripod.
	if (AimSmoothing > 0.f)
	{
		Head->SetWorldRotation(FMath::RInterpTo(Head->GetComponentRotation(), Aim, DeltaSeconds, AimSmoothing));
	}

	// Dolly: ease up to speed and coast to a stop instead of starting and stopping dead.
	if (DollyEase > 0.f && Rig == EDCCamRig::Dolly && Rail)
	{
		RailVelocity = FMath::FInterpTo(RailVelocity, RigInput * DollySpeed * GetRailSpeedScale(), DeltaSeconds, DollyEase);
		RigInput = 0.f;
		if (!FMath::IsNearlyZero(RailVelocity, 0.5f))
		{
			const float Unclamped = RailDistance + RailVelocity * DeltaSeconds;
			RailDistance = FMath::Clamp(Unclamped, 0.f, Rail->GetLength());
			if (RailDistance != Unclamped)
			{
				RailVelocity = 0.f; // hit the end of the rail
			}
			ApplyRigPosition();
		}
	}
}

float ADCMultiCam::GetFOV() const
{
	return Camera->FieldOfView;
}

void ADCMultiCam::SetFOV(float NewFOV)
{
	TargetFOV = FMath::Clamp(NewFOV, MinFOV, MaxFOV);
}

void ADCMultiCam::UpdateZoom(float DeltaSeconds)
{
	// Ease toward the target so wheel zooms read as a smooth push in / pull out.
	const float Current = Camera->FieldOfView;
	if (!FMath::IsNearlyEqual(Current, TargetFOV, 0.01f))
	{
		Camera->SetFieldOfView(FMath::FInterpTo(Current, TargetFOV, DeltaSeconds, 6.f));
	}
}

float ADCMultiCam::GetRailSpeedScale() const
{
	return RailSpeedScales[RailSpeedIndex];
}

void ADCMultiCam::AdjustRailSpeed(int32 Direction)
{
	RailSpeedIndex = FMath::Clamp(RailSpeedIndex + Direction, 0, int32(UE_ARRAY_COUNT(RailSpeedScales)) - 1);
}

void ADCMultiCam::SaveHome()
{
	HomeLocation = GetActorLocation();
	HomeAim = Aim;
	HomeFOV = TargetFOV;
	HomeRailDistance = RailDistance;
}

void ADCMultiCam::RestoreHome()
{
	if (Rig != EDCCamRig::Dolly)
	{
		SetActorLocation(HomeLocation);
		UpdateMount(MountSurface);
	}
	SetAim(HomeAim);
	TargetFOV = HomeFOV;
	Camera->SetFieldOfView(HomeFOV);
	RailDistance = HomeRailDistance;
	RailVelocity = 0.f;
	RigInput = 0.f;
	ApplyRigPosition();
}

void ADCMultiCam::SetLive(bool bLive)
{
	DCVisuals::PaintWith(Tally, RigMaterial, bLive ? FLinearColor(1.f, 0.05f, 0.05f) : FLinearColor(0.15f, 0.02f, 0.02f));
}

void ADCMultiCam::SetFrameMasked(bool bMasked)
{
	for (UStaticMeshComponent* Part : { Body.Get(), Lens.Get(), Tally.Get(), Mount.Get(), Boom.Get() })
	{
		DCVisuals::SetFrameMask(Part, bMasked);
	}
	Label->SetVisibility(!bMasked);
	if (Rail)
	{
		Rail->SetFrameMasked(bMasked);
	}
}

void ADCMultiCam::SetRigHidden(bool bHide)
{
	SetActorHiddenInGame(bHide);
	if (Rail)
	{
		Rail->SetActorHiddenInGame(bHide);
	}
}

void ADCMultiCam::SetTint(const FLinearColor& Color)
{
	DCVisuals::PaintWith(Body, RigMaterial, Color);
	DCVisuals::PaintWith(Lens, RigMaterial, Color * 0.5f);
	DCVisuals::PaintWith(Mount, RigMaterial, Color * 1.5f);
	DCVisuals::PaintWith(Boom, RigMaterial, Color * 1.5f);
}
