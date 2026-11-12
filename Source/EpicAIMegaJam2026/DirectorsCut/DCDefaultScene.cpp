// Director's Cut prototype - editor button that places the default "Dracula: The Midnight Visit" scene as editable actors.

#include "DCTakeDirector.h"

#if WITH_EDITOR

#include "DCPerformer.h"
#include "DCSceneMarks.h"
#include "DCSetPiece.h"
#include "DCShotMarker.h"
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
	T* FinishPlace(T* Actor, const FTransform& Transform, const TCHAR* Label, const TCHAR* Folder)
	{
		Actor->FinishSpawning(Transform);
		Actor->SetActorLabel(Label);
		Actor->SetFolderPath(FName(Folder));
		return Actor;
	}

	template <typename T>
	T* Place(UWorld* World, const FTransform& Transform, const TCHAR* Label, const TCHAR* Folder)
	{
		T* Actor = World->SpawnActor<T>(T::StaticClass(), Transform);
		Actor->SetActorLabel(Label);
		Actor->SetFolderPath(FName(Folder));
		return Actor;
	}
}

void ADCTakeDirector::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(ADCTakeDirector, bBuildDefaultScene) && bBuildDefaultScene)
	{
		bBuildDefaultScene = false;
		BuildDefaultScene();
	}
}

