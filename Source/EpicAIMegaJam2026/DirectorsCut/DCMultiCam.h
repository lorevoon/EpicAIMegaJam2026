// Director's Cut prototype 3 - a fixed camera the player bolts to a floor stand, wall, or ceiling.
// Once placed it can never move; only its aim (yaw/pitch) and zoom change.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCMultiCam.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class ADCMultiCam : public AActor
{
	GENERATED_BODY()

public:
	ADCMultiCam();

	static constexpr float MinFOV = 20.f;
	static constexpr float MaxFOV = 90.f;
	static constexpr float DefaultFOV = 75.f;

	/** Number shown on the camera (1-4); 0 for the placement ghost. Surface is where the mount is bolted. */
	void Setup(int32 InNumber, const FVector& Surface);

	int32 GetNumber() const { return Number; }
	UCameraComponent* GetCamera() const { return Camera; }

	FRotator GetAim() const { return Aim; }
	void SetAim(const FRotator& NewAim);
	void AddAim(float DeltaYaw, float DeltaPitch) { SetAim(Aim + FRotator(DeltaPitch, DeltaYaw, 0.f)); }

	float GetFOV() const;
	void SetFOV(float NewFOV);

	/** The aim and zoom set during preparation; every take starts from them. */
	void SaveHome();
	void RestoreHome();

	/** Red tally light when this camera is the live feed. */
	void SetLive(bool bLive);

	/** Shows or hides the floating "CAM n" label (hidden while filming so other cameras don't see it). */
	void SetLabelVisible(bool bVisible);

	/** Tints the whole camera (used for the placement ghost). */
	void SetTint(const FLinearColor& Color);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Head;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Lens;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Tally;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mount;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> Label;

	int32 Number = 0;
	FRotator Aim = FRotator::ZeroRotator;
	FRotator HomeAim = FRotator::ZeroRotator;
	float HomeFOV = DefaultFOV;
};
