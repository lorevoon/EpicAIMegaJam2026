// Director's Cut - multi-camera take director.
// The player places static, dolly, and crane cameras during preparation. During the take they are locked to the
// camera feeds, cut between them (1-9, [ and ]), and can re-aim, zoom, and drive the live camera's rig.
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
class ADCRail;
class APlayerController;

/** Which kind of camera the next placement builds. */
UENUM()
enum class EDCCamTool : uint8
{
	Static,
	Dolly,
	Crane
};

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

	/** How many cameras the player may place; 0 = unlimited. Keys 1-9 pick the first nine, [ and ] step through all. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cameras", meta = (ClampMin = "0"))
	int32 MaxCameras = 4;

	/** Lets the player build dolly (rail) and crane cameras as well as static ones. T cycles the camera type. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cameras")
	bool bAllowMovingRigs = false;

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
	virtual void HandleInteract();
	virtual void HandleStartTake();
	virtual void HandleRestart();
	/** W/S while looking through a dolly or crane camera: travel along the rail / raise and lower the boom. */
	void HandleRigMove(float Axis, float DeltaSeconds);
	void HandleCycleTool();
	void HandleFinishRail();
	void HandleCycleCamera(int32 Direction);
	/** Scrub the scene clock while planning (overridden by the zombie director). */
	virtual void HandleScrub(float DeltaSeconds) {}
	/** Re-watch the rehearsal (overridden by the zombie director). */
	virtual void HandleRehearsal() {}
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
	int32 GetSlotCount() const { return Cameras.Num(); }
	bool IsUnlimited() const { return MaxCameras <= 0; }
	EDCCamTool GetTool() const { return Tool; }
	int32 GetPendingRailPoints() const;
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

protected:
	void GatherScene();
	void BeginFilming();
	virtual void EvaluateBeats(float PrevTime, float NowTime);
	bool IsFramed(const FDCMultiCamBeat& State, const ADCMultiCam* Cam) const;
	virtual void EndTake();
	virtual void ResetScene();
	void UpdatePrepPreview();
	void ViewCamera(int32 Slot);
	void ViewWalking();
	void SetLabelsVisible(bool bVisible);
	void Flash(const FString& Text, float Seconds = 2.5f);
	FCollisionQueryParams MakeTraceParams(const FName& Tag) const;

	ADCMultiCamCharacter* GetPlayer() const;
	APlayerController* GetPC() const;

	/** First empty camera slot; grows the list when cameras are unlimited. INDEX_NONE when the budget is spent. */
	int32 FindFreeSlot();
	bool HasCameraBudget() const;
	ADCMultiCam* SpawnCamera(const FVector& Location);
	void FinishPendingRail(bool bQuiet);

	/** Rail being laid for the next dolly camera. */
	UPROPERTY()
	TObjectPtr<ADCRail> PendingRail;

	EDCCamTool Tool = EDCCamTool::Static;
	bool bPreviewFloor = false;

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
