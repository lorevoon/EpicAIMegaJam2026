// Director's Cut - placeholder-art helpers.

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

	void PaintWith(UStaticMeshComponent* Mesh, UMaterialInterface* Base, const FLinearColor& Color)
	{
		if (!Mesh || !Base)
		{
			return;
		}
		UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0));
		if (!MID || MID->Parent != Base)
		{
			MID = UMaterialInstanceDynamic::Create(Base, Mesh);
			MID->SetFlags(RF_Transient);
			Mesh->SetMaterial(0, MID);
		}
		MID->SetVectorParameterValue(TEXT("Color"), Color);
	}

	void SetFrameMask(UMaterialInterface* Material, bool bMasked)
	{
		if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Material))
		{
			MID->SetScalarParameterValue(TEXT("FrameMask"), bMasked ? 1.f : 0.f);
		}
	}

	void SetFrameMask(UStaticMeshComponent* Mesh, bool bMasked)
	{
		if (Mesh)
		{
			SetFrameMask(Mesh->GetMaterial(0), bMasked);
		}
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
