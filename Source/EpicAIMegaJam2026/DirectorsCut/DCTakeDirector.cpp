// Director's Cut prototype - take director.

#include "DCTakeDirector.h"
#include "DCCharacter.h"
#include "DCPerformer.h"
#include "DCRail.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FLinearColor MandatoryColor(1.f, 0.65f, 0.1f);
	const FLinearColor BonusColor(0.1f, 0.75f, 1.f);
	const FLinearColor CapturedColor(0.2f, 1.f, 0.3f);
	const FLinearColor MissedColor(0.25f, 0.25f, 0.25f);

	constexpr float SafeFrameFraction = 0.70f;
	constexpr float VerticalTolerance = 200.f;
	constexpr float PlaceRange = 800.f;
	constexpr float ClapperRange = 300.f;
	constexpr float MountRange = 250.f;
	constexpr int32 AllBonusReward = 250;
}

ADCTakeDirector::ADCTakeDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	// Evaluate after the player has moved and aimed this frame.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = Cube.Object;
	CylinderMesh = Cylinder.Object;
	BaseMaterial = Material.Object;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	StartTransform = FTransform(FRotator(0.f, 0.f, 0.f), FVector(-1000.f, 0.f, 100.f));
	ClapperLocation = FVector(-1050.f, -200.f, 0.f);
}

void ADCTakeDirector::BeginPlay()
{
	Super::BeginPlay();

	BuildSet();
	if (bSpawnLighting)
	{
		BuildLighting();
	}
	SpawnCast();
	DefineShots();

	FActorSpawnParameters Params;
	Params.Owner = this;
	Rail = GetWorld()->SpawnActor<ADCRail>(ADCRail::StaticClass(), FTransform::Identity, Params);
	Rail->IgnoredActors.Add(Dracula.Get());
	Rail->IgnoredActors.Add(Victim.Get());

	ResetScene();
	Flash(TEXT("Lay a camera rail, then hit the clapperboard (E) or press Enter to call Action!"), 6.f);
}

// ---------------------------------------------------------------------------------------------
// Set construction (placeholder blockout of "Dracula: The Midnight Visit")

UMaterialInstanceDynamic* ADCTakeDirector::MakeMaterial(const FLinearColor& Color)
{
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	MID->SetVectorParameterValue(TEXT("Color"), Color);
	return MID;
}

