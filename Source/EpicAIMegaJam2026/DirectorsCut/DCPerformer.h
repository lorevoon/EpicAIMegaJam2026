// Director's Cut prototype - a placeholder actor that plays back authored keyframes on the scene clock.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCTypes.h"
#include "DCPerformer.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;

UCLASS()
class ADCPerformer : public AActor
{
	GENERATED_BODY()

public:
	ADCPerformer();

	void Setup(const FLinearColor& BodyColor, const FLinearColor& HeadColor, bool bWithCape, bool bWithProp, TArray<FDCKey> InKeys);

	/** Poses the performer deterministically for the given scene time. */
	void Evaluate(float SceneTime);

	USceneComponent* GetHead() const { return Head; }
	USceneComponent* GetProp() const { return Prop; }

private:
	UStaticMeshComponent* MakePart(FName Name, UStaticMesh* Mesh, USceneComponent* Parent);
	void Paint(UStaticMeshComponent* Part, const FLinearColor& Color);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Cape;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Head;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Nose;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Prop;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	TArray<FDCKey> Keys;
};
