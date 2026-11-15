// Director's Cut - the director on foot.

#include "DCMultiCamCharacter.h"
#include "DCMultiCamDirector.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet/GameplayStatics.h"
#if WITH_EDITOR
#include "Framework/Commands/InputBindingManager.h"
#include "Framework/Commands/UICommandInfo.h"
#endif

ADCMultiCamCharacter::ADCMultiCamCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(40.f, 96.f);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(75.f);
	Camera->SetConstraintAspectRatio(true);
	Camera->SetAspectRatio(DCFrame::ScreenAspect);
	Camera->PostProcessSettings.bOverride_MotionBlurAmount = true;
	Camera->PostProcessSettings.MotionBlurAmount = 0.f;

	bUseControllerRotationYaw = true;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->JumpZVelocity = 0.f;
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanJump = false;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxStepHeight = 45.f;
}

void ADCMultiCamCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
#if WITH_EDITOR
	if (bRestoreEditorEscape)
	{
		const auto Stop = FInputBindingManager::Get().FindCommandInContext(FName("PlayWorld"), FName("StopPlaySession"));
		if (Stop.IsValid() && *Stop->GetActiveChord(EMultipleKeyBindingIndex::Primary) == FInputChord(EModifierKey::Shift, EKeys::Escape))
			Stop->SetActiveChord(FInputChord(EKeys::Escape), EMultipleKeyBindingIndex::Primary);
		bRestoreEditorEscape = false;
	}
#endif
	Super::EndPlay(EndPlayReason);
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

#if WITH_EDITOR
	// Let gameplay receive Escape in PIE. Restore the editor shortcut when this play session ends.
	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		const auto Stop = FInputBindingManager::Get().FindCommandInContext(FName("PlayWorld"), FName("StopPlaySession"));
		if (Stop.IsValid() && *Stop->GetActiveChord(EMultipleKeyBindingIndex::Primary) == FInputChord(EKeys::Escape))
		{
			Stop->SetActiveChord(FInputChord(EModifierKey::Shift, EKeys::Escape), EMultipleKeyBindingIndex::Primary);
			bRestoreEditorEscape = true;
		}
	}
#endif
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
	Input->BindAction(MapButton(EKeys::RightMouseButton), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnSecondary);
	Input->BindAction(MapButton(EKeys::Escape), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnCameraCancel);
	Input->BindAction(MapButton(EKeys::F), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnCameraInteract);
	Input->BindAction(MapButton(EKeys::Enter), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnStartTake);
	Input->BindAction(MapButton(EKeys::R), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnRestart);
	Input->BindAction(MapButton(EKeys::C), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnClearCameras);
	Input->BindAction(MapButton(EKeys::P, true), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnTogglePause);

	// Hold E (or Right) to scrub the timeline forward, Q (or Left) to scrub back.
	UInputAction* Scrub = MakeAction(EInputActionValueType::Axis1D);
	Mapping->MapKey(Scrub, EKeys::E);
	Mapping->MapKey(Scrub, EKeys::Right);
	Mapping->MapKey(Scrub, EKeys::Q).Modifiers.Add(NewObject<UInputModifierNegate>(this));
	Mapping->MapKey(Scrub, EKeys::Left).Modifiers.Add(NewObject<UInputModifierNegate>(this));
	Input->BindAction(Scrub, ETriggerEvent::Triggered, this, &ADCMultiCamCharacter::OnScrub);
	Input->BindAction(MapButton(EKeys::LeftBracket), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnCycleCamera, -1);
	Input->BindAction(MapButton(EKeys::RightBracket), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnCycleCamera, 1);
	Input->BindAction(MapButton(EKeys::SpaceBar), ETriggerEvent::Started, this, &ADCMultiCamCharacter::OnTransportToggle);
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

bool ADCMultiCamCharacter::IsInputBlocked()
{
	// During a cutscene or a fade only Enter and clicks get through (they move the cutscene on).
	const ADCMultiCamDirector* D = GetDirector();
	return D && D->IsInputBlocked();
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
	if (IsInputBlocked())
	{
		return;
	}
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
	if (IsInputBlocked())
	{
		return;
	}
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
	if (IsInputBlocked())
	{
		return;
	}
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleWheel(Value.Get<float>());
	}
}

void ADCMultiCamCharacter::OnPrimary()
{
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandlePrimaryPressed();
	}
}

void ADCMultiCamCharacter::OnCameraCancel()
{
	if (IsInputBlocked())
	{
		return;
	}
	if (ADCMultiCamDirector* D = GetDirector()) D->HandleCameraCancel();
}

void ADCMultiCamCharacter::OnCameraInteract()
{
	if (IsInputBlocked())
	{
		return;
	}
	if (ADCMultiCamDirector* D = GetDirector()) D->HandleCameraInteract();
}

void ADCMultiCamCharacter::OnSecondary()
{
	if (IsInputBlocked())
	{
		return;
	}
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleSecondaryPressed();
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
	if (IsInputBlocked())
	{
		return;
	}
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleRestart();
	}
}

void ADCMultiCamCharacter::OnClearCameras()
{
	if (IsInputBlocked())
	{
		return;
	}
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleClearCameras();
	}
}

void ADCMultiCamCharacter::OnSelectCamera(int32 Number)
{
	if (IsInputBlocked())
	{
		return;
	}
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleSelectCamera(Number);
	}
}

void ADCMultiCamCharacter::OnScrub(const FInputActionValue& Value)
{
	if (IsInputBlocked())
	{
		return;
	}
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleScrub(Value.Get<float>() * GetWorld()->GetDeltaSeconds() * D->KeyScrubRate);
	}
}

void ADCMultiCamCharacter::OnFinishRail()
{
	if (IsInputBlocked())
	{
		return;
	}
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleFinishRail();
	}
}

void ADCMultiCamCharacter::OnCycleCamera(int32 Direction)
{
	if (IsInputBlocked())
	{
		return;
	}
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleCycleCamera(Direction);
	}
}

void ADCMultiCamCharacter::OnTransportToggle()
{
	if (IsInputBlocked())
	{
		return;
	}

	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleTransportToggle();
	}
}

void ADCMultiCamCharacter::OnRailSpeed(int32 Direction)
{
	if (IsInputBlocked())
	{
		return;
	}
	if (ADCMultiCamDirector* D = GetDirector())
	{
		D->HandleRailSpeed(Direction);
	}
}

void ADCMultiCamCharacter::OnRehearsalToggle()
{
	if (IsInputBlocked())
	{
		return;
	}
	if (ADCMultiCamDirector* D = GetDirector()) D->HandleRehearsalToggle();
}

void ADCMultiCamCharacter::OnRehearsalSpeed(int32 Direction)
{
	if (IsInputBlocked())
	{
		return;
	}
	if (ADCMultiCamDirector* D = GetDirector()) D->HandleRehearsalSpeed(Direction);
}

void ADCMultiCamCharacter::OnTogglePause()
{
	// Pausing the game freezes the scene clock, performers, and capture accumulation together.
	UGameplayStatics::SetGamePaused(this, !UGameplayStatics::IsGamePaused(this));
}
