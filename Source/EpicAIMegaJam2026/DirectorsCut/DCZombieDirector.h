// Director's Cut - "Love in the Ruins": a romance shot in a zombie apocalypse.
// Built on the multi-camera director, with unlimited static and dolly cameras.
//
//   Storyboard  the scenes are listed in order (from the DCBeat display names) - no times, no places
//   Preparation place as many static / dolly cameras as you like; the scene only moves when the
//               player scrubs its timeline (mouse wheel or arrows), so cameras can be tested at any moment
//   Shooting    cut between cameras; storyboard beats on the live camera score, zombies on it cost points
//   Review      the recorded film plays back, then the evaluation

#pragma once

#include "CoreMinimal.h"
#include "DCMultiCamDirector.h"
#include "DCZombieDirector.generated.h"

UCLASS()
class ADCZombieDirector : public ADCMultiCamDirector
{
	GENERATED_BODY()

public:
	ADCZombieDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Points lost per second while any zombie is visible on the live camera. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Zombies", meta = (ClampMin = "0"))
	int32 ZombiePenaltyPerSecond = 150;

	/** Zombies farther than this from the camera are too small to notice. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Zombies", meta = (Units = "cm"))
	float ZombieSightRange = 4000.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	FString MovieTitle = TEXT("Love in the Ruins");

#if WITH_EDITORONLY_DATA
	/** Tick to place the default zombie-town scene into this level (same as the Build Zombie Scene button). */
	UPROPERTY(EditAnywhere, Transient, Category = "Directors Cut|Editor")
	bool bBuildZombieScene = false;
#endif

#if WITH_EDITOR
	/** Places the ruined town square, the two lovers, four zombies, their blocking, and the storyboard beats. */
	UFUNCTION(CallInEditor, Category = "Directors Cut|Editor")
	void BuildZombieScene();

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// Player commands
	virtual void HandleStartTake() override;
	virtual void HandleRestart() override;
	virtual void HandleScrub(float DeltaSeconds) override;


	// HUD
	bool IsZombieInShot() const { return bZombieInShot; }
	const FString& GetZombieInShotName() const { return ZombieInShotName; }
	float GetZombieSeconds() const { return ZombieSeconds; }
	int32 GetPenalty() const { return FMath::RoundToInt(ZombieSeconds * ZombiePenaltyPerSecond); }
	int32 GetBeatPoints() const;
	int32 GetNetScore() const { return NetScore; }
	int32 GetBestNet() const { return BestNet; }
	/** While planning and looking through a camera: storyboard beats it frames and whether a zombie shows, at the scrubbed time. */
	void GetPlanPreview(TArray<FString>& OutFramedBeats, bool& bOutZombie) const;

protected:
	virtual void EvaluateBeats(float PrevTime, float NowTime) override;
	virtual void EndTake() override;
	virtual void ResetScene() override;
	virtual bool IsLiveShotSpoiled() const override { return bZombieInShot; }

private:
	void EnterPreparation();
	bool IsZombieVisible(const ADCMultiCam* Cam, FString* OutName) const;
	float GetAspect() const;

	UPROPERTY()
	TArray<TObjectPtr<ADCPerformer>> Zombies;

	float ZombieSeconds = 0.f;
	bool bZombieInShot = false;
	FString ZombieInShotName;
	int32 NetScore = 0;
	int32 BestNet = 0;
};
