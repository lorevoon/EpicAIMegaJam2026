// Director's Cut - a camera the player places on a stand, wall, or ceiling (static), on a rail
// (dolly), or on a crane. Its mount never moves after placement; aim, zoom, and the dolly/crane travel can.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCMultiCam.generated.h"

class ADCRail;
class UMaterialInterface;
class UCameraComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** How a camera is mounted. Static cameras never move; dollies ride a rail; cranes raise and lower a boom. */
UENUM()
enum class EDCCamRig : uint8
{
	Static,
	Dolly,
	Crane
};

UCLASS()
class ADCMultiCam : public AActor
{
	GENERATED_BODY()

public:
	ADCMultiCam();

	static constexpr float MinFOV = 20.f;
	static constexpr float MaxFOV = 90.f;
	static constexpr float DefaultFOV = 75.f;
	static constexpr float DollySpeed = 600.f;
	static constexpr float CraneSpeed = 250.f;
	static constexpr float CraneMinHeight = 40.f;
	static constexpr float CraneMaxHeight = 800.f;

	/** Static camera. Number shown on the camera; 0 for the placement ghost. Surface is where the mount is bolted. */
	void Setup(int32 InNumber, const FVector& Surface);

	/** Dolly camera riding a rail (the rail's points are camera positions). The camera owns the rail. */
	void SetupDolly(int32 InNumber, ADCRail* InRail);

	/** Crane camera standing on Floor with its lens Height above it. */
	void SetupCrane(int32 InNumber, const FVector& Floor, float Height);

	EDCCamRig GetRig() const { return Rig; }
	ADCRail* GetRail() const { return Rail; }
	float GetCraneHeight() const { return CraneHeight; }
	float GetRailProgress() const;

	/** W/S input: travel along the rail or raise/lower the crane. Static cameras ignore it. */
	void MoveRig(float Axis, float DeltaSeconds);

	int32 GetNumber() const { return Number; }
	UCameraComponent* GetCamera() const { return Camera; }

	FRotator GetAim() const { return Aim; }
	void SetAim(const FRotator& NewAim);
	void AddAim(float DeltaYaw, float DeltaPitch) { SetAim(Aim + FRotator(DeltaPitch, DeltaYaw, 0.f)); }

	/** Current (eased) field of view. */
	float GetFOV() const;
	/** Where the zoom is heading; the lens eases toward it every frame. */
	float GetTargetFOV() const { return TargetFOV; }
	void SetFOV(float NewFOV);
	void UpdateZoom(float DeltaSeconds);

	float GetRailSpeedScale() const;
	void AdjustRailSpeed(int32 Direction);

	/** The aim and zoom set during preparation; every take starts from them. */
	void SaveHome();
	void RestoreHome();

	/** Red tally light when this camera is the live feed. */
	void SetLive(bool bLive);

	/**
	 * While shooting, fades the rig (camera, mount, and a dolly's rail) to a ghost wherever it falls inside the
	 * capture frame; it stays solid in the dimmed margin. Also hides the "CAM n" label.
	 */
	void SetFrameMasked(bool bMasked);

	/** Tints the whole camera (used for the placement ghost). */
	void SetTint(const FLinearColor& Color);

	/** A dolly's rail goes away with its camera. */
	virtual void Destroyed() override;

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

	UPROPERTY()
	TObjectPtr<ADCRail> Rail;

	/** Rig material (falls back to the basic shape material if the asset is missing). */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> RigMaterial;

	void UpdateMount(const FVector& Surface);
	void ApplyRigPosition();
	void SetNumberLabel();

	int32 Number = 0;
	EDCCamRig Rig = EDCCamRig::Static;
	float RailDistance = 0.f;
	float HomeRailDistance = 0.f;
	FVector CraneBase = FVector::ZeroVector;
	float CraneHeight = 170.f;
	float HomeCraneHeight = 170.f;
	FRotator Aim = FRotator::ZeroRotator;
	FRotator HomeAim = FRotator::ZeroRotator;
	float HomeFOV = DefaultFOV;
	float TargetFOV = DefaultFOV;
	int32 RailSpeedIndex = 2;
};
