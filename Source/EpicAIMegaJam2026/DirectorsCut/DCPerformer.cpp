// Director's Cut prototype - performer playback.

#include "DCPerformer.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FVector HeadRest(0.f, 0.f, 178.f);
	const FVector PropLowered(25.f, 22.f, 95.f);
	const FVector PropRaised(42.f, 0.f, 138.f);
}

ADCPerformer::ADCPerformer()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = Material.Object;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Body = MakePart(TEXT("Body"), CylinderMesh.Object, Root);
	Body->SetRelativeLocation(FVector(0.f, 0.f, 78.f));
	Body->SetRelativeScale3D(FVector(0.55f, 0.55f, 1.56f));

	Cape = MakePart(TEXT("Cape"), ConeMesh.Object, Root);
	Cape->SetRelativeLocation(FVector(-12.f, 0.f, 75.f));
	Cape->SetRelativeScale3D(FVector(0.85f, 0.95f, 1.5f));

	Head = MakePart(TEXT("Head"), SphereMesh.Object, Root);
	Head->SetRelativeLocation(HeadRest);
	Head->SetRelativeScale3D(FVector(0.35f));

	// Nose shows which way the performer faces.
	Nose = MakePart(TEXT("Nose"), CubeMesh.Object, Head);
	Nose->SetRelativeLocation(FVector(50.f, 0.f, 0.f));
	Nose->SetRelativeScale3D(FVector(0.3f, 0.2f, 0.2f));

	// The letter has no collision so it never blocks its own framing trace.
	Prop = MakePart(TEXT("Prop"), CubeMesh.Object, Root);
	Prop->SetRelativeLocation(PropLowered);
	Prop->SetRelativeScale3D(FVector(0.02f, 0.22f, 0.3f));
	Prop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

UStaticMeshComponent* ADCPerformer::MakePart(FName Name, UStaticMesh* Mesh, USceneComponent* Parent)
{
	UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Part->SetupAttachment(Parent);
	Part->SetStaticMesh(Mesh);
	// Performers block the camera's line of sight but never push the camera operator.
	Part->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Part->SetCollisionResponseToAllChannels(ECR_Ignore);
	Part->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Part->SetCanEverAffectNavigation(false);
	return Part;
}

void ADCPerformer::Paint(UStaticMeshComponent* Part, const FLinearColor& Color)
{
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	MID->SetVectorParameterValue(TEXT("Color"), Color);
	Part->SetMaterial(0, MID);
}

void ADCPerformer::Setup(const FLinearColor& BodyColor, const FLinearColor& HeadColor, bool bWithCape, bool bWithProp, TArray<FDCKey> InKeys)
{
	Keys = MoveTemp(InKeys);
	Keys.Sort([](const FDCKey& A, const FDCKey& B) { return A.Time < B.Time; });

	Paint(Body, BodyColor);
	Paint(Head, HeadColor);
	Paint(Nose, HeadColor * 0.7f);
	Paint(Cape, FLinearColor(0.35f, 0.01f, 0.02f));
	Paint(Prop, FLinearColor(0.95f, 0.9f, 0.7f));
	Cape->SetVisibility(bWithCape);
	Cape->SetCollisionEnabled(bWithCape ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	Prop->SetVisibility(bWithProp);

	Evaluate(0.f);
}

void ADCPerformer::Evaluate(float SceneTime)
{
	if (Keys.Num() == 0)
	{
		return;
	}

	FDCKey Pose = Keys.Last();
	for (int32 i = 0; i + 1 < Keys.Num(); ++i)
	{
		const FDCKey& A = Keys[i];
		const FDCKey& B = Keys[i + 1];
		if (SceneTime < B.Time)
		{
			const float Span = FMath::Max(B.Time - A.Time, KINDA_SMALL_NUMBER);
			const float Alpha = FMath::InterpEaseInOut(0.f, 1.f, FMath::Clamp((SceneTime - A.Time) / Span, 0.f, 1.f), 2.f);
			Pose.Location = FMath::Lerp(A.Location, B.Location, Alpha);
			Pose.Yaw = FQuat::Slerp(FRotator(0.f, A.Yaw, 0.f).Quaternion(), FRotator(0.f, B.Yaw, 0.f).Quaternion(), Alpha).Rotator().Yaw;
			Pose.Lean = FMath::Lerp(A.Lean, B.Lean, Alpha);
			Pose.PropRaise = FMath::Lerp(A.PropRaise, B.PropRaise, Alpha);
			break;
		}
	}

	SetActorLocationAndRotation(FVector(Pose.Location, 0.f), FRotator(0.f, Pose.Yaw, 0.f));
	Head->SetRelativeLocation(HeadRest + FVector(Pose.Lean, 0.f, -FMath::Abs(Pose.Lean) * 0.5f));
	Prop->SetRelativeLocation(FMath::Lerp(PropLowered, PropRaised, Pose.PropRaise));
}