UStaticMeshComponent* ADCTakeDirector::AddBlock(const FVector& Center, const FVector& Size, const FLinearColor& Color, bool bCollide)
{
	UStaticMeshComponent* Block = NewObject<UStaticMeshComponent>(this);
	Block->SetStaticMesh(CubeMesh);
	Block->SetupAttachment(Root);
	Block->SetCollisionProfileName(bCollide ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
	Block->RegisterComponent();
	Block->SetWorldLocation(Center);
	Block->SetWorldScale3D(Size / 100.f);
	Block->SetMaterial(0, MakeMaterial(Color));
	return Block;
}

UStaticMeshComponent* ADCTakeDirector::AddDisc(const FVector& Center, float Radius, const FLinearColor& Color)
{
	UStaticMeshComponent* Disc = NewObject<UStaticMeshComponent>(this);
	Disc->SetStaticMesh(CylinderMesh);
	Disc->SetupAttachment(Root);
	Disc->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Disc->SetCastShadow(false);
	Disc->RegisterComponent();
	Disc->SetWorldLocation(Center + FVector(0.f, 0.f, 1.f));
	Disc->SetWorldScale3D(FVector(Radius / 50.f, Radius / 50.f, 0.02f));
	UMaterialInstanceDynamic* MID = MakeMaterial(Color);
	Disc->SetMaterial(0, MID);
	MarkerMaterials.Add(MID);
	return Disc;
}

void ADCTakeDirector::BuildSet()
{
	const FLinearColor Wood(0.16f, 0.09f, 0.05f);
	const FLinearColor Stone(0.32f, 0.3f, 0.36f);
	const FLinearColor DarkStone(0.15f, 0.14f, 0.17f);
	const FLinearColor Velvet(0.4f, 0.03f, 0.06f);
	const FLinearColor Concrete(0.12f, 0.12f, 0.13f);
	constexpr float WallH = 450.f;

	// Floors: castle bedroom plus the backstage area behind the entry arch.
	AddBlock(FVector(0, 0, -10), FVector(2400, 1800, 20), Wood);
	AddBlock(FVector(-600, -1110, -10), FVector(820, 420, 20), Concrete);

	// Perimeter walls with the entry arch gap in the south wall (x -700..-500).
	AddBlock(FVector(0, 910, WallH / 2), FVector(2440, 20, WallH), Stone);
	AddBlock(FVector(1210, 0, WallH / 2), FVector(20, 1840, WallH), Stone);
	AddBlock(FVector(-1210, 0, WallH / 2), FVector(20, 1840, WallH), Stone);
	AddBlock(FVector(-950, -910, WallH / 2), FVector(500, 20, WallH), Stone);
	AddBlock(FVector(350, -910, WallH / 2), FVector(1700, 20, WallH), Stone);
	AddBlock(FVector(-600, -910, 375), FVector(200, 20, 150), DarkStone);
	AddBlock(FVector(-720, -910, 150), FVector(40, 60, 300), DarkStone);
	AddBlock(FVector(-480, -910, 150), FVector(40, 60, 300), DarkStone);

	// Backstage walls.
	AddBlock(FVector(-1020, -1110, WallH / 2), FVector(20, 420, WallH), Concrete);
	AddBlock(FVector(-180, -1110, WallH / 2), FVector(20, 420, WallH), Concrete);
	AddBlock(FVector(-600, -1330, WallH / 2), FVector(860, 20, WallH), Concrete);

	// Set dressing.
	AddBlock(FVector(-300, 800, 37.5f), FVector(180, 90, 75), Wood);                       // writing desk
	AddBlock(FVector(800, 650, 27.5f), FVector(260, 260, 55), Velvet);                     // bed
	AddBlock(FVector(800, 790, 90), FVector(280, 20, 180), Wood);                          // headboard
	AddBlock(FVector(150, -775, 200), FVector(20, 250, 400), Velvet);                      // curtain
	AddBlock(FVector(-750, 350, 60), FVector(150, 100, 120), Wood);                        // trunk
	AddBlock(FVector(950, -300, 110), FVector(120, 200, 220), Wood);                       // wardrobe
	AddBlock(FVector(1190, 150, 150), FVector(10, 120, 200), FLinearColor(0.7f, 0.75f, 0.8f)); // mirror

	// Clapperboard on a stand next to the start mark.
	AddBlock(ClapperLocation + FVector(0, 0, 50), FVector(30, 30, 100), DarkStone);
	AddBlock(ClapperLocation + FVector(0, 0, 118), FVector(8, 60, 36), FLinearColor(0.02f, 0.02f, 0.02f));
	AddBlock(ClapperLocation + FVector(0, 0, 140), FVector(8, 60, 8), FLinearColor(0.9f, 0.9f, 0.9f));

	// Start mark (the player always begins the take here).
	AddDisc(FVector(StartTransform.GetLocation().X, StartTransform.GetLocation().Y, 0.f), 60.f, FLinearColor(0.9f, 0.9f, 0.9f));
	MarkerMaterials.Reset(); // the start disc is not a shot marker
}

void ADCTakeDirector::BuildLighting()
{
	UDirectionalLightComponent* Moon = NewObject<UDirectionalLightComponent>(this);
	Moon->SetupAttachment(Root);
	Moon->SetMobility(EComponentMobility::Movable);
	Moon->RegisterComponent();
	Moon->SetWorldRotation(FRotator(-35.f, 40.f, 0.f));
	Moon->SetIntensity(2.5f);
	Moon->SetLightColor(FLinearColor(0.6f, 0.7f, 1.f));
	Moon->SetAtmosphereSunLight(true);

	USkyAtmosphereComponent* Sky = NewObject<USkyAtmosphereComponent>(this);
	Sky->SetupAttachment(Root);
	Sky->RegisterComponent();

	USkyLightComponent* SkyLight = NewObject<USkyLightComponent>(this);
	SkyLight->SetupAttachment(Root);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->bRealTimeCapture = true;
	SkyLight->RegisterComponent();
	SkyLight->SetIntensity(0.6f);

	UExponentialHeightFogComponent* Fog = NewObject<UExponentialHeightFogComponent>(this);
	Fog->SetupAttachment(Root);
	Fog->RegisterComponent();
	Fog->SetFogDensity(0.01f);

	auto AddLamp = [this](const FVector& Location, const FLinearColor& Color, float Intensity)
	{
		UPointLightComponent* Lamp = NewObject<UPointLightComponent>(this);
		Lamp->SetupAttachment(Root);
		Lamp->SetMobility(EComponentMobility::Movable);
		Lamp->RegisterComponent();
		Lamp->SetWorldLocation(Location);
		Lamp->SetIntensity(Intensity);
		Lamp->SetLightColor(Color);
		Lamp->SetAttenuationRadius(1400.f);
	};
	const FLinearColor Warm(1.f, 0.65f, 0.35f);
	AddLamp(FVector(-300, 700, 250), Warm, 6000.f);
	AddLamp(FVector(800, 500, 250), Warm, 6000.f);
	AddLamp(FVector(0, -300, 380), FLinearColor(0.5f, 0.6f, 1.f), 4000.f);
	AddLamp(FVector(-600, -1100, 300), FLinearColor(0.9f, 0.9f, 1.f), 2500.f);
	AddLamp(FVector(-1000, 0, 350), FLinearColor::White, 3000.f);
}

void ADCTakeDirector::SpawnCast()
{
	FActorSpawnParameters Params;
	Params.Owner = this;

	auto Key = [](float T, float X, float Y, float Yaw, float Lean = 0.f, float Raise = 0.f)
	{
		FDCKey K;
		K.Time = T;
		K.Location = FVector2D(X, Y);
		K.Yaw = Yaw;
		K.Lean = Lean;
		K.PropRaise = Raise;
		return K;
	};

	// Authored performance, one shared clock (see GDD section 05).
	Victim = GetWorld()->SpawnActor<ADCPerformer>(ADCPerformer::StaticClass(), FTransform::Identity, Params);
	Victim->Setup(FLinearColor(0.85f, 0.85f, 0.9f), FLinearColor(0.9f, 0.72f, 0.6f), false, true, {
		Key(0.f, -300, 620, -90),
		Key(5.5f, -300, 620, -90, 0, 0),
		Key(6.5f, -300, 620, -90, 0, 1),   // letter reveal
		Key(12.f, -300, 620, -90, 0, 1),
		Key(13.f, -300, 620, -90, 0, 0),
		Key(14.f, -300, 620, -90),
		Key(21.f, 560, 420, -135),         // waits by the bed
		Key(45.f, 560, 420, -135),
	});

	Dracula = GetWorld()->SpawnActor<ADCPerformer>(ADCPerformer::StaticClass(), FTransform::Identity, Params);
	Dracula->Setup(FLinearColor(0.04f, 0.03f, 0.04f), FLinearColor(0.85f, 0.88f, 0.85f), true, false, {
		Key(0.f, -600, -1150, 90),
		Key(2.f, -600, -1150, 90),
		Key(6.f, -600, -700, 77),          // entrance through the arch
		Key(12.f, -600, -700, 77),
		Key(16.f, -150, -250, 43),         // approach
		Key(17.f, -150, -250, 43, -14),    // hesitation recoil
		Key(21.f, -150, -250, 43, -14),
		Key(22.f, -150, -250, 43, 0),
		Key(26.f, 480, 340, 45),           // reaches the victim
		Key(27.f, 480, 340, 45, 32),       // bite
		Key(32.f, 480, 340, 45, 32),
		Key(33.f, 480, 340, 45, 0),
		Key(37.f, 350, -600, -98),         // retreat to the curtain
		Key(38.5f, 350, -600, 78),         // looks back
		Key(45.f, 350, -600, 78),
	});
}

void ADCTakeDirector::DefineShots()
{
	auto Anchor = [](USceneComponent* Component, AActor* AnchorOwner)
	{
		FDCAnchor A;
		A.Component = Component;
		A.Owner = AnchorOwner;
		return A;
	};

	auto MakeShot = [](const TCHAR* Id, const TCHAR* Name, const TCHAR* Caption, bool bMandatory, float Start, float End, float Hold,
		FVector2D Mark, float MinD, float MaxD, int32 Points)
	{
		FDCShot S;
		S.Id = Id;
		S.Name = Name;
		S.Caption = Caption;
		S.bMandatory = bMandatory;
		S.WindowStart = Start;
		S.WindowEnd = End;
		S.RequiredHold = Hold;
		S.MarkLocation = FVector(Mark, 0.f);
		S.MinDistance = MinD;
		S.MaxDistance = MaxD;
		S.Points = Points;
		return S;
	};

	Shots.Reset();

	FDCShot B01 = MakeShot(TEXT("B01"), TEXT("The invitation"), TEXT("She was expecting a visitor."), false, 6.f, 12.f, 1.5f, FVector2D(-300, 200), 150.f, 500.f, 250);
	B01.Anchors.Add(Anchor(Victim->GetProp(), Victim));
	Shots.Add(B01);

	FDCShot B02 = MakeShot(TEXT("B02"), TEXT("A moment of doubt"), TEXT("The monster hesitates before the attack."), false, 16.f, 22.f, 1.5f, FVector2D(-550, -350), 200.f, 650.f, 250);
	B02.Anchors.Add(Anchor(Dracula->GetHead(), Dracula));
	Shots.Add(B02);

	FDCShot M01 = MakeShot(TEXT("M01"), TEXT("The bite"), TEXT("The studio's essential story beat."), true, 26.f, 33.f, 2.f, FVector2D(650, 50), 200.f, 600.f, 1000);
	M01.Anchors.Add(Anchor(Dracula->GetHead(), Dracula));
	M01.Anchors.Add(Anchor(Victim->GetHead(), Victim));
	Shots.Add(M01);

	FDCShot B03 = MakeShot(TEXT("B03"), TEXT("The aftermath"), TEXT("He looks back - a hint of remorse."), false, 37.f, 43.f, 1.5f, FVector2D(-50, -450), 200.f, 650.f, 250);
	B03.Anchors.Add(Anchor(Dracula->GetHead(), Dracula));
	Shots.Add(B03);

	for (const FDCShot& Shot : Shots)
	{
		check(Shot.WindowStart < Shot.WindowEnd && Shot.WindowEnd <= TakeDuration && Shot.RequiredHold <= Shot.WindowEnd - Shot.WindowStart);
		AddDisc(Shot.MarkLocation, Shot.Radius, Shot.bMandatory ? MandatoryColor : BonusColor);
	}
}

// ---------------------------------------------------------------------------------------------
// Phases

void ADCTakeDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	switch (Phase)
	{
	case EDCPhase::Preparation:
		UpdatePrepPreview();
		break;

	case EDCPhase::Countdown:
		CountdownRemaining -= DeltaSeconds;
		if (CountdownRemaining <= 0.f)
		{
			BeginFilming();
		}
		break;

	case EDCPhase::Filming:
	{
		const float Prev = SceneTime;
		SceneTime = FMath::Min(SceneTime + DeltaSeconds, TakeDuration);
		Dracula->Evaluate(SceneTime);
		Victim->Evaluate(SceneTime);
		EvaluateShots(Prev, SceneTime);
		break;
	}

	case EDCPhase::Results:
		break;
	}
}

