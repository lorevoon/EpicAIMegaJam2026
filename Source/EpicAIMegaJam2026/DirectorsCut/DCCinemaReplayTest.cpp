#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "DCCinemaReplay.h"
#include "DCMultiCamDirector.h"
#include "DCBeat.h"
#include "DCPerformer.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "EngineUtils.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCCinemaReplayTest,"DirectorsCut.Replay.Cinema",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCCinemaReplayTest::RunTest(const FString& Parameters)
{
 AutomationOpenMap(TEXT("/Game/DirectorsCut/Levels/L_DC_Act1_Promise"));
 const double Deadline=FPlatformTime::Seconds()+90;
 ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this,Deadline]()
 {
  UWorld* W=nullptr;
  for(const auto& C:GEngine->GetWorldContexts()) if(C.WorldType==EWorldType::PIE) { W=C.World(); break; }
  if(!W || !W->HasBegunPlay())
  {
   if(FPlatformTime::Seconds()<Deadline) return false;
   AddError(TEXT("Cinema test could not start PIE")); return true;
  }
  ADCMultiCamDirector* D=nullptr;
  for(TActorIterator<ADCMultiCamDirector> It(W);It;++It) { D=*It; break; }
  if(!TestNotNull(TEXT("Director exists"),D)) return true;
  D->ResetScene();
  for(int i=0;i<=200;++i)
  {
   FDCTakeSample A; A.Time=i*.1f; A.FilmTime=A.Time;
   A.Location=FVector(650,-1300,210);
   const float TargetY=FMath::Lerp(300.f,-600.f,FMath::Clamp((A.Time-7.f)/5.f,0.f,1.f));
   A.Rotation=(FVector(1150,TargetY,105)-A.Location).Rotation(); A.FilmFOV=DCFrame::CaptureFOV(90.f,16.f/9.f); A.Slot=0;
   D->Recording.Add(A);
  }
  TestEqual(TEXT("Scene to film maps exact recorded time"),ADCCinemaReplay::SceneToFilmTime(D,5.f),5.f);
  // Simulate a pause from original seconds 5 through 7.
  D->Recording.RemoveAll([](const FDCTakeSample& A){return A.Time>=5 && A.Time<7;});
  for(auto& A:D->Recording) if(A.Time>=7) { A.FilmTime-=2; A.bJumpCut=FMath::IsNearlyEqual(A.Time,7.f); }
  TestEqual(TEXT("Omitted mission maps to next visible frame"),ADCCinemaReplay::SceneToFilmTime(D,6.f),5.f);
  TestEqual(TEXT("Later mission accounts for pause"),ADCCinemaReplay::SceneToFilmTime(D,10.f),8.f);
  for(int i=0;i<D->Beats.Num();++i)
  {
   auto& B=D->Beats[i]; B.Status=i%2?EDCShotStatus::Missed:EDCShotStatus::Captured;
   B.CapturedFilmTime=ADCCinemaReplay::SceneToFilmTime(D,B.Beat->GetCaptureStartTime());
  }
  D->SetReplayMissionOutcome(TEXT("future-test"),false);
  D->StartPremiere();
  ADCCinemaReplay* Cinema=D->GetCinema();
  if(!TestNotNull(TEXT("Cinema loaded"),Cinema)) return true;
  TestTrue(TEXT("Cinema lives in a separate streamed scene"),Cinema->GetLevel()!=W->PersistentLevel);
  TestTrue(TEXT("Opening holds the recorded film"),Cinema->IsOpening());
  D->TickPremiere(1.f); TestEqual(TEXT("First movie second not consumed by title"),D->SceneTime,0.f);
  TestTrue(TEXT("Captured storyboard applauded"),Cinema->HasPositiveReaction(TEXT("unknown"),true));
  TestFalse(TEXT("Missing storyboard disapproved"),Cinema->HasPositiveReaction(TEXT("unknown"),false));
  TestFalse(TEXT("Failed mission overrides captured storyboard"),Cinema->HasPositiveReaction(TEXT("future-test"),true));
  Cinema->SetMissionOutcome(TEXT("future-test"),true);
  TestTrue(TEXT("Successful mission and capture applauded"),Cinema->HasPositiveReaction(TEXT("future-test"),true));
  TestFalse(TEXT("Mission alone cannot fix missing item"),Cinema->HasPositiveReaction(TEXT("future-test"),false));
  Cinema->Tick(4.f);
  D->bPremierePaused=true;
  D->SceneTime=11.f; D->ShowPremiereFrame(11.f);
  if(!D->Beats.IsEmpty()) {D->Beats[0].CapturedFilmTime=10.5f; D->Beats[0].Status=EDCShotStatus::Captured;}
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.f));
  ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([D]()
  {
   FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("CinemaAudit/Screening.png"),false,false);
   return true;
  }));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
  ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this,D,Cinema]()
  {
   D->FinishPremiere(); Cinema->Tick(5.f);
   TestTrue(TEXT("Skip reaches results"),D->IsFilmFinished());
   TestTrue(TEXT("Results animation advances"),Cinema->GetResultAge()>0);
   return true;
  }));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
  ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([]()
  {
   FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("CinemaAudit/Results.png"),false,false); return true;
  }));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
  ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this,D,Cinema]()
  {
   D->HandleTransportToggle();
   TestTrue(TEXT("Replay reopens curtains"),Cinema->IsOpening());
   TestFalse(TEXT("Replay leaves results"),D->IsFilmFinished());
   D->HandleScrub(4.f); TestEqual(TEXT("Scrub updates film clock"),D->SceneTime,4.f);
   Cinema->SetMissionOutcome(TEXT("future-test"),false);
   D->ResetScene();
   TestFalse(TEXT("Retry stops projector"),Cinema->IsActive());
   TestTrue(TEXT("Retry clears mission verdicts"),Cinema->HasPositiveReaction(TEXT("future-test"),true));
   D->StartPremiere(); TestTrue(TEXT("Empty recording goes to results"),D->IsFilmFinished());
   D->ResetScene(); return true;
  }));
  return true;
 }));
 return true;
}
#endif
