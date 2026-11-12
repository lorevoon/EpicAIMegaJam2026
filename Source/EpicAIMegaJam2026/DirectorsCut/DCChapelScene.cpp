// Director's Cut prototype 4 - editor button that places the chapel scene and its reference film as editable actors.
//
// Layout (cm, X east / Y north): nave x -1200..1200, y -800..800, walls 600 high (room for crane and bird's-eye shots).
// West door at y -150..150; centre aisle along y = 0 between two banks of pews; altar rail and altar at the east end.
//
// Performance (30 s): the Worshipper kneels in prayer at the altar rail. The Vampire glides up the aisle from the
// west door, stops behind her, bites, she collapses, and he turns to face the chapel.
//
// Reference film:
//   1  0-10 s  Full shot from behind - dolly following the vampire up the aisle toward her turned back
//   2 10-17 s  Bird's eye - straight down from above, tracking the vampire
//   3 17-25 s  Close-up - the bite, from her side, slow push in
//   4 25-30 s  Wide - the reveal: low in the aisle as he turns toward us, pulling back

#include "DCReplicaDirector.h"

#if WITH_EDITOR

#include "DCPerformer.h"
#include "DCRefCamKey.h"
#include "DCSceneMarks.h"
#include "DCSetPiece.h"
#include "DCWaypoint.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	template <typename T>
	T* BeginPlace(UWorld* World, const FTransform& Transform)
	{
		return World->SpawnActorDeferred<T>(T::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	}

	template <typename T>
	T* FinishPlace(T* Actor, const FTransform& Transform, const FString& Label, const TCHAR* Folder)
	{
		Actor->FinishSpawning(Transform);
		Actor->SetActorLabel(Label);
		Actor->SetFolderPath(FName(Folder));
		return Actor;
	}

	template <typename T>
	T* Place(UWorld* World, const FTransform& Transform, const FString& Label, const TCHAR* Folder)
	{
		T* Actor = World->SpawnActor<T>(T::StaticClass(), Transform);
		Actor->SetActorLabel(Label);
		Actor->SetFolderPath(FName(Folder));
		return Actor;
	}

	constexpr float ChapelWallH = 600.f;
	constexpr float ChapelWallT = 20.f;
}

void ADCReplicaDirector::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(ADCReplicaDirector, bBuildChapelScene) && bBuildChapelScene)
	{
		bBuildChapelScene = false;
		BuildChapelScene();
	}
}