void ADCTakeDirector::BuildDefaultScene()
{
	UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		return;
	}
	for (TActorIterator<ADCShotMarker> It(World); It; ++It)
	{
		UE_LOG(LogTemp, Warning, TEXT("Directors Cut: this level already has shot markers; delete the existing scene before building the default one."));
		return;
	}

	// --- Set pieces -----------------------------------------------------------------------
	const FLinearColor Wood(0.16f, 0.09f, 0.05f);
	const FLinearColor Stone(0.32f, 0.3f, 0.36f);
	const FLinearColor DarkStone(0.15f, 0.14f, 0.17f);
	const FLinearColor Velvet(0.4f, 0.03f, 0.06f);
	const FLinearColor Concrete(0.12f, 0.12f, 0.13f);
	constexpr float WallH = 450.f;

	auto Block = [World](const TCHAR* Label, const TCHAR* Folder, const FVector& Center, const FVector& Size, const FLinearColor& Color)
	{
		const FTransform Xf(FRotator::ZeroRotator, Center, Size / 100.f);
		ADCSetPiece* Piece = BeginPlace<ADCSetPiece>(World, Xf);
		Piece->Color = Color;
		FinishPlace(Piece, Xf, Label, Folder);
	};

	const TCHAR* Structure = TEXT("DirectorsCut/Set/Structure");
	Block(TEXT("Floor_Bedroom"), Structure, FVector(0, 0, -10), FVector(2400, 1800, 20), Wood);
	Block(TEXT("Floor_Backstage"), Structure, FVector(-600, -1110, -10), FVector(820, 420, 20), Concrete);
	Block(TEXT("Wall_North"), Structure, FVector(0, 910, WallH / 2), FVector(2440, 20, WallH), Stone);
	Block(TEXT("Wall_East"), Structure, FVector(1210, 0, WallH / 2), FVector(20, 1840, WallH), Stone);
	Block(TEXT("Wall_West"), Structure, FVector(-1210, 0, WallH / 2), FVector(20, 1840, WallH), Stone);
	Block(TEXT("Wall_South_West"), Structure, FVector(-950, -910, WallH / 2), FVector(500, 20, WallH), Stone);
	Block(TEXT("Wall_South_East"), Structure, FVector(350, -910, WallH / 2), FVector(1700, 20, WallH), Stone);
	Block(TEXT("Arch_Lintel"), Structure, FVector(-600, -910, 375), FVector(200, 20, 150), DarkStone);
	Block(TEXT("Arch_Pillar_West"), Structure, FVector(-720, -910, 150), FVector(40, 60, 300), DarkStone);
	Block(TEXT("Arch_Pillar_East"), Structure, FVector(-480, -910, 150), FVector(40, 60, 300), DarkStone);
	Block(TEXT("Backstage_Wall_West"), Structure, FVector(-1020, -1110, WallH / 2), FVector(20, 420, WallH), Concrete);
	Block(TEXT("Backstage_Wall_East"), Structure, FVector(-180, -1110, WallH / 2), FVector(20, 420, WallH), Concrete);
	Block(TEXT("Backstage_Wall_South"), Structure, FVector(-600, -1330, WallH / 2), FVector(860, 20, WallH), Concrete);

	const TCHAR* Dressing = TEXT("DirectorsCut/Set/Dressing");
	Block(TEXT("Desk"), Dressing, FVector(-300, 800, 37.5f), FVector(180, 90, 75), Wood);
	Block(TEXT("Bed"), Dressing, FVector(800, 650, 27.5f), FVector(260, 260, 55), Velvet);
	Block(TEXT("Bed_Headboard"), Dressing, FVector(800, 790, 90), FVector(280, 20, 180), Wood);
	Block(TEXT("Curtain"), Dressing, FVector(150, -775, 200), FVector(20, 250, 400), Velvet);
	Block(TEXT("Trunk"), Dressing, FVector(-750, 350, 60), FVector(150, 100, 120), Wood);
	Block(TEXT("Wardrobe"), Dressing, FVector(950, -300, 110), FVector(120, 200, 220), Wood);
	Block(TEXT("Mirror"), Dressing, FVector(1190, 150, 150), FVector(10, 120, 200), FLinearColor(0.7f, 0.75f, 0.8f));

	// --- Lighting -------------------------------------------------------------------------
	const TCHAR* Lighting = TEXT("DirectorsCut/Lighting");
	ADirectionalLight* Moon = Place<ADirectionalLight>(World, FTransform(FRotator(-35.f, 40.f, 0.f), FVector(0, 0, 600)), TEXT("Moonlight"), Lighting);
	if (UDirectionalLightComponent* MoonLight = Cast<UDirectionalLightComponent>(Moon->GetLightComponent()))
	{
		MoonLight->SetIntensity(2.5f);
		MoonLight->SetLightColor(FLinearColor(0.6f, 0.7f, 1.f));
		MoonLight->SetAtmosphereSunLight(true);
	}
	Place<ASkyAtmosphere>(World, FTransform::Identity, TEXT("SkyAtmosphere"), Lighting);
	ASkyLight* Sky = Place<ASkyLight>(World, FTransform(FVector(0, 0, 600)), TEXT("SkyLight"), Lighting);
	if (USkyLightComponent* SkyLight = Sky->GetLightComponent())
	{
		SkyLight->SetMobility(EComponentMobility::Movable);
		SkyLight->bRealTimeCapture = true;
		SkyLight->SetIntensity(0.6f);
	}
	AExponentialHeightFog* Fog = Place<AExponentialHeightFog>(World, FTransform::Identity, TEXT("HeightFog"), Lighting);
	Fog->GetComponent()->SetFogDensity(0.01f);

	auto Lamp = [World, Lighting](const TCHAR* Label, const FVector& Location, const FLinearColor& Color, float Intensity)
	{
		APointLight* Light = Place<APointLight>(World, FTransform(Location), Label, Lighting);
		if (UPointLightComponent* Point = Cast<UPointLightComponent>(Light->GetLightComponent()))
		{
			Point->SetIntensity(Intensity);
			Point->SetLightColor(Color);
			Point->SetAttenuationRadius(1400.f);
		}
	};
	const FLinearColor Warm(1.f, 0.65f, 0.35f);
	Lamp(TEXT("Lamp_Desk"), FVector(-300, 700, 250), Warm, 6000.f);
	Lamp(TEXT("Lamp_Bed"), FVector(800, 500, 250), Warm, 6000.f);
	Lamp(TEXT("Lamp_MoonFill"), FVector(0, -300, 380), FLinearColor(0.5f, 0.6f, 1.f), 4000.f);
	Lamp(TEXT("Lamp_Backstage"), FVector(-600, -1100, 300), FLinearColor(0.9f, 0.9f, 1.f), 2500.f);
	Lamp(TEXT("Lamp_Start"), FVector(-1000, 0, 350), FLinearColor::White, 3000.f);

	// --- Cast and blocking ----------------------------------------------------------------
	auto Performer = [World](const TCHAR* Label, const FString& Name, float X, float Y, float Yaw,
		const FLinearColor& Body, const FLinearColor& Head, bool bCape, bool bProp)
	{
		const FTransform Xf(FRotator(0.f, Yaw, 0.f), FVector(X, Y, 0.f));
		ADCPerformer* P = BeginPlace<ADCPerformer>(World, Xf);
		P->DisplayName = Name;
		P->BodyColor = Body;
		P->HeadColor = Head;
		P->bHasCape = bCape;
		P->bHasProp = bProp;
		return FinishPlace(P, Xf, Label, TEXT("DirectorsCut/Cast"));
	};

	auto Waypoint = [World](ADCPerformer* P, const TCHAR* Folder, float Time, float X, float Y, float Yaw, float Lean = 0.f, float Raise = 0.f)
	{
		const FTransform Xf(FRotator(0.f, Yaw, 0.f), FVector(X, Y, 0.f));
		ADCWaypoint* W = BeginPlace<ADCWaypoint>(World, Xf);
		W->Performer = P;
		W->Time = Time;
		W->Lean = Lean;
		W->PropRaise = Raise;
		const FString Label = FString::Printf(TEXT("WP_%s_%05.1f"), *P->DisplayName, Time);
		FinishPlace(W, Xf, *Label, Folder);
	};

	// Victim: reads the letter at the desk, then waits by the bed.
	ADCPerformer* Victim = Performer(TEXT("Victim"), TEXT("Victim"), -300, 620, -90,
		FLinearColor(0.85f, 0.85f, 0.9f), FLinearColor(0.9f, 0.72f, 0.6f), false, true);
	const TCHAR* VictimFolder = TEXT("DirectorsCut/Cast/Victim_Blocking");
	Waypoint(Victim, VictimFolder, 5.5f, -300, 620, -90, 0, 0);
	Waypoint(Victim, VictimFolder, 6.5f, -300, 620, -90, 0, 1);   // letter reveal
	Waypoint(Victim, VictimFolder, 12.f, -300, 620, -90, 0, 1);
	Waypoint(Victim, VictimFolder, 13.f, -300, 620, -90, 0, 0);
	Waypoint(Victim, VictimFolder, 14.f, -300, 620, -90);
	Waypoint(Victim, VictimFolder, 21.f, 560, 420, -135);         // waits by the bed

	// Dracula: enters through the arch, hesitates, bites, retreats, looks back.
	ADCPerformer* Dracula = Performer(TEXT("Dracula"), TEXT("Dracula"), -600, -1150, 90,
		FLinearColor(0.04f, 0.03f, 0.04f), FLinearColor(0.85f, 0.88f, 0.85f), true, false);
	const TCHAR* DraculaFolder = TEXT("DirectorsCut/Cast/Dracula_Blocking");
	Waypoint(Dracula, DraculaFolder, 2.f, -600, -1150, 90);
	Waypoint(Dracula, DraculaFolder, 6.f, -600, -700, 77);        // entrance through the arch
	Waypoint(Dracula, DraculaFolder, 12.f, -600, -700, 77);
	Waypoint(Dracula, DraculaFolder, 16.f, -150, -250, 43);       // approach
	Waypoint(Dracula, DraculaFolder, 17.f, -150, -250, 43, -14);  // hesitation recoil
	Waypoint(Dracula, DraculaFolder, 21.f, -150, -250, 43, -14);
	Waypoint(Dracula, DraculaFolder, 22.f, -150, -250, 43, 0);
	Waypoint(Dracula, DraculaFolder, 26.f, 480, 340, 45);         // reaches the victim
	Waypoint(Dracula, DraculaFolder, 27.f, 480, 340, 45, 32);     // bite
	Waypoint(Dracula, DraculaFolder, 32.f, 480, 340, 45, 32);
	Waypoint(Dracula, DraculaFolder, 33.f, 480, 340, 45, 0);
	Waypoint(Dracula, DraculaFolder, 37.f, 350, -600, -98);       // retreat to the curtain
	Waypoint(Dracula, DraculaFolder, 38.5f, 350, -600, 78);       // looks back

	// --- Shots ----------------------------------------------------------------------------
	auto Shot = [World](const TCHAR* Id, const TCHAR* Name, const TCHAR* Caption, bool bMandatory, float Start, float End, float Hold,
		float X, float Y, float MinD, float MaxD, int32 Points, TArray<FDCAnchorRef> Anchors)
	{
		const FTransform Xf(FVector(X, Y, 0.f));
		ADCShotMarker* M = BeginPlace<ADCShotMarker>(World, Xf);
		M->ShotId = Id;
		M->DisplayName = Name;
		M->Caption = Caption;
		M->bMandatory = bMandatory;
		M->WindowStart = Start;
		M->WindowEnd = End;
		M->RequiredHold = Hold;
		M->MinDistance = MinD;
		M->MaxDistance = MaxD;
		M->Points = Points;
		M->Anchors = MoveTemp(Anchors);
		FinishPlace(M, Xf, *FString::Printf(TEXT("Shot_%s"), Id), TEXT("DirectorsCut/Shots"));
	};
	auto Anchor = [](ADCPerformer* P, EDCAnchorPart Part)
	{
		FDCAnchorRef Ref;
		Ref.Performer = P;
		Ref.Part = Part;
		return Ref;
	};

	Shot(TEXT("B01"), TEXT("The invitation"), TEXT("She was expecting a visitor."), false, 6.f, 12.f, 1.5f, -300, 200, 150.f, 500.f, 250,
		{ Anchor(Victim, EDCAnchorPart::Prop) });
	Shot(TEXT("B02"), TEXT("A moment of doubt"), TEXT("The monster hesitates before the attack."), false, 16.f, 22.f, 1.5f, -550, -350, 200.f, 650.f, 250,
		{ Anchor(Dracula, EDCAnchorPart::Head) });
	Shot(TEXT("M01"), TEXT("The bite"), TEXT("The studio's essential story beat."), true, 26.f, 33.f, 2.f, 650, 50, 200.f, 600.f, 1000,
		{ Anchor(Dracula, EDCAnchorPart::Head), Anchor(Victim, EDCAnchorPart::Head) });
	Shot(TEXT("B03"), TEXT("The aftermath"), TEXT("He looks back - a hint of remorse."), false, 37.f, 43.f, 1.5f, -50, -450, 200.f, 650.f, 250,
		{ Anchor(Dracula, EDCAnchorPart::Head) });

	// --- Player marks ---------------------------------------------------------------------
	Place<ADCStartMark>(World, FTransform(FVector(-1000, 0, 0)), TEXT("StartMark"), TEXT("DirectorsCut/Marks"));
	Place<ADCClapperboard>(World, FTransform(FVector(-1050, -200, 0)), TEXT("Clapperboard"), TEXT("DirectorsCut/Marks"));

	SetFolderPath(TEXT("DirectorsCut"));
	World->MarkPackageDirty();
	UE_LOG(LogTemp, Log, TEXT("Directors Cut: default scene placed. Save the level to keep it."));
}

#endif // WITH_EDITOR
