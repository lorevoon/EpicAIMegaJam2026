// Director's Cut - placeholder-art helpers shared by the editable actors.

#pragma once

#include "CoreMinimal.h"

class UMaterialInstanceDynamic;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;

namespace DCVisuals
{
	UStaticMesh* Cube();
	UStaticMesh* Cylinder();
	UStaticMesh* Sphere();
	UStaticMesh* Cone();

	/** Transient flat-color material on the engine's basic shape material. Safe to call from OnConstruction. */
	UMaterialInstanceDynamic* MakeColor(UObject* Outer, const FLinearColor& Color);

	/** Applies (or recolors) a transient flat-color material on a mesh. */
	void Paint(UStaticMeshComponent* Mesh, const FLinearColor& Color);

	/** Editor-only floating label (hidden in game; the HUD draws its own labels). */
	void SetupLabel(UTextRenderComponent* Label, const FColor& Color, float Size);
}
