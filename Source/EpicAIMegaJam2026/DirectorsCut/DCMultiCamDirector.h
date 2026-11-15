// Director's Cut - multi-camera take director.
// The player places static and dolly cameras during preparation. During the take they are locked to the
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
class UCameraComponent;
class URectLightComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class APlayerController;
class UMaterialParameterCollection;
class UAudioComponent;
class UTexture2D;
class UTextureCube;
class USoundWave;

/**
 * Where things sit on the main menu, as a share of the screen (0,0 top-left, 1,1 bottom-right). The title is the
 * TitleTypography image in the bottom-left corner; "Press [ENTER] to Start" sits bottom-right.
 */
USTRUCT()
struct FDCMainMenuLayout
{
	GENERATED_BODY()

	/** The title image's bottom-left corner. */
	UPROPERTY(EditAnywhere, Category = "Title")
	FVector2D TitleCorner = FVector2D(0.035f, 0.95f);

	/** The title image's width, as a share of the screen's width (its height follows the image). */
	UPROPERTY(EditAnywhere, Category = "Title", meta = (ClampMin = "0.05", ClampMax = "1"))
	float TitleWidth = 0.42f;

	/** The prompt's right end, at its vertical middle. */
	UPROPERTY(EditAnywhere, Category = "Start Prompt")
	FVector2D PromptCorner = FVector2D(0.965f, 0.9f);

	/** Text size in pixels at 1080p (scales with the screen). */
	UPROPERTY(EditAnywhere, Category = "Start Prompt", meta = (ClampMin = "10"))
	float PromptTextSize = 56.f;
};

/** One line of a director cutscene: his face, and what he says. */
USTRUCT()
struct FDCCutsceneLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Cutscene")
	TObjectPtr<UTexture2D> Portrait;

	/** What he says. Words in [square brackets] are highlighted in blue (the brackets aren't shown). */
	UPROPERTY(EditAnywhere, Category = "Cutscene", meta = (MultiLine = true))
	FString Line;

	/** Filming tutorial: while he says this, the reference cut and the camera switcher are spotlit. */
	UPROPERTY(EditAnywhere, Category = "Cutscene")
	bool bHighlightShootingUI = false;

	/** Filming tutorial: animated mouse graphics under his bubble - move the mouse to aim, scroll to zoom. */
	UPROPERTY(EditAnywhere, Category = "Cutscene")
	bool bShowCameraControls = false;
};

class ADCCinemaReplay;

/** Runtime state of one hidden beat. */
struct FDCMultiCamBeat
{
	ADCBeat* Beat = nullptr;
	TArray<FDCAnchor> Anchors;
	EDCShotStatus Status = EDCShotStatus::Pending;
	float Hold = 0.f;
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
	/** Position in the finished film (the take is filmed straight through, so the same as Time for a recording). */
	float FilmTime = 0.f;
	/** Playback jump-cuts here instead of blending from the previous frame. */
	bool bJumpCut = false;
	/** Camera slot that was live; a change between samples is a cut. */
	int32 Slot = INDEX_NONE;
	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	/** Horizontal FOV of the film frame itself (the capture frame of the live lens at that moment). */
	float FilmFOV = 50.f;
};

/** The film director's face in the shooting HUD; his portrait follows how well the live shot matches the reference. */
enum class EDCDirectorMood : uint8
{
	Default,
	/** Close - a small correction. */
	Almost,
	/** Way off, or the wrong camera is live. */
	Disgust,
	/** Matching the reference cut. */
	Joy
};

/**
 * How alike two shots look, each 0-1: judged on the picture, not the camera settings - so getting closer and zooming
 * in are interchangeable. For a whole take it is the average over the take (the grade).
 */
struct FDCMatch
{
	/** Where the subjects sit in the frame. */
	float Framing = 0.f;
	/** How big they appear. */
	float Size = 0.f;
	/** Which side and how high they are seen from. */
	float Viewpoint = 0.f;
	/** Weighted blend of the three. */
	float Overall = 0.f;
};

/** What a film camera shows of the scene's subjects (their heads and feet). */
struct FDCFrameReading
{
	/** Every subject point is in front of the camera. */
	bool bVisible = false;
	/** Centre of the subjects on the film frame: x runs -1..1 across its width, y uses the same scale (up). */
	FVector2D Center = FVector2D::ZeroVector;
	/** Their extent on the frame, in the same units. */
	float Size = 0.f;
	/** Unit direction from the subjects to the camera. */
	FVector ViewDir = FVector::ZeroVector;
};

