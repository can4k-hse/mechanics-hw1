// Fill out your copyright notice in the Description page of Project Settings.


#include "aaiakovlev.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
Aaaiakovlev::Aaaiakovlev()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void Aaaiakovlev::BeginPlay()
{
	Super::BeginPlay();
	DefaultWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
}

// Called every frame
void Aaaiakovlev::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void Aaaiakovlev::SetupPlayerInputComponent(
    UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* Input =
        Cast<UEnhancedInputComponent>(PlayerInputComponent);

    if (!Input)
    {
        return;
    }

    if (SprintAction)
    {
        Input->BindAction(
            SprintAction, ETriggerEvent::Started,
            this, &Aaaiakovlev::StartSprint);

        Input->BindAction(
            SprintAction, ETriggerEvent::Completed,
            this, &Aaaiakovlev::StopSprint);

        Input->BindAction(
            SprintAction, ETriggerEvent::Canceled,
            this, &Aaaiakovlev::StopSprint);
    }

    if (InteractAction)
    {
        Input->BindAction(
            InteractAction, ETriggerEvent::Started,
            this, &Aaaiakovlev::Interact);
    }
}


void Aaaiakovlev::StartSprint()
{
	bIsSprinting = true;
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void Aaaiakovlev::StopSprint()
{
	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;
}

void Aaaiakovlev::Interact()
{
    OnInteraction.Broadcast();
}