void ADCTakeDirector::HandleStartTake()
{
	if (Phase != EDCPhase::Preparation)
	{
		return;
	}
	ResetScene();
	Rail->SetPreview(false, FVector::ZeroVector, false);
	Phase = EDCPhase::Countdown;
	CountdownRemaining = CountdownDuration;
}

void ADCTakeDirector::BeginFilming()
{
	Phase = EDCPhase::Filming;
	SceneTime = 0.f;
	Flash(TEXT("ACTION!"), 1.5f);
}

void ADCTakeDirector::HandleRestart()
{
	ResetScene();
	Flash(TEXT("Back to preparation. Your rail is preserved."));
}

void ADCTakeDirector::ResetScene()
{
	Phase = EDCPhase::Preparation;
	SceneTime = 0.f;
	CountdownRemaining = 0.f;
	bApproved = false;
	Score = 0;

	for (FDCShot& Shot : Shots)
	{
		Shot.ResetRuntime();
	}
	Dracula->Evaluate(0.f);
	Victim->Evaluate(0.f);
	RefreshMarkerColors();

	if (ADCCharacter* Player = GetPlayer())
	{
		Player->ResetToStart(StartTransform);
	}
}

void ADCTakeDirector::EndTake()
{
	if (Phase != EDCPhase::Filming)
	{
		return; // end-take fires once
	}

	Phase = EDCPhase::Results;
	bApproved = true;
	int32 Bonus = 0;
	int32 BonusTotal = 0;
	Score = 0;
	for (FDCShot& Shot : Shots)
	{
		if (Shot.Status == EDCShotStatus::Pending)
		{
			Shot.Status = EDCShotStatus::Missed;
		}
		if (Shot.bMandatory)
		{
			bApproved &= Shot.Status == EDCShotStatus::Captured;
		}
		else
		{
			++BonusTotal;
		}
		if (Shot.Status == EDCShotStatus::Captured)
		{
			Score += Shot.Points;
			Bonus += Shot.bMandatory ? 0 : 1;
		}
	}
	if (Bonus == BonusTotal && BonusTotal > 0)
	{
		Score += AllBonusReward;
	}
	if (!bApproved)
	{
		Score = 0;
	}
	else
	{
		BestScore = FMath::Max(BestScore, Score);
	}
	RefreshMarkerColors();

	if (ADCCharacter* Player = GetPlayer())
	{
		Player->ReleaseInputs();
	}
	Flash(TEXT("CUT!"), 2.f);
}