UCLASS()
class ADCMultiCamDirector : public AActor
{
	GENERATED_BODY()
	friend class FDCDualSceneReplayTest;
	friend class FDCDialoguePlaybackTest;
	friend class FDCAuthoredReferenceTest;
	friend class FDCAct2PlacementTest;
	friend class FDCAct1PerformanceTest;
	friend class FDCSharedUITest;
	friend class FDCCinemaReplayTest;

public:
	ADCMultiCamDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	/** Leaves hardware ray tracing on (it may have been switched off for the real environment). */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "Directors Cut", meta = (ClampMin = "1", Units = "s"))
	float TakeDuration = 60.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut", meta = (ClampMin = "0", Units = "s"))
	float CountdownDuration = 3.f;

	/** Dressing-only sublevels at identity transforms. Blocking, slopes and cast remain in the persistent level. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Replay")
	FName StudioEnvironmentLevel;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Replay")
	FName RealEnvironmentLevel;

	/** Brightness (candelas) of the soft light the film camera carries in the real environment. It stands in for the
	 *  studio's rect lights, so the actors keep the same waxy highlights in the reference cut and the playback.
	 *  Lights only the actors (lighting channel 1). 0 turns it off. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Replay", meta = (ClampMin = "0"))
	float FilmKeyLightIntensity = 1500.f;

	/** Separate presentation level, loaded on demand and retained for repeat screenings. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Replay")
	TSoftObjectPtr<UWorld> CinemaLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/DirectorsCut/Levels/L_DC_Cinema.L_DC_Cinema")));

	ADCCinemaReplay* GetCinema() const { return Cinema; }

	/** Future mission system: submit the timing/item verdict before the take ends. */
	UFUNCTION(BlueprintCallable, Category = "Directors Cut|Replay")
	void SetReplayMissionOutcome(FName BeatId, bool bSucceeded);

	/**
	 * The storyboard opens on a reference cut: a short film of this level shot well, one camera per beat, computed at
	 * play time with the game's own framing and "ruins the shot" checks (see DCReferenceCut.cpp).
	 */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Reference Cut")
	bool bShowReferenceCut = true;

	/** The reference cut cuts to each beat's camera this long before the beat can be captured. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Reference Cut", meta = (ClampMin = "0", Units = "s"))
	float ReferenceCutLead = 1.f;

	/** How much of the frame the action may fill in a reference shot; lower leaves more of the scenery around it. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Reference Cut", meta = (ClampMin = "0.1", ClampMax = "0.9"))
	float ReferenceActionShare = 0.35f;

	/** The reference cut's lenses never go narrower than this (keeps shots wide). */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Reference Cut", meta = (ClampMin = "20", ClampMax = "90", Units = "deg"))
	float ReferenceMinFOV = 55.f;

	/** Distance from the action the reference cut prefers when several spots work. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Reference Cut", meta = (ClampMin = "100", Units = "cm"))
	float ReferencePreferredDistance = 1200.f;

	/**
	 * Legacy authoring radius used to space automatically generated reference cameras.
	 * Preparation missions are evaluated by reference-image likeness, not this distance.
	 */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Reference Cut", meta = (ClampMin = "10", Units = "cm"))
	float ReferenceSpotRadius = 600.f;

	// Retained for saved-map compatibility; preparation coverage now measures ground position only.
	UPROPERTY()
	float ReferenceSpotHeightTolerance = 250.f;

	bool IsReferencePlaying() const { return bReferencePlaying; }
	bool IsReferencePaused() const { return bReferencePaused; }
	float GetReferenceTime() const { return ReferenceTime; }
	float GetReferenceLength() const { return ReferenceCut.Num() > 0 ? ReferenceCut.Last().FilmTime : 0.f; }
	/** The storyboard beat the reference cut is showing right now (null between beats). */
	const ADCBeat* GetReferenceBeat() const;
	/** The beat the reference cut shows at scene time Time (null between beats). */
	const ADCBeat* GetReferenceBeatAt(float Time) const;
	/** Whether a placed camera stands near the reference cut's camera for this scene (index into GetBeats). */
	bool IsSceneCovered(int32 BeatIndex) const { return SceneCovered.IsValidIndex(BeatIndex) && SceneCovered[BeatIndex]; }
	/** Real time the scene was last ticked off, for the storyboard's animation (negative if it never was). */
	float GetSceneCoveredTime(int32 BeatIndex) const { return SceneCoveredTime.IsValidIndex(BeatIndex) ? SceneCoveredTime[BeatIndex] : -1.f; }
	/**
	 * Filming: the reference cut's next cut the player should follow. OutSecondsToCut counts down to it (0 or less once
	 * it's due, for a moment, until that camera is live); OutCameraNumber is the placed camera to cut to. False when
	 * no cut is coming up within CutCueLead seconds, or the right camera is already live.
	 */
	bool GetCutCue(float& OutSecondsToCut, int32& OutCameraNumber) const;
	/** Placed camera slot that takes reference shot Shot: the one on its spot (paired as the storyboard ticks scenes off),
	 *  else the nearest. INDEX_NONE without cameras. */
	int32 GetCameraForReferenceShot(int32 Shot) const;

	/** Filming: how many seconds before each of the reference cut's cuts the countdown to it starts. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Reference Cut", meta = (ClampMin = "0", Units = "s"))
	float CutCueLead = 3.f;

	/** Preparation: how long the director's praise stays up once he has popped up (a scene was ticked off). */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Director Coach", meta = (ClampMin = "1", Units = "s"))
	float CheerSeconds = 2.8f;

	const FString& GetCheerLine() const { return CheerLine; }
	/** Real time the current praise started (its typewriter), and since when he's been up (he stays up through praise in a row). */
	float GetCheerLineStart() const { return CheerLineStart; }
	float GetCheerUpSince() const { return CheerUpSince; }
	/** Counts praise lines; the HUD alternates his two delighted faces by it. */
	int32 GetCheerCount() const { return CheerCount; }

	/** During preparation and shooting the reference cut also plays in a corner inset, in step with the scene clock. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Reference Cut")
	bool bShowReferenceInset = true;

	/** During shooting, the film's director coaches the player (bottom-left) toward the reference cut. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Director Coach")
	bool bShowDirectorCoach = true;

	/** Shortest time a coaching line stays up before the director moves on to the next one. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Director Coach", meta = (ClampMin = "0.5", Units = "s"))
	float CoachLineSeconds = 2.2f;

	/** The game opens on the main menu (title and a start button), before the intro cutscene. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cutscenes")
	bool bShowMainMenu = true;

	/** Main menu layout; the HUD reads it every frame, so changes show up straight away. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Main Menu", meta = (ShowOnlyInnerProperties))
	FDCMainMenuLayout MainMenu;

	bool IsInMainMenu() const { return bInMainMenu; }

	/** Between phases the director talks to the player in a short cutscene; Enter or click moves on. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cutscenes")
	bool bShowCutscenes = true;

	/** Before the reference cut. Each act has its own lines; anything put here replaces them. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cutscenes")
	TArray<FDCCutsceneLine> IntroCutscene;

	/** After the reference cut, before preparation (empty: the act's own lines). */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cutscenes")
	TArray<FDCCutsceneLine> BriefingCutscene;

	/** After preparation, before shooting (empty: the act's own lines). */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cutscenes")
	TArray<FDCCutsceneLine> ActionCutscene;

	/** After the evaluation, before the next act (empty: the act's own lines). */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cutscenes")
	TArray<FDCCutsceneLine> OutroCutscene;

	/** The first take only: once on set behind the live camera, before the countdown, he explains the shooting controls
	 *  over the scene (empty: the act's own lines, if any). */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cutscenes")
	TArray<FDCCutsceneLine> FilmingTutorial;

	/** When the take ends: the scene holds while he says these in his corner bubble, one after another, each with its
	 *  own face; then the premiere starts (empty: the act's own lines, else straight to the premiere). */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cutscenes")
	TArray<FDCCutsceneLine> WrapUpLines;

	/** How long each wrap-up line stays up. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cutscenes", meta = (ClampMin = "0.5", Units = "s"))
	float WrapUpLineSeconds = 3.5f;

	/** The take is over and he's wrapping up before the premiere. */
	bool IsWrappingUp() const { return WrapUpLine != INDEX_NONE; }
	/** A face picked for what he's saying (the wrap-up), or null to show the face for his mood. */
	UTexture2D* GetCoachPortrait() const { return CoachPortrait; }


	/** Length of each half of a fade through black between phases. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cutscenes", meta = (ClampMin = "0.05", Units = "s"))
	float PhaseFadeSeconds = 0.6f;

	/** Characters per second a cutscene line types out at. */
	static constexpr float CutsceneTypeRate = 40.f;

	bool IsInCutscene() const { return Cutscene != nullptr; }
	/** The cutscene is played over the live scene (the filming tutorial) rather than on its own dark stage. */
	bool IsCutsceneOverScene() const { return IsInCutscene() && bCutsceneOverScene; }
	/** The main menu or a cutscene is up, or the screen is fading: player input only moves those on. */
	bool IsInputBlocked() const { return OverviewRemaining > 0.f || bInMainMenu || IsInCutscene() || FadeDirection != 0; }
	/** The cutscene line being said (null outside cutscenes), its index and count, and the real time it started. */
	const FDCCutsceneLine* GetCutsceneLine() const { return Cutscene && Cutscene->IsValidIndex(CutsceneLine) ? &(*Cutscene)[CutsceneLine] : nullptr; }
	int32 GetCutsceneLineIndex() const { return CutsceneLine; }
	int32 GetCutsceneLineCount() const { return Cutscene ? Cutscene->Num() : 0; }
	float GetCutsceneLineStart() const { return CutsceneLineStart; }
	/** How black the screen is (0-1), eased. */
	float GetFadeAlpha() const { return FMath::SmoothStep(0.f, 1.f, FadeAlpha); }

	/** Live likeness (0-1) from which a shot counts as perfect: the director cheers and the frame celebrates. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Director Coach", meta = (ClampMin = "0.5", ClampMax = "1"))
	float PerfectLikeness = 0.85f;

	/** While shooting: how alike the live shot is to the reference cut right now (0-1); negative when there's nothing to compare. */
	float GetLiveLikeness() const { return LiveLikeness; }
	/** Live composition error in film-width units (Y up), and subject size / reference size. */
	bool GetFilmingGuide(FVector2D& OutAim, float& OutZoomRatio) const;

	const FString& GetCoachLine() const { return CoachLine; }
	EDCDirectorMood GetCoachMood() const { return CoachMood; }
	/** Real time the current line started (for the HUD's typewriter). */
	float GetCoachLineStart() const { return CoachLineStart; }

	/** The inset picture during preparation and shooting; null when there's nothing to show. */
	UTextureRenderTarget2D* GetReferenceInset() const;
 void ToggleReferenceFocus() { if (Phase == EDCPhase::Preparation && !IsInputBlocked()) bReferenceFocused = !bReferenceFocused; }
 bool IsReferenceFocused() const { return bReferenceFocused && Phase == EDCPhase::Preparation; }
 bool IsSetOverview() const { return OverviewRemaining > 0.f; }
 void StartSetOverview();
 float GetPreparationSlateRemaining() const { return OverviewRemaining; }
 TArray<float> GetReferenceCameraCutTimes(bool bSceneTime) const;
 UTextureRenderTarget2D* GetPlacementPreview() const { return bPreviewHit && Phase == EDCPhase::Preparation && !IsViewingCamera() ? PlacementTarget.Get() : nullptr; }
 FVector GetPlacementHintLocation() const { return PreviewPoint; }
 /** Where the stand of the camera being placed meets its tripod legs, a little above the ground. */
 FVector GetPlacementJointLocation() const;
 UPROPERTY() TObjectPtr<USceneCaptureComponent2D> PlacementCapture;
 UPROPERTY() TObjectPtr<UTextureRenderTarget2D> PlacementTarget;
 bool bReferenceFocused = false;
 float OverviewRemaining = 0.f;
 float PlacementCaptureElapsed = 0.f;

	/**
	 * Grading: each moment of the take is compared with the reference cut on the picture itself - where the
	 * scene's characters sit in the frame, how big they appear, and which side they're seen from. A part scores 100%
	 * when it matches and falls off gently (near misses cost little) to 0% at its tolerance; moments missing from the
	 * film (recording paused) score 0.
	 */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Grading", meta = (ClampMin = "0.05"))
	float MatchFramingTolerance = 0.5f; // offset of the subjects' centre, in half frame widths

	/** Size ratio (bigger or smaller) at which the size score reaches 0. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Grading", meta = (ClampMin = "1.1"))
	float MatchSizeTolerance = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Grading", meta = (ClampMin = "1", Units = "deg"))
	float MatchViewpointTolerance = 75.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Grading", meta = (ClampMin = "0"))
	float MatchFramingWeight = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Grading", meta = (ClampMin = "0"))
	float MatchSizeWeight = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Grading", meta = (ClampMin = "0"))
	float MatchViewpointWeight = 0.3f;

	/** How many cameras the player may place; 0 = unlimited. Keys 1-9 pick the first nine, [ and ] step through all. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cameras", meta = (ClampMin = "0"))
	int32 MaxCameras = 4;

	/** How far from a wall or ceiling a mounted camera sits. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cameras", meta = (Units = "cm"))
	float WallStandoff = 35.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cameras", meta = (Units = "cm"))
	float MinStandHeight = 110.f; // Fixed tripod feet and camera head must fit below the lens.

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cameras", meta = (Units = "cm"))
	float MaxStandHeight = 320.f;

	/** Stabilizer, always on while shooting: how quickly the camera head catches up with the mouse (lower = smoother). */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Stabilizer", meta = (ClampMin = "0.1"))
	float StabilizerAimSmoothing = 4.f;

	/** Stabilizer, always on while shooting: how quickly a (legacy) dolly reaches and comes off its speed. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Stabilizer", meta = (ClampMin = "0.1"))
	float StabilizerDollyEase = 1.5f;

	/** Camera-spot hint circles (beats with Show Camera Spot): how strongly they show while shooting (1 = as in preparation). */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Camera Spots", meta = (ClampMin = "0", ClampMax = "1"))
	float ShootingCameraSpotOpacity = 0.3f;

	/** A beat's circle appears this long before its window opens, so there's time to get the camera there. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Camera Spots", meta = (ClampMin = "0", Units = "s"))
	float CameraSpotLeadTime = 1.5f;

	/** How far one mouse-wheel notch scrubs the storyboard's reference cut and the review film. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cameras", meta = (ClampMin = "0.05", Units = "s"))
	float WheelScrubSeconds = 0.5f;

	/** Holding Q / E (or Left / Right) scrubs the timeline at this many scene seconds per second. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cameras", meta = (ClampMin = "0.5"))
	float KeyScrubRate = 4.f;

	/** How far one mouse-wheel notch raises or lowers the next camera before it's placed. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cameras", meta = (ClampMin = "1", Units = "cm"))
	float StandHeightStep = 20.f;

	/** Zooming in lets a camera capture subjects proportionally farther than a beat's Max Distance. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Cameras")
	bool bZoomExtendsRange = true;

	/**
	 * The reference-cut and placement insets skip Lumen to stay cheap, so sets lit mostly by bounce light (Act 2's
	 * studio) come out dark in them. This flat cubemap fill, seen only by the insets, stands in for that bounce light.
	 * 0 = off.
	 */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Insets", meta = (ClampMin = "0"))
	float InsetFillIntensity = 0.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Insets")
	TObjectPtr<UTextureCube> InsetFillCubemap;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Insets")
	FLinearColor InsetFillTint = FLinearColor::White;

	// Player commands. Look/zoom return true when they were applied to a camera instead of the walking view.
	void HandlePrimaryPressed();
	void HandleCameraCancel();
	void HandleCameraInteract();
	int32 GetNearbyCameraSlot() const;
	bool IsPlacementPending() const { return PendingPlacementSlot != INDEX_NONE; }
	/** Right Mouse: removes the camera aimed at while walking, or steps back out of the camera being looked through. */
	void HandleSecondaryPressed();
	virtual void HandleStartTake();
	virtual void HandleRestart();
	/** W/S while looking through a dolly camera: travel along its rail. */
	void HandleRigMove(FVector2D Axis, float DeltaSeconds);
	void HandleFinishRail();
	void HandleCycleCamera(int32 Direction);
	/** Wheel or Left/Right: scrub the film in review (the zombie director also scrubs its preparation timeline). */
	virtual void HandleScrub(float DeltaSeconds);
	/** Space in review: pause/resume the film, or watch it again once it has ended. */
	void HandleTransportToggle();
	void HandleRehearsalToggle();
	bool IsRehearsalPlaying() const { return Phase == EDCPhase::Preparation && bRehearsalPlaying; }
	float GetRehearsalSpeed() const { return 1.f; }
	/** Z / X: slower / faster dolly travel for the camera being looked through. */
	void HandleRailSpeed(int32 Direction);
	void HandleClearCameras();
	void HandleSelectCamera(int32 Number);
	bool HandleLook(float DeltaYaw, float DeltaPitch);
	/**
	 * Mouse wheel: zooms the camera being looked through (preparation and shooting); while walking in preparation it
	 * raises/lowers the next camera; in the storyboard and review it scrubs the film.
	 */
	void HandleWheel(float Delta);
	/** Raises or lowers the next camera by this many wheel notches. */
	void HandlePlacementHeight(float Notches);

	// Read-only state for the HUD and character
	EDCPhase GetPhase() const { return Phase; }
	float GetSceneTime() const { return SceneTime; }
	float GetCountdownRemaining() const { return CountdownRemaining; }
	const TArray<FDCMultiCamBeat>& GetBeats() const { return Beats; }
	ADCMultiCam* GetCameraSlot(int32 Index) const { return Cameras.IsValidIndex(Index) ? Cameras[Index].Get() : nullptr; }
	int32 GetCameraCount() const;
	int32 GetSlotCount() const { return Cameras.Num(); }
	bool IsUnlimited() const { return MaxCameras <= 0; }
	/**
	 * Slot of the camera being placed right now: the dolly whose rail is still being laid, else the slot the next
	 * click would fill (may be one past the end when cameras are unlimited). INDEX_NONE when not placing.
	 */
	int32 GetPlacementSlot() const;
	int32 GetPendingRailPoints() const;
	/** 0-based slot being looked through, or INDEX_NONE while walking. */
	int32 GetViewingSlot() const { return ViewingSlot; }
	bool IsViewingCamera() const { return ViewingSlot != INDEX_NONE; }
	float GetPendingHeight() const { return PendingHeight; }
	/** The last take's match with the reference cut, and the best match so far. */
	const FDCMatch& GetMatch() const { return Match; }
	float GetBestMatch() const { return BestMatch; }
	/** Letter grade (S, A, B, C, D) and cut title for the last take's match. */
	FString GetGrade() const;
	FString GetCutName() const;
	/** 0 = S ... 4 = D. B or better counts as a good take. */
	int32 GetGradeRank() const;
	bool IsGoodTake() const { return GetGradeRank() <= 2; }
	int32 GetCutCount() const { return CutCount; }
	FString GetMessage() const;
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
	bool IsPremierePaused() const { return bPremierePaused; }


