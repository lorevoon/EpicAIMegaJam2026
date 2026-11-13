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
class ACameraActor;
class APlayerController;
class ADCCinemaReplay;

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
	/** Where in the finished film it was captured (for the review timeline). */
	float CapturedFilmTime = 0.f;
	// Knowledge that survives retries (the player's notebook).
	bool bSpotted = false;
	bool bEverCaptured = false;
};

/** One frame of the take as it went out live: replaying these re-shoots the film exactly. */
struct FDCTakeSample
{
	/** Scene time this frame was filmed at. */
	float Time = 0.f;
	/** Position in the finished film: scene time minus every second the recording was paused. */
	float FilmTime = 0.f;
	/** First frame after a recording pause: playback jump-cuts here instead of blending from the previous frame. */
	bool bJumpCut = false;
	/** Camera slot that was live; a change between samples is a cut. */
	int32 Slot = INDEX_NONE;
	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	float FOV = 75.f;
	/** Something that ruins the shot (a zombie) was on screen. */
	bool bSpoiled = false;
};

UCLASS()
class ADCMultiCamDirector : public AActor
{
	GENERATED_BODY()
	friend class FDCDualSceneReplayTest;
	friend class FDCCinemaReplayTest;

public:
	ADCMultiCamDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Directors Cut", meta = (ClampMin = "1", Units = "s"))
	float TakeDuration = 60.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut", meta = (ClampMin = "0", Units = "s"))
	float CountdownDuration = 3.f;

	/** Dressing-only sublevels at identity transforms. Blocking, slopes and cast remain in the persistent level. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Replay")
	FName StudioEnvironmentLevel;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Replay")
	FName RealEnvironmentLevel;

	/** Separate presentation level, loaded on demand and retained for repeat screenings. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Replay")
	TSoftObjectPtr<UWorld> CinemaLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/DirectorsCut/Levels/L_DC_Cinema.L_DC_Cinema")));

	ADCCinemaReplay* GetCinema() const { return Cinema; }

	/** Future mission system: submit the timing/item verdict before the take ends. */
	UFUNCTION(BlueprintCallable, Category = "Directors Cut|Replay")
	void SetReplayMissionOutcome(FName BeatId, bool bSucceeded);

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

	/** How far one mouse-wheel notch scrubs the timeline in preparation and review. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cameras", meta = (ClampMin = "0.05", Units = "s"))
	float WheelScrubSeconds = 0.5f;

	/** How fast holding Q/E lowers or raises the next camera before it's placed. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cameras", meta = (ClampMin = "1", Units = "cm"))
	float StandHeightSpeed = 150.f;

	/** Zooming in lets a camera capture subjects proportionally farther than a beat's Max Distance. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cameras")
	bool bZoomExtendsRange = true;

	// Player commands. Look/zoom return true when they were applied to a camera instead of the walking view.
	void HandlePrimaryPressed();
	void HandlePrimaryReleased();
	void HandleSecondaryPressed();
	virtual void HandleInteract();
	virtual void HandleStartTake();
	virtual void HandleRestart();
	/** W/S while looking through a dolly or crane camera: travel along the rail / raise and lower the boom. */
	void HandleRigMove(float Axis, float DeltaSeconds);
	void HandleCycleTool();
	void HandleFinishRail();
	void HandleCycleCamera(int32 Direction);
	/** Wheel or Left/Right: scrub the film in review (the zombie director also scrubs its preparation timeline). */
	virtual void HandleScrub(float DeltaSeconds);
	/** Space in review: pause/resume the film, or watch it again once it has ended. */
	void HandleTransportToggle();
	void HandleRehearsalToggle();
	void HandleRehearsalSpeed(int32 Direction);
	bool IsRehearsalPlaying() const { return Phase == EDCPhase::Preparation && bRehearsalPlaying; }
	float GetRehearsalSpeed() const { return RehearsalSpeed; }
	/** Z / X: slower / faster dolly travel for the camera being looked through. */
	void HandleRailSpeed(int32 Direction);
	void HandleClearCameras();
	void HandleSelectCamera(int32 Number);
	bool HandleLook(float DeltaYaw, float DeltaPitch);
	/** Zooms while shooting; scrubs the timeline in preparation and review. bZoom (Shift) zooms in preparation instead. */
	void HandleWheel(float Delta, bool bZoom);
	/** Q/E held while walking in preparation: Axis is -1..1 scaled by frame time. */
	void HandlePlacementHeight(float AxisTimesDelta);

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
	/** The take is over and only being played back and evaluated. */
	bool IsReviewing() const { return Phase == EDCPhase::Review; }
	/** Review with the film played to its end: the evaluation is up. */
	bool IsFilmFinished() const { return IsReviewing() && SceneTime >= GetPremiereLength(); }
	const TArray<FDCTakeSample>& GetRecording() const { return Recording; }
	float GetPremiereLength() const { return Recording.Num() > 0 ? Recording.Last().FilmTime : 0.f; }
	/** Left click held during the take: nothing is recorded, captured, or penalized until it's released. */
	bool IsRecordingPaused() const { return Phase == EDCPhase::Filming && bRecordingPauseHeld; }
	bool IsPremierePaused() const { return bPremierePaused; }

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
	/** Keeps the view on a placed camera from Action! to the results. False (and back to planning) if none exist. */
	bool EnsureLiveCamera();
	/** Fades every camera rig (and its rail) inside the capture frame during the take; off again in preparation. */
	void SetRigsFrameMasked(bool bMasked);
	/** Hides every rig completely, so the played-back film is clean. */
	void SetRigsHidden(bool bHide);
	/** Swap dressing before showing a replay frame; keep both layers resident for instant retries. */
	void SetReviewEnvironment(bool bReview);
	TMap<TWeakObjectPtr<AActor>, bool> SetOnlyHiddenStates;

	/** Whether the live shot is spoiled right now (zombie in frame); recorded with each sample. */
	virtual bool IsLiveShotSpoiled() const { return false; }
	/** Appends what went out live this frame, or skips it (and marks a jump cut) while recording is paused. */
	void RecordSample(float DeltaSeconds);
	void StartPremiere();
	void TickPremiere(float DeltaSeconds);
	/** Re-shoots the recording at Time: performers to that moment, playback camera to the recorded pose. */
	void ShowPremiereFrame(float Time);
	void FinishPremiere();
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

	/** Plays the recorded take back as a film. */
	UPROPERTY()
	TObjectPtr<ACameraActor> PlaybackCamera;

	UPROPERTY()
	TObjectPtr<ADCCinemaReplay> Cinema;
	TMap<FName, bool> ReplayMissionOutcomes;

	TArray<FDCTakeSample> Recording;
	bool bPremierePaused = false;
	bool bRehearsalPlaying = true;
	float RehearsalSpeed = 1.f;
	bool bRecordingPauseHeld = false;
	float PausedSeconds = 0.f;
	bool bJumpCutPending = false;

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