// ---------------------------------------------------------------------------------------------
// Capture contract (GDD section 04)

void ADCTakeDirector::EvaluateShots(float PrevTime, float NowTime)
{
	for (FDCShot& Shot : Shots)
	{
		if (Shot.Status != EDCShotStatus::Pending)
		{
			continue;
		}

		const float Eligible = FMath::Max(0.f, FMath::Min(NowTime, Shot.WindowEnd) - FMath::Max(PrevTime, Shot.WindowStart));
		const bool bInWindow = NowTime >= Shot.WindowStart && PrevTime <= Shot.WindowEnd;
		Shot.LastFail = CheckShot(Shot, bInWindow);

		if (Shot.LastFail == EDCFail::None && Eligible > 0.f)
		{
			Shot.Hold += Eligible;
		}
		else
		{
			Shot.Hold = 0.f;
		}

		if (Shot.Hold >= Shot.RequiredHold)
		{
			Shot.Status = EDCShotStatus::Captured;
			Shot.CapturedAt = NowTime;
			Flash(FString::Printf(TEXT("Captured %s: %s"), *Shot.Id, *Shot.Name));
			RefreshMarkerColors();
		}
	}

	for (FDCShot& Shot : Shots)
	{
		if (Shot.Status == EDCShotStatus::Pending && NowTime > Shot.WindowEnd)
		{
			Shot.Status = EDCShotStatus::Missed;
			RefreshMarkerColors();
			if (Shot.bMandatory)
			{
				EndTake();
				return;
			}
		}
	}

	if (NowTime >= TakeDuration)
	{
		EndTake();
	}
}

