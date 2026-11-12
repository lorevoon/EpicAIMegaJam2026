// Director's Cut prototype - owns the set, the single scene clock, the shot list, and the take phases.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCTypes.h"
#include "DCTakeDirector.generated.h"

class ADCCharacter;
class ADCPerformer;
class ADCRail;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS()
class ADCTakeDirector : public AActor
{
	GENERATED_BODY()

public:
	ADCTakeDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Spawn the moonlight, sky and practical lamps. Turn off when dropping the set into a lit level. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	bool bSpawnLighting = true;

	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	float TakeDuration = 45.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	float CountdownDuration = 3.f;

	// Player commands
	void HandlePrimaryPressed();
	void HandleSecondaryPressed();
	void HandleInteract();
	void HandleStartTake();
	void HandleRestart();
	void HandleClearRail();

	// Read-only state for the HUD
	EDCPhase GetPhase() const { return Phase; }
	float GetSceneTime() const { return SceneTime; }
	float GetCountdownRemaining() const { return CountdownRemaining; }
	const TArray<FDCShot>& GetShots() const { return Shots; }
	int32 GetFocusShotIndex() const;
	bool WasApproved() const { return bApproved; }
	int32 GetScore() const { return Score; }
	int32 GetBestScore() const { return BestScore; }
	int32 GetCapturedBonusCount() const;
	FString GetCutName() const;
	FString GetMessage() const;
	ADCRail* GetRail() const { return Rail; }
	FVector GetClapperLocation() const { return ClapperLocation; }
	FVector GetStartLocation() const { return StartTransform.GetLocation(); }
	bool IsNearClapper() const;
	bool CanMountRail() const;

	bool bShowShotList = true;

	const FTransform& GetStartTransform() const { return StartTransform; }

	static FString DescribeFail(EDCFail Fail);

private:
	void BuildSet();
	void BuildLighting();
	void SpawnCast();
	void DefineShots();

	UStaticMeshComponent* AddBlock(const FVector& Center, const FVector& Size, const FLinearColor& Color, bool bCollide = true);
	UStaticMeshComponent* AddDisc(const FVector& Center, float Radius, const FLinearColor& Color);
	UMaterialInstanceDynamic* MakeMaterial(const FLinearColor& Color);

	void BeginFilming();
	void EvaluateShots(float PrevTime, float NowTime);
	EDCFail CheckShot(const FDCShot& Shot, bool bInWindow) const;
	void EndTake();
	void ResetScene();
	void UpdatePrepPreview();
	void RefreshMarkerColors();
	void Flash(const FString& Text, float Seconds = 2.5f);

	ADCCharacter* GetPlayer() const;

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY()
	TObjectPtr<ADCPerformer> Dracula;

	UPROPERTY()
	TObjectPtr<ADCPerformer> Victim;

	UPROPERTY()
	TObjectPtr<ADCRail> Rail;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> MarkerMaterials;

	TArray<FDCShot> Shots;

	EDCPhase Phase = EDCPhase::Preparation;
	float SceneTime = 0.f;
	float CountdownRemaining = 0.f;
	bool bApproved = false;
	int32 Score = 0;
	int32 BestScore = 0;

	FTransform StartTransform;
	FVector ClapperLocation;

	FVector PreviewPoint = FVector::ZeroVector;
	bool bPreviewHit = false;
	bool bPreviewValid = false;

	FString Message;
	float MessageExpireTime = 0.f;
};
