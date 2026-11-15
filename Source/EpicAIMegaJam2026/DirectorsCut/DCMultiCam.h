// Director's Cut - eye-level and elevated cameras with bounded local movement and saved presets.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCMultiCam.generated.h"

class ADCRail;
class UMaterialInterface;
class UCameraComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** How a camera is mounted. Rail rigs are retained for legacy scenes. */
UENUM()
enum class EDCCamRig : uint8
{
	Static,
	Dolly // Legacy rail rigs remain loadable.
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

	/** Static camera on a stand. Number 0 is the placement ghost; Surface anchors the mount. */
	void Setup(int32 InNumber, const FVector& Surface);
	void MoveLocal(const FVector2D& Axis, float DeltaSeconds);
	float GetMovementRadius() const { return 200.f; }
	static constexpr float StaticHeight = 170.f;
	static constexpr float MinStaticHeight = 110.f;

	/** Dolly camera riding a rail (the rail's points are camera positions). The camera owns the rail. */
	void SetupDolly(int32 InNumber, ADCRail* InRail);

	EDCCamRig GetRig() const { return Rig; }
	/** Ground-level position under the camera, where the player stands to use it. */
	FVector GetInteractionPoint() const { return FVector(GetActorLocation().X, GetActorLocation().Y, MountSurface.Z); }
	ADCRail* GetRail() const { return Rail; }
	float GetRailProgress() const;

	/** W/S input this frame: travel along the rail (eased by the stabilizer). Static cameras ignore it. */
	void MoveRig(float Axis, float DeltaSeconds);

	int32 GetNumber() const { return Number; }
	UCameraComponent* GetCamera() const { return Camera; }

	FRotator GetAim() const { return Aim; }
	/** Points the camera at NewAim right away (no stabilizer). */
	void SetAim(const FRotator& NewAim);
	/** Mouse aiming: moves the aim target; with the stabilizer on, the head glides after it. */
	void AddAim(float DeltaYaw, float DeltaPitch);

	/**
	 * Shooting stabilizer. AimSmoothing: how quickly the head catches up with the aim. DollyEase: how quickly a dolly
	 * reaches (and comes off) its travel speed. 0 for either means instant, as in preparation.
	 */
	void SetStabilizer(float InAimSmoothing, float InDollyEase);
	void UpdateStabilizer(float DeltaSeconds);

	/** Current (eased) field of view. */
	float GetFOV() const;
	/** Height above the floor at the middle of the adjustable tripod neck. */
	float GetPlacementAimHeight(float StandHeight) const;
	/** Where the zoom is heading; the lens eases toward it every frame. */
	float GetTargetFOV() const { return TargetFOV; }
	void SetFOV(float NewFOV);
	/** Sets the lens immediately, without the eased zoom. */
	void SnapFOV(float NewFOV);
	void UpdateZoom(float DeltaSeconds);

	float GetRailSpeedScale() const;
	void AdjustRailSpeed(int32 Direction);

	/** The position, aim and zoom set during preparation; every take starts from them. */
	void SaveHome();
	void RestoreHome();

	/** Red tally light when this camera is the live feed. */
	void SetLive(bool bLive);

	/**
	 * While shooting, fades the rig (camera, mount, and a dolly's rail) to a ghost wherever it falls inside the
	 * capture frame; it stays solid in the dimmed margin. Also hides the "CAM n" label.
	 */
	void SetFrameMasked(bool bMasked);

	/** Hides the whole rig (camera, mount, label, and a dolly's rail), e.g. while the recorded film plays back. */
	void SetRigHidden(bool bHide);

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

	/** Authored camera and tripod, used by static rigs. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Tripod;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> TripodNeck;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> TripodHead;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY()
	TObjectPtr<ADCRail> Rail;

	/** Rig material (falls back to the basic shape material if the asset is missing). */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> RigMaterial;

	void UpdateMount(const FVector& Surface);
	void ApplyRigPosition();
	void SetAimTarget(const FRotator& NewAim);
	void SetNumberLabel();

	FVector PlacementCenter = FVector::ZeroVector;
	FVector MountSurface = FVector::ZeroVector;
	FVector HomeLocation = FVector::ZeroVector;
	int32 Number = 0;
	EDCCamRig Rig = EDCCamRig::Static;
	float RailDistance = 0.f;
	float HomeRailDistance = 0.f;
	/** Where the camera is meant to point; the head follows it through the stabilizer. */
	FRotator Aim = FRotator::ZeroRotator;
	float AimSmoothing = 0.f;
	float DollyEase = 0.f;
	/** W/S held this frame (-1..1), consumed by UpdateStabilizer. */
	float RigInput = 0.f;
	/** Current dolly travel speed along the rail, cm/s. */
	float RailVelocity = 0.f;
	FRotator HomeAim = FRotator::ZeroRotator;
	float HomeFOV = DefaultFOV;
	float TargetFOV = DefaultFOV;
	int32 RailSpeedIndex = 2;
};
