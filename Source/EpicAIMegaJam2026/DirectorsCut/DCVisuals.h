// Director's Cut - placeholder-art helpers shared by the editable actors.

#pragma once

#include "CoreMinimal.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
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

	/**
	 * Camera-rig material: translucent flat color, plus a FrameMask switch that fades every pixel inside the capture
	 * frame (the central 70% of the screen) to InFrameOpacity, so rigs read as ghosts in the shot but stay solid in
	 * the dimmed margin around it.
	 */
	constexpr const TCHAR* RigMaterialPath = TEXT("/Game/DirectorsCut/Materials/M_DCRig.M_DCRig");

	/** Like Paint, on a specific base material (keeps recoloring the same instance once created). */
	void PaintWith(UStaticMeshComponent* Mesh, UMaterialInterface* Base, const FLinearColor& Color);

	/** Turns the capture-frame fade on or off for a rig material instance. */
	void SetFrameMask(UMaterialInterface* Material, bool bMasked);
	void SetFrameMask(UStaticMeshComponent* Mesh, bool bMasked);

	/** Editor-only floating label (hidden in game; the HUD draws its own labels). */
	void SetupLabel(UTextRenderComponent* Label, const FColor& Color, float Size);
}