protected:
	/** The optional DCSceneDialogue-tagged ambient sound follows the visible scene clock. */
	void UpdateSceneDialogue(float DeltaSeconds);
	TWeakObjectPtr<UAudioComponent> SceneDialogue;
	float DialoguePosition = -1.f;
	float DialogueRate = 1.f;
	EDCPhase DialoguePhase = EDCPhase::Reference;

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
	/** Hands every camera the stabilizer strength for the current phase (none outside shooting). */
	void ApplyStabilizer();
	/** Shows each pending beat's camera-spot circle around its window: full in preparation, faint while shooting, never in review. */
	void UpdateCameraSpots();
	/** Swap dressing before showing a replay frame; keep both layers resident for instant retries. */
	/**
	 * Swaps the studio set for the real environment (reference cut, review) and back. The real environment is far
	 * heavier, so while it shows, Lumen uses software tracing and hardware ray tracing is off; the studio set
	 * (preparation, shooting) keeps the full hardware look.
	 */
	void SetReviewEnvironment(bool bReview);
	TMap<TWeakObjectPtr<AActor>, bool> SetOnlyHiddenStates;

	/** Appends what went out live this frame, or skips it (and marks a jump cut) while recording is paused. */
	void RecordSample();
	void StartPremiere();
	void TickPremiere(float DeltaSeconds);
	/** Re-shoots the recording at Time: performers to that moment, playback camera to the recorded pose. */
	void ShowPremiereFrame(float Time);
	/** Shows any film (the recording or the reference cut) at film time Time through the playback camera. */
	void ShowFilmFrame(const TArray<FDCTakeSample>& Film, float Time);
	/** Where a film's camera is at film time Time (blended within a shot), and the scene time that frame shows. */
	bool GetFilmView(const TArray<FDCTakeSample>& Film, float Time, FVector& OutLocation, FQuat& OutRotation, float& OutFOV, float& OutScenePoint) const;
	void EvaluatePerformers(float Time);


	// Reference cut (DCReferenceCut.cpp)
	void BuildReferenceCut();
	/** Inverse of the reference film's scene-time mapping, including slow motion. */
	float ReferenceFilmTimeAtSceneTime(float Time) const;
	/** Grades the recording against the reference cut (sets Match). */
	void ComputeMatch();

	// Frame likeness (DCFrameLikeness.cpp)
	/** The reference cut's subjects at scene time Time - head and feet of each character in its beat - as posed now. */
	void GetSubjectPoints(float Time, TArray<FVector>& OutPoints) const;
	/** What a camera at CamLocation/CamRotation with film field of view FilmFOV shows of those points. */
	static FDCFrameReading ReadFrame(const TArray<FVector>& Points, const FVector& CamLocation, const FQuat& CamRotation, float FilmFOV);
	/** How alike two readings look. */
	FDCMatch CompareFrames(const FDCFrameReading& Shot, const FDCFrameReading& Ref) const;
	/** Best spot and lens for one beat; false if even the best can't hold it long enough to capture. */
	bool FindReferenceShot(const FDCMultiCamBeat& State, const TArray<FVector>& Taken, FVector& OutLocation, float& OutFOV, float& OutCleanFrom);
	/**
	 * How far apart (across the ground) two scenes' reference cameras must be: more than two spot radii, so the film
	 * visibly cuts between scenes and no single placed camera stands on two scenes' spots.
	 */
	float GetReferenceShotSpacing() const { return 2.f * ReferenceSpotRadius + 100.f; }
	void StartReferenceCut();
	void StopReferenceCut();
	/** The reference cut played to its end (or was skipped). By default, back to the storyboard page. */
	virtual void OnReferenceCutFinished();
	/** Enter on the evaluation screen. By default, back to preparation for another take. */
	virtual void OnEvaluationFinished();
	void TickReferenceCut(float DeltaSeconds);
	void ScrubReferenceCut(float DeltaSeconds);
	void ToggleReferenceCut();
	/** Renders the reference cut's camera into the preparation inset at 15 Hz while its scene time changes. */
	void UpdateReferenceInset(float DeltaSeconds);
	/** Captures an inset with the main view's volume settings (exposure, grade) plus the cheap inset fill light. */
	void CaptureInset(USceneCaptureComponent2D* Capture) const;
	TArray<FDCTakeSample> ReferenceCut;
	bool bReferenceCutBuilt = false;
	/**
	 * Where the player can stand a camera in preparation: inside the studio set's walls (StudioEnvironmentLevel's
	 * solid parts), across the ground. Reference cameras are kept in it. Invalid when the level has no studio.
	 */
	FBox2D ReferenceStageBounds = FBox2D(ForceInit);
	FBox2D GetStudioBounds() const;

	UPROPERTY()
	TObjectPtr<USceneCaptureComponent2D> ReferenceCapture;

	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> ReferenceInsetTarget;
	float ReferenceInsetCaptureElapsed = 1.f;
	float LastReferenceInsetSceneTime = -TNumericLimits<float>::Max();
	/** Beat index (into Beats) and camera position for each reference shot (FDCTakeSample::Slot). */
	TArray<int32> ReferenceShotBeats;
	TArray<FVector> ReferenceShotLocations;
	/** Per beat: an installed camera has matched its reference picture. Updated in preparation. */
	TArray<bool> SceneCovered;
	// Evidence belongs to a camera's configured picture, not its distance from an authoring marker.
	TArray<TWeakObjectPtr<ADCMultiCam>> CoverageCameras;
	TArray<FDCTakeSample> CoveragePoses;
	TArray<TArray<float>> CoverageMatches;
	TArray<int32> SceneCoverageCameras;
	/** Per beat: real time it was last ticked off (negative if never). */
	TArray<float> SceneCoveredTime;
	/** Ticks scenes off and announces reference-image matches from installed cameras. */
	void UpdateCoveredScenes();
	bool bReferencePlaying = false;
	bool bReferencePaused = false;
	/** Set by a director that opens on the reference film: the reference cut starts on its first tick. */
	bool bReferenceAutoStart = false;
	float ReferenceTime = 0.f;
	void FinishPremiere();
	void Flash(const FString& Text, float Seconds = 2.5f);

	// Director coach (DCDirectorCoach.cpp)
	/** Shooting: compares the live camera with the reference cut right now and says the most useful thing. */
	void UpdateCoach();
	/** Changes the director's line (one of Variants, {cam} replaced by Camera) unless it was just said or is too new to replace. */
	void Coach(FName Key, EDCDirectorMood Mood, std::initializer_list<const TCHAR*> Variants, int32 Camera = 0, bool bForce = false);
	float LiveLikeness = -1.f;
	FString CoachLine;
	EDCDirectorMood CoachMood = EDCDirectorMood::Default;
	FName CoachKey;
	float CoachLineStart = 0.f;
	int32 CoachLineCount = 0;
	/** Preparation: the director pops up to praise a scene that just got its camera (BeatIndex into Beats). */
	void Cheer(int32 BeatIndex);
	FString CheerLine;
	float CheerLineStart = -100.f;

	// Cutscenes and fades (DCCutscene.cpp)
	/** Plays Lines (or nothing, when cutscenes are off), then fades through black and calls OnDone at full black. */
	/** bOverScene: drawn over the live scene, and it ends without a fade (the scene is already showing). */
	void PlayCutscene(const TArray<FDCCutsceneLine>& Lines, TFunction<void()> OnDone, bool bOverScene = false);
	/** Starts the countdown to Action! - the director's places-everyone line goes with it. */
	void RollCountdown();
	/** Enter or click during a cutscene: finishes the line being typed, else moves to the next. False if no cutscene took it. */
	bool HandleCutsceneInput();
	/** Fades to black, calls AtBlack there, then fades back in. */
	void FadeThrough(TFunction<void()> AtBlack);
	/** Starts the screen black, fading in. */
	void FadeInFromBlack();
	void TickFade(float DeltaSeconds);
	const TArray<FDCCutsceneLine>* Cutscene = nullptr;
	int32 CutsceneLine = 0;
	float CutsceneLineStart = 0.f;
	TFunction<void()> CutsceneDone;
	/** The wrap-up line being said (INDEX_NONE when not wrapping up), and for how long. */
	int32 WrapUpLine = INDEX_NONE;
	float WrapUpElapsed = 0.f;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CoachPortrait;
	void SayWrapUpLine(int32 Index);
	void TickWrapUp(float DeltaSeconds);
	/** Grades the take and goes to the premiere (after the wrap-up, if any). */
	void FinishTake();
	bool bCutsceneOverScene = false;
	/** The filming tutorial plays before the first take only. */
	bool bFilmingTutorialShown = false;
	TFunction<void()> FadeAction;
	float FadeAlpha = 0.f;
	/** +1 fading to black, -1 fading back in, 0 not fading. */
	int32 FadeDirection = 0;
	bool bInMainMenu = false;
	/** Preparation's checks passed: the take starts (after the action cutscene). */
	void StartCountdown();
	float CheerUpSince = -100.f;
	int32 CheerCount = 0;
	FCollisionQueryParams MakeTraceParams(const FName& Tag) const;

	ADCMultiCamCharacter* GetPlayer() const;
	APlayerController* GetPC() const;
	/** Width / height of the gameplay view: always DCFrame::ScreenAspect (other window shapes get black bars). */
	float GetViewAspect() const;
	/** Tells M_DCRig (through MPC_DCFrame) where the capture frame is on screen; only does work when the view's shape changes. */
	void UpdateFrameParams();
	/** Clay look: keeps the actors sharp in the shallow, miniature-style depth of field of the player's current view.
	 *  Off when dc.ClayLook.FStop is 0. The films pull their own focus as each frame is posed (ShowFilmFrame). */
	void UpdateFocus(float DeltaSeconds);
	/** Focuses Lens on the visible performer nearest the middle of its frame; bSnap skips the focus pull (cuts). */
	void PullFocus(UCameraComponent* Lens, float DeltaSeconds, bool bSnap) const;

	/** First empty camera slot; grows the list when cameras are unlimited. INDEX_NONE when the budget is spent. */
	int32 FindFreeSlot();
	bool HasCameraBudget() const;
	ADCMultiCam* SpawnCamera(const FVector& Location);
	void FinishPendingRail(bool bQuiet);

	/** Rail being laid for the next dolly camera. */
	UPROPERTY()
	TObjectPtr<ADCRail> PendingRail;

	UPROPERTY()
	TArray<TObjectPtr<ADCPerformer>> Performers;

	UPROPERTY()
	TObjectPtr<ADCClapperboard> Clapperboard;

	/** One entry per slot; empty slots are null. */
	UPROPERTY()
	TArray<TObjectPtr<ADCMultiCam>> Cameras;

	UPROPERTY()
	TObjectPtr<ADCMultiCam> Ghost;

	/** Plays the recorded take back as a film, locked to DCFrame::FilmAspect. */
	UPROPERTY()
	TObjectPtr<ACameraActor> PlaybackCamera;

	/** Soft key light riding the playback camera; on only while the real environment replaces the studio. */
	UPROPERTY()
	TObjectPtr<URectLightComponent> FilmKeyLight;

	/** Holds the capture frame's on-screen rectangle (CaptureRect) for M_DCRig's in-frame fade. */
	UPROPERTY()
	TObjectPtr<UMaterialParameterCollection> FrameParams;

	float FrameParamsAspect = 0.f;
	UPROPERTY()
	TObjectPtr<ADCCinemaReplay> Cinema;
	TMap<FName, bool> ReplayMissionOutcomes;

	TArray<FDCTakeSample> Recording;
	bool bPremierePaused = false;
	bool bRehearsalPlaying = true;

	TArray<FDCMultiCamBeat> Beats;
	FString SetupProblem;

	EDCPhase Phase = EDCPhase::Preparation;
	int32 ViewingSlot = INDEX_NONE;
	int32 PendingPlacementSlot = INDEX_NONE;
	float PendingHeight = 170.f;
	float SceneTime = 0.f;
	float CountdownRemaining = 0.f;
	int32 LastCountdownSoundNumber = INDEX_NONE;
	UPROPERTY()
	TObjectPtr<USoundWave> TimerTickSound;
	UPROPERTY()
	TObjectPtr<USoundWave> DirectorContentSound;
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> DirectorContentAudio;
	void PlayDirectorContent();
	FDCMatch Match;
	float BestMatch = 0.f;
	int32 CutCount = 0;

	FTransform StartTransform;

	FVector PreviewPoint = FVector::ZeroVector;
	FVector PreviewSurface = FVector::ZeroVector;
	bool bPreviewHit = false;
	bool bPreviewValid = false;

	FString Message;
	float MessageExpireTime = 0.f;
};
