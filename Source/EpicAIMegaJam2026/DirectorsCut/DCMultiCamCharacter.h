// Director's Cut - the director on foot. Walks the set to place cameras; while looking through a
// camera (always, during a take) mouse look re-aims that camera instead of the walking view.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "DCMultiCamCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class ADCMultiCamDirector;

UCLASS()
class ADCMultiCamCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ADCMultiCamCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	float WalkSpeed = 450.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	float SprintSpeed = 700.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	float LookSensitivity = 1.f;

	UCameraComponent* GetCamera() const { return Camera; }
	void ResetToStart(const FTransform& Start);

private:
#if WITH_EDITOR
	bool bRestoreEditorEscape = false;
#endif
	ADCMultiCamDirector* GetDirector();
	UInputAction* MakeAction(EInputActionValueType Type, bool bWhenPaused = false);
	bool IsLockedToCamera();

	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnWheel(const FInputActionValue& Value);
	void OnSprintPressed() { bSprintHeld = true; }
	void OnSprintReleased() { bSprintHeld = false; }
	void OnPrimary();
	void OnPrimaryReleased();
	void OnSecondary();
	void OnCameraCancel();
	void OnCameraInteract();
	void OnStartTake();
	void OnRestart();
	void OnClearCameras();
	void OnSelectCamera(int32 Number);
	void OnScrub(const FInputActionValue& Value);
	void OnCycleTool();
	void OnFinishRail();
	void OnCycleCamera(int32 Direction);
	void OnTransportToggle();
	void OnRehearsalToggle();
	void OnRehearsalSpeed(int32 Direction);
	void OnRailSpeed(int32 Direction);
	void OnTogglePause();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> Mapping;

	UPROPERTY()
	TArray<TObjectPtr<UInputAction>> Actions;

	TWeakObjectPtr<ADCMultiCamDirector> Director;
	bool bSprintHeld = false;
};
