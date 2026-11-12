// Director's Cut prototype 4 - "Replica": watch a reference film, rig the set, then reproduce the film as
// closely as possible. Built on prototype 2's free rigs (rails anywhere, tripods/cranes) plus spray marks.
//
//   Watching    the reference film plays from ADCRefCamKey actors (R replays, Enter skips)
//   Preparation rails / cranes / numbered spray marks; V rewatches the reference
//   Filming     the player's camera is sampled 10x per second and compared to the reference at the same time
//   Results     per-shot similarity, grade, and replays of the reference (V) and the player's take (T)

#pragma once

#include "CoreMinimal.h"
#include "DCFreeDirector.h"
#include "DCReplicaDirector.generated.h"

class ACameraActor;
class ADCRefCamKey;
class ADCSprayMark;

/** One shot of the reference film plus the player's running similarity against it. */
struct FDCRefShot
{
	int32 Number = 0;
	FString Title;
	float Start = 0.f;
	float End = 0.f;
	TArray<ADCRefCamKey*> Keys;
	TArray<FDCAnchor> Subjects;

	// Evaluation of the current take
	int32 Samples = 0;
	float SumScore = 0.f;
	float SumFraming = 0.f;
	float SumPositionError = 0.f;
	float SumAngleError = 0.f;
	float SumFov = 0.f;
	float SumRefFov = 0.f;

	void ResetEvaluation()
	{
		Samples = 0;
		SumScore = SumFraming = SumPositionError = SumAngleError = SumFov = SumRefFov = 0.f;
	}
	float GetScore() const { return Samples > 0 ? SumScore / Samples : 0.f; }
	float GetFraming() const { return Samples > 0 ? SumFraming / Samples : 0.f; }
	float GetPositionError() const { return Samples > 0 ? SumPositionError / Samples : 0.f; }
	float GetAngleError() const { return Samples > 0 ? SumAngleError / Samples : 0.f; }
	float GetFov() const { return Samples > 0 ? SumFov / Samples : 75.f; }
	float GetRefFov() const { return Samples > 0 ? SumRefFov / Samples : 75.f; }
};

/** A recorded sample of the player's camera. */
struct FDCCamSample
{
	float Time = 0.f;
	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	float Fov = 75.f;
};

UENUM()
enum class EDCReplay : uint8
{
	None,
	Reference,
	Take
};

UCLASS()
class ADCReplicaDirector : public ADCFreeDirector
{
	GENERATED_BODY()

public:
	ADCReplicaDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Camera samples per second of scene time, for scoring and replay. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Evaluation", meta = (ClampMin = "2", ClampMax = "60"))
	float SampleRate = 10.f;

	/** Position error at which the position score reaches zero. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Evaluation", meta = (Units = "cm"))
	float PositionTolerance = 500.f;

	/** Angle error at which the angle score reaches zero. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Evaluation", meta = (Units = "deg"))
	float AngleTolerance = 45.f;

	/** Weight of framing (subjects' screen position and size) vs. camera angle and position. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Evaluation", meta = (ClampMin = "0", ClampMax = "1"))
	float FramingWeight = 0.5f;

	/** Show a live similarity meter while filming. Turn off for a harder, blind take. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Evaluation")
	bool bShowLiveMatch = true;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Rigs", meta = (ClampMin = "0"))
	int32 MaxSprayMarks = 30;

#if WITH_EDITORONLY_DATA
	/** Tick to place the default chapel scene into this level (same as the Build Chapel Scene button). */
	UPROPERTY(EditAnywhere, Transient, Category = "Directors Cut|Editor")
	bool bBuildChapelScene = false;
#endif

#if WITH_EDITOR
	/** Places the chapel set, the vampire and the worshipper, their blocking, and the reference film's camera keys. */
	UFUNCTION(CallInEditor, Category = "Directors Cut|Editor")
	void BuildChapelScene();

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// Player commands
	virtual void HandlePrimaryPressed() override;
	virtual void HandleSecondaryPressed() override;
	virtual void HandleInteract() override;
	virtual void HandleStartTake() override;
	virtual void HandleRestart() override;
	virtual void HandleClearRigs() override;
	virtual void HandleWatchReference() override;
	virtual void HandleWatchTake() override;

	// HUD
	virtual FString GetBrief() const override;
	const TArray<FDCRefShot>& GetShots() const { return Shots; }
	int32 GetShotIndexAt(float Time) const;
	EDCReplay GetReplay() const { return Replay; }
	bool IsShowingFilm() const { return Phase == EDCPhase::Watching || Replay != EDCReplay::None; }
	float GetLiveMatch() const { return LiveMatch; }
	float GetOverallScore() const { return OverallScore; }
	float GetBestOverall() const { return BestOverall; }
	int32 GetSprayCount() const { return SprayMarks.Num(); }
	bool HasRecording() const { return Recording.Num() > 1; }
	static FString GradeFor(float Score);
	virtual FString GetShotTip(const FDCRefShot& Shot) const;

protected:
	virtual bool UsesBeats() const override { return false; }
	virtual bool SupportsTool(EDCRigTool InTool) const override { return true; }
	virtual void EvaluateBeats(float PrevTime, float NowTime) override;
	virtual void EndTake() override;
	virtual void ResetScene() override;
	virtual void UpdatePrepPreview() override;

private:
	void GatherReference();
	bool GetReferencePose(float Time, FVector& OutLocation, FRotator& OutRotation, float& OutFov) const;
	bool GetTakePose(float Time, FVector& OutLocation, FRotator& OutRotation, float& OutFov) const;
	void StartFilm(EDCReplay Mode);
	void StopFilm();
	void TickFilm(float DeltaSeconds);
	float ScoreSample(const FDCRefShot& Shot, const FVector& RefLoc, const FRotator& RefRot, float RefFov, const FVector& Loc, const FRotator& Rot, float Fov,
		float& OutFraming, float& OutPositionError, float& OutAngleError) const;
	float GetAspect() const;

	UPROPERTY()
	TObjectPtr<ACameraActor> FilmCamera;

	UPROPERTY()
	TArray<TObjectPtr<ADCSprayMark>> SprayMarks;

	UPROPERTY()
	TObjectPtr<ADCSprayMark> SprayGhost;

	TArray<FDCRefShot> Shots;
	TArray<FDCCamSample> Recording;

	EDCReplay Replay = EDCReplay::None;
	bool bPendingIntro = true;
	float NextSampleTime = 0.f;
	float LiveMatch = 0.f;
	float OverallScore = 0.f;
	float BestOverall = 0.f;
};
