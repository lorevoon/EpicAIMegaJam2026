#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "DCMultiCamDirector.h"
#include "DCBeat.h"
#include "DCPerformer.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UnrealClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCAuthoredReferenceTest, "DirectorsCut.Reference.AuthoredRaid",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCAuthoredReferenceTest::RunTest(const FString& Parameters)
{
 AutomationOpenMap(TEXT("/Game/DirectorsCut/Levels/L_DC_Act2_BrokenPromise"));
 const double Deadline = FPlatformTime::Seconds() + 60.;
 ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Deadline]()
 {
  UWorld* World = nullptr;
  for (const FWorldContext& C : GEngine->GetWorldContexts())
   if (C.WorldType == EWorldType::PIE) { World = C.World(); break; }
  if (!World || !World->HasBegunPlay())
  {
   if (FPlatformTime::Seconds() < Deadline) return false;
   AddError(TEXT("Act 2 PIE did not start")); return true;
  }
  ADCMultiCamDirector* D = nullptr;
  for (TActorIterator<ADCMultiCamDirector> It(World); It; ++It) { D = *It; break; }
  if (!TestNotNull(TEXT("Act 2 director"), D)) return true;
  D->bReferenceAutoStart = false;
  D->ResetScene();
  D->SceneTime = 11.f;
  D->EvaluatePerformers(11.f);
  TMap<ADCPerformer*, FTransform> Before;
  for (ADCPerformer* P : D->Performers) Before.Add(P, P->GetActorTransform());
  D->BuildReferenceCut();
  TestEqual(TEXT("Five story scenes"), D->Beats.Num(), 5);
  TestEqual(TEXT("Five authored story sections"), D->ReferenceShotBeats.Num(), 5);
  TestTrue(TEXT("Reference remains 38 seconds"), FMath::IsNearlyEqual(D->GetReferenceLength(), 38.f, .001f));
  if (D->ReferenceCut.IsEmpty()) { AddError(TEXT("No reference frames")); return true; }
  TSet<int32> Slots;
  TArray<FVector> Positions;
  float MinimumRate = 10.f, MaximumRate = 0.f;
  for (int32 i = 0; i < D->ReferenceCut.Num(); ++i)
  {
   const FDCTakeSample& S = D->ReferenceCut[i];
   if (!Slots.Contains(S.Slot)) { Slots.Add(S.Slot); Positions.Add(S.Location); }
   if (i > 0)
   {
    const FDCTakeSample& P = D->ReferenceCut[i - 1];
    if (!TestTrue(TEXT("Scene and film clocks both strictly advance"), S.Time > P.Time && S.FilmTime > P.FilmTime)) break;
    const float Rate = (S.Time - P.Time) / (S.FilmTime - P.FilmTime);
    MinimumRate = FMath::Min(MinimumRate, Rate); MaximumRate = FMath::Max(MaximumRate, Rate);
    if (S.Time >= 26.05f && S.Time <= 37.8f)
     TestTrue(TEXT("Death scream and final dialogue stay at natural speed"), FMath::IsNearlyEqual(Rate, 1.f, .001f));
    if (S.Time > 18.1f)
     TestTrue(TEXT("Fight, fall and Juliet approach retain one camera position"), S.Location.Equals(D->ReferenceCut[550].Location, .01f));
    if (S.Time > 28.9f && S.Time < 29.2f)
     TestTrue(TEXT("Following Juliet creates no camera snap"), S.Rotation.Quaternion().AngularDistance(P.Rotation.Quaternion()) < FMath::DegreesToRadians(4.f));
   }
  }
  TestEqual(TEXT("All five story sections used"), Slots.Num(), 5);
  if (Positions.Num() == 5)
   TestTrue(TEXT("Ending continues the combat angle"), Positions[3].Equals(Positions[4], .01f));
  TestTrue(TEXT("Fall zoom opens the frame"), D->ReferenceCut[850].FilmFOV > D->ReferenceCut[580].FilmFOV + 3.f);
  TestTrue(TEXT("Ending zoom moves closer"), D->ReferenceCut[1100].FilmFOV < D->ReferenceCut[880].FilmFOV - 5.f);
  TestTrue(TEXT("Impact is slowed"), MinimumRate < .85f && MinimumRate >= .25f);
  TestTrue(TEXT("Remaining action compensates without a large speed-up"), MaximumRate > 1.f && MaximumRate < 1.2f);
  if (D->ReferenceShotBeats.Num() == 5)
  {
   TSet<int32> StoryBeats; for (int32 B : D->ReferenceShotBeats) StoryBeats.Add(B);
   TestEqual(TEXT("All five story scenes covered"), StoryBeats.Num(), 5);
  }
  for (float Scene : {0.f, 7.5f, 10.f, 18.f, 23.5f, 26.f, 29.f, 35.f, 38.f})
  {
   FVector P; FQuat R; float FOV = 0.f, Recovered = 0.f;
   const float Film = D->ReferenceFilmTimeAtSceneTime(Scene);
   TestTrue(TEXT("Inverse reference time mapping has a frame"), D->GetFilmView(D->ReferenceCut, Film, P, R, FOV, Recovered));
   TestTrue(TEXT("Inset and grading recover the original scene time, including cuts"), FMath::IsNearlyEqual(Scene, Recovered, .001f));
  }
  for (const auto& Pair : Before) TestTrue(TEXT("Building the film preserves rehearsal pose"), Pair.Key->GetActorTransform().Equals(Pair.Value, .01));
  // Main's picture-based scoring and live coach consume scene time. A slowed
  // reference must not leave them inspecting the previous shot's characters.
  const float SubjectTimes[] = {6.01f, 10.01f, 18.01f, 29.01f};
  const TCHAR* SubjectNames[] = {TEXT("Mr. Smith"), TEXT("Juliet"), TEXT("Armoured monster"), TEXT("Juliet")};
  for (int32 i = 0; i < UE_ARRAY_COUNT(SubjectTimes); ++i)
  {
   D->EvaluatePerformers(SubjectTimes[i]);
   TArray<FVector> Points; D->GetSubjectPoints(SubjectTimes[i], Points);
   for (ADCPerformer* P : D->Performers)
    if (P->GetActorLabel() == SubjectNames[i])
     TestTrue(TEXT("Scoring subjects switch on the scene clock"), Points.ContainsByPredicate([P](const FVector& Point)
      { return Point.Equals(P->GetAnchor(EDCAnchorPart::Head)->GetComponentLocation(), .01f); }));
  }
  D->Phase = EDCPhase::Storyboard; D->bReferencePlaying = true; D->bReferencePaused = false;
  D->ReferenceTime = D->ReferenceFilmTimeAtSceneTime(25.5f); D->UpdateSceneDialogue(0.f);
  if (TestNotNull(TEXT("Synchronized raid audio"), D->SceneDialogue.Get()))
   TestTrue(TEXT("Sound slows with the fatal strike"), D->SceneDialogue->PitchMultiplier > .25f && D->SceneDialogue->PitchMultiplier < .85f);
  D->ReferenceTime = D->ReferenceFilmTimeAtSceneTime(26.2f); D->UpdateSceneDialogue(0.f);
  TestTrue(TEXT("Juliet's Romeo scream has natural pitch"), FMath::IsNearlyEqual(D->SceneDialogue->PitchMultiplier, 1.f, .001f));
  // Film timestamps lose precision late in the take. Their per-frame derivative
  // must not repeatedly change pitch or trigger the transport's rate-change seek.
  D->ReferenceTime = D->ReferenceFilmTimeAtSceneTime(30.f);
  D->UpdateSceneDialogue(0.f);
  const float StableRate = D->DialogueRate;
  float PreviousFilm = D->ReferenceTime;
  for (int32 Frame = 901; Frame <= 1110; ++Frame)
  {
   const float Film = D->ReferenceFilmTimeAtSceneTime(Frame / 30.f);
   const float Delta = Film - PreviousFilm;
   const float Expected = D->DialoguePosition + Delta * D->DialogueRate;
   D->ReferenceTime = Film;
   D->UpdateSceneDialogue(Delta);
   TestEqual(TEXT("Normal-speed audio rate is stable across late-film frame quantization"), D->DialogueRate, StableRate);
   TestTrue(TEXT("Continuous playback stays below the transport seek threshold"), FMath::Abs(D->DialoguePosition - Expected) < .01f);
   PreviousFilm = Film;
  }
  D->ReferenceTime = D->ReferenceFilmTimeAtSceneTime(35.f);
  const ADCBeat* Ending = D->GetReferenceBeat();
  TestTrue(TEXT("Ending caption follows scene time after slow motion"), Ending && Ending->BeatId == TEXT("2.5"));
  D->ResetScene();
  return true;
 }));
 // Optional rendered evidence uses the same runtime film as the player's reference.
 if (FParse::Param(FCommandLine::Get(), TEXT("Act2VisualQA")))
 {
  for (float Moment : {0.5f, 6.5f, 11.f, 24.f, 28.f, 35.f, 37.f})
  {
   ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Moment]()
   {
    for (const FWorldContext& C : GEngine->GetWorldContexts())
     if (C.WorldType == EWorldType::PIE)
      for (TActorIterator<ADCMultiCamDirector> It(C.World()); It; ++It)
      {
       ADCMultiCamDirector* D = *It;
       D->Phase = EDCPhase::Storyboard;
       D->StartReferenceCut(); D->bReferencePaused = true;
       D->ReferenceTime = D->ReferenceFilmTimeAtSceneTime(Moment);
       D->ShowFilmFrame(D->ReferenceCut, D->ReferenceTime);
       return true;
      }
    return true;
   }));
   ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
   ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Moment]()
   {
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Act2/ActingRevision4") /
     FString::Printf(TEXT("reference_%04d.png"), FMath::RoundToInt(Moment * 10)), false, false);
    return true;
   }));
   ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.5f));
  }
 }
 return true;
}
#endif
