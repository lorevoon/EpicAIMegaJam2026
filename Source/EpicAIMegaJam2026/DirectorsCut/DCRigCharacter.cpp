// Director's Cut prototype 5 - "Rigs & Lenses" camera operator.

#include "DCRigCharacter.h"
#include "DCFreeDirector.h"
#include "DCJib.h"
#include "DCReplicaDirector.h"
#include "DCRail.h"
#include "DCTripod.h"
#include "CineCameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"

namespace
{
	// Full-frame 16:9 sensor: a 24 mm lens gives ~74 degrees, matching the reference film's camera.
	constexpr float SensorWidth = 36.f;
	constexpr float SensorHeight = 20.25f;

	/** Frame-rate independent smoothing factor for a spring of the given stiffness. */
	float SmoothAlpha(float Stiffness, float DeltaSeconds)
	{
		return 1.f - FMath::Exp(-Stiffness * DeltaSeconds);
	}

	float Noise(float T, float Seed)
	{
		return FMath::PerlinNoise1D(T + Seed);
	}
}

ADCRigCharacter::ADCRigCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCineCameraComponent>(TEXT("Camera")))
{
	Lenses = {
		{ TEXT("16mm prime"), 16.f, 16.f, 1.8f },
		{ TEXT("24mm prime"), 24.f, 24.f, 1.4f },
		{ TEXT("35mm prime"), 35.f, 35.f, 1.4f },
		{ TEXT("50mm prime"), 50.f, 50.f, 1.2f },
		{ TEXT("85mm prime"), 85.f, 85.f, 1.4f },
		{ TEXT("24-135mm zoom"), 24.f, 135.f, 4.f },
	};

	// The rig feel drives the camera's rotation itself, so it can lag, float, and shake.
	GetCamera()->bUsePawnControlRotation = false;

	if (UCineCameraComponent* Cine = Cast<UCineCameraComponent>(GetCamera()))
	{
		Cine->Filmback.SensorWidth = SensorWidth;
		Cine->Filmback.SensorHeight = SensorHeight;
		Cine->Filmback.SensorAspectRatio = SensorWidth / SensorHeight;
		Cine->LensSettings.MinFocalLength = 12.f;
		Cine->LensSettings.MaxFocalLength = 200.f;
		Cine->LensSettings.MinFStop = 1.2f;
		Cine->LensSettings.MaxFStop = 22.f;
		Cine->LensSettings.MinimumFocusDistance = 15.f;
		Cine->FocusSettings.FocusMethod = ECameraFocusMethod::Manual;
		Cine->FocusSettings.ManualFocusDistance = FocusDistance;
		Cine->FocusSettings.bSmoothFocusChanges = false;
		Cine->CurrentFocalLength = 24.f;
		Cine->CurrentAperture = 1.4f;
		// Horizontal FOV follows the lens; the viewport is not letterboxed.
		Cine->bConstrainAspectRatio = false;
	}
}

UCineCameraComponent* ADCRigCharacter::GetCine() const
{
	return Cast<UCineCameraComponent>(GetCamera());
}

float ADCRigCharacter::FocalToFov(float FocalMm)
{
	return FMath::RadiansToDegrees(2.f * FMath::Atan(SensorWidth * 0.5f / FMath::Max(FocalMm, 1.f)));
}

float ADCRigCharacter::FovToFocal(float FovDeg)
{
	return SensorWidth * 0.5f / FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(FovDeg, 1.f, 170.f) * 0.5f));
}

float ADCRigCharacter::GetCameraFOV() const
{
	return FocalToFov(FocalLength);
}

void ADCRigCharacter::BeginPlay()
{
	Super::BeginPlay();
	const FDCLens& Lens = Lenses[LensIndex];
	TargetFocal = FocalLength = Lens.MinFocal;
}

void ADCRigCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input || !Mapping)
	{
		return;
	}
	auto MapButton = [this](FKey Key)
	{
		UInputAction* Action = MakeAction(EInputActionValueType::Boolean);
		Mapping->MapKey(Action, Key);
		return Action;
	};
	Input->BindAction(MapButton(EKeys::Q), ETriggerEvent::Started, this, &ADCRigCharacter::OnNextLens);
	Input->BindAction(MapButton(EKeys::Z), ETriggerEvent::Started, this, &ADCRigCharacter::OnPrevLens);
	Input->BindAction(MapButton(EKeys::G), ETriggerEvent::Started, this, &ADCRigCharacter::OnToggleBodyRig);
	Input->BindAction(MapButton(EKeys::Four), ETriggerEvent::Started, this, &ADCRigCharacter::OnJibTool);
	UInputAction* Brake = MapButton(EKeys::SpaceBar);
	Input->BindAction(Brake, ETriggerEvent::Started, this, &ADCRigCharacter::OnBrakePressed);
	Input->BindAction(Brake, ETriggerEvent::Completed, this, &ADCRigCharacter::OnBrakeReleased);

	// The base class already registered the mapping context; re-add it so the new keys are picked up.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(Mapping, 0);
		}
	}
}

bool ADCRigCharacter::IsFilming()
{
	const ADCFreeDirector* D = GetDirector();
	return D && D->GetPhase() == EDCPhase::Filming;
}

// ---------------------------------------------------------------------------------------------
// Tick

void ADCRigCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds); // moves the body and the mounted rig (TickRig)

	if (!IsOnRig() && BodyRig == EDCBodyRig::Steadicam)
	{
		// A Steadicam vest is heavy: no running.
		GetCharacterMovement()->MaxWalkSpeed = SteadicamWalkSpeed;
	}

	TickLens(DeltaSeconds);
	TickCameraFeel(DeltaSeconds);
}

void ADCRigCharacter::TickRig(float DeltaSeconds)
{
	if (ADCJib* Jib = MountedJib.Get())
	{
		// The arm is heavy: input accelerates it, letting go lets it drift to a stop.
		constexpr float MaxYawSpeed = 45.f;   // deg/s
		constexpr float MaxPitchSpeed = 24.f; // deg/s
		constexpr float Accel = 60.f;         // deg/s^2 while pushing
		constexpr float Drag = 30.f;          // deg/s^2 while coasting
		JibYawVelocity = FMath::FInterpConstantTo(JibYawVelocity, JibInput.X * MaxYawSpeed, DeltaSeconds, JibInput.X != 0.f ? Accel : Drag);
		JibPitchVelocity = FMath::FInterpConstantTo(JibPitchVelocity, JibInput.Y * MaxPitchSpeed, DeltaSeconds, JibInput.Y != 0.f ? Accel : Drag);

		float Pitch = Jib->GetArmPitch() + JibPitchVelocity * DeltaSeconds;
		if (Pitch <= ADCJib::MinPitch || Pitch >= ADCJib::MaxPitch)
		{
			Jolt = FMath::Max(Jolt, FMath::Abs(JibPitchVelocity) / MaxPitchSpeed * 0.6f); // the arm hits its stop
			JibPitchVelocity = 0.f;
		}
		Jib->SetArm(Jib->GetArmYaw() + JibYawVelocity * DeltaSeconds, Pitch);
		PlaceCameraAt(Jib->GetCameraLocation());
		return;
	}

	if (ADCRail* Rail = MountedRail.Get())
	{
		// The dolly has momentum: W/S push it, it coasts when released, Space brakes.
		constexpr float Friction = 70.f;  // cm/s^2
		constexpr float BrakeDecel = 900.f;
		if (bBraking)
		{
			DollyVelocity = FMath::FInterpConstantTo(DollyVelocity, 0.f, DeltaSeconds, BrakeDecel);
		}
		else if (RailInput != 0.f)
		{
			DollyVelocity += RailInput * DollyPush * DeltaSeconds;
		}
		else
		{
			DollyVelocity = FMath::FInterpConstantTo(DollyVelocity, 0.f, DeltaSeconds, Friction);
		}
		DollyVelocity = FMath::Clamp(DollyVelocity, -DollyMaxSpeed, DollyMaxSpeed);

		RailDistance += DollyVelocity * DeltaSeconds;
		if (RailDistance <= 0.f || RailDistance >= Rail->GetLength())
		{
			RailDistance = FMath::Clamp(RailDistance, 0.f, Rail->GetLength());
			Jolt = FMath::Max(Jolt, FMath::Abs(DollyVelocity) / DollyMaxSpeed); // end stop
			DollyVelocity = 0.f;
		}
		PlaceCameraAt(Rail->GetFloorLocationAtDistance(RailDistance));
		return;
	}

	Super::TickRig(DeltaSeconds);
}

