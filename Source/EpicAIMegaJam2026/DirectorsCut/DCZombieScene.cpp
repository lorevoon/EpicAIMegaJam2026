// Director's Cut - editor button that places the "Love in the Ruins" town square as editable actors.
//
// Layout (cm, X east / Y north): square x -1500..1500, y -1200..1200 inside ruined brick walls, exit gap east at
// y -250..250 (the sunset). Fountain in the middle; wrecked car south-west; crate barricade south-east; burned-out
// bus north-west; kiosk north-east. Big props are there to hide zombies behind.
//
// Storyboard (60 s):        Zombies that ruin the obvious angles:
//   1  7-13  peaches           Car zombie crosses south of the car 0-10 s, then lingers in the south-west corner
//   2  9-15  Ellie watches     Alley zombie shuffles in behind the barricade from the south-east 4-16 s
//   3 19-25  the standoff      Shambler crosses north of the fountain 18-40 s
//   4 26-32  the offer         Bus lurker steps out from behind the bus 36-60 s toward the lovers
//   5 33-39  the laugh         Kiosk zombie lurches toward the exit 44-60 s
//   6 41-47  THE KISS
//   7 50-58  walking off into the sunset

#include "DCZombieDirector.h"

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

	constexpr float TownWallH = 350.f;
	constexpr float TownWallT = 30.f;
}

void ADCZombieDirector::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(ADCZombieDirector, bBuildZombieScene) && bBuildZombieScene)
	{
		bBuildZombieScene = false;
		BuildZombieScene();
	}
}

