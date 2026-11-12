// Director's Cut prototype - placeholder-art helpers.

#include "DCVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace DCVisuals
{
	static UStaticMesh* LoadMesh(const TCHAR* Path)
	{
		return LoadObject<UStaticMesh>(nullptr, Path);
	}

	UStaticMesh* Cube()     { return LoadMesh(TEXT("/Engine/BasicShapes/Cube.Cube")); }
	UStaticMesh* Cylinder() { return LoadMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")); }
	UStaticMesh* Sphere()   { return LoadMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere")); }
	UStaticMesh* Cone()     { return LoadMesh(TEXT("/Engine/BasicShapes/Cone.Cone")); }

	UMaterialInstanceDynamic* MakeColor(UObject* Outer, const FLinearColor& Color)
	{
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, Outer);
		MID->SetFlags(RF_Transient);
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		return MID;
	}

	void Paint(UStaticMeshComponent* Mesh, const FLinearColor& Color)
	{
		if (!Mesh)
		{
			return;
		}
		if (UMaterialInstanceDynamic* Existing = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0)))
		{
			Existing->SetVectorParameterValue(TEXT("Color"), Color);
			return;
		}
		Mesh->SetMaterial(0, MakeColor(Mesh, Color));
	}

	void SetupLabel(UTextRenderComponent* Label, const FColor& Color, float Size)
	{
		Label->SetHiddenInGame(true);
		Label->SetHorizontalAlignment(EHTA_Center);
		Label->SetVerticalAlignment(EVRTA_TextBottom);
		Label->SetTextRenderColor(Color);
		Label->SetWorldSize(Size);
		Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Label->SetCastShadow(false);
	}
}
