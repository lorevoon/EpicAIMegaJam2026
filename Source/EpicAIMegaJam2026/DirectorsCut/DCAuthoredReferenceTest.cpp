#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "DCMultiCamDirector.h"
#include "DCBeat.h"
#include "DCPerformer.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UnrealClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "GameFramework/HUD.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "GameFramework/PlayerController.h"

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
    if (S.Time > 19.1f && S.Time < 25.9f)
     TestTrue(TEXT("Fight and fall retain one camera position"), S.Location.Equals(D->ReferenceCut[580].Location, .01f));
   }
  }
  TestEqual(TEXT("All five story sections used"), Slots.Num(), 5);
  TestEqual(TEXT("Smith falls within the continuous raid shot"), D->ReferenceCut[240].Slot, D->ReferenceCut[0].Slot);
  TestTrue(TEXT("Raid camera dollies without a cut"), FVector::Dist(D->ReferenceCut[0].Location, D->ReferenceCut[240].Location) > 100.f);
  TestEqual(TEXT("Juliet has her own shot at sixteen seconds"), D->ReferenceCut[480].Slot, 2);
  TestEqual(TEXT("Duel camera starts at nineteen seconds"), D->ReferenceCut[571].Slot, 3);
  for (const FVector& Position : Positions)
   TestTrue(TEXT("Reference camera stays inside the studio walls"), D->ReferenceStageBounds.IsInside(FVector2D(Position)));
  // Every scene cuts to a camera of its own, too far from the others for one placed camera to stand on two spots.
  for (int32 i = 0; i < Positions.Num(); ++i)
   for (int32 j = i + 1; j < Positions.Num(); ++j)
    TestTrue(TEXT("Scene camera zones do not overlap"), FVector::Dist2D(Positions[i], Positions[j]) >= D->GetReferenceShotSpacing());
  TestTrue(TEXT("Fall zoom opens the frame"), D->ReferenceCut[850].FilmFOV > D->ReferenceCut[580].FilmFOV + 3.f);
  TestTrue(TEXT("Ending zoom moves closer"), D->ReferenceCut[1100].FilmFOV < D->ReferenceCut[880].FilmFOV - 5.f);
  TestTrue(TEXT("All action stays at natural speed"), FMath::IsNearlyEqual(MinimumRate, 1.f, .001f));
  TestTrue(TEXT("No compensating speed-up"), FMath::IsNearlyEqual(MaximumRate, 1.f, .001f));
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
  // The charge must come to the couple, and arrows must survive reverse seeking.
  ADCPerformer* Romeo = nullptr;
  ADCPerformer* Killer = nullptr;
  for (ADCPerformer* P : D->Performers)
  {
   if (P->GetActorLabel() == TEXT("Romeo")) Romeo = P;
   if (P->GetActorLabel() == TEXT("Armoured monster")) Killer = P;
  }
  if (TestNotNull(TEXT("Romeo"), Romeo) && TestNotNull(TEXT("Smith's killer"), Killer))
  {
   D->EvaluatePerformers(13.f); const FVector Stand = Romeo->GetActorLocation();
   const float BeforeCharge = FVector::Dist2D(Stand, Killer->GetActorLocation());
   D->EvaluatePerformers(18.8f);
   TestTrue(TEXT("Romeo holds his ground"), FVector::Dist2D(Stand, Romeo->GetActorLocation()) < 1.f);
   TestTrue(TEXT("Smith's killer charges into sword range"), BeforeCharge > 700.f && FVector::Dist2D(Stand, Killer->GetActorLocation()) < 200.f);
  }
  int32 ArrowCount = 0;
  for (ADCPerformer* P : D->Performers)
  {
   if (!P->bPropOnly) continue;
   ++ArrowCount;
   UStaticMeshComponent* Prop = nullptr;
   TArray<UStaticMeshComponent*> Components; P->GetComponents(Components);
   for (UStaticMeshComponent* C : Components) if (C->GetFName() == TEXT("Prop")) Prop = C;
   if (!TestNotNull(TEXT("Arrow mesh component"), Prop)) continue;
   P->Evaluate(P->PropVisibleFrom - .1f); TestFalse(TEXT("No arrow before drawing"), Prop->IsVisible());
   P->Evaluate(P->PropVisibleFrom + .75f); const FTransform Flying = Prop->GetComponentTransform();
   TestTrue(TEXT("Arrow visible in flight"), Prop->IsVisible());
   P->Evaluate(37.f); TestFalse(TEXT("Arrow clears after passing through victim"), Prop->IsVisible());
   P->Evaluate(P->PropVisibleFrom + .75f);
   TestTrue(TEXT("Reverse seek restores the exact arrow pose"), Prop->GetComponentTransform().Equals(Flying, .01f));
  }
  TestEqual(TEXT("Juliet fires two real arrows"), ArrowCount, 2);
  // Main's picture-based scoring and live coach consume scene time. A slowed
  // reference must not leave them inspecting the previous shot's characters.
  const float SubjectTimes[] = {6.01f, 10.01f, 19.01f, 29.01f};
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
  D->Phase = EDCPhase::Reference; D->bReferencePlaying = true; D->bReferencePaused = false;
  D->ReferenceTime = D->ReferenceFilmTimeAtSceneTime(25.5f); D->UpdateSceneDialogue(0.f);
  if (TestNotNull(TEXT("Synchronized raid audio"), D->SceneDialogue.Get()))
   TestTrue(TEXT("Fight audio stays at natural pitch"), FMath::IsNearlyEqual(D->SceneDialogue->PitchMultiplier, 1.f, .001f));
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
  for (float Moment : {0.5f, 3.f, 5.8f, 7.8f, 9.f, 10.5f, 13.f, 15.5f, 17.7f, 18.1f, 18.8f, 19.5f, 22.5f, 25.5f, 26.8f, 27.8f, 29.5f, 35.f})
  {
   ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Moment]()
   {
    for (const FWorldContext& C : GEngine->GetWorldContexts())
     if (C.WorldType == EWorldType::PIE)
      for (TActorIterator<ADCMultiCamDirector> It(C.World()); It; ++It)
      {
       ADCMultiCamDirector* D = *It;
       D->Phase = EDCPhase::Reference;
       D->StartReferenceCut(); D->bReferencePaused = true;
       D->ReferenceTime = D->ReferenceFilmTimeAtSceneTime(Moment);
       D->ShowFilmFrame(D->ReferenceCut, D->ReferenceTime);
       if (APlayerController* PC = D->GetPC())
        if (AHUD* HUD = PC->GetHUD()) HUD->bShowHUD = false;
       return true;
      }
    return true;
   }));
   ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
   ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Moment]()
   {
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("PerformanceRevision11/Act2") /
     FString::Printf(TEXT("reference_%04d.png"), FMath::RoundToInt(Moment * 10)), false, false);
    return true;
   }));
   ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.5f));
  }
 }
 if (FParse::Param(FCommandLine::Get(), TEXT("Act2PrepQA")))
 {
  for (float Moment : {13.f, 17.f, 20.f, 24.f, 31.f, 35.f})
  {
   ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Moment]()
   {
    for (const FWorldContext& C : GEngine->GetWorldContexts())
     if (C.WorldType == EWorldType::PIE)
      for (TActorIterator<ADCMultiCamDirector> It(C.World()); It; ++It)
      {
       ADCMultiCamDirector* D = *It;
       D->ResetScene(); D->bRehearsalPlaying = false; D->SetReviewEnvironment(false);
       D->Phase = EDCPhase::Preparation; D->SceneTime = Moment;
       D->EvaluatePerformers(Moment); D->UpdateReferenceInset(1.f);
      }
    return true;
   }));
   ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
   ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Moment]()
   {
    for (const FWorldContext& C : GEngine->GetWorldContexts())
     if (C.WorldType == EWorldType::PIE)
      for (TActorIterator<ADCMultiCamDirector> It(C.World()); It; ++It)
      {
       ADCMultiCamDirector* D = *It;
       UTextureRenderTarget2D* Target = D->ReferenceInsetTarget;
       if (!TestNotNull(TEXT("Preparation reference render target"), Target)) continue;
       TestTrue(TEXT("Preparation capture follows scene clock"), FMath::IsNearlyEqual(D->LastReferenceInsetSceneTime, Moment, .001f));
       TArray<FColor> Pixels;
       if (TestTrue(TEXT("Preparation inset renders pixels"), Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels)))
       {
        TArray64<uint8> PNG;
        FImageUtils::PNGCompressImageArray(Target->SizeX, Target->SizeY, Pixels, PNG);
        const FString Path = FPaths::ProjectSavedDir() / TEXT("PerformanceRevision11/Act2") / FString::Printf(TEXT("preparation_%04d.png"), FMath::RoundToInt(Moment * 10));
        FFileHelper::SaveArrayToFile(PNG, *Path);
       }
      }
    return true;
   }));
  }
 }

 return true;
}
#endif
