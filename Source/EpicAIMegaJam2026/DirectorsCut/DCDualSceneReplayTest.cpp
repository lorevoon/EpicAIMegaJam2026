#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "DCMultiCamDirector.h"
#include "DCMultiCam.h"
#include "DCPerformer.h"
#include "DCRail.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/LevelStreaming.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCDualSceneReplayTest, "DirectorsCut.Replay.AlignedEnvironments",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCDualSceneReplayTest::RunTest(const FString& Parameters)
{
	AutomationOpenMap(TEXT("/Game/DirectorsCut/Levels/L_DC_Act1_Promise"));
	const double Deadline = FPlatformTime::Seconds() + 60.;
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Deadline]()
	{
		UWorld* World = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE) { World = Context.World(); break; }
		}
		if (!World || !World->HasBegunPlay())
		{
			if (FPlatformTime::Seconds() < Deadline) return false;
			AddError(TEXT("Act 1 PIE did not start")); return true;
		}
		ADCMultiCamDirector* D = nullptr;
		for (TActorIterator<ADCMultiCamDirector> It(World); It; ++It) { D = *It; break; }
		if (!TestNotNull(TEXT("Shared director"), D)) return true;
		D->ResetScene();
		ADCPerformer* Romeo = nullptr;
		for (ADCPerformer* Actor : D->Performers)
			if (Actor->GetActorLabel() == TEXT("Romeo")) Romeo = Actor;
		if (!TestNotNull(TEXT("Rehearsal has Romeo"), Romeo)) return true;
		const FVector StartPosition = Romeo->GetActorLocation();
		D->Tick(8.f);
		TestEqual(TEXT("Preparation advances rehearsal clock"), D->GetSceneTime(), 8.f);
		TestTrue(TEXT("Rehearsal actually moves the actor"), FVector::Dist(StartPosition, Romeo->GetActorLocation()) > 100.f);
		TestTrue(TEXT("Rehearsal does not record or grade"), D->Recording.IsEmpty() && D->GetMatch().Overall == 0.f);
		D->HandleRehearsalToggle();
		const FVector PausedPosition = Romeo->GetActorLocation();
		D->Tick(1.f);
		TestTrue(TEXT("Paused rehearsal holds actor pose"), Romeo->GetActorLocation().Equals(PausedPosition));
		TestEqual(TEXT("Paused rehearsal holds time"), D->GetSceneTime(), 8.f);
		D->HandleScrub(2.f);
		TestEqual(TEXT("Paused rehearsal scrubs"), D->GetSceneTime(), 10.f);
		D->HandleRehearsalSpeed(1);
		D->HandleRehearsalToggle();
		D->Tick(1.f);
		TestEqual(TEXT("Rehearsal speed controls clock"), D->GetSceneTime(), 12.f);
		D->HandleScrub(7.5f);
		TestFalse(TEXT("Scrub pauses playback"), D->IsRehearsalPlaying());
		D->HandleRehearsalToggle();
		D->Tick(.5f);
		TestEqual(TEXT("Rehearsal loops at take duration"), D->GetSceneTime(), .5f);
		TestTrue(TEXT("Rehearsal stays in preparation"), D->GetPhase() == EDCPhase::Preparation);
		D->ResetScene();
		TestTrue(TEXT("Reset rewinds actor to authored start"), Romeo->GetActorLocation().Equals(StartPosition));
		TestTrue(TEXT("Reset resumes normal rehearsal"), D->IsRehearsalPlaying() && D->GetRehearsalSpeed() == 1.f);
		// Cameras move in a bounded horizontal disc and keep independent presets.
		TArray<ADCMultiCam*> FreeCameras;
		TArray<int32> FreeSlots;
		for (int32 Index=0; Index<3; ++Index)
		{
			const bool bLookDown = Index==1;
			const FVector Floor(Index*2000.f, 0.f, 10000.f);
			ADCMultiCam* C = D->SpawnCamera(Floor+FVector(0,0,ADCMultiCam::StaticHeight));
			C->Setup(Index+1, Floor);
			C->SetAim(FRotator(bLookDown ? -90.f : 0.f,90.f,0.f));
			const FVector Center=C->GetActorLocation();
			C->MoveLocal(FVector2D(0,1),.5f);
			TestTrue(TEXT("W follows camera yaw even looking straight down"),C->GetActorLocation().Y>Center.Y);
			TestEqual(TEXT("WASD keeps camera height"),C->GetActorLocation().Z,Center.Z);
			C->MoveLocal(FVector2D(1,1),100.f);
			TestTrue(TEXT("Diagonal travel is clamped to placement radius"),FVector::Dist(Center,C->GetActorLocation())<=C->GetMovementRadius()+.01f);
			C->SaveHome();
			const FVector Preset=C->GetActorLocation();
			C->MoveLocal(FVector2D(-1,-1),.3f); C->AddAim(30,10);
			C->RestoreHome();
			TestTrue(TEXT("Retry restores prepared camera position"),C->GetActorLocation().Equals(Preset,.01f));
			TestTrue(TEXT("Retry restores prepared camera angle"),C->GetAim().Equals(FRotator(bLookDown ? -90.f : 0.f,90.f,0.f),.01f));
			TestNull(TEXT("New camera needs no rail"),C->GetRail());
			FreeSlots.Add(D->Cameras.Add(C)); FreeCameras.Add(C);
		}
		D->Phase=EDCPhase::Filming;
		for (int32 Index : {0,1,2,0})
		{
			const FVector Before=FreeCameras[Index]->GetActorLocation();
			D->HandleSelectCamera(FreeSlots[Index]+1);
			TestEqual(TEXT("Number selects the requested camera"),D->GetViewingSlot(),FreeSlots[Index]);
			TestTrue(TEXT("Cut retains the camera preset"),FreeCameras[Index]->GetActorLocation().Equals(Before));
		}
		D->ResetScene();
		for (int32 Index=0;Index<3;++Index) { D->Cameras[FreeSlots[Index]]=nullptr; FreeCameras[Index]->Destroy(); }
		TMap<AActor*, FTransform> SharedProps;
		for (TActorIterator<AActor> It(World); It; ++It)
			if (It->ActorHasTag(TEXT("DC_MillAssembly")) || It->GetActorLabel() == TEXT("Practical promise tree") || It->ActorHasTag(TEXT("DC_OriginalTerrain")))
				SharedProps.Add(*It, It->GetActorTransform());
		TestTrue(TEXT("Shared mill, tree and slope are present"), SharedProps.Num() >= 9);
		ULevelStreaming* Ground = UGameplayStatics::GetStreamingLevel(D, TEXT("L_DC_Act1_Ground"));
		if (!TestNotNull(TEXT("Original WindmillValley terrain"), Ground)) return true;
		TestTrue(TEXT("Original terrain visible during rehearsal"), Ground->IsLevelVisible());
		ULevelStreaming* Studio = UGameplayStatics::GetStreamingLevel(D, D->StudioEnvironmentLevel);
		ULevelStreaming* Real = UGameplayStatics::GetStreamingLevel(D, D->RealEnvironmentLevel);
		if (!TestNotNull(TEXT("Studio layer"), Studio) || !TestNotNull(TEXT("Real layer"), Real)) return true;
		TestTrue(TEXT("Studio visible during preparation"), Studio->IsLevelVisible());
		TestFalse(TEXT("Real hidden during preparation"), Real->IsLevelVisible());
		TestTrue(TEXT("Real preloaded"), Real->IsLevelLoaded());
		TestTrue(TEXT("Both layers at identical world origin"), Studio->LevelTransform.Equals(FTransform::Identity) && Real->LevelTransform.Equals(FTransform::Identity));
		const FTransform BeforeReferenceSolve = Romeo->GetActorTransform();
		D->BuildReferenceCut();
		TestTrue(TEXT("Reference solve restores studio preparation"), Studio->IsLevelVisible() && !Real->IsLevelVisible());
		TestTrue(TEXT("Reference solve restores the rehearsal pose"), Romeo->GetActorTransform().Equals(BeforeReferenceSolve, .01));
		TestTrue(TEXT("Reference solve produces camera samples"), !D->ReferenceCut.IsEmpty());
		ADCMultiCam* Cam = D->SpawnCamera(FVector(1000,-1200,250));
		D->Cameras.Add(Cam);
		D->ViewingSlot = D->Cameras.Num()-1;
		D->Phase = EDCPhase::Filming;
		// Exercise 60 fps motion, zoom, and a hard camera cut.
		for (int32 Frame=0; Frame<=1200; ++Frame)
		{
			D->SceneTime = Frame/60.f;
			Cam->GetCamera()->SetWorldLocationAndRotation(FVector(900+Frame*.2,-1200+Frame*.1,250+Frame*.03), FRotator(-5,45+Frame*.04,0));
			Cam->GetCamera()->SetFieldOfView(75-Frame*.02);
			if (Frame==600) { D->Cameras.Add(Cam); D->ViewingSlot=D->Cameras.Num()-1; }
			D->RecordSample();
		}
		TestEqual(TEXT("Every frame recorded"), D->Recording.Num(), 1201);
		ADCRail* UnattachedRail = World->SpawnActor<ADCRail>();
		TMap<ADCPerformer*,FTransform> CastBefore;
		for (ADCPerformer* Actor : D->Performers) CastBefore.Add(Actor,Actor->GetActorTransform());
		D->SetReviewEnvironment(true);
		for (const auto& Pair : CastBefore) TestTrue(TEXT("Environment swap preserves cast transform"), Pair.Key->GetActorTransform().Equals(Pair.Value));
		D->StartPremiere();
		for (const auto& Prop : SharedProps)
			TestTrue(TEXT("Set and real scene use identical prop transforms"), Prop.Key->GetActorTransform().Equals(Prop.Value));
		TestFalse(TEXT("No studio in replay"), Studio->IsLevelVisible());
		TestTrue(TEXT("Real visible in replay"), Real->IsLevelVisible());
		TestTrue(TEXT("Same original terrain remains visible in replay"), Ground->IsLevelVisible());
		TestTrue(TEXT("Unattached rails removed too"), UnattachedRail->IsHidden());
		for (TActorIterator<AActor> It(World); It; ++It)
			if (It->ActorHasTag(TEXT("DC_SetOnly"))) TestTrue(TEXT("Crew and set-only props removed"), It->IsHidden());
		for (const FDCTakeSample& Sample : D->Recording)
		{
			D->ShowPremiereFrame(Sample.FilmTime);
			TestTrue(TEXT("Recorded lens position matches replay"), D->PlaybackCamera->GetActorLocation().Equals(Sample.Location,.01));
			TestTrue(TEXT("Recorded lens rotation matches replay"), D->PlaybackCamera->GetActorRotation().Equals(Sample.Rotation,.01));
			TestTrue(TEXT("Recorded framing matches replay"), FMath::IsNearlyEqual(D->PlaybackCamera->GetCameraComponent()->FieldOfView, Sample.FilmFOV,.001f));
		}
		D->HandleScrub(-10.f);
		TestTrue(TEXT("Scrubbing stays in real environment"), Real->IsLevelVisible());
		D->ResetScene();
		TestTrue(TEXT("Retry restores studio"), Studio->IsLevelVisible());
		TestFalse(TEXT("Retry hides real environment"), Real->IsLevelVisible());
		TestFalse(TEXT("Retry restores rails"), UnattachedRail->IsHidden());
		for (TActorIterator<AActor> It(World); It; ++It)
			if (It->ActorHasTag(TEXT("DC_SetOnly"))) TestFalse(TEXT("Retry restores crew"), It->IsHidden());
		return true;
	}));
	return true;
}
#endif
