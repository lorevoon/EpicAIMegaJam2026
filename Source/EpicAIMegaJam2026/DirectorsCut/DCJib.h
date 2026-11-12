// Director's Cut prototype 5 - a jib arm the player places during preparation.
// The camera hangs off the end of a counterweighted arm: W/S booms it up and down, A/D swings it around the post.
// The arm has weight, so moves have to be started early and caught before they overshoot.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCJib.generated.h"

class UStaticMeshComponent;

UCLASS()
class ADCJib : public AActor
{
	GENERATED_BODY()

public:
	ADCJib();

	static constexpr float PivotHeight = 170.f;
	static constexpr float ArmLength = 420.f;
	static constexpr float MinPitch = -30.f;
	static constexpr float MaxPitch = 60.f;

	/** Arm yaw is in world degrees; pitch is the arm's elevation above horizontal. */
	void SetArm(float InYaw, float InPitch);
	float GetArmYaw() const { return ArmYaw; }
	float GetArmPitch() const { return ArmPitch; }

	FVector GetPivotLocation() const { return GetActorLocation() + FVector(0.f, 0.f, PivotHeight); }
	/** Where the camera sits at the end of the arm. */
	FVector GetCameraLocation() const;

	/** Tints the jib (used for the placement ghost). */
	void SetTint(const FLinearColor& Color);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Base;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Post;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> ArmPivot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Arm;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Counterweight;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Head;

	float ArmYaw = 0.f;
	float ArmPitch = 10.f;
};
