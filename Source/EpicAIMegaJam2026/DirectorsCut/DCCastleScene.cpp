// Director's Cut prototype 2 - editor button that places the default castle scene as editable actors.
//
// Layout (cm, X east / Y north):
//   Cell       x -1800..-600, y -1400..0     Renfield's cell; door to hallway at y -800..-600
//   Library    x -1800..-600, y 0..1400      Van Helsing's workroom; door to hallway at y 600..800
//   Hallway    x -600..400                   start mark at the south end; two pillars
//   Bedroom    x 400..1800,   y 200..1400    Lucy; door at y 700..900, window to hallway y 300..500
//   Terrace    x 400..1800,   y -1400..200   open air; window into the bedroom at x 1000..1300
//
// Overlapping 60 s performance: Renfield and Dracula in the cell, Lucy's diary, Van Helsing's stake, Dracula stalking
// the hallway, Harker witnessing the bite through the window and fainting, Van Helsing questioning Renfield,
// Dracula noticing the witness, and the final confrontation.

#include "DCFreeDirector.h"

#if WITH_EDITOR

#include "DCBeat.h"
#include "DCPerformer.h"
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

	constexpr float WallH = 450.f;
	constexpr float WallT = 20.f;
}

void ADCFreeDirector::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(ADCFreeDirector, bBuildCastleScene) && bBuildCastleScene)
	{
		bBuildCastleScene = false;
		BuildCastleScene();
	}
}

