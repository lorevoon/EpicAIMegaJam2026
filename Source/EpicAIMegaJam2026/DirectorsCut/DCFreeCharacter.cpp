// Director's Cut prototype 2 - camera operator.

#include "DCFreeCharacter.h"
#include "DCFreeDirector.h"
#include "DCRail.h"
#include "DCTripod.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet/GameplayStatics.h"

ADCFreeCharacter::ADCFreeCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(40.f, 96.f);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	Camera->bUsePawnControlRotation = true;
	// Fixed horizontal FOV so framing checks are predictable.
	Camera->SetFieldOfView(75.f);
	Camera->PostProcessSettings.bOverride_MotionBlurAmount = true;
	Camera->PostProcessSettings.MotionBlurAmount = 0.f;

	bUseControllerRotationYaw = true;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
}

UInputAction* ADCFreeCharacter::MakeAction(EInputActionValueType Type, bool bWhenPaused)
{
	UInputAction* Action = NewObject<UInputAction>(this);
	Action->ValueType = Type;
	Action->bTriggerWhenPaused = bWhenPaused;
	Actions.Add(Action);
	return Action;
}

void ADCFreeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		return;
	}

	Mapping = NewObject<UInputMappingContext>(this);
	Actions.Reset();

	UInputAction* Move = MakeAction(EInputActionValueType::Axis2D);
	{
		Mapping->MapKey(Move, EKeys::D);
		FEnhancedActionKeyMapping& Left = Mapping->MapKey(Move, EKeys::A);
		Left.Modifiers.Add(NewObject<UInputModifierNegate>(this));
		FEnhancedActionKeyMapping& Forward = Mapping->MapKey(Move, EKeys::W);
		Forward.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(this));
		FEnhancedActionKeyMapping& Back = Mapping->MapKey(Move, EKeys::S);
		Back.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(this));
		Back.Modifiers.Add(NewObject<UInputModifierNegate>(this));
	}
	UInputAction* Look = MakeAction(EInputActionValueType::Axis2D);
	Mapping->MapKey(Look, EKeys::Mouse2D);
	UInputAction* Height = MakeAction(EInputActionValueType::Axis1D);
	Mapping->MapKey(Height, EKeys::MouseWheelAxis);

	auto MapButton = [this](FKey Key, bool bWhenPaused = false)
	{
		UInputAction* Action = MakeAction(EInputActionValueType::Boolean, bWhenPaused);
		Mapping->MapKey(Action, Key);
		return Action;
	};

	Input->BindAction(Move, ETriggerEvent::Triggered, this, &ADCFreeCharacter::OnMove);
	Input->BindAction(Move, ETriggerEvent::Completed, this, &ADCFreeCharacter::OnMoveReleased);
	Input->BindAction(Look, ETriggerEvent::Triggered, this, &ADCFreeCharacter::OnLook);
	Input->BindAction(Height, ETriggerEvent::Triggered, this, &ADCFreeCharacter::OnHeight);
	UInputAction* Sprint = MapButton(EKeys::LeftShift);
	Input->BindAction(Sprint, ETriggerEvent::Started, this, &ADCFreeCharacter::OnSprintPressed);
	Input->BindAction(Sprint, ETriggerEvent::Completed, this, &ADCFreeCharacter::OnSprintReleased);
	Input->BindAction(MapButton(EKeys::LeftMouseButton), ETriggerEvent::Started, this, &ADCFreeCharacter::OnPrimary);
	Input->BindAction(MapButton(EKeys::RightMouseButton), ETriggerEvent::Started, this, &ADCFreeCharacter::OnSecondary);
	Input->BindAction(MapButton(EKeys::E), ETriggerEvent::Started, this, &ADCFreeCharacter::OnInteract);
	Input->BindAction(MapButton(EKeys::Enter), ETriggerEvent::Started, this, &ADCFreeCharacter::OnStartTake);
	Input->BindAction(MapButton(EKeys::R), ETriggerEvent::Started, this, &ADCFreeCharacter::OnRestart);
	Input->BindAction(MapButton(EKeys::C), ETriggerEvent::Started, this, &ADCFreeCharacter::OnClearRigs);
	Input->BindAction(MapButton(EKeys::F), ETriggerEvent::Started, this, &ADCFreeCharacter::OnFinishRail);
	Input->BindAction(MapButton(EKeys::One), ETriggerEvent::Started, this, &ADCFreeCharacter::OnRailTool);
	Input->BindAction(MapButton(EKeys::Two), ETriggerEvent::Started, this, &ADCFreeCharacter::OnTripodTool);
	Input->BindAction(MapButton(EKeys::Tab), ETriggerEvent::Started, this, &ADCFreeCharacter::OnToggleNotebook);
	Input->BindAction(MapButton(EKeys::P, true), ETriggerEvent::Started, this, &ADCFreeCharacter::OnTogglePause);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(Mapping, 0);
		}
	}
}

void ADCFreeCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	GetCharacterMovement()->MaxWalkSpeed = bSprintHeld ? SprintSpeed : WalkSpeed;

	if (ADCTripod* Tripod = MountedTripod.Get())
	{
		PlaceCameraAt(Tripod->GetCameraLocation());
	}
	else if (ADCRail* Rail = MountedRail.Get())
	{
		RailDistance = FMath::Clamp(RailDistance + RailInput * RailSpeed * DeltaSeconds, 0.f, Rail->GetLength());
		PlaceCameraAt(Rail->GetFloorLocationAtDistance(RailDistance));
	}
}

