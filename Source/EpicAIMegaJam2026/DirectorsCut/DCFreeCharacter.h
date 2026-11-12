// Director's Cut prototype 2 - camera operator with free rigs: handheld, tripods, and rails on any surface.
// The camera is always rolling during a take, so there is no record button.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "DCFreeCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class ADCFreeDirector;
class ADCRail;
class ADCTripod;

UCLASS()
class ADCFreeCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ADCFreeCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	float WalkSpeed = 450.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	float SprintSpeed = 700.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	float RailSpeed = 700.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	float LookSensitivity = 1.f;

	UCameraComponent* GetCamera() const { return Camera; }
	virtual bool IsOnRig() const { return MountedRail.IsValid() || MountedTripod.IsValid(); }
	ADCRail* GetMountedRail() const { return MountedRail.Get(); }
	/** Horizontal field of view the camera is filming with (prototype 5 lenses change it). */
	virtual float GetCameraFOV() const;
	/** True while the image is unusable, e.g. during a lens change. Such samples score zero. */
	virtual bool IsViewObstructed() const { return false; }
	ADCTripod* GetMountedTripod() const { return MountedTripod.Get(); }
	float GetRailProgress() const;

	void MountRail(ADCRail* Rail, float Distance);
	void MountTripod(ADCTripod* Tripod);
	virtual void Dismount();
	void ResetToStart(const FTransform& Start);
	virtual void ReleaseInputs();

protected:
	ADCFreeDirector* GetDirector();
	UInputAction* MakeAction(EInputActionValueType Type, bool bWhenPaused = false);
	void PlaceCameraAt(const FVector& CameraLocation);
	void BeginRig();
	/** Moves the camera with whichever rig is mounted. */
	virtual void TickRig(float DeltaSeconds);

	virtual void OnMove(const FInputActionValue& Value);
	virtual void OnMoveReleased(const FInputActionValue& Value) { RailInput = 0.f; }
	virtual void OnLook(const FInputActionValue& Value);
	virtual void OnHeight(const FInputActionValue& Value);
	void OnSprintPressed() { bSprintHeld = true; }
	void OnSprintReleased() { bSprintHeld = false; }
	void OnPrimary();
	void OnSecondary();
	void OnInteract();
	void OnStartTake();
	void OnRestart();
	void OnClearRigs();
	void OnFinishRail();
	void OnRailTool();
	void OnTripodTool();
	void OnSprayTool();
	void OnWatchReference();
	void OnWatchTake();
	void OnToggleNotebook();
	void OnTogglePause();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> Mapping;

	UPROPERTY()
	TArray<TObjectPtr<UInputAction>> Actions;

	TWeakObjectPtr<ADCFreeDirector> Director;
	TWeakObjectPtr<ADCRail> MountedRail;
	TWeakObjectPtr<ADCTripod> MountedTripod;
	float RailDistance = 0.f;
	float RailInput = 0.f;
	bool bSprintHeld = false;
	/** Camera position relative to the capsule centre (eye height). */
	FVector CameraOffset = FVector(0.f, 0.f, 64.f);
};
