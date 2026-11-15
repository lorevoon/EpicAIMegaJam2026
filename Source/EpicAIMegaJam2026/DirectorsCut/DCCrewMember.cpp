#include "DCCrewMember.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/World.h"
ADCCrewMember::ADCCrewMember()
{
 PrimaryActorTick.bCanEverTick = true;
 Character = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Character"));
 RootComponent = Character;
 Character->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Character->SetAnimationMode(EAnimationMode::AnimationSingleNode);
 Tags.Add(TEXT("DC_SetOnly"));
}
void ADCCrewMember::BeginPlay()
{
 Super::BeginPlay();
 if (Activity) { Character->PlayAnimation(Activity, true); Character->SetPosition(FMath::FRandRange(0.f, Activity->GetPlayLength()), false); }
}
void ADCCrewMember::Celebrate()
{
 if (!bDirector || !Celebration) return;
 Character->PlayAnimation(Celebration, false);
 CelebrationRemaining = Celebration->GetPlayLength();
}
void ADCCrewMember::PreviewActivity(float Time, bool bSuccess)
{
 UAnimSequence* Clip = bSuccess ? Celebration.Get() : Activity.Get();
 if (!Clip) return;
 Character->PlayAnimation(Clip, !bSuccess);
 Character->SetPosition(Time, false);
 Character->TickAnimation(0.f, false);
 Character->RefreshBoneTransforms();
}
void ADCCrewMember::Tick(float DeltaSeconds)
{
 Super::Tick(DeltaSeconds);
 if (CelebrationRemaining > 0.f)
 {
  CelebrationRemaining -= DeltaSeconds;
  if (CelebrationRemaining <= 0.f && Activity) Character->PlayAnimation(Activity, true);
 }
}
FString ADCCrewMember::GetDialogue() const
{
 if (CelebrationRemaining > 0.f) return TEXT("That's the shot. Keep that camera right there!");
 return Lines.IsEmpty() ? FString() : Lines[(int32(GetWorld()->GetTimeSeconds() / 7.f) + GetUniqueID()) % Lines.Num()];
}
