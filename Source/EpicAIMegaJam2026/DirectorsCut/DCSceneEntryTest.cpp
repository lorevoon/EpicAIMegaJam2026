#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/IConsoleManager.h"
#include "Tests/AutomationCommon.h"
#include "DCMultiCamDirector.h"
#include "DCBeat.h"
#include "DCPerformer.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDCSceneEntryTest, "DirectorsCut.SceneEntry",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FDCSceneEntryTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
 for (const TCHAR* Map : {TEXT("L_DC_Act1_Set"), TEXT("L_DC_Act1_Real"), TEXT("L_DC_Act2_Set"), TEXT("L_DC_Act2_Real")})
 { Names.Add(Map); Commands.Add(Map); }
}

bool FDCSceneEntryTest::RunTest(const FString& Parameters)
{
 FString Startup, Game;
 GConfig->GetString(TEXT("/Script/EngineSettings.GameMapsSettings"), TEXT("EditorStartupMap"), Startup, GEngineIni);
 GConfig->GetString(TEXT("/Script/EngineSettings.GameMapsSettings"), TEXT("GameDefaultMap"), Game, GEngineIni);
 TestFalse(TEXT("Editor does not start at title"), Startup.Contains(TEXT("L_DC_Title")));
 TestFalse(TEXT("Game does not start at title"), Game.Contains(TEXT("L_DC_Title")));
 IConsoleVariable* Cache = IConsoleManager::Get().FindConsoleVariable(TEXT("r.EyeAdaptation.CachedLightingPreExposure"));
 if (TestNotNull(TEXT("Cached-lighting exposure setting exists"), Cache))
  TestEqual(TEXT("Project uses EV -4..16 cache range"), Cache->GetFloat(), 8.f);
 const bool bAct2 = Parameters.Contains(TEXT("Act2"));
 const FString Expected = bAct2 ? TEXT("L_DC_Act2_BrokenPromise") : TEXT("L_DC_Act1_Promise");
 AutomationOpenMap(FString(TEXT("/Game/DirectorsCut/Levels/")) + Parameters);
 const double Deadline = FPlatformTime::Seconds() + 60.;
 ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Expected, bAct2, Deadline]()
 {
  UWorld* World = nullptr;
  for (const FWorldContext& Context : GEngine->GetWorldContexts())
   if (Context.WorldType == EWorldType::PIE) { World = Context.World(); break; }
  if (!World || !World->HasBegunPlay() || UGameplayStatics::GetCurrentLevelName(World, true) != Expected)
  {
   if (FPlatformTime::Seconds() < Deadline) return false;
   AddError(FString(TEXT("Environment map did not enter complete scene: ")) + Expected); return true;
  }
  int32 Directors = 0, Beats = 0, Cast = 0;
  for (TActorIterator<ADCMultiCamDirector> It(World); It; ++It) ++Directors;
  for (TActorIterator<ADCBeat> It(World); It; ++It) ++Beats;
  for (TActorIterator<ADCPerformer> It(World); It; ++It) ++Cast;
  TestEqual(TEXT("One gameplay director, no duplicate scene"), Directors, 1);
  TestEqual(TEXT("All act beats loaded"), Beats, bAct2 ? 5 : 4);
  TestTrue(TEXT("Performance actors loaded"), Cast >= (bAct2 ? 8 : 3));
  TestNotNull(TEXT("Player controller available for scene testing"), World->GetFirstPlayerController());
  return true;
 }));
 return true;
}
#endif