EDCFail ADCTakeDirector::CheckShot(const FDCShot& Shot, bool bInWindow) const
{
	if (!bInWindow)
	{
		return EDCFail::WrongTime;
	}

	ADCCharacter* Player = GetPlayer();
	APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
	if (!PC)
	{
		return EDCFail::WrongPosition;
	}

	const FVector Feet = Player->GetActorLocation() - FVector(0.f, 0.f, Player->GetSimpleCollisionHalfHeight());
	if (FVector::Dist2D(Feet, Shot.MarkLocation) > Shot.Radius || FMath::Abs(Feet.Z - Shot.MarkLocation.Z) > VerticalTolerance)
	{
		return EDCFail::WrongPosition;
	}
	if (!Player->IsRecording())
	{
		return EDCFail::NotRecording;
	}
	if (Player->IsSprinting())
	{
		return EDCFail::Sprinting;
	}

	const FVector CamLoc = Player->GetCamera()->GetComponentLocation();
	const FVector CamFwd = Player->GetCamera()->GetForwardVector();
	int32 ViewW = 0, ViewH = 0;
	PC->GetViewportSize(ViewW, ViewH);

	for (const FDCAnchor& A : Shot.Anchors)
	{
		const FVector P = A.Component->GetComponentLocation();
		const FVector ToP = P - CamLoc;
		FVector2D Screen;
		if (FVector::DotProduct(ToP, CamFwd) <= 0.f || !PC->ProjectWorldLocationToScreen(P, Screen, true))
		{
			return EDCFail::OutOfFrame;
		}
		const float HalfSafeW = ViewW * SafeFrameFraction * 0.5f;
		const float HalfSafeH = ViewH * SafeFrameFraction * 0.5f;
		if (FMath::Abs(Screen.X - ViewW * 0.5f) > HalfSafeW || FMath::Abs(Screen.Y - ViewH * 0.5f) > HalfSafeH)
		{
			return EDCFail::OutOfFrame;
		}
		const float Dist = ToP.Size();
		if (Dist < Shot.MinDistance)
		{
			return EDCFail::TooClose;
		}
		if (Dist > Shot.MaxDistance)
		{
			return EDCFail::TooFar;
		}
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(DCShotVisibility), false, Player);
	Params.AddIgnoredActor(Rail);
	for (const FDCAnchor& A : Shot.Anchors)
	{
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, A.Component->GetComponentLocation(), ECC_Visibility, Params) && Hit.GetActor() != A.Owner)
		{
			return EDCFail::Blocked;
		}
	}

	return EDCFail::None;
}

