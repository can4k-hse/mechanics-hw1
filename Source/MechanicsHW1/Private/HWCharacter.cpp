#include "HWCharacter.h"

#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputAction.h"

AHWCharacter::AHWCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AHWCharacter::BeginPlay()
{
	Super::BeginPlay();
	DefaultWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	Stamina = MaxStamina;
}

void AHWCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!ensureMsgf(Input, TEXT("HWCharacter requires Enhanced Input")))
	{
		return;
	}
	if (ensureMsgf(SprintAction, TEXT("Assign IA_Sprint in BP_ThirdPersonCharacter")))
	{
		Input->BindAction(SprintAction, ETriggerEvent::Started, this, &AHWCharacter::StartSprint);
		Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &AHWCharacter::StopSprint);
		Input->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AHWCharacter::StopSprint);
	}
	if (ensureMsgf(InteractAction, TEXT("Assign IA_Interact in BP_ThirdPersonCharacter")))
	{
		Input->BindAction(InteractAction, ETriggerEvent::Started, this, &AHWCharacter::Interact);
	}
}

void AHWCharacter::SetSprinting(bool bEnabled)
{
	bIsSprinting = bEnabled;
	GetCharacterMovement()->MaxWalkSpeed = bEnabled ? SprintSpeed : DefaultWalkSpeed;
}

void AHWCharacter::StartSprint()
{
	bSprintHeld = true;
	if (!bExhaustedUntilRelease && Stamina > 0.0f)
	{
		SetSprinting(true);
	}
}

void AHWCharacter::StopSprint()
{
	bSprintHeld = false;
	bExhaustedUntilRelease = false;
	SetSprinting(false);
}

void AHWCharacter::Interact()
{
	OnInteraction.Broadcast();
}

void AHWCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bIsSprinting)
	{
		Stamina = FMath::Max(0.0f, Stamina - StaminaDrainPerSecond * DeltaSeconds);
		if (Stamina <= 0.0f)
		{
			bExhaustedUntilRelease = true;
			SetSprinting(false);
		}
	}
	else if (!bSprintHeld)
	{
		// Releasing Shift allows recovery. Holding an exhausted sprint cannot restart it.
		Stamina = FMath::Min(MaxStamina, Stamina + StaminaRecoveryPerSecond * DeltaSeconds);
	}
}
