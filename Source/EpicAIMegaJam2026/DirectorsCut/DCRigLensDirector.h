// Director's Cut prototype 5 - "Rigs & Lenses": prototype 4 (Replica) plus rigs that each handle differently
// (handheld, Steadicam, fluid-head tripod, dolly with momentum, jib arm) and a lens kit on a cinema camera.
// The reference film and scoring are unchanged; framing is compared lens-for-lens, so the reference can be
// matched with different glass from a different spot - at the cost of the position score.

#pragma once

#include "CoreMinimal.h"
#include "DCReplicaDirector.h"
#include "DCRigLensDirector.generated.h"

class ADCJib;
class ADCRigCharacter;

UCLASS()
class ADCRigLensDirector : public ADCReplicaDirector
{
	GENERATED_BODY()

public:
	ADCRigLensDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Rigs", meta = (ClampMin = "0"))
	int32 MaxJibs = 2;

	// Player commands
	virtual void HandlePrimaryPressed() override;
	virtual void HandleSecondaryPressed() override;
	virtual void HandleInteract() override;
	virtual void HandleClearRigs() override;

	// HUD
	virtual FString GetBrief() const override;
	virtual FString GetInteractHint() const override;
	virtual FString GetShotTip(const FDCRefShot& Shot) const override;
	int32 GetJibCount() const { return Jibs.Num(); }

protected:
	virtual void UpdatePrepPreview() override;

private:
	ADCRigCharacter* GetRigPlayer() const;
	ADCJib* FindNearestJib(const FVector& Location, float Radius) const;

	UPROPERTY()
	TArray<TObjectPtr<ADCJib>> Jibs;

	UPROPERTY()
	TObjectPtr<ADCJib> JibGhost;

	bool bJibPreviewValid = false;
	float JibPreviewYaw = 0.f;
};
