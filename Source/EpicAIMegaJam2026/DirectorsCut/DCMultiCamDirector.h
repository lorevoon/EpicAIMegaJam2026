// Director's Cut prototype 3 - multi-camera take director.
// The player places a fixed number of cameras during preparation. During the take they are locked to the
// camera feeds, cut between them with 1-4, and can only re-aim (and zoom) the live camera.
// Only the live feed captures hidden beats (ADCBeat), so choosing when to cut is the game.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCTypes.h"
#include "DCMultiCamDirector.generated.h"

class ADCBeat;
class ADCClapperboard;
class ADCMultiCam;
class ADCPerformer;
class ADCMultiCamCharacter;
class APlayerController;

/** Runtime state of one hidden beat. */
struct FDCMultiCamBeat
{
	ADCBeat* Beat = nullptr;
	TArray<FDCAnchor> Anchors;
	EDCShotStatus Status = EDCShotStatus::Pending;
	float Hold = 0.f;
	/** Scene time the gauge last moved, for HUD fade-out. */
	float LastProgressTime = -100.f;
	/** Camera number that captured it. */
	int32 CapturedBy = 0;
	// Knowledge that survives retries (the player's notebook).
	bool bSpotted = false;
	bool bEverCaptured = false;
};

UCLASS()
class ADCMultiCamDirector : public AActor
{
	GENERATED_BODY()

public:
	ADCMultiCamDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Directors Cut", meta = (ClampMin = "1", Units = "s"))
	float TakeDuration = 60.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut", meta = (ClampMin = "0", Units = "s"))
	float CountdownDuration = 3.f;

	/** How many cameras the player may place (1-9 are selectable with the number keys). */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cameras", meta = (ClampMin = "1", ClampMax = "9"))
	int32 MaxCameras = 4;

	/** How far from a wall or ceiling a mounted camera sits. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cameras", meta = (Units = "cm"))
	float WallStandoff = 35.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cameras", meta = (Units = "cm"))
	float MinStandHeight = 40.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cameras", meta = (Units = "cm"))
	float MaxStandHeight = 320.f;

	/** Zooming in lets a camera capture subjects proportionally farther than a beat's Max Distance. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cameras")
	bool bZoomExtendsRange = true;

	// Player commands. Look/zoom return true when they were applied to a camera instead of the walking view.
	void HandlePrimaryPressed();
	void HandleSecondaryPressed();
	void HandleInteract();
	void HandleStartTake();
	void HandleRestart();
	void HandleClearCameras();
	void HandleSelectCamera(int32 Number);
	bool HandleLook(float DeltaYaw, float DeltaPitch);
	void HandleWheel(float Delta);

	// Read-only state for the HUD and character
	EDCPhase GetPhase() const { return Phase; }
	float GetSceneTime() const { return SceneTime; }
	float GetCountdownRemaining() const { return CountdownRemaining; }
	const TArray<FDCMultiCamBeat>& GetBeats() const { return Beats; }
	ADCMultiCam* GetCameraSlot(int32 Index) const { return Cameras.IsValidIndex(Index) ? Cameras[Index].Get() : nullptr; }
	int32 GetCameraCount() const;
	/** 0-based slot being looked through, or INDEX_NONE while walking. */
	int32 GetViewingSlot() const { return ViewingSlot; }
	bool IsViewingCamera() const { return ViewingSlot != INDEX_NONE; }
	float GetPendingHeight() const { return PendingHeight; }
	bool WasApproved() const { return bApproved; }
	int32 GetScore() const { return Score; }
	int32 GetBestScore() const { return BestScore; }
	int32 GetCutCount() const { return CutCount; }
	FString GetCutName() const;
	FString GetMessage() const;
	FString GetBrief() const;
	const FString& GetSetupProblem() const { return SetupProblem; }
	ADCClapperboard* GetClapperboard() const { return Clapperboard; }
	bool IsNearClapper() const;
	int32 GetSpottedCount() const;
	bool IsLiveFeedRolling() const;

	bool bShowNotebook = true;

private:
	void GatherScene();
	void BeginFilming();
	void EvaluateBeats(float PrevTime, float NowTime);
	bool IsFramed(const FDCMultiCamBeat& State, const ADCMultiCam* Cam) const;
	void EndTake();
	void ResetScene();
	void UpdatePrepPreview();
	void ViewCamera(int32 Slot);
	void ViewWalking();
	void SetLabelsVisible(bool bVisible);
	void Flash(const FString& Text, float Seconds = 2.5f);
	FCollisionQueryParams MakeTraceParams(const FName& Tag) const;

	ADCMultiCamCharacter* GetPlayer() const;
	APlayerController* GetPC() const;

	UPROPERTY()
	TArray<TObjectPtr<ADCPerformer>> Performers;

	UPROPERTY()
	TObjectPtr<ADCClapperboard> Clapperboard;

	/** One entry per slot; empty slots are null. */
	UPROPERTY()
	TArray<TObjectPtr<ADCMultiCam>> Cameras;

	UPROPERTY()
	TObjectPtr<ADCMultiCam> Ghost;

	TArray<FDCMultiCamBeat> Beats;
	FString SetupProblem;

	EDCPhase Phase = EDCPhase::Preparation;
	int32 ViewingSlot = INDEX_NONE;
	float PendingHeight = 170.f;
	float SceneTime = 0.f;
	float CountdownRemaining = 0.f;
	bool bApproved = false;
	int32 Score = 0;
	int32 BestScore = 0;
	int32 CutCount = 0;

	FTransform StartTransform;

	FVector PreviewPoint = FVector::ZeroVector;
	FVector PreviewSurface = FVector::ZeroVector;
	bool bPreviewHit = false;
	bool bPreviewValid = false;

	FString Message;
	float MessageExpireTime = 0.f;
};
