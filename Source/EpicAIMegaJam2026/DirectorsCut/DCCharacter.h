// Director's Cut prototype - first-person camera operator. Input is built in code so no input assets are needed.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "DCCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class ADCRail;
class ADCTakeDirector;
struct FInputActionValue;

UCLASS()
class ADCCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ADCCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	float WalkSpeed = 450.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	float SprintSpeed = 700.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	float DollySpeed = 900.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	float LookSensitivity = 1.f;

	UCameraComponent* GetCamera() const { return Camera; }
	bool IsRecording() const { return bRecordHeld; }
	bool IsSprinting() const;
	bool IsOnRail() const { return MountedRail.IsValid(); }
	float GetRailProgress() const;

	void MountRail(ADCRail* Rail, float Distance);
	void DismountRail();
	void ResetToStart(const FTransform& Start);
	void ReleaseInputs();

private:
	ADCTakeDirector* GetDirector();

	UInputAction* MakeAction(EInputActionValueType Type, bool bWhenPaused = false);

	void OnMove(const FInputActionValue& Value);
	void OnMoveReleased(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnSprintPressed() { bSprintHeld = true; }
	void OnSprintReleased() { bSprintHeld = false; }
	void OnPrimaryPressed();
	void OnPrimaryReleased() { bRecordHeld = false; }
	void OnSecondaryPressed();
	void OnInteract();
	void OnStartTake();
	void OnRestart();
	void OnClearRail();
	void OnToggleShotList();
	void OnTogglePause();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> Mapping;

	UPROPERTY()
	TArray<TObjectPtr<UInputAction>> Actions;

	TWeakObjectPtr<ADCTakeDirector> Director;
	TWeakObjectPtr<ADCRail> MountedRail;
	float RailDistance = 0.f;
	float RailInput = 0.f;

	bool bRecordHeld = false;
	bool bSprintHeld = false;
};