void ADCFreeCharacter::PlaceCameraAt(const FVector& CameraLocation)
{
	SetActorLocation(CameraLocation - Camera->GetRelativeLocation());
}

ADCFreeDirector* ADCFreeCharacter::GetDirector()
{
	if (!Director.IsValid())
	{
		for (TActorIterator<ADCFreeDirector> It(GetWorld()); It; ++It)
		{
			Director = *It;
			break;
		}
	}
	return Director.Get();
}

float ADCFreeCharacter::GetRailProgress() const
{
	const ADCRail* Rail = MountedRail.Get();
	return Rail && Rail->GetLength() > 0.f ? RailDistance / Rail->GetLength() : 0.f;
}

void ADCFreeCharacter::BeginRig()
{
	Dismount();
	RailInput = 0.f;
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	// Rigs can hold the camera against walls; don't let the capsule shove it around.
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADCFreeCharacter::MountRail(ADCRail* Rail, float Distance)
{
	BeginRig();
	MountedRail = Rail;
	RailDistance = Distance;
}

void ADCFreeCharacter::MountTripod(ADCTripod* Tripod)
{
	BeginRig();
	MountedTripod = Tripod;
}

void ADCFreeCharacter::Dismount()
{
	if (!IsOnRig())
	{
		return;
	}
	const ADCTripod* Tripod = MountedTripod.Get();
	MountedRail.Reset();
	MountedTripod.Reset();
	RailInput = 0.f;
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	if (Tripod)
	{
		// Step back off the tripod onto the floor beside it.
		TeleportTo(Tripod->GetActorLocation() - GetActorForwardVector() * 80.f + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.f), GetActorRotation());
	}
	GetCharacterMovement()->SetMovementMode(MOVE_Falling);
}

void ADCFreeCharacter::ResetToStart(const FTransform& Start)
{
	Dismount();
	ReleaseInputs();
	GetCharacterMovement()->StopMovementImmediately();
	TeleportTo(Start.GetLocation(), Start.Rotator(), false, true);
	if (AController* C = GetController())
	{
		C->SetControlRotation(Start.Rotator());
	}
}

void ADCFreeCharacter::ReleaseInputs()
{
	bSprintHeld = false;
	RailInput = 0.f;
}

// ---------------------------------------------------------------------------------------------
// Input handlers

void ADCFreeCharacter::OnMove(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (MountedRail.IsValid())
	{
		// W travels toward the rail's last point, S toward its first.
		RailInput = Axis.Y;
		return;
	}
	if (MountedTripod.IsValid())
	{
		return;
	}
	AddMovementInput(GetActorForwardVector(), Axis.Y);
	AddMovementInput(GetActorRightVector(), Axis.X);
}

void ADCFreeCharacter::OnLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>() * LookSensitivity;
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(-Axis.Y);
}

void ADCFreeCharacter::OnHeight(const FInputActionValue& Value)
{
	if (ADCFreeDirector* D = GetDirector())
	{
		D->HandleHeightInput(Value.Get<float>());
	}
}

void ADCFreeCharacter::OnPrimary()
{
	if (ADCFreeDirector* D = GetDirector())
	{
		D->HandlePrimaryPressed();
	}
}

void ADCFreeCharacter::OnSecondary()
{
	if (ADCFreeDirector* D = GetDirector())
	{
		D->HandleSecondaryPressed();
	}
}

void ADCFreeCharacter::OnInteract()
{
	if (ADCFreeDirector* D = GetDirector())
	{
		D->HandleInteract();
	}
}

void ADCFreeCharacter::OnStartTake()
{
	if (ADCFreeDirector* D = GetDirector())
	{
		if (D->GetPhase() == EDCPhase::Results)
		{
			D->HandleRestart();
		}
		else
		{
			D->HandleStartTake();
		}
	}
}

void ADCFreeCharacter::OnRestart()
{
	if (ADCFreeDirector* D = GetDirector())
	{
		D->HandleRestart();
	}
}

void ADCFreeCharacter::OnClearRigs()
{
	if (ADCFreeDirector* D = GetDirector())
	{
		D->HandleClearRigs();
	}
}

void ADCFreeCharacter::OnFinishRail()
{
	if (ADCFreeDirector* D = GetDirector())
	{
		D->HandleFinishRail();
	}
}

void ADCFreeCharacter::OnRailTool()
{
	if (ADCFreeDirector* D = GetDirector())
	{
		D->HandleSelectTool(EDCRigTool::Rail);
	}
}

void ADCFreeCharacter::OnTripodTool()
{
	if (ADCFreeDirector* D = GetDirector())
	{
		D->HandleSelectTool(EDCRigTool::Tripod);
	}
}

void ADCFreeCharacter::OnToggleNotebook()
{
	if (ADCFreeDirector* D = GetDirector())
	{
		D->bShowNotebook = !D->bShowNotebook;
	}
}

void ADCFreeCharacter::OnTogglePause()
{
	// Pausing the game freezes the scene clock, performers, and capture accumulation together.
	UGameplayStatics::SetGamePaused(this, !UGameplayStatics::IsGamePaused(this));
}
