#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "DCCrewMember.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDCCrewTest, "DirectorsCut.LivelySet",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
void FDCCrewTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
 Names.Add(TEXT("Act1")); Commands.Add(TEXT("L_DC_Act1_Promise"));
 Names.Add(TEXT("Act2")); Commands.Add(TEXT("L_DC_Act2_BrokenPromise"));
}
bool FDCCrewTest::RunTest(const FString& Parameters)
{
 AutomationOpenMap(TEXT("/Game/DirectorsCut/Levels/") + Parameters);
 const double Deadline=FPlatformTime::Seconds()+60.;
 ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Deadline]()
 {
  UWorld* World=nullptr;
  for (const FWorldContext& C : GEngine->GetWorldContexts())
   if (C.WorldType==EWorldType::PIE) {World=C.World();break;}
  int32 Count=0, Directors=0;
  if (World && World->HasBegunPlay())
   for (TActorIterator<ADCCrewMember> It(World); It; ++It) ++Count;
  if (Count<9 && FPlatformTime::Seconds()<Deadline) return false;
  TestEqual(TEXT("Nine crew including director"),Count,9);
  if (!World) {AddError(TEXT("No play world"));return true;}
  for (TActorIterator<ADCCrewMember> It(World); It; ++It)
  {
   TestTrue(TEXT("Crew stays in set layer"),It->ActorHasTag(TEXT("DC_SetOnly")));
   TestNotNull(TEXT("Activity animation"),It->Activity.Get());
   TestTrue(TEXT("Natural dialogue assigned"),It->Lines.Num()>=2);
   auto* Instance=It->Character->GetSingleNodeInstance();
   if (!TestNotNull(TEXT("Single node playback"),Instance)) continue;
   TestTrue(TEXT("Activity running"),Instance->GetCurrentAsset()==It->Activity);
   if (It->bDirector)
   {
    ++Directors; if (!TestNotNull(TEXT("Thumbs-up clip"),It->Celebration.Get())) continue;
    It->PreviewActivity(1.f, false);
    const FVector IdleHand = It->Character->GetSocketLocation(TEXT("RightHand"));
    It->PreviewActivity(1.6f, true);
    TestTrue(TEXT("Success visibly moves the director's hand"), FVector::Dist(IdleHand, It->Character->GetSocketLocation(TEXT("RightHand"))) > 15.f);
    It->Celebrate();TestTrue(TEXT("Success switches to thumbs-up"),Instance->GetCurrentAsset()==It->Celebration);
    It->Tick(It->Celebration->GetPlayLength()+.1f);
    TestTrue(TEXT("Returns to idle"),Instance->GetCurrentAsset()==It->Activity);
   }
  }
  TestEqual(TEXT("Exactly one visible director character"),Directors,1);
  return true;
 }));
 return true;
}
#endif