void ADCRigCharacter::TickLens(float DeltaSeconds)
{
	if (LensChangeRemaining > 0.f)
	{
		LensChangeRemaining -= DeltaSeconds;
		if (LensChangeRemaining <= 0.f)
		{
			LensChangeRemaining = 0.f;
			SelectLens(PendingLensIndex);
		}
	}

	const FDCLens& Lens = Lenses[LensIndex];
	// Zooming is smooth but fast enough for a crash zoom; primes are fixed.
	FocalLength = Lens.IsZoom() ? FMath::FInterpTo(FocalLength, TargetFocal, DeltaSeconds, 7.f) : Lens.MinFocal;

	// Autofocus on whatever is under the centre of the frame.
	const FVector Start = GetCamera()->GetComponentLocation();
	const FVector End = Start + GetCamera()->GetForwardVector() * 6000.f;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DCAutofocus), false, this);
	FHitResult Hit;
	const float Target = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) ? Hit.Distance : 3000.f;
	FocusDistance = FMath::FInterpTo(FocusDistance, FMath::Max(Target, 30.f), DeltaSeconds, 5.f);

	if (UCineCameraComponent* Cine = GetCine())
	{
		Cine->SetCurrentFocalLength(FocalLength);
		Cine->SetCurrentAperture(Lens.FStop);
		Cine->FocusSettings.ManualFocusDistance = FocusDistance;
	}
}

