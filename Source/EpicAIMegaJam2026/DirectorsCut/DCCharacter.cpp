// Director's Cut prototype - camera operator.

#include "DCCharacter.h"
#include "DCRail.h"
#include "DCTakeDirector.h"
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

ADCCharacter::ADCCharacter()
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

UInputAction* ADCCharacter::MakeAction(EInputActionValueType Type, bool bWhenPaused)
{
	UInputAction* Action = NewObject<UInputAction>(this);
	Action->ValueType = Type;
	Action->bTriggerWhenPaused = bWhenPaused;
	Actions.Add(Action);
	return Action;
}

void ADCCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
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

	auto MapButton = [this](FKey Key, bool bWhenPaused = false)
	{
		UInputAction* Action = MakeAction(EInputActionValueType::Boolean, bWhenPaused);
		Mapping->MapKey(Action, Key);
		return Action;
	};
	UInputAction* Sprint = MapButton(EKeys::LeftShift);
	UInputAction* Primary = MapButton(EKeys::LeftMouseButton);
	UInputAction* Secondary = MapButton(EKeys::RightMouseButton);
	UInputAction* Interact = MapButton(EKeys::E);
	UInputAction* StartTake = MapButton(EKeys::Enter);
	UInputAction* Restart = MapButton(EKeys::R);
	UInputAction* ClearRail = MapButton(EKeys::C);
	UInputAction* ShotList = MapButton(EKeys::Tab);
	UInputAction* Pause = MapButton(EKeys::P, true);

	Input->BindAction(Move, ETriggerEvent::Triggered, this, &ADCCharacter::OnMove);
	Input->BindAction(Move, ETriggerEvent::Completed, this, &ADCCharacter::OnMoveReleased);
	Input->BindAction(Look, ETriggerEvent::Triggered, this, &ADCCharacter::OnLook);
	Input->BindAction(Sprint, ETriggerEvent::Started, this, &ADCCharacter::OnSprintPressed);
	Input->BindAction(Sprint, ETriggerEvent::Completed, this, &ADCCharacter::OnSprintReleased);
	Input->BindAction(Primary, ETriggerEvent::Started, this, &ADCCharacter::OnPrimaryPressed);
	Input->BindAction(Primary, ETriggerEvent::Completed, this, &ADCCharacter::OnPrimaryReleased);
	Input->BindAction(Secondary, ETriggerEvent::Started, this, &ADCCharacter::OnSecondaryPressed);
	Input->BindAction(Interact, ETriggerEvent::Started, this, &ADCCharacter::OnInteract);
	Input->BindAction(StartTake, ETriggerEvent::Started, this, &ADCCharacter::OnStartTake);
	Input->BindAction(Restart, ETriggerEvent::Started, this, &ADCCharacter::OnRestart);
	Input->BindAction(ClearRail, ETriggerEvent::Started, this, &ADCCharacter::OnClearRail);
	Input->BindAction(ShotList, ETriggerEvent::Started, this, &ADCCharacter::OnToggleShotList);
	Input->BindAction(Pause, ETriggerEvent::Started, this, &ADCCharacter::OnTogglePause);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(Mapping, 0);
		}
	}
}

void ADCCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	GetCharacterMovement()->MaxWalkSpeed = bSprintHeld ? SprintSpeed : WalkSpeed;

	if (ADCRail* Rail = MountedRail.Get())
	{
		RailDistance = FMath::Clamp(RailDistance + RailInput * DollySpeed * DeltaSeconds, 0.f, Rail->GetLength());
		const FVector Floor = Rail->GetFloorLocationAtDistance(RailDistance);
		SetActorLocation(Floor + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f));
	}
}

ADCTakeDirector* ADCCharacter::GetDirector()
{
	if (!Director.IsValid())
	{
		for (TActorIterator<ADCTakeDirector> It(GetWorld()); It; ++It)
		{
			Director = *It;
			break;
		}
	}
	return Director.Get();
}

bool ADCCharacter::IsSprinting() const
{
	return bSprintHeld && !IsOnRail() && GetVelocity().Size2D() > 10.f;
}

float ADCCharacter::GetRailProgress() const
{
	const ADCRail* Rail = MountedRail.Get();
	return Rail && Rail->GetLength() > 0.f ? RailDistance / Rail->GetLength() : 0.f;
}

void ADCCharacter::MountRail(ADCRail* Rail, float Distance)
{
	MountedRail = Rail;
	RailDistance = Distance;
	RailInput = 0.f;
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
}

void ADCCharacter::DismountRail()
{
	if (!MountedRail.IsValid())
	{
		return;
	}
	MountedRail.Reset();
	RailInput = 0.f;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

void ADCCharacter::ResetToStart(const FTransform& Start)
{
	DismountRail();
	ReleaseInputs();
	GetCharacterMovement()->StopMovementImmediately();
	TeleportTo(Start.GetLocation(), Start.Rotator(), false, true);
	if (AController* C = GetController())
	{
		C->SetControlRotation(Start.Rotator());
	}
}

void ADCCharacter::ReleaseInputs()
{
	bRecordHeld = false;
	bSprintHeld = false;
	RailInput = 0.f;
}

// ---------------------------------------------------------------------------------------------
// Input handlers

void ADCCharacter::OnMove(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (IsOnRail())
	{
		// W pushes the dolly toward the rail's last point, S toward its first.
		RailInput = Axis.Y;
		return;
	}
	AddMovementInput(GetActorForwardVector(), Axis.Y);
	AddMovementInput(GetActorRightVector(), Axis.X);
}

void ADCCharacter::OnMoveReleased(const FInputActionValue& Value)
{
	RailInput = 0.f;
}

void ADCCharacter::OnLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>() * LookSensitivity;
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(-Axis.Y);
}

void ADCCharacter::OnPrimaryPressed()
{
	ADCTakeDirector* D = GetDirector();
	if (D && D->GetPhase() == EDCPhase::Filming)
	{
		bRecordHeld = true;
	}
	else if (D)
	{
		D->HandlePrimaryPressed();
	}
}

void ADCCharacter::OnSecondaryPressed()
{
	if (ADCTakeDirector* D = GetDirector())
	{
		D->HandleSecondaryPressed();
	}
}

void ADCCharacter::OnInteract()
{
	if (ADCTakeDirector* D = GetDirector())
	{
		D->HandleInteract();
	}
}

void ADCCharacter::OnStartTake()
{
	if (ADCTakeDirector* D = GetDirector())
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

void ADCCharacter::OnRestart()
{
	if (ADCTakeDirector* D = GetDirector())
	{
		D->HandleRestart();
	}
}

void ADCCharacter::OnClearRail()
{
	if (ADCTakeDirector* D = GetDirector())
	{
		D->HandleClearRail();
	}
}

void ADCCharacter::OnToggleShotList()
{
	if (ADCTakeDirector* D = GetDirector())
	{
		D->bShowShotList = !D->bShowShotList;
	}
}

void ADCCharacter::OnTogglePause()
{
	// Pausing the game freezes the scene clock, performers, and capture accumulation together.
	UGameplayStatics::SetGamePaused(this, !UGameplayStatics::IsGamePaused(this));
}