// ---------------------------------------------------------------------------------------------
// Preparation: rail editing

void ADCTakeDirector::UpdatePrepPreview()
{
	ADCCharacter* Player = GetPlayer();
	bPreviewHit = false;
	bPreviewValid = false;
	if (!Player || Player->IsOnRail())
	{
		Rail->SetPreview(false, FVector::ZeroVector, false);
		return;
	}

	const FVector CamLoc = Player->GetCamera()->GetComponentLocation();
	const FVector End = CamLoc + Player->GetCamera()->GetForwardVector() * PlaceRange;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DCRailPreview), false, Player);
	Params.AddIgnoredActor(Rail);
	Params.AddIgnoredActor(Dracula);
	Params.AddIgnoredActor(Victim);

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, End, ECC_Visibility, Params))
	{
		bPreviewHit = true;
		PreviewPoint = Hit.ImpactPoint;
		bPreviewValid = Hit.ImpactNormal.Z > 0.7f && Rail->NumPoints() < ADCRail::MaxPoints;
	}
	Rail->SetPreview(bPreviewHit, PreviewPoint, bPreviewValid);
}

void ADCTakeDirector::HandlePrimaryPressed()
{
	if (Phase != EDCPhase::Preparation)
	{
		return;
	}
	if (!bPreviewHit)
	{
		Flash(TEXT("Aim at the floor within 8 m to place a rail point"));
		return;
	}
	if (!bPreviewValid)
	{
		Flash(Rail->NumPoints() >= ADCRail::MaxPoints ? TEXT("Rail point limit reached") : TEXT("Rail points go on the floor"));
		return;
	}
	FString Reason;
	if (!Rail->TryAddPoint(PreviewPoint, Reason))
	{
		Flash(Reason);
	}
}

void ADCTakeDirector::HandleSecondaryPressed()
{
	if (Phase != EDCPhase::Preparation)
	{
		return;
	}
	if (!Rail->RemovePointNear(bPreviewHit ? PreviewPoint : FVector(TNumericLimits<float>::Max()), 150.f))
	{
		Flash(Rail->NumPoints() == 0 ? TEXT("No rail points to remove") : TEXT("Removing that point would route the rail through scenery"));
	}
}

void ADCTakeDirector::HandleClearRail()
{
	if (Phase == EDCPhase::Preparation)
	{
		Rail->Clear();
		Flash(TEXT("Rail cleared"));
	}
}

bool ADCTakeDirector::IsNearClapper() const
{
	ADCCharacter* Player = GetPlayer();
	return Player && FVector::Dist2D(Player->GetActorLocation(), ClapperLocation) < ClapperRange;
}

bool ADCTakeDirector::CanMountRail() const
{
	ADCCharacter* Player = GetPlayer();
	if (!Player || Player->IsOnRail() || !Rail->IsUsable() || (Phase != EDCPhase::Preparation && Phase != EDCPhase::Filming))
	{
		return false;
	}
	float Gap = 0.f;
	Rail->FindClosestDistance(Player->GetActorLocation(), Gap);
	return Gap < MountRange;
}

