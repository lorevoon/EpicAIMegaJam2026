#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCCinemaReplay.generated.h"

class ADCMultiCamDirector;
class ADCZombieDirector;
class AHUD;
class UCanvas;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class ACameraActor;

UENUM(BlueprintType)
enum class EDCCinemaMood : uint8 { Neutral, Love, Sad };

/** Lives in L_DC_Cinema. The action stays loaded so the actual recorded take can be projected. */
UCLASS()
class ADCCinemaReplay : public AActor
{
	GENERATED_BODY()
public:
	ADCCinemaReplay();
	virtual void Tick(float DeltaSeconds) override;
	void BeginScreening(ADCMultiCamDirector* InDirector);
	void EndScreening();
	void ProjectFrame(ACameraActor* Camera);
	void DrawCinema(AHUD* HUD, UCanvas* Canvas, ADCZombieDirector* Film);
	bool IsOpening() const { return bActive && PresentationTime < OpeningDuration; }
	float GetResultAge() const { return ResultAge; }
	bool IsActive() const { return bActive; }
	/** Evaluated in original scene seconds, then mapped through recording pauses into film time. */
	UFUNCTION(BlueprintCallable, Category="Cinema|Missions")
	void SetMissionOutcome(FName BeatId, bool bSucceeded);
	void ClearMissionOutcomes() { MissionOutcomes.Reset(); }
	bool HasPositiveReaction(FName BeatId, bool bStoryboardCaptured) const;
	static float SceneToFilmTime(const ADCMultiCamDirector* Film, float SceneSeconds);

	UPROPERTY(EditAnywhere, Category="Cinema", meta=(ClampMin="0"))
	float OpeningDuration = 3.5f;
	UPROPERTY(EditAnywhere, Category="Cinema", meta=(ClampMin="0.5"))
	float ReactionDuration = 2.8f;

private:
	UPROPERTY() TObjectPtr<USceneCaptureComponent2D> Projector;
	UPROPERTY() TObjectPtr<UTextureRenderTarget2D> FilmTexture;
	UPROPERTY() TObjectPtr<ADCMultiCamDirector> Director;
	TMap<FName, bool> MissionOutcomes;
	float PresentationTime = 0.f;
	float ResultAge = -1.f;
	bool bActive = false;
	int32 FramesToSettle = 0;
};
