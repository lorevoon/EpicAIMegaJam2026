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
	ADCMultiCamDirector* GetDirector();
	UInputAction* MakeAction(EInputActionValueType Type, bool bWhenPaused = false);
	bool IsLockedToCamera();

	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnWheel(const FInputActionValue& Value);
	void OnSprintPressed() { bSprintHeld = true; }
	void OnSprintReleased() { bSprintHeld = false; }
	void OnPrimary();
	void OnSecondary();
	void OnInteract();
	void OnStartTake();
	void OnRestart();
	void OnClearCameras();
	void OnSelectCamera(int32 Number);
	void OnScrub(const FInputActionValue& Value);
	void OnRehearsal();
	void OnCycleTool();
	void OnFinishRail();
	void OnCycleCamera(int32 Direction);
	void OnToggleNotebook();
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