void ADCTakeDirector::HandleInteract()
{
	ADCCharacter* Player = GetPlayer();
	if (!Player)
	{
		return;
	}

	if (Phase == EDCPhase::Results)
	{
		HandleRestart();
	}
	else if (Player->IsOnRail())
	{
		Player->DismountRail();
	}
	else if (Phase == EDCPhase::Preparation && IsNearClapper())
	{
		HandleStartTake();
	}
	else if (CanMountRail())
	{
		float Gap = 0.f;
		Player->MountRail(Rail, Rail->FindClosestDistance(Player->GetActorLocation(), Gap));
	}
	else
	{
		Flash(TEXT("Nothing to interact with here"), 1.5f);
	}
}

// ---------------------------------------------------------------------------------------------
// Helpers

int32 ADCTakeDirector::GetFocusShotIndex() const
{
	// The pending shot whose window is open, otherwise the next one to open.
	int32 Next = INDEX_NONE;
	for (int32 i = 0; i < Shots.Num(); ++i)
	{
		const FDCShot& Shot = Shots[i];
		if (Shot.Status != EDCShotStatus::Pending)
		{
			continue;
		}
		if (SceneTime >= Shot.WindowStart && SceneTime <= Shot.WindowEnd)
		{
			return i;
		}
		if (Shot.WindowStart > SceneTime && (Next == INDEX_NONE || Shot.WindowStart < Shots[Next].WindowStart))
		{
			Next = i;
		}
	}
	return Next;
}

int32 ADCTakeDirector::GetCapturedBonusCount() const
{
	int32 Count = 0;
	for (const FDCShot& Shot : Shots)
	{
		Count += (!Shot.bMandatory && Shot.Status == EDCShotStatus::Captured) ? 1 : 0;
	}
	return Count;
}

FString ADCTakeDirector::GetCutName() const
{
	if (!bApproved)
	{
		return TEXT("Retake required");
	}
	const int32 Bonus = GetCapturedBonusCount();
	int32 BonusTotal = 0;
	for (const FDCShot& Shot : Shots)
	{
		BonusTotal += Shot.bMandatory ? 0 : 1;
	}
	if (Bonus == 0)
	{
		return TEXT("Studio Cut");
	}
	return Bonus == BonusTotal ? TEXT("Director's Cut") : TEXT("Extended Cut");
}

FString ADCTakeDirector::GetMessage() const
{
	return GetWorld()->GetTimeSeconds() < MessageExpireTime ? Message : FString();
}

void ADCTakeDirector::Flash(const FString& Text, float Seconds)
{
	Message = Text;
	MessageExpireTime = GetWorld()->GetTimeSeconds() + Seconds;
}

void ADCTakeDirector::RefreshMarkerColors()
{
	for (int32 i = 0; i < Shots.Num() && i < MarkerMaterials.Num(); ++i)
	{
		const FDCShot& Shot = Shots[i];
		FLinearColor Color = Shot.bMandatory ? MandatoryColor : BonusColor;
		if (Shot.Status == EDCShotStatus::Captured)
		{
			Color = CapturedColor;
		}
		else if (Shot.Status == EDCShotStatus::Missed)
		{
			Color = MissedColor;
		}
		MarkerMaterials[i]->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

ADCCharacter* ADCTakeDirector::GetPlayer() const
{
	return Cast<ADCCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
}

FString ADCTakeDirector::DescribeFail(EDCFail Fail)
{
	switch (Fail)
	{
	case EDCFail::None:          return TEXT("Rolling - hold it!");
	case EDCFail::WrongTime:     return TEXT("Not yet - wait for the window");
	case EDCFail::WrongPosition: return TEXT("Get to the shot mark");
	case EDCFail::NotRecording:  return TEXT("Hold Left Mouse to record");
	case EDCFail::Sprinting:     return TEXT("Can't record while sprinting");
	case EDCFail::OutOfFrame:    return TEXT("Frame the subject");
	case EDCFail::TooClose:      return TEXT("Too close - back up");
	case EDCFail::TooFar:        return TEXT("Too far - move closer");
	case EDCFail::Blocked:       return TEXT("View is blocked");
	}
	return FString();
}
