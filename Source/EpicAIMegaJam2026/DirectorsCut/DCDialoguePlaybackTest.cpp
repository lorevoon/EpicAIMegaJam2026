#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "DCMultiCamDirector.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Sound/SoundBase.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCDialoguePlaybackTest, "DirectorsCut.Dialogue.SceneClock",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCDialoguePlaybackTest::RunTest(const FString& Parameters)
{
 AutomationOpenMap(TEXT("/Game/DirectorsCut/Levels/L_DC_Act1_Promise"));
 const double Deadline = FPlatformTime::Seconds() + 60.;
 ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Deadline]()
 {
  UWorld* World = nullptr;
  for (const FWorldContext& C : GEngine->GetWorldContexts())
   if (C.WorldType == EWorldType::PIE) { World = C.World(); break; }
  if (!World || !World->HasBegunPlay())
  {
   if (FPlatformTime::Seconds() < Deadline) return false;
   AddError(TEXT("Act 1 PIE did not start")); return true;
  }
  ADCMultiCamDirector* D = nullptr;
  for (TActorIterator<ADCMultiCamDirector> It(World); It; ++It) { D = *It; break; }
  if (!TestNotNull(TEXT("Director exists"), D)) return true;
  D->bReferenceAutoStart = false;
  D->ResetScene();
  UAudioComponent* Audio = D->SceneDialogue.Get();
  if (!TestNotNull(TEXT("Map references dialogue audio"), Audio)) return true;
  TestNotNull(TEXT("Dialogue is imported"), Audio->Sound.Get());
  TestFalse(TEXT("Dialogue does not auto-play"), Audio->bAutoActivate);
  D->SceneTime = 1.f; D->UpdateSceneDialogue(0.f);
  TestEqual(TEXT("Rehearsal starts at scene offset"), D->DialoguePosition, 1.f);
  D->HandleRehearsalToggle(); D->UpdateSceneDialogue(.1f);
  TestFalse(TEXT("Paused rehearsal stops audio"), Audio->IsPlaying());
  TestEqual(TEXT("Paused audio must seek on resume"), D->DialoguePosition, -1.f);
  D->HandleScrub(4.f); D->HandleRehearsalToggle(); D->UpdateSceneDialogue(0.f);
  TestEqual(TEXT("Resume seeks to scrubbed scene time"), D->DialoguePosition, 5.f);
  D->SceneTime = 5.5f; D->UpdateSceneDialogue(.5f);
  TestEqual(TEXT("Rehearsal keeps normal audio rate"), Audio->PitchMultiplier, 1.f);
  D->Phase = EDCPhase::Countdown; D->UpdateSceneDialogue(.1f);
  TestFalse(TEXT("Countdown is silent"), Audio->IsPlaying());
  D->Phase = EDCPhase::Review; D->bPremierePaused = false;
  D->Recording.Reset();
  FDCTakeSample A; A.Time=2.f; A.FilmTime=0.f; A.Slot=0;
  FDCTakeSample B=A; B.Time=8.f; B.FilmTime=1.f; B.bJumpCut=true;
  FDCTakeSample C=B; C.Time=10.f; C.FilmTime=3.f; C.bJumpCut=false;
  D->Recording={A,B,C}; D->SceneTime=1.25f; D->UpdateSceneDialogue(.25f);
  TestEqual(TEXT("Review dialogue follows scene time across omitted footage"), D->DialoguePosition, 8.25f);
  D->bPremierePaused=true; D->UpdateSceneDialogue(.1f);
  TestFalse(TEXT("Paused review stops audio"), Audio->IsPlaying());
  D->Phase=EDCPhase::Reference; D->ReferenceCut=D->Recording;
  D->bReferencePlaying=true; D->bReferencePaused=false; D->ReferenceTime=1.5f;
  D->UpdateSceneDialogue(0.f);
  TestEqual(TEXT("Reference film uses mapped scene time"),D->DialoguePosition,8.5f);
  D->bReferencePaused=true; D->UpdateSceneDialogue(.1f);
  TestFalse(TEXT("Paused reference stops audio"),Audio->IsPlaying());
  D->ResetScene();
  TestEqual(TEXT("Retry resets dialogue transport"),D->DialoguePosition,-1.f);
  return true;
 }));
 return true;
}
#endif
