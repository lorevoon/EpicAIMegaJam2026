// Director's Cut - the director on foot.

#include "DCMultiCamCharacter.h"
#include "DCMultiCamDirector.h"
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

ADCMultiCamCharacter::ADCMultiCamCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(40.f, 96.f);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(75.f);
	Camera->PostProcessSettings.bOverride_MotionBlurAmount = true;
	Camera->PostProcessSettings.MotionBlurAmount = 0.f;

	bUseControllerRotationYaw = true;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->JumpZVelocity = 520.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxStepHeight = 45.f;
}

UInputAction* ADCMultiCamCharacter::MakeAction(EInputActionValueType Type, bool bWhenPaused)
{
	UInputAction* Action = NewObject<UInputAction>(this);
	Action->ValueType = Type;
	Action->bTriggerWhenPaused = bWhenPaused;
	Actions.Add(Action);
	return Action;
}

void ADCMultiCamCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
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
	UInputAction* Wheel = MakeAction(EInputActionValueType::Axis1D);
	Mapping->MapKey(Wheel, EKeys::MouseWheelAxis);

	auto MapButton = [this](FKey Key, bool bWhenPaused = false)
	{
		UInputAction* Action = MakeAction(EInputActionValueType::Boolean, bWhenPaused);
		Mapping->MapKey(Action, Key);
		return Action;
	};

	Input->BindAction(Move, ETriggerEvent::Triggered, this, &ADCMultiCamCharacter::OnMove);
	Input->BindAction(Look, ETriggerEvent::Triggered, this, &ADCMultiCamCharacter::OnLook);
	Input->BindAction(Wheel, ETriggerEvent::Triggered, this, &ADCMultiCamCharacter::OnWheel);
	UInputAction* Sprint = MapButton(EKeys::LeftShift);
	Input->BindAction(Sprint, ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnSprintPressed);
	Input->BindAction(Sprint, ETriggerEvent::Completed, this, &ADCMultiCamCharacter::OnSprintReleased);
	UInputAction* Primary = MapButton(EKeys::LeftMouseButton);
	Input->BindAction(Primary, ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnPrimary);
	Input->BindAction(Primary, ETriggerEvent::Completed, this, &ADCMultiCamCharacter::OnPrimaryReleased);
	Input->BindAction(MapButton(EKeys::RightMouseButton), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnSecondary);
	Input->BindAction(MapButton(EKeys::E), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnInteract);
	Input->BindAction(MapButton(EKeys::Enter), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnStartTake);
	Input->BindAction(MapButton(EKeys::R), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnRestart);
	Input->BindAction(MapButton(EKeys::C), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnClearCameras);
	Input->BindAction(MapButton(EKeys::Tab), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnToggleNotebook);
	Input->BindAction(MapButton(EKeys::P, true), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnTogglePause);

	UInputAction* Scrub = MakeAction(EInputActionValueType::Axis1D);
	Mapping->MapKey(Scrub, EKeys::Right);
	FEnhancedActionKeyMapping& ScrubBack = Mapping->MapKey(Scrub, EKeys::Left);
	ScrubBack.Modifiers.Add(NewObject<UInputModifierNegate>(this));
	Input->BindAction(Scrub, ETriggerEvent::Triggered, this, &ADCMultiCamCharacter::OnScrub);

	// Q lowers / E raises the next camera while placing (E still interacts when there's something to interact with).
	UInputAction* Height = MakeAction(EInputActionValueType::Axis1D);
	Mapping->MapKey(Height, EKeys::E);
	FEnhancedActionKeyMapping& Lower = Mapping->MapKey(Height, EKeys::Q);
	Lower.Modifiers.Add(NewObject<UInputModifierNegate>(this));
	Input->BindAction(Height, ETriggerEvent::Triggered, this, &ADCMultiCamCharacter::OnPlacementHeight);
	Input->BindAction(MapButton(EKeys::T), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnCycleTool);
	Input->BindAction(MapButton(EKeys::G), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnCycleStabilizer);
	Input->BindAction(MapButton(EKeys::LeftBracket), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnCycleCamera, -1);
	Input->BindAction(MapButton(EKeys::RightBracket), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnCycleCamera, 1);
	UInputAction* JumpOrTransport = MapButton(EKeys::SpaceBar);
	Input->BindAction(JumpOrTransport, ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnTransportToggle);
	Input->BindAction(JumpOrTransport, ETriggerEvent::Completed, this, &ADCMultiCamCharacter::StopJumping);
	Input->BindAction(JumpOrTransport, ETriggerEvent::Canceled, this, &ADCMultiCamCharacter::StopJumping);
	Input->BindAction(MapButton(EKeys::H), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnRehearsalToggle);
	Input->BindAction(MapButton(EKeys::Comma), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnRehearsalSpeed, -1);
	Input->BindAction(MapButton(EKeys::Period), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnRehearsalSpeed, 1);

	const FKey NumberKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
	for (int32 i = 0; i < UE_ARRAY_COUNT(NumberKeys); ++i)
	{
		Input->BindAction(MapButton(NumberKeys[i]), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnSelectCamera, i + 1);
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(Mapping, 0);
		}
	}
}

void ADCMultiCamCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	GetCharacterMovement()->MaxWalkSpeed = bSprintHeld ? SprintSpeed : WalkSpeed;
}

ADCMultiCamDirector* ADCMultiCamCharacter::GetDirector()
{
	if (!Director.IsValid())
	{
		for (TActorIterator<ADCMultiCamDirector> It(GetWorld()); It; ++It)
		{
			Director = *It;
			break;
		}
	}
	return Director.Get();
}

bool ADCMultiCamCharacter::IsLockedToCamera()
{
	ADCMultiCamDirector* D = GetDirector();
	// During the take the director is locked to the camera feeds.
	return D && (D->IsViewingCamera() || D->GetPhase() == EDCPhase::Countdown || D->GetPhase() == EDCPhase::Filming);
}

void ADCMultiCamCharacter::ResetToStart(const FTransform& Start)
{
	bSprintHeld = false;
	StopJumping();
	GetCharacterMovement()->StopMovementImmediately();
	TeleportTo(Start.GetLocation(), Start.Rotator(), false, true);
	if (AController* C = GetController())
	{
		C->SetControlRotation(Start.Rotator());
	}
}

// ---------------------------------------------------------------------------------------------
// Input handlers

void ADCMultiCamCharacter::OnMove(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (IsLockedToCamera())
	{
		// WASD adjusts the active camera within its placement radius.
		if (ADCMultiCamDirector* D = GetDirector())
		{
			D->HandleRigMove(Axis, GetWorld()->GetDeltaSeconds());
		}
		return;
	}
	AddMovementInput(GetActorForwardVector(), Axis.Y);
	AddMovementInput(GetActorRightVector(), Axis.X);
}

void ADCMultiCamCharacter::OnLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>() * LookSensitivity;
	ADCMultiCamDirector* D = GetDirector();
	if (D && D->HandleLook(Axis.X, Axis.Y))
	{
		return;
	}
	if (!IsLockedToCamera())
	{
		AddControllerYawInput(Axis.X);
		AddControllerPitchInput(-Axis.Y);
	}
}

void ADCMultiCamCharacter::OnWheel(const FInputActionValue& Value)
{
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleWheel(Value.Get<float>(), bSprintHeld); // Shift + wheel zooms in preparation
	}
}

void ADCMultiCamCharacter::OnPrimary()
{
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandlePrimaryPressed();
	}
}

void ADCMultiCamCharacter::OnPrimaryReleased()
{
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandlePrimaryReleased();
	}
}

void ADCMultiCamCharacter::OnSecondary()
{
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleSecondaryPressed();
	}
}

void ADCMultiCamCharacter::OnInteract()
{
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleInteract();
	}
}

void ADCMultiCamCharacter::OnStartTake()
{
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleStartTake();
	}
}

void ADCMultiCamCharacter::OnRestart()
{
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleRestart();
	}
}

void ADCMultiCamCharacter::OnClearCameras()
{
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleClearCameras();
	}
}

void ADCMultiCamCharacter::OnSelectCamera(int32 Number)
{
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleSelectCamera(Number);
	}
}

void ADCMultiCamCharacter::OnScrub(const FInputActionValue& Value)
{
	if (ADCMultiCamDirector* D = GetDirector())
	{
		// Holding an arrow key scrubs the scene at 8x speed.
		D->HandleScrub(Value.Get<float>() * GetWorld()->GetDeltaSeconds() * 8.f);
	}
}

void ADCMultiCamCharacter::OnPlacementHeight(const FInputActionValue& Value)
{
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandlePlacementHeight(Value.Get<float>() * GetWorld()->GetDeltaSeconds());
	}
}

void ADCMultiCamCharacter::OnCycleStabilizer()
{
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleCycleStabilizer();
	}
}

void ADCMultiCamCharacter::OnCycleTool()
{
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleCycleTool();
	}
}

void ADCMultiCamCharacter::OnFinishRail()
{
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleFinishRail();
	}
}

void ADCMultiCamCharacter::OnCycleCamera(int32 Direction)
{
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleCycleCamera(Direction);
	}
}

void ADCMultiCamCharacter::OnTransportToggle()
{
	ADCMultiCamDirector* DirectorActor = GetDirector();
	if (!IsLockedToCamera() && (!DirectorActor || DirectorActor->GetPhase() == EDCPhase::Preparation))
	{
		Jump();
		return;
	}
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleTransportToggle();
	}
}

void ADCMultiCamCharacter::OnRailSpeed(int32 Direction)
{
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleRailSpeed(Direction);
	}
}

void ADCMultiCamCharacter::OnRehearsalToggle()
{
	if (ADCMultiCamDirector* D = GetDirector()) D->HandleRehearsalToggle();
}

void ADCMultiCamCharacter::OnRehearsalSpeed(int32 Direction)
{
	if (ADCMultiCamDirector* D = GetDirector()) D->HandleRehearsalSpeed(Direction);
}

void ADCMultiCamCharacter::OnToggleNotebook()
{
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->bShowNotebook = !D->bShowNotebook;
	}
}

void ADCMultiCamCharacter::OnTogglePause()
{
	// Pausing the game freezes the scene clock, performers, and capture accumulation together.
	UGameplayStatics::SetGamePaused(this, !UGameplayStatics::IsGamePaused(this));
}
