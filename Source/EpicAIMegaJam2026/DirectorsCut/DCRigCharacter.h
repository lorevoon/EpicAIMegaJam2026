// Director's Cut prototype 5 - "Rigs & Lenses" camera operator.
//
// The camera is a cinema camera (full-frame sensor, real focal lengths, depth of field, autofocus), and every rig
// handles differently:
//   Handheld   free and fast, but breathes and shakes - more when walking, much more when sprinting
//   Steadicam  floats: the camera lags behind your body and banks into turns; slower walk, no shake
//   Tripod     fluid head - pans and tilts are damped and speed-limited, so they come out silky
//   Dolly      the rail now has momentum: W/S push the dolly, it coasts, Space brakes, the track rumbles
//   Jib        W/S boom the arm up and down, A/D swing it; the arm is heavy, so it overshoots if you're late
// Lenses: Q / Z swap through the kit (primes take time to change mid-take, during which nothing usable is
// recorded); the zoom lens zooms with the mouse wheel.

#pragma once

#include "CoreMinimal.h"
#include "DCFreeCharacter.h"
#include "DCRigCharacter.generated.h"

class ADCJib;
class UCineCameraComponent;

UENUM()
enum class EDCBodyRig : uint8
{
	Handheld,
	Steadicam
};

/** One lens in the camera kit. A prime has MinFocal == MaxFocal. */
struct FDCLens
{
	FString Name;
	float MinFocal = 24.f;
	float MaxFocal = 24.f;
	float FStop = 2.f;

	bool IsZoom() const { return MaxFocal > MinFocal; }
};

UCLASS()
class ADCRigCharacter : public ADCFreeCharacter
{
	GENERATED_BODY()

public:
	ADCRigCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** How long swapping a lens blinds the camera during a take. Free during preparation. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Lenses", meta = (Units = "s"))
	float LensChangeTime = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Rigs")
	float SteadicamWalkSpeed = 320.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Rigs")
	float DollyMaxSpeed = 450.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Rigs")
	float DollyPush = 380.f;

	// ADCFreeCharacter
	virtual bool IsOnRig() const override { return Super::IsOnRig() || MountedJib.IsValid(); }
	virtual float GetCameraFOV() const override;
	virtual bool IsViewObstructed() const override { return LensChangeRemaining > 0.f; }
	virtual void Dismount() override;
	virtual void ReleaseInputs() override;

	void MountJib(ADCJib* Jib);
	ADCJib* GetMountedJib() const { return MountedJib.Get(); }

	// HUD
	FString GetRigName() const;
	EDCBodyRig GetBodyRig() const { return BodyRig; }
	const FDCLens& GetLens() const { return Lenses[LensIndex]; }
	int32 GetLensIndex() const { return LensIndex; }
	int32 GetLensCount() const { return Lenses.Num(); }
	const FDCLens& GetLensAt(int32 Index) const { return Lenses[Index]; }
	float GetFocalLength() const { return FocalLength; }
	float GetFocusDistance() const { return FocusDistance; }
	float GetLensChangeRemaining() const { return LensChangeRemaining; }
	/** Signed dolly speed as a fraction of its maximum. */
	float GetDollySpeed01() const { return DollyMaxSpeed > 0.f ? DollyVelocity / DollyMaxSpeed : 0.f; }
	/** Recent camera shake, 0 (rock steady) to 1 (very shaky). */
	float GetShake01() const { return FMath::Clamp(ShakeLevel, 0.f, 1.f); }

	/** Horizontal FOV of a focal length on this camera's sensor. */
	static float FocalToFov(float FocalMm);
	static float FovToFocal(float FovDeg);

protected:
	virtual void TickRig(float DeltaSeconds) override;
	virtual void OnMove(const FInputActionValue& Value) override;
	virtual void OnMoveReleased(const FInputActionValue& Value) override;
	virtual void OnLook(const FInputActionValue& Value) override;
	virtual void OnHeight(const FInputActionValue& Value) override;

private:
	void TickLens(float DeltaSeconds);
	void TickCameraFeel(float DeltaSeconds);
	void SelectLens(int32 NewIndex);
	bool IsFilming();

	void OnNextLens() { SelectLens((LensIndex + 1) % Lenses.Num()); }
	void OnPrevLens() { SelectLens((LensIndex + Lenses.Num() - 1) % Lenses.Num()); }
	void OnToggleBodyRig();
	void OnJibTool();
	/** Space: skips the reference film / replay when one is playing, otherwise brakes the dolly. */
	void OnBrakePressed();
	void OnBrakeReleased() { bBraking = false; }

	UCineCameraComponent* GetCine() const;

	TWeakObjectPtr<ADCJib> MountedJib;

	TArray<FDCLens> Lenses;
	int32 LensIndex = 1;
	int32 PendingLensIndex = INDEX_NONE;
	float LensChangeRemaining = 0.f;
	float TargetFocal = 24.f;
	float FocalLength = 24.f;
	float FocusDistance = 500.f;

	EDCBodyRig BodyRig = EDCBodyRig::Handheld;

	// Camera feel
	FQuat SmoothRot = FQuat::Identity;
	FVector SmoothLoc = FVector::ZeroVector;
	bool bFeelInitialized = false;
	float FeelTime = 0.f;
	float WalkPhase = 0.f;
	float Roll = 0.f;
	float ShakeLevel = 0.f;
	float Jolt = 0.f;

	// Dolly
	float DollyVelocity = 0.f;
	bool bBraking = false;

	// Jib
	FVector2D JibInput = FVector2D::ZeroVector;
	float JibYawVelocity = 0.f;
	float JibPitchVelocity = 0.f;
};