void ADCRigCharacter::TickCameraFeel(float DeltaSeconds)
{
	const float Dt = FMath::Max(DeltaSeconds, 1e-4f);
	FeelTime += Dt;

	AController* C = GetController();
	const FQuat TargetQ = (C ? C->GetControlRotation() : GetActorRotation()).Quaternion();
	const FVector RigPoint = GetActorLocation() + CameraOffset;
	if (!bFeelInitialized || FVector::Dist(SmoothLoc, RigPoint) > 300.f)
	{
		SmoothRot = TargetQ;
		SmoothLoc = RigPoint;
		bFeelInitialized = true;
	}

	// Per-rig handling.
	float Stiffness = 22.f;     // how quickly the camera follows your aim
	float MaxTurnRate = 0.f;    // deg/s, 0 = unlimited
	float MaxLead = 0.f;        // how far your aim may run ahead of the camera, 0 = unlimited
	float Shake = 0.f;          // shake amplitude in degrees
	float Bob = 0.f;            // footstep bob, 0..1+
	float BankScale = 0.f;      // roll into turns
	bool bFloat = false;        // camera position floats behind the body

	const float Speed = GetVelocity().Size2D();
	if (MountedJib.IsValid())
	{
		Stiffness = 9.f;
		MaxLead = 20.f;
		Shake = 0.03f + 0.12f * FMath::Clamp((FMath::Abs(JibYawVelocity) + FMath::Abs(JibPitchVelocity)) / 45.f, 0.f, 1.f);
	}
	else if (MountedTripod.IsValid())
	{
		// Fluid head: heavy damping and a speed limit make every pan smooth.
		Stiffness = 5.f;
		MaxTurnRate = 70.f;
		MaxLead = 15.f;
	}
	else if (MountedRail.IsValid())
	{
		Stiffness = 12.f;
		MaxLead = 30.f;
		Shake = 0.04f + 0.3f * FMath::Abs(GetDollySpeed01());
	}
	else if (BodyRig == EDCBodyRig::Steadicam)
	{
		Stiffness = 4.5f;
		MaxLead = 35.f;
		Shake = 0.06f;
		BankScale = 1.f;
		bFloat = true;
	}
	else
	{
		const float Walk01 = FMath::Clamp(Speed / FMath::Max(WalkSpeed, 1.f), 0.f, 2.f);
		Stiffness = 22.f;
		Shake = 0.22f + 0.55f * Walk01 + (Walk01 > 1.05f ? 0.9f : 0.f); // breathing, walking, running
		Bob = Walk01;
	}

	// Aim can only run so far ahead of a heavy rig - the rig drags it back.
	if (MaxLead > 0.f && C)
	{
		const float Lead = FMath::RadiansToDegrees(SmoothRot.AngularDistance(TargetQ));
		if (Lead > MaxLead)
		{
			const FQuat Clamped = FQuat::Slerp(SmoothRot, TargetQ, MaxLead / Lead);
			FRotator R = Clamped.Rotator();
			R.Roll = 0.f;
			C->SetControlRotation(R);
		}
	}

	const FQuat PrevRot = SmoothRot;
	FQuat NewRot = FQuat::Slerp(SmoothRot, TargetQ, SmoothAlpha(Stiffness, Dt));
	if (MaxTurnRate > 0.f)
	{
		const float Step = FMath::RadiansToDegrees(SmoothRot.AngularDistance(NewRot));
		const float MaxStep = MaxTurnRate * Dt;
		if (Step > MaxStep)
		{
			NewRot = FQuat::Slerp(SmoothRot, NewRot, MaxStep / Step);
		}
	}
	SmoothRot = NewRot;

	// Bank into turns (Steadicam).
	const float YawRate = FRotator::NormalizeAxis(SmoothRot.Rotator().Yaw - PrevRot.Rotator().Yaw) / Dt;
	Roll = FMath::FInterpTo(Roll, FMath::Clamp(-YawRate * 0.05f * BankScale, -7.f, 7.f), Dt, 4.f);

	// Position: float behind the body on a Steadicam, otherwise locked to the rig.
	if (bFloat)
	{
		SmoothLoc = FMath::Lerp(SmoothLoc, RigPoint, SmoothAlpha(5.f, Dt));
		const FVector Lag = SmoothLoc - RigPoint;
		if (Lag.Size() > 60.f)
		{
			SmoothLoc = RigPoint + Lag.GetSafeNormal() * 60.f;
		}
	}
	else
	{
		SmoothLoc = RigPoint;
	}

	// Shake and bob.
	Jolt = FMath::Max(0.f, Jolt - Dt * 2.5f);
	ShakeLevel = FMath::FInterpTo(ShakeLevel, (Shake + Jolt) / 1.2f, Dt, 6.f);
	const float Slow = FeelTime * 1.2f;
	const float Fast = FeelTime * 7.f;
	const float ShakePitch = Shake * (Noise(Slow, 0.f) + 0.35f * Noise(Fast, 11.f)) + Jolt * 1.5f * FMath::Sin(FeelTime * 45.f);
	const float ShakeYaw = Shake * (Noise(Slow, 23.f) + 0.35f * Noise(Fast, 37.f));
	const float ShakeRoll = Shake * 0.6f * Noise(Slow, 51.f);

	FVector Offset = FVector::ZeroVector;
	if (Bob > 0.f)
	{
		WalkPhase += Speed * Dt / 140.f * PI; // one step every ~70 cm
		Offset.Z = FMath::Sin(WalkPhase * 2.f) * 1.8f * FMath::Min(Bob, 1.5f);
		Offset.Y = FMath::Sin(WalkPhase) * 1.2f * FMath::Min(Bob, 1.5f);
	}
	if (const ADCRail* Rail = MountedRail.Get())
	{
		// Track joints every 45 cm.
		Offset.Z += FMath::Sin(RailDistance / 45.f * 2.f * PI) * 0.35f * FMath::Abs(GetDollySpeed01());
	}
	Offset = FRotator(0.f, SmoothRot.Rotator().Yaw, 0.f).RotateVector(Offset);

	const FQuat ShakeQ = FRotator(ShakePitch, ShakeYaw, ShakeRoll + Roll).Quaternion();
	GetCamera()->SetWorldLocationAndRotation(SmoothLoc + Offset, SmoothRot * ShakeQ);
}

// ---------------------------------------------------------------------------------------------
// Lenses

void ADCRigCharacter::SelectLens(int32 NewIndex)
{
	if (!Lenses.IsValidIndex(NewIndex))
	{
		return;
	}
	if (LensChangeRemaining <= 0.f && IsFilming() && NewIndex != LensIndex)
	{
		// Mid-take, a lens change takes time and nothing usable is recorded meanwhile.
		PendingLensIndex = NewIndex;
		LensChangeRemaining = LensChangeTime;
		return;
	}
	if (LensChangeRemaining > 0.f && NewIndex != PendingLensIndex)
	{
		PendingLensIndex = NewIndex; // still swapping - change which lens goes on
		return;
	}
	LensIndex = NewIndex;
	PendingLensIndex = INDEX_NONE;
	const FDCLens& Lens = Lenses[LensIndex];
	TargetFocal = FMath::Clamp(FocalLength, Lens.MinFocal, Lens.MaxFocal);
	if (!Lens.IsZoom())
	{
		FocalLength = TargetFocal = Lens.MinFocal;
	}
}

