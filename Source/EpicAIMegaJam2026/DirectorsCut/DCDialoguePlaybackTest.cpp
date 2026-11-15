#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "DCMultiCamDirector.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Sound/SoundBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

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
  TestEqual(TEXT("Preparation dialogue stays muted"), D->DialoguePosition, -1.f);
  D->HandleRehearsalToggle(); D->UpdateSceneDialogue(.1f);
  TestFalse(TEXT("Paused rehearsal stops audio"), Audio->IsPlaying());
  TestEqual(TEXT("Paused audio must seek on resume"), D->DialoguePosition, -1.f);
  D->HandleScrub(4.f); D->HandleRehearsalToggle(); D->UpdateSceneDialogue(0.f);
  TestEqual(TEXT("Scrubbing and resuming preparation stays muted"), D->DialoguePosition, -1.f);
  D->SceneTime = 5.5f; D->UpdateSceneDialogue(.5f);
  TestEqual(TEXT("Rehearsal keeps normal audio rate"), Audio->PitchMultiplier, 1.f);
  D->Phase = EDCPhase::Countdown; D->UpdateSceneDialogue(.1f);
  TestFalse(TEXT("Countdown is silent"), Audio->IsPlaying());
  D->Phase = EDCPhase::Review; D->bPremierePaused = false;
  D->Recording.Reset();
  FDCTakeSample A; A.Time=2.f; A.FilmTime=0.f; A.Slot=0;
  FDCTakeSample B=A; B.Time=8.f; B.FilmTime=1.f; B.bJumpCut=true; B.Slot=1;
  FDCTakeSample C=B; C.Time=10.f; C.FilmTime=3.f; C.bJumpCut=false;
  D->Recording={A,B,C}; D->SceneTime=1.25f; D->UpdateSceneDialogue(.25f);
  TestEqual(TEXT("Review dialogue follows scene time across omitted footage"), D->DialoguePosition, 8.25f);
  D->bPremierePaused=true; D->UpdateSceneDialogue(.1f);
  TestFalse(TEXT("Paused review stops audio"), Audio->IsPlaying());
  D->Phase=EDCPhase::Reference; D->ReferenceCut=D->Recording;
  const TArray<float> SceneCuts = D->GetReferenceCameraCutTimes(true);
  const TArray<float> FilmCuts = D->GetReferenceCameraCutTimes(false);
  TestEqual(TEXT("One bar per camera change, not per sample"), SceneCuts.Num(), 1);
  if (SceneCuts.Num() == 1 && FilmCuts.Num() == 1)
  {
   TestEqual(TEXT("Preparation bar uses scene time"), SceneCuts[0], 8.f);
   TestEqual(TEXT("Reference bar uses film time"), FilmCuts[0], 1.f);
  }
  D->bReferencePlaying=true; D->bReferencePaused=false; D->ReferenceTime=1.5f;
  D->UpdateSceneDialogue(0.f);
  TestEqual(TEXT("Reference film uses mapped scene time"),D->DialoguePosition,8.5f);
  D->bReferencePaused=true; D->UpdateSceneDialogue(.1f);
  TestFalse(TEXT("Paused reference stops audio"),Audio->IsPlaying());
  D->ResetScene();
  TestEqual(TEXT("Retry resets dialogue transport"),D->DialoguePosition,-1.f);
  D->StartSetOverview();
  TestTrue(TEXT("Preparation starts with a slate"), D->IsSetOverview());
  APlayerController* PC = World->GetFirstPlayerController();
  TestTrue(TEXT("Slate retains walking view instead of bird's-eye view"), PC && PC->GetViewTarget() == PC->GetPawn());
  const float BeforeSlate = D->SceneTime;
  D->Tick(2.5f);
  TestFalse(TEXT("Slate completes and unlocks preparation"), D->IsSetOverview());
  TestEqual(TEXT("Slate does not advance rehearsal"), D->SceneTime, BeforeSlate);
  return true;
 }));
 if (FParse::Param(FCommandLine::Get(), TEXT("DCRevisionVisuals")))
 {
  for (int32 Stage = 0; Stage < 3; ++Stage)
  {
   ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Stage]()
   {
    for (const FWorldContext& Context : GEngine->GetWorldContexts())
     if (Context.WorldType == EWorldType::PIE)
      for (TActorIterator<ADCMultiCamDirector> It(Context.World()); It; ++It)
      {
       ADCMultiCamDirector* D = *It;
       D->bInMainMenu = false; D->Cutscene = nullptr; D->FadeAlpha = 0.f; D->FadeDirection = 0;
       D->SetActorTickEnabled(false);
       if (Stage == 0) { D->BuildReferenceCut(); D->StartSetOverview(); D->OverviewRemaining = 1.5f; }
       if (Stage == 1) { D->OverviewRemaining = 0.f; D->Phase = EDCPhase::Preparation; }
       if (Stage == 2) { D->Phase = EDCPhase::Reference; D->StartReferenceCut(); D->bReferencePaused = true; }
      }
    return true;
   }));
   ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
   ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Stage]()
   {
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("LivelySet/Revision") / FString::Printf(TEXT("ui_%d.png"), Stage), false, false);
    return true;
   }));
   ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
  }
 }
 return true;
}
#endif
