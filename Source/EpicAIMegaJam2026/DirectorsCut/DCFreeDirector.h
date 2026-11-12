// Director's Cut prototype 2 - free-rig, discovery-driven take director.
// Beats (ADCBeat) are hidden: they capture whenever the camera frames them during their window, from anywhere.
// The player rigs the set with free rails (floor/wall/ceiling) and height-adjustable tripods.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCTypes.h"
#include "DCFreeDirector.generated.h"

class ADCBeat;
class ADCClapperboard;
class ADCFreeCharacter;
class ADCPerformer;
class ADCRail;
class ADCTripod;

UENUM()
enum class EDCRigTool : uint8
{
	Rail,
	Tripod
};

/** Runtime state of one hidden beat. */
struct FDCBeatState
{
	ADCBeat* Beat = nullptr;
	TArray<FDCAnchor> Anchors;
	EDCShotStatus Status = EDCShotStatus::Pending;
	float Hold = 0.f;
	/** Scene time the gauge last moved, for HUD fade-out. */
	float LastProgressTime = -100.f;
	// Knowledge that survives retries (the player's notebook).
	bool bSpotted = false;
	bool bEverCaptured = false;
};

UCLASS()
class ADCFreeDirector : public AActor
{
	GENERATED_BODY()

public:
	ADCFreeDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Directors Cut", meta = (ClampMin = "1", Units = "s"))
	float TakeDuration = 60.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut", meta = (ClampMin = "0", Units = "s"))
	float CountdownDuration = 3.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Rigs", meta = (ClampMin = "0"))
	int32 MaxRails = 3;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Rigs", meta = (ClampMin = "2"))
	int32 MaxPointsPerRail = 12;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Rigs", meta = (ClampMin = "0"))
	int32 MaxTripods = 4;

	/** How far from a surface a wall/ceiling rail holds the camera. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Rigs", meta = (Units = "cm"))
	float RailStandoff = 45.f;

#if WITH_EDITORONLY_DATA
	/** Tick to place the default castle scene into this level (same as the Build Castle Scene button). */
	UPROPERTY(EditAnywhere, Transient, Category = "Directors Cut|Editor")
	bool bBuildCastleScene = false;
#endif

#if WITH_EDITOR
	/** Places the default multi-room castle: set, cast, blocking, hidden beats, lights, start mark, clapperboard. */
	UFUNCTION(CallInEditor, Category = "Directors Cut|Editor")
	void BuildCastleScene();

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// Player commands
	void HandlePrimaryPressed();
	void HandleSecondaryPressed();
	void HandleInteract();
	void HandleStartTake();
	void HandleRestart();
	void HandleClearRigs();
	void HandleFinishRail();
	void HandleSelectTool(EDCRigTool Tool);
	void HandleHeightInput(float Delta);

	// Read-only state for the HUD
	EDCPhase GetPhase() const { return Phase; }
	float GetSceneTime() const { return SceneTime; }
	float GetCountdownRemaining() const { return CountdownRemaining; }
	const TArray<FDCBeatState>& GetBeats() const { return Beats; }
	EDCRigTool GetTool() const { return Tool; }
	int32 GetRailsUsed() const;
	int32 GetActiveRailPoints() const;
	int32 GetTripodCount() const { return Tripods.Num(); }
	float GetPendingTripodHeight() const { return PendingTripodHeight; }
	bool WasApproved() const { return bApproved; }
	int32 GetScore() const { return Score; }
	int32 GetBestScore() const { return BestScore; }
	FString GetCutName() const;
	FString GetMessage() const;
	FString GetBrief() const;
	const FString& GetSetupProblem() const { return SetupProblem; }
	ADCClapperboard* GetClapperboard() const { return Clapperboard; }
	FVector GetStartLocation() const { return StartTransform.GetLocation(); }
	bool IsNearClapper() const;
	FString GetInteractHint() const;
	int32 GetSpottedCount() const;
	bool IsAnyBeatRolling() const;

	bool bShowNotebook = true;

private:
	void GatherScene();
	void BeginFilming();
	void EvaluateBeats(float PrevTime, float NowTime);
	bool IsFramed(const FDCBeatState& State) const;
	void EndTake();
	void ResetScene();
	void UpdatePrepPreview();
	void Flash(const FString& Text, float Seconds = 2.5f);
	ADCRail* GetOrStartActiveRail();
	ADCRail* SpawnRail();
	ADCTripod* FindNearestTripod(const FVector& Location, float Radius) const;
	ADCRail* FindNearestRail(const FVector& Location, float Radius, float& OutDistance) const;
	FCollisionQueryParams MakeTraceParams(const FName& Tag) const;

	ADCFreeCharacter* GetPlayer() const;

	UPROPERTY()
	TArray<TObjectPtr<ADCPerformer>> Performers;

	UPROPERTY()
	TObjectPtr<ADCClapperboard> Clapperboard;

	UPROPERTY()
	TArray<TObjectPtr<ADCRail>> Rails;

	UPROPERTY()
	TArray<TObjectPtr<ADCTripod>> Tripods;

	UPROPERTY()
	TObjectPtr<ADCTripod> TripodGhost;

	/** Never gets points; only shows the placement ghost for the rail tool. */
	UPROPERTY()
	TObjectPtr<ADCRail> RailGhost;

	TArray<FDCBeatState> Beats;
	FString SetupProblem;

	EDCPhase Phase = EDCPhase::Preparation;
	EDCRigTool Tool = EDCRigTool::Rail;
	int32 ActiveRail = INDEX_NONE;
	float PendingTripodHeight = 160.f;
	float SceneTime = 0.f;
	float CountdownRemaining = 0.f;
	bool bApproved = false;
	int32 Score = 0;
	int32 BestScore = 0;

	FTransform StartTransform;

	FVector PreviewPoint = FVector::ZeroVector;
	FVector PreviewSurface = FVector::ZeroVector;
	bool bPreviewHit = false;
	bool bPreviewValid = false;

	FString Message;
	float MessageExpireTime = 0.f;
};
