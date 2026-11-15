#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "DCMultiCamDirector.h"
#include "DCMultiCam.h"
#include "DCMultiCamCharacter.h"
#include "DCBeat.h"
#include "DCPerformer.h"
#include "DCPlacement.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "GameFramework/PlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCAct2PlacementTest, "DirectorsCut.Act2.Placement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCAct2PlacementTest::RunTest(const FString& Parameters)
{
	const bool bAct1 = FParse::Param(FCommandLine::Get(), TEXT("PlacementAct1"));
	AutomationOpenMap(bAct1 ? TEXT("/Game/DirectorsCut/Levels/L_DC_Act1_Promise") : TEXT("/Game/DirectorsCut/Levels/L_DC_Act2_BrokenPromise"));
	const double Deadline = FPlatformTime::Seconds() + 60.;
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Deadline, bAct1]()
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
		if (!TestNotNull(TEXT("Director"), D)) return true;
		D->ResetScene();
		D->Cutscene = nullptr;
		D->bShowCutscenes = false;
		D->FadeDirection = 0;
		D->FadeAlpha = 0.f;
		D->bInMainMenu = false;
		D->OverviewRemaining = 0.f;
		D->bReferenceAutoStart = false;
		D->bRehearsalPlaying = false;
		D->BuildReferenceCut();
		D->Cameras.Reset();
		D->PendingPlacementSlot = INDEX_NONE;
		FCollisionQueryParams Params(TEXT("Act2PlacementTest"), false);
		for (ADCPerformer* P : D->Performers) Params.AddIgnoredActor(P);
		if (!bAct1) TestEqual(TEXT("Five placement targets"), D->ReferenceShotLocations.Num(), 5);
		TestTrue(TEXT("Reference contains preparation missions"), D->ReferenceShotLocations.Num() > 0);
		for (int32 Shot = 0; Shot < D->ReferenceShotLocations.Num(); ++Shot)
		{
			const FDCTakeSample* Sample = nullptr;
			for (const FDCTakeSample& Frame : D->ReferenceCut)
				if (Frame.Slot == Shot && Frame.Time >= D->Beats[D->ReferenceShotBeats[Shot]].Beat->WindowStart + 1.f) { Sample = &Frame; break; }
			if (!TestNotNull(TEXT("Reference frame for mission"), Sample)) continue;
			D->SceneTime = Sample->Time;
			D->EvaluatePerformers(D->SceneTime);
			const FVector Spot = D->ReferenceShotLocations[Shot];
			FHitResult Hit;
			if (!TestTrue(TEXT("Target has ground"), DCPlacement::TraceGround(World, Hit, Spot + FVector(0,0,1000), Spot - FVector(0,0,3000), Params))) continue;
			D->GetPlayer()->SetActorLocation(Hit.ImpactPoint + FVector(-160,0,100));
			D->GetPC()->SetControlRotation(FRotator::ZeroRotator);
			D->ViewWalking();
			D->UpdatePrepPreview();
			AddInfo(FString::Printf(TEXT("Scene %d preview valid=%d surface=%s target=%s"), Shot+1, D->bPreviewValid, *D->PreviewSurface.ToString(), *Spot.ToString()));
			if (!TestTrue(TEXT("Marked ground supports the real placement preview"), D->bPreviewValid)) continue;
			const int32 BeforeCheer = D->GetCheerCount();
			D->HandlePrimaryPressed();
			if (!TestTrue(TEXT("First click enters camera setup"), D->IsPlacementPending())) continue;
			ADCMultiCam* Cam = D->GetCameraSlot(D->ViewingSlot);
			Cam->SetAim(Sample->Rotation + FRotator(0,180,0));
			D->HandlePrimaryPressed();
			TestFalse(TEXT("Prescribed location with the wrong picture cannot clear a mission"), D->IsSceneCovered(D->ReferenceShotBeats[Shot]));
			// Make the same picture from farther away, well outside the obsolete marker radius.
			TArray<FVector> Subjects;
			D->GetSubjectPoints(D->SceneTime, Subjects);
			FVector Centre = FVector::ZeroVector;
			for (const FVector& P : Subjects) Centre += P / Subjects.Num();
			const FVector Offset = Sample->Location - Centre;
			const float Travel = (D->ReferenceSpotRadius + 150.f) / FMath::Max(Offset.Size2D(), 1.f);
			const float Scale = bAct1 || Shot == 0 ? 1.f : Sample->FilmFOV > 25.f ? 1.f + Travel : FMath::Max(0.25f, 1.f - Travel);
			const FVector Alternative = Centre + Offset * Scale;
			if (!bAct1 && Shot > 0) TestTrue(TEXT("Matching camera is outside obsolete placement zone"), FVector::Dist2D(Alternative, Spot) > D->ReferenceSpotRadius);
			Cam->SetActorLocation(Alternative);
			Cam->SetAim(Sample->Rotation);
			const float FullFOV = FMath::RadiansToDegrees(2.f * FMath::Atan(FMath::Tan(FMath::DegreesToRadians(Sample->FilmFOV * 0.5f)) / Scale / DCFrame::CaptureExtent(D->GetViewAspect()).X));
			Cam->SnapFOV(FullFOV);
			D->ViewCamera(Shot);
			D->HandlePrimaryPressed();
			TestTrue(TEXT("Confirmed installation clears the mission"), D->IsSceneCovered(D->ReferenceShotBeats[Shot]));
			TestEqual(TEXT("Confirmed installation fires director popup"), D->GetCheerCount(), BeforeCheer + 1);
			TestTrue(TEXT("Mission board receives animation time"), D->GetSceneCoveredTime(D->ReferenceShotBeats[Shot]) >= 0.f);
			Params.AddIgnoredActor(Cam);

		}
		D->UpdateCoveredScenes();
		for (int32 Shot = 0; Shot < D->ReferenceShotBeats.Num(); ++Shot)
		{
			TestTrue(FString::Printf(TEXT("Scene %d clears by matching the reference image"), Shot+1), D->SceneCovered[D->ReferenceShotBeats[Shot]]);
			TestEqual(TEXT("Cut cue uses the same placement pairing"), D->GetCameraForReferenceShot(Shot), Shot);
		}
		// Editing a successful camera invalidates its old match; deletion does too.
		if (D->Cameras.Num() == 5)
		{
			const FRotator GoodAim = D->Cameras[4]->GetAim();
			D->Cameras[4]->SetAim(GoodAim + FRotator(0,180,0));
			D->UpdateCoveredScenes();
			TestFalse(TEXT("Turning away removes obsolete picture credit"), D->IsSceneCovered(D->ReferenceShotBeats[4]));
			D->Cameras[4]->SetAim(GoodAim);
			D->UpdateCoveredScenes();
		}
		int32 TreeHits = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (!bAct1) TestFalse(TEXT("Requested extras are absent"), It->GetActorLabel() == TEXT("Exhausted runner") || It->GetActorLabel() == TEXT("Lighting technician"));
			if (!It->ActorHasTag(TEXT("DCNoCameraGround"))) continue;
			for (int32 X = -300; X <= 300; X += 100)
			for (int32 Y = -300; Y <= 300; Y += 100)
			{
				const FVector Start = It->GetActorLocation() + FVector(X,Y,2000);
				const FVector End = Start - FVector(0,0,4000);
				FHitResult Raw, Ground;
				if (!World->LineTraceSingleByChannel(Raw, Start, End, ECC_Visibility, Params) || Raw.GetActor() != *It) continue;
				++TreeHits;
				TestTrue(TEXT("Ground trace continues below tree"), DCPlacement::TraceGround(World, Ground, Start, End, Params));
				TestTrue(TEXT("Tree cannot support the camera"), Ground.GetActor() != *It && Ground.ImpactPoint.Z < Raw.ImpactPoint.Z);
			}
		}
		if (!bAct1) TestTrue(TEXT("Regression exercised actual tree collision"), TreeHits > 0);
		if (FParse::Param(FCommandLine::Get(), TEXT("PlacementVisualQA")))
		{
			FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Act2ImageMatching.png"), false, false);
			return true;
		}
		for (ADCMultiCam* Cam : D->Cameras) if (Cam) Cam->Destroy();
		D->Cameras.Reset();
		D->ResetScene();
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.f));
	return true;
}
#endif
