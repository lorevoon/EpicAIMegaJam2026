// Director's Cut prototype - owns the single scene clock, the shot list, and the take phases.
// The scene itself (set pieces, performers, waypoints, shot markers, start mark, clapperboard) is placed in the level.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCTypes.h"
#include "DCTakeDirector.generated.h"

class ADCCharacter;
class ADCClapperboard;
class ADCPerformer;
class ADCRail;
class ADCStartMark;

UCLASS()
class ADCTakeDirector : public AActor
{
	GENERATED_BODY()

public:
	ADCTakeDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Directors Cut", meta = (ClampMin = "1", Units = "s"))
	float TakeDuration = 45.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut", meta = (ClampMin = "0", Units = "s"))
	float CountdownDuration = 3.f;

#if WITH_EDITORONLY_DATA
	/** Tick to place the default Dracula scene into this level (same as the Build Default Scene button). */
	UPROPERTY(EditAnywhere, Transient, Category = "Directors Cut|Editor")
	bool bBuildDefaultScene = false;
#endif

#if WITH_EDITOR
	/** Places the default "Midnight Visit" set, cast, blocking, shots, and lights into this level as editable actors. */
	UFUNCTION(CallInEditor, Category = "Directors Cut|Editor")
	void BuildDefaultScene();

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

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
	const FString& GetSetupProblem() const { return SetupProblem; }
	ADCRail* GetRail() const { return Rail; }
	ADCClapperboard* GetClapperboard() const { return Clapperboard; }
	FVector GetStartLocation() const { return StartTransform.GetLocation(); }
	bool IsNearClapper() const;
	bool CanMountRail() const;

	bool bShowShotList = true;

	/** Where the player starts every take: the level's ADCStartMark, or a default spot. */
	static FTransform FindStartTransform(UWorld* World);

	static FString DescribeFail(EDCFail Fail);

private:
	void GatherScene();
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
	TArray<TObjectPtr<ADCPerformer>> Performers;

	UPROPERTY()
	TObjectPtr<ADCClapperboard> Clapperboard;

	UPROPERTY()
	TObjectPtr<ADCRail> Rail;

	TArray<FDCShot> Shots;
	FString SetupProblem;

	EDCPhase Phase = EDCPhase::Preparation;
	float SceneTime = 0.f;
	float CountdownRemaining = 0.f;
	bool bApproved = false;
	int32 Score = 0;
	int32 BestScore = 0;

	FTransform StartTransform;

	FVector PreviewPoint = FVector::ZeroVector;
	bool bPreviewHit = false;
	bool bPreviewValid = false;

	FString Message;
	float MessageExpireTime = 0.f;
};
