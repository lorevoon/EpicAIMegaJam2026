#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCCrewMember.generated.h"
class USkeletalMeshComponent;
class UAnimSequence;
UCLASS()
class ADCCrewMember : public AActor
{
 GENERATED_BODY()
public:
 ADCCrewMember();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USkeletalMeshComponent> Character;
 UPROPERTY(EditAnywhere, Category="Crew") TObjectPtr<UAnimSequence> Activity;
 UPROPERTY(EditAnywhere, Category="Crew") TObjectPtr<UAnimSequence> Celebration;
 UPROPERTY(EditAnywhere, Category="Crew") bool bDirector = false;
 UPROPERTY(EditAnywhere, Category="Crew") FString CrewRole;
 UPROPERTY(EditAnywhere, Category="Crew") TArray<FString> Lines;
 UPROPERTY(EditAnywhere, Category="Crew") float HeadHeight = 195.f;
 void Celebrate();
 UFUNCTION(CallInEditor, Category="Crew")
 void PreviewActivity(float Time = 0.f, bool bSuccess = false);
 FString GetDialogue() const;
private:
 float CelebrationRemaining = 0.f;
};
