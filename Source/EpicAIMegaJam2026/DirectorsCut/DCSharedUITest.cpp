#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "DCZombieDirector.h"
#include "DCZombieHUD.h"
#include "DCMultiCam.h"
#include "DCMultiCamCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "Framework/Commands/InputBindingManager.h"
#include "Framework/Commands/UICommandInfo.h"
#include "UnrealClient.h"

namespace
{
ADCZombieDirector* FindUIDirector()
{
 for (const FWorldContext& Context : GEngine->GetWorldContexts())
  if (Context.WorldType == EWorldType::PIE)
   for (TActorIterator<ADCZombieDirector> It(Context.World()); It; ++It) return *It;
 return nullptr;
}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDCSharedUITest, "DirectorsCut.UI.SharedActs",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FDCSharedUITest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
 Names.Add(TEXT("Act1")); Commands.Add(TEXT("L_DC_Act1_Promise"));
 Names.Add(TEXT("Act2")); Commands.Add(TEXT("L_DC_Act2_BrokenPromise"));
}

bool FDCSharedUITest::RunTest(const FString& Parameters)
{
 AutomationOpenMap(TEXT("/Game/DirectorsCut/Levels/") + Parameters);
 const double Deadline = FPlatformTime::Seconds() + 120.;
 ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Deadline]()
 {
  ADCZombieDirector* D = FindUIDirector();
  if (D && D->bShowCutscenes)
  {
   // These checks are about placement and phases: skip the main menu, the director's cutscenes and the fades.
   D->bShowCutscenes = false;
   D->FadeDirection = 0;
   D->FadeAlpha = 0.f;
   if (D->IsInMainMenu())
   {
    D->bInMainMenu = false;
    D->bReferenceAutoStart = true;
   }
  }
  if (!D || !D->IsReferencePlaying())
  {
   if (FPlatformTime::Seconds() < Deadline) return false;
   AddError(TEXT("Reference did not start")); return true;
  }
 const TSharedPtr<FUICommandInfo> Stop = FInputBindingManager::Get().FindCommandInContext(FName("PlayWorld"), FName("StopPlaySession"));
 if (TestTrue(TEXT("Editor stop command exists"), Stop.IsValid()))
  TestTrue(TEXT("Escape is reserved for gameplay; Shift+Escape stops PIE"), Stop->GetActiveChord(EMultipleKeyBindingIndex::Primary)->NeedsShift());
  TestTrue(TEXT("Both acts use the shared HUD"), D->GetWorld()->GetFirstPlayerController()->GetHUD()->IsA<ADCZombieHUD>());
  if (!D->IsReferencePaused()) D->HandleTransportToggle();
  D->HandleScrub(3.f);
  return true;
 }));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.f));
 ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Parameters]()
 {
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("UIVerification") / (Parameters + TEXT("_reference.png")), false, false);
  return true;
 }));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
 ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
 {
  ADCZombieDirector* D = FindUIDirector();
  if (!D) return true;
  D->HandleStartTake();
  TestTrue(TEXT("Skipping reference goes directly to preparation"), D->GetPhase() == EDCPhase::Preparation);
  D->Phase = EDCPhase::Reference;
  D->StartReferenceCut();
  D->ReferenceTime = D->GetReferenceLength();
  D->TickReferenceCut(0.01f);
  TestTrue(TEXT("Finishing reference goes directly to preparation"), D->GetPhase() == EDCPhase::Preparation);
  TestTrue(TEXT("Preparation entered"), D->GetPhase() == EDCPhase::Preparation);
  TestTrue(TEXT("Removed rehearsal guide stays absent"), D->GetMessage().IsEmpty());
  if (D->IsRehearsalPlaying()) D->HandleRehearsalToggle();
  const int32 Before = D->GetCameraCount();
  D->bPreviewHit = D->bPreviewValid = true;
  D->PreviewPoint = D->GetPlayer()->GetActorLocation() + FVector(100.f, 0.f, 80.f);
  D->PreviewSurface = D->PreviewPoint - FVector(0.f, 0.f, 170.f);
  D->HandlePrimaryPressed();
  TestTrue(TEXT("Placement opens pending camera view"), D->IsPlacementPending() && D->IsViewingCamera());
  TestEqual(TEXT("Preview is not installed yet"), D->GetCameraCount(), Before);
  D->HandleSelectCamera(1);
  TestTrue(TEXT("Number keys cannot abandon pending placement"), D->IsPlacementPending() && D->IsViewingCamera());
  D->HandleCameraCancel();
  TestTrue(TEXT("Escape cancels preview and returns to walking"), !D->IsPlacementPending() && !D->IsViewingCamera());
  TestEqual(TEXT("Cancel leaves installed count unchanged"), D->GetCameraCount(), Before);
  D->bPreviewHit = D->bPreviewValid = true;
  D->HandlePrimaryPressed();
  D->HandleStartTake();
  TestTrue(TEXT("Enter confirms without advancing phase"), D->GetPhase() == EDCPhase::Preparation && !D->IsPlacementPending() && !D->IsViewingCamera());
  TestEqual(TEXT("Confirmed camera installed"), D->GetCameraCount(), Before + 1);
  TestTrue(TEXT("Nearby installed camera offers interaction"), D->GetNearbyCameraSlot() != INDEX_NONE);
  D->HandleCameraInteract();
  TestTrue(TEXT("F opens installed camera"), D->IsViewingCamera());
  D->HandleStartTake();
  TestTrue(TEXT("Enter inside installed camera cannot start filming"), D->GetPhase() == EDCPhase::Preparation);
  D->HandleCameraInteract();
  D->HandleCameraCancel();
  TestTrue(TEXT("Escape exits installed camera without removal"), !D->IsViewingCamera() && D->GetCameraCount() == Before + 1);
  D->GetPlayer()->SetActorLocation(D->GetPlayer()->GetActorLocation() + FVector(1000.f, 0.f, 0.f));
  TestEqual(TEXT("Distant cameras offer no interaction"), D->GetNearbyCameraSlot(), INDEX_NONE);
  D->HandleCameraInteract();
  TestFalse(TEXT("F at distance does not enter a camera"), D->IsViewingCamera());
  D->HandleClearCameras();
  D->ResetScene();
  ADCMultiCam* Camera = D->GetWorld()->SpawnActor<ADCMultiCam>(FVector(0.f, 0.f, 170.f), FRotator::ZeroRotator);
  if (TestNotNull(TEXT("Static camera spawns"), Camera))
  {
   Camera->Setup(1, FVector::ZeroVector);
   bool bFound = false;
   TInlineComponentArray<UStaticMeshComponent*> Meshes(Camera);
   for (UStaticMeshComponent* Mesh : Meshes)
    if (Mesh->GetStaticMesh() && Mesh->GetStaticMesh()->GetPathName().Contains(TEXT("CINEMA_CAMERA_with_simple_tripod")))
    {
     bFound = true;
     TestTrue(TEXT("Authored tripod visible"), Mesh->IsVisible());
     TestTrue(TEXT("Tripod feet on placement surface"), FMath::IsNearlyZero(Mesh->Bounds.Origin.Z - Mesh->Bounds.BoxExtent.Z, 0.1));
    }
   TestTrue(TEXT("Supplied camera asset loaded"), bFound);
   const FVector Position = D->ReferenceShotLocations[0];
   Camera->SetActorLocation(Position);
   Camera->Setup(1, Position - FVector(0.f, 0.f, 170.f));
   Camera->SetAim(D->ReferenceCut[0].Rotation);
   Camera->SaveHome();
   D->Cameras.Add(Camera);
  }
  return true;
 }));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.f));
 ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Parameters]()
 {
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("UIVerification") / (Parameters + TEXT("_preparation.png")), false, false);
  return true;
 }));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
 ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([]()
 {
  if (ADCZombieDirector* D = FindUIDirector())
  {
   D->bPreviewHit = D->bPreviewValid = true;
   D->PreviewPoint = D->GetPlayer()->GetActorLocation() + FVector(100.f, 0.f, 80.f);
   D->PreviewSurface = D->PreviewPoint - FVector(0.f, 0.f, 170.f);
   D->HandlePrimaryPressed();
  }
  return true;
 }));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
 ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Parameters]()
 {
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("UIVerification") / (Parameters + TEXT("_placement.png")), false, false);
  return true;
 }));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
 ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([]()
 {
  if (ADCZombieDirector* D = FindUIDirector())
  {
   D->HandleCameraCancel();
   if (ADCMultiCam* Cam = D->GetCameraSlot(0))
    D->GetPlayer()->SetActorLocation(Cam->GetInteractionPoint() + FVector(80.f, 0.f, 96.f));
  }
  return true;
 }));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.2f));
 ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Parameters]()
 {
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("UIVerification") / (Parameters + TEXT("_interact.png")), false, false);
  return true;
 }));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
 ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
 {
  if (ADCZombieDirector* D = FindUIDirector())
  {
   D->CountdownDuration = 0.1f;
   D->HandleStartTake();
   TestTrue(TEXT("Camera starts countdown"), D->GetPhase() == EDCPhase::Countdown);
  }
  return true;
 }));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.f));
 ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Parameters]()
 {
  if (ADCZombieDirector* D = FindUIDirector())
   TestTrue(TEXT("Filming entered"), D->GetPhase() == EDCPhase::Filming);
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("UIVerification") / (Parameters + TEXT("_filming.png")), false, false);
  return true;
 }));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
 return true;
}
#endif
