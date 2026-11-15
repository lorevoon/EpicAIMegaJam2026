#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "DCMultiCamDirector.h"
#include "DCBeat.h"
#include "DCPerformer.h"
#include "DCSceneMarks.h"
#include "GameFramework/Pawn.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCAct1PerformanceTest, "DirectorsCut.Reference.PairedPerformance", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCAct1PerformanceTest::RunTest(const FString& Parameters)
{
 AutomationOpenMap(TEXT("/Game/DirectorsCut/Levels/L_DC_Act1_Promise"));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(8.f));
 ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
 {
  for (const FWorldContext& Context : GEngine->GetWorldContexts()) if (Context.WorldType == EWorldType::PIE)
  {
   UWorld* World = Context.World();
   const FTransform Start = ADCStartMark::FindStartTransform(World);
   FBox2D Studio;
   for (TActorIterator<ADCMultiCamDirector> It(World); It; ++It) { It->BuildReferenceCut(); Studio = It->ReferenceStageBounds; break; }
   TestTrue(TEXT("Spawn is inside studio walls"), Studio.bIsValid && Studio.IsInside(FVector2D(Start.GetLocation())));
   APawn* Pawn = World->GetFirstPlayerController()->GetPawn();
   if (TestNotNull(TEXT("Preparation pawn"), Pawn)) TestTrue(TEXT("Preparation spawns at studio"), FVector::Dist2D(Pawn->GetActorLocation(), Start.GetLocation()) < 100.f);
   for (TActorIterator<ADCStartMark> It(World); It; ++It) TestTrue(TEXT("Start mark invisible in gameplay"), It->IsHidden());
   ADCStartMark* Mark = nullptr; for (TActorIterator<ADCStartMark> It(World); It; ++It) {Mark=*It; break;}
   if (Mark) Mark->Destroy();
   const FTransform Fallback = ADCStartMark::FindStartTransform(World);
   TestTrue(TEXT("Fallback is inside studio walls"), Studio.bIsValid && Studio.IsInside(FVector2D(Fallback.GetLocation())));
   TestTrue(TEXT("Missing marker fallback stays near the set"), FVector::Dist2D(Start.GetLocation(), Fallback.GetLocation()) < 500.f);
  }
  return true;
 }));
  for (float Moment : {0.f, .4f, .8f, 1.2f, 2.f, 4.7f, 12.5f, 14.8f, 15.3f, 15.8f, 16.5f, 18.5f, 19.5f})
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
       if (FParse::Param(FCommandLine::Get(), TEXT("Act1DetailQA")))
        for (FDCTakeSample& Frame : D->ReferenceCut) Frame.FilmFOV = Moment >= 16.f ? 12.f : 35.f;
       if (FParse::Param(FCommandLine::Get(), TEXT("Act1FaceQA")) && Moment < 2.f)
       {
        D->EvaluatePerformers(Moment);
        for (ADCPerformer* Actor : D->Performers) if (Actor->GetActorLabel() == TEXT("Romeo"))
        {
         const FVector Head = Actor->GetComponentByClass<USkeletalMeshComponent>()->GetSocketLocation(TEXT("head_joint"));
         for (FDCTakeSample& Frame : D->ReferenceCut) { Frame.FilmFOV = 8.f; Frame.Rotation = (Head - Frame.Location).Rotation(); }
        }
       }
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
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Act1Revision14/Preview") /
     FString::Printf(TEXT("reference_%04d.png"), FMath::RoundToInt(Moment * 10)), false, false);
    return true;
   }));
   ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.5f));
  }

 return true;
}
#endif