void ADCZombieDirector::BuildZombieScene()
{
	UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		return;
	}
	for (TActorIterator<ADCBeat> It(World); It; ++It)
	{
		UE_LOG(LogTemp, Warning, TEXT("Directors Cut: this level already has beats; delete the existing scene before building the zombie town."));
		return;
	}

	// --- Set ------------------------------------------------------------------------------
	const FLinearColor Asphalt(0.07f, 0.07f, 0.075f);
	const FLinearColor Brick(0.35f, 0.17f, 0.11f);
	const FLinearColor Concrete(0.3f, 0.3f, 0.3f);
	const FLinearColor Rust(0.35f, 0.15f, 0.06f);
	const FLinearColor BusYellow(0.6f, 0.45f, 0.08f);
	const FLinearColor Wood(0.25f, 0.16f, 0.08f);
	const FLinearColor Burnt(0.05f, 0.04f, 0.04f);

	auto Block = [World](const FString& Label, const TCHAR* Folder, const FVector& Center, const FVector& Size, const FLinearColor& Color)
	{
		const FTransform Xf(FRotator::ZeroRotator, Center, Size / 100.f);
		ADCSetPiece* Piece = BeginPlace<ADCSetPiece>(World, Xf);
		Piece->Color = Color;
		FinishPlace(Piece, Xf, Label, Folder);
	};
	auto Wall = [&Block](const FString& Label, float X1, float Y1, float X2, float Y2, const FLinearColor& Color, float Height = TownWallH)
	{
		const bool bAlongX = FMath::Abs(X2 - X1) > FMath::Abs(Y2 - Y1);
		const FVector Size = bAlongX ? FVector(FMath::Abs(X2 - X1) + TownWallT, TownWallT, Height) : FVector(TownWallT, FMath::Abs(Y2 - Y1) + TownWallT, Height);
		Block(Label, TEXT("DirectorsCut/Town/Walls"), FVector((X1 + X2) * 0.5f, (Y1 + Y2) * 0.5f, Height * 0.5f), Size, Color);
	};

	Block(TEXT("Ground"), TEXT("DirectorsCut/Town/Walls"), FVector(0, 0, -10), FVector(3000, 2400, 20), Asphalt);
	Wall(TEXT("Wall_North"), -1500, 1200, 1500, 1200, Brick);
	Wall(TEXT("Wall_South"), -1500, -1200, 1500, -1200, Brick);
	Wall(TEXT("Wall_West"), -1500, -1200, -1500, 1200, Brick);
	Wall(TEXT("Wall_East_North"), 1500, 250, 1500, 1200, Brick);
	Wall(TEXT("Wall_East_South"), 1500, -1200, 1500, -250, Brick);
	Wall(TEXT("Wall_North_Collapsed"), -200, 1200, 400, 1200, Brick, 520.f);

	const TCHAR* Props = TEXT("DirectorsCut/Town/Props");
	Block(TEXT("Fountain_Basin"), Props, FVector(0, 0, 30), FVector(320, 320, 60), Concrete);
	Block(TEXT("Fountain_Pillar"), Props, FVector(0, 0, 110), FVector(60, 60, 220), Concrete);
	Block(TEXT("Wrecked_Car"), Props, FVector(-900, -650, 70), FVector(420, 190, 140), Rust);
	Block(TEXT("Barricade_Crates"), Props, FVector(600, -650, 75), FVector(120, 420, 150), Wood);
	Block(TEXT("Burned_Bus"), Props, FVector(-650, 700, 150), FVector(1000, 260, 300), BusYellow);
	Block(TEXT("Kiosk"), Props, FVector(850, 750, 140), FVector(320, 320, 280), Concrete);
	Block(TEXT("Dead_Tree"), Props, FVector(-500, 100, 200), FVector(60, 60, 400), Burnt);
	Block(TEXT("Dumpster"), Props, FVector(1050, -800, 70), FVector(220, 120, 140), FLinearColor(0.1f, 0.25f, 0.12f));
	Block(TEXT("Rubble"), Props, FVector(250, -1000, 40), FVector(250, 150, 80), Concrete);
	Block(TEXT("Fire_Barrel"), Props, FVector(-200, -750, 50), FVector(60, 60, 100), Rust);

	// --- Lighting: sunset over the eastern exit ---------------------------------------------
	const TCHAR* Lighting = TEXT("DirectorsCut/Lighting");
	ADirectionalLight* Sun = Place<ADirectionalLight>(World, FTransform(FRotator(-12.f, 180.f, 0.f), FVector(0, 0, 800)), TEXT("Sunset"), Lighting);
	if (UDirectionalLightComponent* SunLight = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
	{
		SunLight->SetIntensity(6.f);
		SunLight->SetLightColor(FLinearColor(1.f, 0.55f, 0.3f));
		SunLight->SetAtmosphereSunLight(true);
	}
	Place<ASkyAtmosphere>(World, FTransform::Identity, TEXT("SkyAtmosphere"), Lighting);
	ASkyLight* Sky = Place<ASkyLight>(World, FTransform(FVector(0, 0, 800)), TEXT("SkyLight"), Lighting);
	if (USkyLightComponent* SkyLight = Sky->GetLightComponent())
	{
		SkyLight->SetMobility(EComponentMobility::Movable);
		SkyLight->bRealTimeCapture = true;
		SkyLight->SetIntensity(0.8f);
	}
	AExponentialHeightFog* Fog = Place<AExponentialHeightFog>(World, FTransform::Identity, TEXT("HeightFog"), Lighting);
	Fog->GetComponent()->SetFogDensity(0.02f);
	APointLight* Fire = Place<APointLight>(World, FTransform(FVector(-200, -750, 140)), TEXT("Barrel_Fire"), Lighting);
	if (UPointLightComponent* FireLight = Cast<UPointLightComponent>(Fire->GetLightComponent()))
	{
		FireLight->SetIntensity(5000.f);
		FireLight->SetLightColor(FLinearColor(1.f, 0.45f, 0.15f));
		FireLight->SetAttenuationRadius(900.f);
	}

	// --- Cast -----------------------------------------------------------------------------
	auto Performer = [World](const FString& Name, float X, float Y, float Yaw, const FLinearColor& Body, const FLinearColor& Head, bool bProp, bool bZombie)
	{
		const FTransform Xf(FRotator(0.f, Yaw, 0.f), FVector(X, Y, 0.f));
		ADCPerformer* P = BeginPlace<ADCPerformer>(World, Xf);
		P->DisplayName = Name;
		P->BodyColor = Body;
		P->HeadColor = Head;
		P->bHasProp = bProp;
		P->bRuinsShot = bZombie;
		return FinishPlace(P, Xf, Name, bZombie ? TEXT("DirectorsCut/Zombies") : TEXT("DirectorsCut/Cast"));
	};
	// Waypoint(Time, X, Y, Yaw, Lean, PropRaise, Tilt)
	struct FWp { float T, X, Y, Yaw, Lean = 0.f, Raise = 0.f, Tilt = 0.f; };
	auto Blocking = [World](ADCPerformer* P, std::initializer_list<FWp> Points)
	{
		const FString Folder = FString::Printf(TEXT("%s/%s_Blocking"), P->bRuinsShot ? TEXT("DirectorsCut/Zombies") : TEXT("DirectorsCut/Cast"), *P->DisplayName);
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
	ADCPerformer* Sam = Performer(TEXT("Sam"), -1350, -1000, 30, FLinearColor(0.2f, 0.3f, 0.55f), Skin, true, false);
	Blocking(Sam, {
		{ 2.f, -1350, -1000, 30 },
		{ 5.f, -900, -830, 90 },                // reaches the wrecked car
		{ 5.5f, -900, -830, 90, 20 },           // rummaging
		{ 6.5f, -900, -830, 90, 0 },
		{ 7.5f, -900, -830, 90, 20 },
		{ 8.5f, -900, -830, 90, 0, 1 },         // a can of peaches!
		{ 12.f, -900, -830, 90, 0, 1 },
		{ 13.f, -900, -830, 45, 0, 0 },
		{ 19.f, -330, -280, 0 },                // walks to the fountain
		{ 19.5f, -330, -280, 0, -12 },          // hands up - crossbow!
		{ 24.f, -330, -280, 0, -12 },
		{ 25.f, -330, -280, 0, 0 },
		{ 26.f, -150, -280, 0, 0, 1 },          // offers the peaches
		{ 31.f, -150, -280, 0, 0, 1 },
		{ 32.f, -150, -280, 0, 0, 0 },
		{ 34.f, -150, -280, 0, -8 },            // laughing
		{ 35.f, -150, -280, 0, 8 },
		{ 36.f, -150, -280, 0, -8 },
		{ 37.f, -150, -280, 0, 8 },
		{ 38.f, -150, -280, 0, 0 },
		{ 40.f, -50, -280, 0 },
		{ 42.f, -50, -280, 0, 25 },             // the kiss
		{ 46.f, -50, -280, 0, 25 },
		{ 47.f, -50, -280, 0, 0 },
		{ 49.f, -50, -280, 0 },
		{ 58.f, 1450, -30, 0 },                 // into the sunset
	});

	ADCPerformer* Ellie = Performer(TEXT("Ellie"), 760, -650, 180, FLinearColor(0.6f, 0.12f, 0.12f), Skin, true, false);
	Blocking(Ellie, {
		{ 8.f, 760, -650, 180, 15 },            // peeks over the barricade
		{ 15.f, 760, -650, 180, 15 },
		{ 16.f, 760, -650, 120, 0 },
		{ 17.f, 700, -380, 160 },
		{ 19.f, 150, -280, 180 },
		{ 19.5f, 150, -280, 180, 0, 1 },        // crossbow up
		{ 24.f, 150, -280, 180, 0, 1 },
		{ 25.f, 150, -280, 180, 0, 0 },
		{ 26.f, 60, -280, 180 },
		{ 34.f, 60, -280, 180, 8 },             // laughing
		{ 35.f, 60, -280, 180, -8 },
		{ 36.f, 60, -280, 180, 8 },
		{ 37.f, 60, -280, 180, -8 },
		{ 38.f, 60, -280, 180, 0 },
		{ 40.f, 50, -280, 180 },
		{ 42.f, 50, -280, 180, 25 },            // the kiss
		{ 46.f, 50, -280, 180, 25 },
		{ 47.f, 50, -280, 180, 0 },
		{ 49.f, 50, -280, 0 },
		{ 58.f, 1450, 30, 0 },                  // into the sunset
	});

	const FLinearColor Rot(0.25f, 0.32f, 0.2f);
	const FLinearColor Pallor(0.55f, 0.65f, 0.45f);
	constexpr float Hunch = -12.f;

	ADCPerformer* CarZombie = Performer(TEXT("Zombie_Car"), -300, -1100, 180, Rot, Pallor, false, true);
	Blocking(CarZombie, {
		{ 0.5f, -300, -1100, 180, 0, 0, Hunch },
		{ 10.f, -1350, -1050, 180, 0, 0, Hunch },  // shambles past behind Sam's back
		{ 13.f, -1350, -650, 0, 0, 0, Hunch },     // then lingers in the corner
	});

	ADCPerformer* AlleyZombie = Performer(TEXT("Zombie_Alley"), 1400, -1100, 160, Rot, Pallor, false, true);
	Blocking(AlleyZombie, {
		{ 4.f, 1400, -1100, 160, 0, 0, Hunch },
		{ 16.f, 1150, -980, 160, 0, 0, Hunch },    // shuffles in behind the barricade
		{ 30.f, 1150, -980, 90, 0, 0, Hunch },
	});

	ADCPerformer* Shambler = Performer(TEXT("Zombie_Shambler"), -1400, 250, 0, Rot, Pallor, false, true);
	Blocking(Shambler, {
		{ 0.5f, -1400, 250, 0, 0, 0, Hunch },
		{ 18.f, -1400, 250, 0, 0, 0, Hunch },
		{ 40.f, 900, 320, 0, 0, 0, Hunch },        // crosses north of the fountain
		{ 50.f, 1300, 600, 45, 0, 0, Hunch },
	});

	ADCPerformer* BusZombie = Performer(TEXT("Zombie_Bus"), -700, 950, 0, Rot, Pallor, false, true);
	Blocking(BusZombie, {
		{ 0.5f, -700, 950, 0, 0, 0, Hunch },
		{ 36.f, -700, 950, 0, 0, 0, Hunch },       // hiding behind the bus
		{ 40.f, -50, 900, -45, 0, 0, Hunch },
		{ 46.f, 0, 500, -60, 0, 0, Hunch },        // steps out behind the lovers
		{ 52.f, 350, 250, -45, 0, 0, Hunch },
		{ 60.f, 500, -100, -45, 0, 0, Hunch },
	});

	ADCPerformer* KioskZombie = Performer(TEXT("Zombie_Kiosk"), 1250, 1050, -90, Rot, Pallor, false, true);
	Blocking(KioskZombie, {
		{ 0.5f, 1250, 1050, -90, 0, 0, Hunch },
		{ 44.f, 1250, 1050, -90, 0, 0, Hunch },
		{ 54.f, 1250, 250, -90, 0, 0, Hunch },     // lurches toward the exit
		{ 60.f, 1250, 100, -90, 0, 0, Hunch },
	});

	// --- Storyboard beats -------------------------------------------------------------------
	auto Anchor = [](ADCPerformer* P, EDCAnchorPart Part, bool bFace = false)
	{
		FDCAnchorRef Ref;
		Ref.Performer = P;
		Ref.Part = Part;
		Ref.bMustSeeFace = bFace;
		return Ref;
	};
	auto Beat = [World](const TCHAR* Id, const TCHAR* Story, const TCHAR* Caption, bool bMandatory, float Start, float End, float Hold,
		int32 Points, float LabelX, float LabelY, TArray<FDCAnchorRef> Anchors)
	{
		const FTransform Xf(FVector(LabelX, LabelY, 260.f));
		ADCBeat* B = BeginPlace<ADCBeat>(World, Xf);
		B->BeatId = Id;
		B->DisplayName = Story;
		B->Caption = Caption;
		B->bMandatory = bMandatory;
		B->WindowStart = Start;
		B->WindowEnd = End;
		B->RequiredHold = Hold;
		B->Points = Points;
		B->MaxDistance = 2000.f;
		B->Anchors = MoveTemp(Anchors);
		FinishPlace(B, Xf, FString::Printf(TEXT("Beat_%s"), Id), TEXT("DirectorsCut/Storyboard"));
	};
	using EP = EDCAnchorPart;

	Beat(TEXT("S1"), TEXT("Sam scavenges a can of peaches from a wrecked car."), TEXT("A man, alone, and very hungry."),
		false, 7.f, 13.f, 2.f, 200, -900, -830, { Anchor(Sam, EP::Prop) });
	Beat(TEXT("S2"), TEXT("Ellie watches him from behind a barricade."), TEXT("She's been watching him for days."),
		false, 9.f, 15.f, 2.f, 200, 760, -650, { Anchor(Ellie, EP::Head, true) });
	Beat(TEXT("S3"), TEXT("They meet at the fountain - she aims her crossbow, he raises his hands."), TEXT("Love at first standoff."),
		false, 19.f, 25.f, 2.f, 250, -100, -280, { Anchor(Ellie, EP::Prop), Anchor(Sam, EP::Head) });
	Beat(TEXT("S4"), TEXT("He offers her the peaches."), TEXT("The most romantic gift in the apocalypse."),
		false, 26.f, 32.f, 2.f, 250, -50, -280, { Anchor(Sam, EP::Prop), Anchor(Ellie, EP::Head, true) });
	Beat(TEXT("S5"), TEXT("They share a laugh by the fountain."), TEXT("For a moment, the world isn't ending."),
		false, 33.f, 39.f, 2.f, 250, 0, -280, { Anchor(Sam, EP::Head), Anchor(Ellie, EP::Head) });
	Beat(TEXT("S6"), TEXT("The kiss."), TEXT("The studio's essential shot."),
		true, 41.f, 47.f, 2.5f, 1000, 0, -300, { Anchor(Sam, EP::Head), Anchor(Ellie, EP::Head) });
	Beat(TEXT("S7"), TEXT("They walk off together toward the sunset."), TEXT("Roll credits."),
		false, 50.f, 58.f, 2.f, 300, 700, 0, { Anchor(Sam, EP::Body), Anchor(Ellie, EP::Body) });

	// --- Player marks ---------------------------------------------------------------------
	Place<ADCStartMark>(World, FTransform(FRotator::ZeroRotator, FVector(-1300, -150, 0)), TEXT("StartMark"), TEXT("DirectorsCut/Marks"));
	Place<ADCClapperboard>(World, FTransform(FVector(-1350, -350, 0)), TEXT("Clapperboard"), TEXT("DirectorsCut/Marks"));

	SetFolderPath(TEXT("DirectorsCut"));
	World->MarkPackageDirty();
	UE_LOG(LogTemp, Log, TEXT("Directors Cut: zombie town placed. Save the level to keep it."));
}

#endif // WITH_EDITOR
