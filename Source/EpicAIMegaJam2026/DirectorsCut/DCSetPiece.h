// Director's Cut prototype - a colored blockout mesh for set dressing. Move, scale, and duplicate it in the editor.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCSetPiece.generated.h"

class UStaticMeshComponent;

UCLASS()
class ADCSetPiece : public AActor
{
	GENERATED_BODY()

public:
	ADCSetPiece();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, Category = "Set Piece")
	FLinearColor Color = FLinearColor(0.5f, 0.5f, 0.5f);

	/** Turn off to keep whatever material you assign to the mesh yourself. */
	UPROPERTY(EditAnywhere, Category = "Set Piece")
	bool bApplyColor = true;

	UStaticMeshComponent* GetMesh() const { return Mesh; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Set Piece")
	TObjectPtr<UStaticMeshComponent> Mesh;
};