// ---------------------------------------------------------------------------------------------
// Rigs

void ADCRigCharacter::MountJib(ADCJib* Jib)
{
	BeginRig();
	MountedJib = Jib;
	JibInput = FVector2D::ZeroVector;
	JibYawVelocity = JibPitchVelocity = 0.f;
}

void ADCRigCharacter::Dismount()
{
	DollyVelocity = 0.f;
	if (ADCJib* Jib = MountedJib.Get())
	{
		MountedJib.Reset();
		JibInput = FVector2D::ZeroVector;
		JibYawVelocity = JibPitchVelocity = 0.f;
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		// Step off behind the post, away from the arm.
		const FVector Back = -FRotator(0.f, Jib->GetArmYaw(), 0.f).Vector() * 110.f;
		TeleportTo(Jib->GetActorLocation() + Back + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.f), GetActorRotation());
		GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		return;
	}
	Super::Dismount();
}

void ADCRigCharacter::ReleaseInputs()
{
	Super::ReleaseInputs();
	JibInput = FVector2D::ZeroVector;
	bBraking = false;
	DollyVelocity = 0.f;
	if (LensChangeRemaining > 0.f)
	{
		LensChangeRemaining = 0.f;
		SelectLens(PendingLensIndex);
	}
}

FString ADCRigCharacter::GetRigName() const
{
	if (MountedJib.IsValid())
	{
		return TEXT("JIB ARM");
	}
	if (MountedRail.IsValid())
	{
		return TEXT("DOLLY");
	}
	if (MountedTripod.IsValid())
	{
		return TEXT("TRIPOD - FLUID HEAD");
	}
	return BodyRig == EDCBodyRig::Steadicam ? TEXT("STEADICAM") : TEXT("HANDHELD");
}

// ---------------------------------------------------------------------------------------------
// Input

void ADCRigCharacter::OnMove(const FInputActionValue& Value)
{
	if (MountedJib.IsValid())
	{
		JibInput = Value.Get<FVector2D>();
		return;
	}
	Super::OnMove(Value);
}

void ADCRigCharacter::OnMoveReleased(const FInputActionValue& Value)
{
	JibInput = FVector2D::ZeroVector;
	Super::OnMoveReleased(Value);
}

void ADCRigCharacter::OnLook(const FInputActionValue& Value)
{
	// Long lenses magnify every movement, so aim slows down with them.
	const float LensScale = FMath::Clamp(GetCameraFOV() / 74.f, 0.2f, 1.3f);
	const FVector2D Axis = Value.Get<FVector2D>() * LookSensitivity * LensScale;
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(-Axis.Y);
}

void ADCRigCharacter::OnHeight(const FInputActionValue& Value)
{
	// With the zoom lens on, the wheel zooms (Shift + wheel still sets rig height).
	const FDCLens& Lens = Lenses[LensIndex];
	if (Lens.IsZoom() && !bSprintHeld && LensChangeRemaining <= 0.f)
	{
		TargetFocal = FMath::Clamp(TargetFocal * FMath::Pow(1.15f, Value.Get<float>()), Lens.MinFocal, Lens.MaxFocal);
		return;
	}
	Super::OnHeight(Value);
}

void ADCRigCharacter::OnBrakePressed()
{
	if (ADCReplicaDirector* D = Cast<ADCReplicaDirector>(GetDirector()))
	{
		if (D->IsShowingFilm())
		{
			D->HandleStartTake(); // skips the film
			return;
		}
	}
	bBraking = true;
}

void ADCRigCharacter::OnToggleBodyRig()
{
	if (IsOnRig())
	{
		return;
	}
	BodyRig = BodyRig == EDCBodyRig::Handheld ? EDCBodyRig::Steadicam : EDCBodyRig::Handheld;
}

void ADCRigCharacter::OnJibTool()
{
	if (ADCFreeDirector* D = GetDirector())
	{
		D->HandleSelectTool(EDCRigTool::Jib);
	}
}
