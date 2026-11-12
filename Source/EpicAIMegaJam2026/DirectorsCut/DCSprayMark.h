// Director's Cut prototype 4 - a numbered spray-paint mark the player leaves on any surface to plan their route.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCSprayMark.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class ADCSprayMark : public AActor
{
	GENERATED_BODY()

public:
	ADCSprayMark();

	/** Paints the mark flat on a surface with the given outward normal. */
	void Setup(int32 InNumber, const FVector& Surface, const FVector& Normal);
	void SetTint(const FLinearColor& Color);
	int32 GetNumber() const { return Number; }

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Splat;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> Digits;

	int32 Number = 0;
};