void ADCReplicaDirector::BuildChapelScene()
{
	UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		return;
	}
	for (TActorIterator<ADCRefCamKey> It(World); It; ++It)
	{
		UE_LOG(LogTemp, Warning, TEXT("Directors Cut: this level already has a reference film; delete the existing scene before building the chapel."));
		return;
	}

	// --- Set ------------------------------------------------------------------------------
	const FLinearColor Stone(0.34f, 0.32f, 0.36f);
	const FLinearColor DarkStone(0.16f, 0.15f, 0.18f);
	const FLinearColor Wood(0.16f, 0.09f, 0.05f);
	const FLinearColor Marble(0.75f, 0.73f, 0.7f);
	const FLinearColor Velvet(0.4f, 0.03f, 0.06f);
	const FLinearColor Brass(0.6f, 0.45f, 0.15f);

	auto Block = [World](const FString& Label, const TCHAR* Folder, const FVector& Center, const FVector& Size, const FLinearColor& Color)
	{
		const FTransform Xf(FRotator::ZeroRotator, Center, Size / 100.f);
		ADCSetPiece* Piece = BeginPlace<ADCSetPiece>(World, Xf);
		Piece->Color = Color;
		FinishPlace(Piece, Xf, Label, Folder);
	};
	auto Wall = [&Block](const FString& Label, float X1, float Y1, float X2, float Y2, const FLinearColor& Color, float Z0 = 0.f, float Z1 = ChapelWallH)
	{
		const bool bAlongX = FMath::Abs(X2 - X1) > FMath::Abs(Y2 - Y1);
		const FVector Size = bAlongX ? FVector(FMath::Abs(X2 - X1) + ChapelWallT, ChapelWallT, Z1 - Z0) : FVector(ChapelWallT, FMath::Abs(Y2 - Y1) + ChapelWallT, Z1 - Z0);
		Block(Label, TEXT("DirectorsCut/Chapel/Walls"), FVector((X1 + X2) * 0.5f, (Y1 + Y2) * 0.5f, (Z0 + Z1) * 0.5f), Size, Color);
	};

	Block(TEXT("Floor_Nave"), TEXT("DirectorsCut/Chapel/Walls"), FVector(0, 0, -10), FVector(2400, 1600, 20), DarkStone);
	Block(TEXT("Floor_AisleRunner"), TEXT("DirectorsCut/Chapel/Walls"), FVector(-150, 0, 0.5f), FVector(1900, 180, 1), Velvet);
	Wall(TEXT("Wall_North"), -1200, 800, 1200, 800, Stone);
	Wall(TEXT("Wall_South"), -1200, -800, 1200, -800, Stone);
	Wall(TEXT("Wall_East"), 1200, -800, 1200, 800, Stone);
	Wall(TEXT("Wall_West_North"), -1200, 150, -1200, 800, Stone);
	Wall(TEXT("Wall_West_South"), -1200, -800, -1200, -150, Stone);
	Wall(TEXT("Wall_West_DoorLintel"), -1200, -150, -1200, 150, DarkStone, 350.f, ChapelWallH);

	const TCHAR* Dressing = TEXT("DirectorsCut/Chapel/Dressing");
	for (int32 Row = 0; Row < 6; ++Row)
	{
		const float X = -600.f + Row * 200.f;
		Block(FString::Printf(TEXT("Pew_North_%d"), Row + 1), Dressing, FVector(X, 400, 45), FVector(60, 500, 90), Wood);
		Block(FString::Printf(TEXT("Pew_South_%d"), Row + 1), Dressing, FVector(X, -400, 45), FVector(60, 500, 90), Wood);
	}
	for (int32 i = 0; i < 5; ++i)
	{
		const float X = -800.f + i * 400.f;
		Block(FString::Printf(TEXT("Column_North_%d"), i + 1), Dressing, FVector(X, 750, ChapelWallH / 2), FVector(70, 70, ChapelWallH), DarkStone);
		Block(FString::Printf(TEXT("Column_South_%d"), i + 1), Dressing, FVector(X, -750, ChapelWallH / 2), FVector(70, 70, ChapelWallH), DarkStone);
	}
	Block(TEXT("Altar"), Dressing, FVector(1080, 0, 55), FVector(120, 400, 110), Marble);
	Block(TEXT("Altar_Cloth"), Dressing, FVector(1080, 0, 111), FVector(124, 300, 2), Velvet);
	Block(TEXT("Altar_Rail"), Dressing, FVector(860, 0, 45), FVector(20, 600, 90), Wood);
	Block(TEXT("Altar_Step"), Dressing, FVector(1020, 0, 10), FVector(340, 700, 20), Marble);
	Block(TEXT("Candlestand_North"), Dressing, FVector(950, 330, 75), FVector(20, 20, 150), Brass);
	Block(TEXT("Candlestand_South"), Dressing, FVector(950, -330, 75), FVector(20, 20, 150), Brass);
	Block(TEXT("Crucifix"), Dressing, FVector(1185, 0, 330), FVector(10, 30, 220), Brass);
	Block(TEXT("Crucifix_Bar"), Dressing, FVector(1185, 0, 380), FVector(10, 140, 25), Brass);

	// --- Lighting -------------------------------------------------------------------------
	const TCHAR* Lighting = TEXT("DirectorsCut/Lighting");
	ADirectionalLight* Moon = Place<ADirectionalLight>(World, FTransform(FRotator(-50.f, 20.f, 0.f), FVector(0, 0, 900)), TEXT("Moonlight"), Lighting);
	if (UDirectionalLightComponent* MoonLight = Cast<UDirectionalLightComponent>(Moon->GetLightComponent()))
	{
		MoonLight->SetIntensity(2.f);
		MoonLight->SetLightColor(FLinearColor(0.6f, 0.7f, 1.f));
		MoonLight->SetAtmosphereSunLight(true);
	}
	Place<ASkyAtmosphere>(World, FTransform::Identity, TEXT("SkyAtmosphere"), Lighting);
	ASkyLight* Sky = Place<ASkyLight>(World, FTransform(FVector(0, 0, 900)), TEXT("SkyLight"), Lighting);
	if (USkyLightComponent* SkyLight = Sky->GetLightComponent())
	{
		SkyLight->SetMobility(EComponentMobility::Movable);
		SkyLight->bRealTimeCapture = true;
		SkyLight->SetIntensity(0.5f);
	}
	AExponentialHeightFog* Fog = Place<AExponentialHeightFog>(World, FTransform::Identity, TEXT("HeightFog"), Lighting);
	Fog->GetComponent()->SetFogDensity(0.015f);

	auto Lamp = [World, Lighting](const FString& Label, const FVector& Location, const FLinearColor& Color, float Intensity)
	{
		APointLight* Light = Place<APointLight>(World, FTransform(Location), Label, Lighting);
		if (UPointLightComponent* Point = Cast<UPointLightComponent>(Light->GetLightComponent()))
		{
			Point->SetIntensity(Intensity);
			Point->SetLightColor(Color);
			Point->SetAttenuationRadius(1400.f);
		}
	};
	const FLinearColor Candle(1.f, 0.6f, 0.3f);
	Lamp(TEXT("Candles_North"), FVector(950, 330, 170), Candle, 4000.f);
	Lamp(TEXT("Candles_South"), FVector(950, -330, 170), Candle, 4000.f);
	Lamp(TEXT("Nave_Fill"), FVector(-200, 0, 500), FLinearColor(0.55f, 0.65f, 1.f), 3500.f);
	Lamp(TEXT("Door_Spill"), FVector(-1100, 0, 300), FLinearColor(0.5f, 0.6f, 1.f), 2500.f);

	// --- Cast and blocking ----------------------------------------------------------------
	auto Performer = [World](const FString& Name, float X, float Y, float Yaw, const FLinearColor& Body, const FLinearColor& Head, bool bCape)
	{
		const FTransform Xf(FRotator(0.f, Yaw, 0.f), FVector(X, Y, 0.f));
		ADCPerformer* P = BeginPlace<ADCPerformer>(World, Xf);
		P->DisplayName = Name;
		P->BodyColor = Body;
		P->HeadColor = Head;
		P->bHasCape = bCape;
		return FinishPlace(P, Xf, Name, TEXT("DirectorsCut/Cast"));
	};
	struct FWp { float T, X, Y, Yaw, Lean = 0.f, Tilt = 0.f; };
	auto Blocking = [World](ADCPerformer* P, std::initializer_list<FWp> Points)
	{
		const FString Folder = FString::Printf(TEXT("DirectorsCut/Cast/%s_Blocking"), *P->DisplayName);
		for (const FWp& W : Points)
		{
			const FTransform Xf(FRotator(0.f, W.Yaw, 0.f), FVector(W.X, W.Y, 0.f));
			ADCWaypoint* Wp = BeginPlace<ADCWaypoint>(World, Xf);
			Wp->Performer = P;
			Wp->Time = W.T;
			Wp->Lean = W.Lean;
			Wp->Tilt = W.Tilt;
			FinishPlace(Wp, Xf, FString::Printf(TEXT("WP_%s_%04.1f"), *P->DisplayName, W.T), *Folder);
		}
	};

	ADCPerformer* Vampire = Performer(TEXT("Vampire"), -850, 0, 0, FLinearColor(0.04f, 0.03f, 0.04f), FLinearColor(0.85f, 0.88f, 0.85f), true);
	Blocking(Vampire, {
		{ 16.f, 690, 0, 0 },          // glides up the aisle
		{ 18.f, 690, 0, 0, 28 },      // the bite
		{ 23.f, 690, 0, 0, 28 },
		{ 24.f, 690, 0, 0, 0 },
		{ 26.5f, 690, 0, 180 },       // turns to face the chapel
		{ 30.f, 690, 0, 180 },
	});

	ADCPerformer* Worshipper = Performer(TEXT("Worshipper"), 780, 0, 0, FLinearColor(0.85f, 0.85f, 0.9f), FLinearColor(0.9f, 0.72f, 0.6f), false);
	Blocking(Worshipper, {
		{ 0.5f, 780, 0, 0, 12 },      // head bowed in prayer
		{ 17.5f, 780, 0, 0, 12 },
		{ 18.5f, 780, 0, 0, 0 },      // startled
		{ 23.5f, 780, 0, 0, 0 },
		{ 25.f, 780, 0, 0, 0, 60 },   // collapses back into his arms
	});

	// --- Reference film -------------------------------------------------------------------
	auto Anchor = [](ADCPerformer* P, EDCAnchorPart Part, bool bFace = false)
	{
		FDCAnchorRef Ref;
		Ref.Performer = P;
		Ref.Part = Part;
		Ref.bMustSeeFace = bFace;
		return Ref;
	};
	auto Key = [World](int32 Shot, float Time, const FVector& Location, const FRotator& Rotation, const TCHAR* Title = nullptr, TArray<FDCAnchorRef> Subjects = {})
	{
		const FTransform Xf(Rotation, Location);
		ADCRefCamKey* K = BeginPlace<ADCRefCamKey>(World, Xf);
		K->Shot = Shot;
		K->Time = Time;
		K->ShotTitle = Title ? Title : TEXT("");
		K->Subjects = MoveTemp(Subjects);
		FinishPlace(K, Xf, FString::Printf(TEXT("RefCam_S%d_%04.1f"), Shot, Time), TEXT("DirectorsCut/ReferenceFilm"));
	};
	using EP = EDCAnchorPart;

	// 1. Full shot from behind: dolly up the aisle after the vampire, her back turned at the altar.
	Key(1, 0.f, FVector(-1150, 0, 170), FRotator(-4, 0, 0), TEXT("Full shot - from behind"),
		{ Anchor(Vampire, EP::Body), Anchor(Worshipper, EP::Head) });
	Key(1, 5.f, FVector(-950, 0, 170), FRotator(-4, 0, 0));
	Key(1, 10.f, FVector(-150, 0, 170), FRotator(-4, 0, 0));

	// 2. Bird's eye: straight down from the roof line, tracking the vampire.
	Key(2, 10.f, FVector(230, 0, 650), FRotator(-80, 0, 0), TEXT("Bird's eye - follow"),
		{ Anchor(Vampire, EP::Head), Anchor(Worshipper, EP::Head) });
	Key(2, 13.f, FVector(555, 0, 650), FRotator(-80, 0, 0));
	Key(2, 16.f, FVector(665, 0, 650), FRotator(-80, 0, 0));

	// 3. Close-up of the bite from her north side, slow push in.
	Key(3, 17.f, FVector(735, 230, 175), FRotator(-3, -90, 0), TEXT("Close-up - the bite"),
		{ Anchor(Vampire, EP::Head), Anchor(Worshipper, EP::Head) });
	Key(3, 25.f, FVector(735, 165, 170), FRotator(-3, -90, 0));

	// 4. Wide reveal: low in the aisle as he turns toward camera, pulling back.
	Key(4, 25.f, FVector(350, 0, 140), FRotator(3, 0, 0), TEXT("Wide - the reveal"),
		{ Anchor(Vampire, EP::Head, true), Anchor(Worshipper, EP::Body) });
	Key(4, 30.f, FVector(80, 0, 130), FRotator(4, 0, 0));

	// --- Player marks ---------------------------------------------------------------------
	Place<ADCStartMark>(World, FTransform(FRotator::ZeroRotator, FVector(-1050, 450, 0)), TEXT("StartMark"), TEXT("DirectorsCut/Marks"));
	Place<ADCClapperboard>(World, FTransform(FVector(-1100, 650, 0)), TEXT("Clapperboard"), TEXT("DirectorsCut/Marks"));

	SetFolderPath(TEXT("DirectorsCut"));
	World->MarkPackageDirty();
	UE_LOG(LogTemp, Log, TEXT("Directors Cut: chapel scene placed. Save the level to keep it."));
}

#endif // WITH_EDITOR