void ADCFreeDirector::BuildCastleScene()
{
	UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		return;
	}
	for (TActorIterator<ADCBeat> It(World); It; ++It)
	{
		UE_LOG(LogTemp, Warning, TEXT("Directors Cut: this level already has beats; delete the existing scene before building the castle."));
		return;
	}

	// --- Set pieces -----------------------------------------------------------------------
	const FLinearColor Wood(0.16f, 0.09f, 0.05f);
	const FLinearColor DarkWood(0.08f, 0.05f, 0.03f);
	const FLinearColor Stone(0.32f, 0.3f, 0.36f);
	const FLinearColor DarkStone(0.15f, 0.14f, 0.17f);
	const FLinearColor Velvet(0.4f, 0.03f, 0.06f);
	const FLinearColor Straw(0.45f, 0.38f, 0.15f);
	const FLinearColor Moss(0.12f, 0.2f, 0.12f);
	const FLinearColor Flagstone(0.2f, 0.2f, 0.22f);

	auto Block = [World](const FString& Label, const TCHAR* Folder, const FVector& Center, const FVector& Size, const FLinearColor& Color)
	{
		const FTransform Xf(FRotator::ZeroRotator, Center, Size / 100.f);
		ADCSetPiece* Piece = BeginPlace<ADCSetPiece>(World, Xf);
		Piece->Color = Color;
		FinishPlace(Piece, Xf, Label, Folder);
	};
	// Axis-aligned wall segment from (X1,Y1) to (X2,Y2) between heights Z0 and Z1.
	auto Wall = [&Block](const FString& Label, float X1, float Y1, float X2, float Y2, const FLinearColor& Color, float Z0 = 0.f, float Z1 = WallH)
	{
		const bool bAlongX = FMath::Abs(X2 - X1) > FMath::Abs(Y2 - Y1);
		const FVector Size = bAlongX ? FVector(FMath::Abs(X2 - X1) + WallT, WallT, Z1 - Z0) : FVector(WallT, FMath::Abs(Y2 - Y1) + WallT, Z1 - Z0);
		Block(Label, TEXT("DirectorsCut/Castle/Walls"), FVector((X1 + X2) * 0.5f, (Y1 + Y2) * 0.5f, (Z0 + Z1) * 0.5f), Size, Color);
	};

	const TCHAR* Floors = TEXT("DirectorsCut/Castle/Floors");
	Block(TEXT("Floor_Indoors"), Floors, FVector(-700, 0, -10), FVector(2200, 2800, 20), Wood);
	Block(TEXT("Floor_Bedroom"), Floors, FVector(1100, 800, -10), FVector(1400, 1200, 20), Wood);
	Block(TEXT("Floor_Terrace"), Floors, FVector(1100, -600, -10), FVector(1400, 1600, 20), Flagstone);
	Block(TEXT("Floor_Cell"), Floors, FVector(-1200, -700, -9), FVector(1200, 1400, 20), DarkStone);

	// Outer shell (the terrace's outer edges are a low balustrade so the night sky shows).
	Wall(TEXT("Outer_North"), -1800, 1400, 1800, 1400, Stone);
	Wall(TEXT("Outer_West"), -1800, -1400, -1800, 1400, Stone);
	Wall(TEXT("Outer_South_Indoors"), -1800, -1400, 400, -1400, Stone);
	Wall(TEXT("Outer_East_Bedroom"), 1800, 200, 1800, 1400, Stone);
	Wall(TEXT("Balustrade_South"), 400, -1400, 1800, -1400, Stone, 0.f, 100.f);
	Wall(TEXT("Balustrade_East"), 1800, -1400, 1800, 200, Stone, 0.f, 100.f);

	// Hallway west wall: cell door (y -800..-600) and library door (y 600..800).
	Wall(TEXT("HallW_South"), -600, -1400, -600, -800, Stone);
	Wall(TEXT("HallW_Mid"), -600, -600, -600, 600, Stone);
	Wall(TEXT("HallW_North"), -600, 800, -600, 1400, Stone);
	Wall(TEXT("HallW_CellDoor_Lintel"), -600, -800, -600, -600, DarkStone, 300.f, WallH);
	Wall(TEXT("HallW_LibraryDoor_Lintel"), -600, 600, -600, 800, DarkStone, 300.f, WallH);

	// Cell / library divider.
	Wall(TEXT("Cell_Library_Divider"), -1800, 0, -600, 0, Stone);

	// Hallway east wall: terrace door (y -1000..-800), window into the bedroom (y 300..500), bedroom door (y 700..900).
	Wall(TEXT("HallE_South"), 400, -1400, 400, -1000, Stone);
	Wall(TEXT("HallE_Mid"), 400, -800, 400, 300, Stone);
	Wall(TEXT("HallE_Window_Sill"), 400, 300, 400, 500, Stone, 0.f, 120.f);
	Wall(TEXT("HallE_Window_Header"), 400, 300, 400, 500, Stone, 260.f, WallH);
	Wall(TEXT("HallE_Between"), 400, 500, 400, 700, Stone);
	Wall(TEXT("HallE_North"), 400, 900, 400, 1400, Stone);
	Wall(TEXT("HallE_TerraceDoor_Lintel"), 400, -1000, 400, -800, DarkStone, 300.f, WallH);
	Wall(TEXT("HallE_BedroomDoor_Lintel"), 400, 700, 400, 900, DarkStone, 300.f, WallH);

	// Bedroom / terrace wall with the window Harker looks through (x 1000..1300, z 100..260).
	Wall(TEXT("Bedroom_South_West"), 400, 200, 1000, 200, Stone);
	Wall(TEXT("Bedroom_Window_Sill"), 1000, 200, 1300, 200, Stone, 0.f, 100.f);
	Wall(TEXT("Bedroom_Window_Header"), 1000, 200, 1300, 200, Stone, 260.f, WallH);
	Wall(TEXT("Bedroom_South_East"), 1300, 200, 1800, 200, Stone);

	const TCHAR* Dressing = TEXT("DirectorsCut/Castle/Dressing");
	// Hallway
	Block(TEXT("Hall_Pillar_South"), Dressing, FVector(-100, -300, WallH / 2), FVector(80, 80, WallH), DarkStone);
	Block(TEXT("Hall_Pillar_North"), Dressing, FVector(-100, 400, WallH / 2), FVector(80, 80, WallH), DarkStone);
	Block(TEXT("Hall_Bench"), Dressing, FVector(-500, -200, 22.5f), FVector(60, 200, 45), DarkWood);
	// Cell
	Block(TEXT("Cell_StrawBed"), Dressing, FVector(-1600, -1250, 15), FVector(200, 120, 30), Straw);
	Block(TEXT("Cell_Stool"), Dressing, FVector(-1300, -380, 22.5f), FVector(50, 50, 45), DarkWood);
	Block(TEXT("Cell_Barrel"), Dressing, FVector(-800, -1250, 50), FVector(80, 80, 100), DarkWood);
	Block(TEXT("Cell_Chains"), Dressing, FVector(-1785, -700, 200), FVector(10, 300, 20), DarkStone);
	// Library
	Block(TEXT("Library_Shelves_West"), Dressing, FVector(-1750, 700, 160), FVector(60, 1200, 320), DarkWood);
	Block(TEXT("Library_Shelves_North"), Dressing, FVector(-1200, 1360, 150), FVector(800, 60, 300), DarkWood);
	Block(TEXT("Library_Table"), Dressing, FVector(-1200, 700, 40), FVector(220, 120, 80), Wood);
	Block(TEXT("Library_Armchair"), Dressing, FVector(-900, 1100, 45), FVector(100, 100, 90), Velvet);
	// Bedroom
	Block(TEXT("Bed"), Dressing, FVector(1500, 1100, 27.5f), FVector(220, 280, 55), Velvet);
	Block(TEXT("Bed_Headboard"), Dressing, FVector(1500, 1255, 80), FVector(240, 20, 160), Wood);
	Block(TEXT("Bedroom_Desk"), Dressing, FVector(800, 1320, 37.5f), FVector(180, 80, 75), Wood);
	Block(TEXT("Bedroom_Wardrobe"), Dressing, FVector(1720, 600, 115), FVector(120, 220, 230), Wood);
	Block(TEXT("Bedroom_Dresser"), Dressing, FVector(1740, 1000, 50), FVector(80, 120, 100), Wood);
	// Terrace
	Block(TEXT("Terrace_Planter_West"), Dressing, FVector(800, -500, 40), FVector(120, 120, 80), Moss);
	Block(TEXT("Terrace_Planter_East"), Dressing, FVector(1650, -600, 40), FVector(120, 120, 80), Moss);
	Block(TEXT("Terrace_Statue"), Dressing, FVector(1000, -1000, 110), FVector(60, 60, 220), Stone);
	Block(TEXT("Terrace_Bench"), Dressing, FVector(1200, -1250, 22.5f), FVector(200, 60, 45), Stone);

	// --- Lighting -------------------------------------------------------------------------
	const TCHAR* Lighting = TEXT("DirectorsCut/Lighting");
	ADirectionalLight* Moon = Place<ADirectionalLight>(World, FTransform(FRotator(-35.f, 130.f, 0.f), FVector(0, 0, 800)), TEXT("Moonlight"), Lighting);
	if (UDirectionalLightComponent* MoonLight = Cast<UDirectionalLightComponent>(Moon->GetLightComponent()))
	{
		MoonLight->SetIntensity(2.5f);
		MoonLight->SetLightColor(FLinearColor(0.6f, 0.7f, 1.f));
		MoonLight->SetAtmosphereSunLight(true);
	}
	Place<ASkyAtmosphere>(World, FTransform::Identity, TEXT("SkyAtmosphere"), Lighting);
	ASkyLight* Sky = Place<ASkyLight>(World, FTransform(FVector(0, 0, 800)), TEXT("SkyLight"), Lighting);
	if (USkyLightComponent* SkyLight = Sky->GetLightComponent())
	{
		SkyLight->SetMobility(EComponentMobility::Movable);
		SkyLight->bRealTimeCapture = true;
		SkyLight->SetIntensity(0.6f);
	}
	AExponentialHeightFog* Fog = Place<AExponentialHeightFog>(World, FTransform::Identity, TEXT("HeightFog"), Lighting);
	Fog->GetComponent()->SetFogDensity(0.01f);

	auto Lamp = [World, Lighting](const FString& Label, const FVector& Location, const FLinearColor& Color, float Intensity)
	{
		APointLight* Light = Place<APointLight>(World, FTransform(Location), Label, Lighting);
		if (UPointLightComponent* Point = Cast<UPointLightComponent>(Light->GetLightComponent()))
		{
			Point->SetIntensity(Intensity);
			Point->SetLightColor(Color);
			Point->SetAttenuationRadius(1300.f);
		}
	};
	const FLinearColor Warm(1.f, 0.65f, 0.35f);
	const FLinearColor Cool(0.55f, 0.65f, 1.f);
	Lamp(TEXT("Lamp_Cell"), FVector(-1200, -700, 320), FLinearColor(0.7f, 0.9f, 0.6f), 3500.f);
	Lamp(TEXT("Lamp_Library"), FVector(-1200, 700, 300), Warm, 6000.f);
	Lamp(TEXT("Lamp_Hall_South"), FVector(-100, -800, 380), Cool, 4000.f);
	Lamp(TEXT("Lamp_Hall_North"), FVector(-100, 800, 380), Cool, 4000.f);
	Lamp(TEXT("Lamp_Bedroom"), FVector(1100, 900, 300), Warm, 6000.f);
	Lamp(TEXT("Lamp_Terrace"), FVector(1100, -600, 380), Cool, 3000.f);

	// --- Cast -----------------------------------------------------------------------------
	auto Performer = [World](const FString& Name, float X, float Y, float Yaw, const FLinearColor& Body, const FLinearColor& Head, bool bCape, bool bProp)
	{
		const FTransform Xf(FRotator(0.f, Yaw, 0.f), FVector(X, Y, 0.f));
		ADCPerformer* P = BeginPlace<ADCPerformer>(World, Xf);
		P->DisplayName = Name;
		P->BodyColor = Body;
		P->HeadColor = Head;
		P->bHasCape = bCape;
		P->bHasProp = bProp;
		return FinishPlace(P, Xf, Name, TEXT("DirectorsCut/Cast"));
	};

	// Waypoint(Time, X, Y, Yaw, Lean, PropRaise, Tilt)
	struct FWp { float T, X, Y, Yaw, Lean = 0.f, Raise = 0.f, Tilt = 0.f; };
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
			Wp->PropRaise = W.Raise;
			Wp->Tilt = W.Tilt;
			FinishPlace(Wp, Xf, FString::Printf(TEXT("WP_%s_%04.1f"), *P->DisplayName, W.T), *Folder);
		}
	};

	const FLinearColor Skin(0.9f, 0.72f, 0.6f);

	ADCPerformer* Dracula = Performer(TEXT("Dracula"), -1600, -1150, 63, FLinearColor(0.04f, 0.03f, 0.04f), FLinearColor(0.85f, 0.88f, 0.85f), true, false);
	Blocking(Dracula, {
		{ 9.f, -1600, -1150, 63 },          // waits in the cell's shadows
		{ 11.f, -1420, -850, 63 },          // steps toward Renfield
		{ 16.f, -1420, -850, 63 },
		{ 18.5f, -800, -700, 15 },          // out of the cell
		{ 20.5f, -450, -700, 0 },
		{ 22.f, 150, -650, 85 },            // stalks north up the hallway
		{ 27.f, 150, 750, 90 },
		{ 29.5f, 650, 800, 0 },             // into the bedroom
		{ 33.f, 1250, 820, 45 },            // reaches Lucy
		{ 34.f, 1250, 820, 45, 32 },        // the bite
		{ 41.f, 1250, 820, 45, 32 },
		{ 42.f, 1250, 820, 45, 0 },
		{ 43.5f, 1250, 820, -100 },         // notices the face at the window
		{ 50.f, 1250, 820, -100 },
		{ 52.f, 1250, 820, -178, -12 },     // Van Helsing! recoils
		{ 58.f, 1250, 820, -178, -12 },
	});

	ADCPerformer* Lucy = Performer(TEXT("Lucy"), 800, 1180, -90, FLinearColor(0.88f, 0.88f, 0.94f), Skin, false, true);
	Blocking(Lucy, {
		{ 5.5f, 800, 1180, -90, 0, 0 },
		{ 6.5f, 800, 1180, -90, 0, 1 },     // reads her diary
		{ 13.5f, 800, 1180, -90, 0, 1 },
		{ 14.5f, 800, 1180, -90, 0, 0 },
		{ 18.f, 800, 1180, -90 },
		{ 22.f, 1150, 330, -90 },           // gazes out of the window
		{ 26.f, 1150, 330, -90 },
		{ 30.f, 1330, 900, -135 },          // turns to the door as he enters
		{ 41.5f, 1330, 900, -135 },
		{ 43.f, 1330, 900, -135, 0, 0, 65 },// swoons
	});

	ADCPerformer* Harker = Performer(TEXT("Harker"), 1600, -1250, -45, FLinearColor(0.35f, 0.25f, 0.15f), Skin, false, false);
	Blocking(Harker, {
		{ 20.f, 1600, -1250, -45 },         // smoking at the balustrade
		{ 24.f, 1250, -300, 110 },          // hears something
		{ 28.f, 1150, 90, 90 },             // peers through the bedroom window
		{ 37.5f, 1150, 90, 90 },
		{ 40.f, 1150, 90, 90, 0, 0, 85 },   // faints
	});

	ADCPerformer* VanHelsing = Performer(TEXT("VanHelsing"), -1200, 560, 90, FLinearColor(0.45f, 0.45f, 0.48f), Skin, false, true);
	Blocking(VanHelsing, {
		{ 7.f, -1200, 560, 90, 0, 0 },
		{ 8.f, -1200, 560, 90, 0, 1 },      // sharpens the stake
		{ 15.5f, -1200, 560, 90, 0, 1 },
		{ 16.5f, -1200, 560, 90, 0, 0 },
		{ 20.f, -1200, 560, 90 },
		{ 23.f, -750, 700, 0 },             // leaves the library
		{ 25.f, -400, 700, -90 },
		{ 30.f, -400, -700, -90 },          // down the hallway to the cell
		{ 31.5f, -480, -700, 180 },         // questions Renfield
		{ 40.f, -480, -700, 180 },
		{ 41.f, -480, -700, 90 },
		{ 44.5f, -400, 650, 90 },           // runs north
		{ 46.f, -300, 800, 0 },
		{ 50.f, 650, 800, 0 },
		{ 51.5f, 720, 805, -2, 0, 1 },      // stake raised
		{ 58.f, 720, 805, -2, 0, 1 },
	});

	ADCPerformer* Renfield = Performer(TEXT("Renfield"), -1300, -600, 0, FLinearColor(0.3f, 0.35f, 0.25f), Skin, false, false);
	Blocking(Renfield, {
		{ 3.5f, -1300, -600, 0 },
		{ 4.5f, -1300, -600, 0, 22 },       // snatching flies
		{ 5.5f, -1300, -600, 0, 0 },
		{ 6.5f, -1300, -600, 0, 22 },
		{ 7.5f, -1300, -600, 0, 0 },
		{ 8.5f, -1300, -600, 0, 22 },
		{ 9.5f, -1300, -600, 0, 0 },
		{ 11.f, -1300, -600, -116 },        // the master arrives
		{ 12.f, -1300, -600, -116, 0, 0, -30 },
		{ 15.5f, -1300, -600, -116, 0, 0, -30 },
		{ 16.5f, -1300, -600, -116 },
		{ 26.f, -1300, -600, 0 },
		{ 30.f, -800, -700, 0 },            // rushes to the bars
		{ 32.f, -800, -700, 0, 15 },
		{ 33.f, -800, -700, 0, 0 },
		{ 34.f, -800, -700, 0, 15 },
		{ 35.f, -800, -700, 0, 0 },
		{ 36.f, -800, -700, 0, 15 },
		{ 37.f, -800, -700, 0, 0 },
		{ 38.f, -800, -700, 0, 15 },
		{ 39.f, -800, -700, 0, 0 },
		{ 42.f, -1300, -600, -116 },
	});

	// --- Hidden beats ---------------------------------------------------------------------
	auto Anchor = [](ADCPerformer* P, EDCAnchorPart Part, bool bFace = false)
	{
		FDCAnchorRef Ref;
		Ref.Performer = P;
		Ref.Part = Part;
		Ref.bMustSeeFace = bFace;
		return Ref;
	};
	auto Beat = [World](const TCHAR* Id, const TCHAR* Name, const TCHAR* Caption, bool bMandatory, float Start, float End, float Hold,
		int32 Points, float LabelX, float LabelY, TArray<FDCAnchorRef> Anchors)
	{
		const FTransform Xf(FVector(LabelX, LabelY, 280.f));
		ADCBeat* B = BeginPlace<ADCBeat>(World, Xf);
		B->BeatId = Id;
		B->DisplayName = Name;
		B->Caption = Caption;
		B->bMandatory = bMandatory;
		B->WindowStart = Start;
		B->WindowEnd = End;
		B->RequiredHold = Hold;
		B->Points = Points;
		B->Anchors = MoveTemp(Anchors);
		FinishPlace(B, Xf, FString::Printf(TEXT("Beat_%s"), Id), TEXT("DirectorsCut/Beats"));
	};
	using EP = EDCAnchorPart;

	Beat(TEXT("R1"), TEXT("Renfield's feast"), TEXT("The madman snatches flies, waiting for his master."), false, 4.f, 10.f, 2.f, 100, -1300, -500,
		{ Anchor(Renfield, EP::Head, true) });
	Beat(TEXT("L1"), TEXT("Lucy's diary"), TEXT("She writes of strange dreams."), false, 6.f, 14.f, 2.f, 150, 800, 1100,
		{ Anchor(Lucy, EP::Prop) });
	Beat(TEXT("V1"), TEXT("Preparing the stake"), TEXT("Van Helsing already suspects."), false, 8.f, 16.f, 2.f, 150, -1200, 500,
		{ Anchor(VanHelsing, EP::Prop) });
	Beat(TEXT("R2"), TEXT("The master's blessing"), TEXT("Renfield bows to the shadow in his cell."), false, 11.f, 16.f, 2.f, 250, -1400, -800,
		{ Anchor(Dracula, EP::Head), Anchor(Renfield, EP::Head) });
	Beat(TEXT("D1"), TEXT("The stalk"), TEXT("He glides through the castle toward her room."), false, 20.f, 28.f, 2.f, 150, 150, 0,
		{ Anchor(Dracula, EP::Head, true) });
	Beat(TEXT("J1"), TEXT("The witness"), TEXT("Harker's face at the window - he sees everything."), false, 29.f, 37.f, 1.5f, 300, 1150, 150,
		{ Anchor(Harker, EP::Head, true) });
	Beat(TEXT("M01"), TEXT("The bite"), TEXT("The studio's essential story beat."), true, 33.f, 42.f, 2.5f, 1000, 1290, 860,
		{ Anchor(Dracula, EP::Head), Anchor(Lucy, EP::Head) });
	Beat(TEXT("V2"), TEXT("Renfield's warning"), TEXT("Through the bars, the madman tells Van Helsing the master has come."), false, 31.f, 40.f, 2.f, 250, -640, -700,
		{ Anchor(VanHelsing, EP::Head), Anchor(Renfield, EP::Head) });
	Beat(TEXT("J2"), TEXT("Harker faints"), TEXT("The witness collapses on the terrace."), false, 38.f, 45.f, 1.5f, 250, 1150, -50,
		{ Anchor(Harker, EP::Body) });
	Beat(TEXT("D2"), TEXT("The master notices"), TEXT("Dracula turns toward the window. He knows he was seen."), false, 44.f, 50.f, 1.5f, 250, 1250, 700,
		{ Anchor(Dracula, EP::Head, true) });
	Beat(TEXT("V3"), TEXT("The confrontation"), TEXT("Van Helsing bursts in, stake raised."), false, 52.f, 58.f, 2.f, 400, 1000, 810,
		{ Anchor(VanHelsing, EP::Prop), Anchor(Dracula, EP::Head) });

	// --- Player marks ---------------------------------------------------------------------
	Place<ADCStartMark>(World, FTransform(FRotator(0.f, 90.f, 0.f), FVector(-100, -1200, 0)), TEXT("StartMark"), TEXT("DirectorsCut/Marks"));
	Place<ADCClapperboard>(World, FTransform(FVector(-350, -1250, 0)), TEXT("Clapperboard"), TEXT("DirectorsCut/Marks"));

	SetFolderPath(TEXT("DirectorsCut"));
	World->MarkPackageDirty();
	UE_LOG(LogTemp, Log, TEXT("Directors Cut: castle scene placed. Save the level to keep it."));
}

#endif // WITH_EDITOR
