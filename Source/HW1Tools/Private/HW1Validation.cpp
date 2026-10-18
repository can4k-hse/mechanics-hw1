#include "HWCharacter.h"
#include "HWStaminaHUD.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimNode_StateMachine.h"
#include "AnimStateNode.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EdGraph/EdGraph.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Misc/OutputDevice.h"
#include "UObject/UnrealType.h"

namespace HW1
{
bool Compile(UBlueprint* BP);

struct FInteractionCapture : public FOutputDevice
{
    int32 Count = 0;
    virtual void Serialize(const TCHAR* Text, ELogVerbosity::Type Verbosity, const FName& Category) override
    {
        if (Category == TEXT("LogBlueprintUserMessages") && FString(Text).Contains(TEXT("Interaction triggered!"))) ++Count;
    }
};

bool Validate()
{
    int32 Failures = 0;
    int32 Checks = 0;
    auto Expect = [&Failures, &Checks](bool bCondition, const TCHAR* Description) {
        ++Checks;
        if (bCondition) { UE_LOG(LogTemp, Display, TEXT("HW1 PASS: %s"), Description); }
        else { ++Failures; UE_LOG(LogTemp, Error, TEXT("HW1 FAIL: %s"), Description); }
    };
    auto* BP = LoadObject<UBlueprint>(nullptr, TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
    auto* AnimBP = LoadObject<UAnimBlueprint>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
    auto* Mapping = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_Default"));
    auto* Sprint = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Actions/IA_Sprint"));
    auto* Interact = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Actions/IA_Interact"));
    auto* GM = LoadObject<UBlueprint>(nullptr, TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode"));
    if (!BP || !AnimBP || !Mapping || !Sprint || !Interact || !GM) return false;
    Expect(BP->ParentClass == AHWCharacter::StaticClass(), TEXT("Blueprint inherits HWCharacter"));
    Expect(Compile(BP) && Compile(AnimBP), TEXT("Character and animation Blueprints compile"));
    auto Mapped = [Mapping](UInputAction* Action, FKey Key) {
        for (const auto& Entry : Mapping->GetMappings())
            if (Entry.Action == Action && Entry.Key == Key) return true;
        return false;
    };
    Expect(Mapped(Sprint, EKeys::LeftShift) && Mapped(Interact, EKeys::E), TEXT("Shift and E use the template mapping context"));
    Expect(Sprint->ValueType == EInputActionValueType::Boolean && Interact->ValueType == EInputActionValueType::Boolean, TEXT("Input actions use Boolean values"));
    Expect(CastChecked<AGameModeBase>(GM->GeneratedClass->GetDefaultObject())->HUDClass == AHWStaminaHUD::StaticClass(), TEXT("Stamina HUD is assigned"));

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("HW1TestWorld"));
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Character = World->SpawnActor<AHWCharacter>(BP->GeneratedClass, FVector(0, 0, 200), FRotator::ZeroRotator, Spawn);
    if (!Character) return false;
    Character->DispatchBeginPlay();
    Expect(Character->OnInteraction.IsBound(), TEXT("Blueprint subscribes to the C++ delegate at BeginPlay"));
    Expect(Character->FindComponentByClass<UCameraComponent>() != nullptr, TEXT("Template camera is preserved"));
    const float WalkSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;
    Expect(WalkSpeed > 0.0f, TEXT("Normal movement speed is valid"));
    auto* Input = NewObject<UEnhancedInputComponent>(Character);
    Character->SetupPlayerInputComponent(Input);
    auto Fire = [Input, &Expect](UInputAction* Action, ETriggerEvent Event) {
        int32 Count = 0;
        for (const auto& Binding : Input->GetActionEventBindings())
        {
            if (Binding->GetAction() == Action && Binding->GetTriggerEvent() == Event)
            {
                Binding->Execute(FInputActionInstance(Action));
                ++Count;
            }
        }
        Expect(Count == 1, TEXT("Exactly one C++ handler is bound to the input event"));
    };
    Fire(Sprint, ETriggerEvent::Started);
    Expect(Character->bIsSprinting && Character->GetCharacterMovement()->MaxWalkSpeed > WalkSpeed, TEXT("Sprint input increases speed and sets bIsSprinting"));
    Character->Tick(1.0f);
    Expect(FMath::IsNearlyEqual(Character->Stamina, 80.0f), TEXT("Sprint drains 20 stamina per second"));
    Fire(Sprint, ETriggerEvent::Completed);
    Expect(!Character->bIsSprinting && FMath::IsNearlyEqual(Character->GetCharacterMovement()->MaxWalkSpeed, WalkSpeed), TEXT("Release restores original speed"));
    Character->Tick(1.0f);
    Expect(FMath::IsNearlyEqual(Character->Stamina, 100.0f), TEXT("Recovery is capped at maximum stamina"));
    Fire(Sprint, ETriggerEvent::Started);
    for (int32 I = 0; I < 60; ++I) Character->Tick(1.0f / 60.0f);
    Expect(FMath::IsNearlyEqual(Character->Stamina, 80.0f, 0.001f), TEXT("Stamina drain is frame-rate independent"));
    Character->Tick(10.0f);
    Expect(Character->Stamina == 0.0f && !Character->bIsSprinting, TEXT("Exhaustion stops sprint without releasing Shift"));
    Expect(FMath::IsNearlyEqual(Character->GetCharacterMovement()->MaxWalkSpeed, WalkSpeed), TEXT("Exhaustion restores normal movement speed"));
    Fire(Sprint, ETriggerEvent::Started);
    Character->Tick(1.0f);
    Expect(!Character->bIsSprinting && Character->Stamina == 0.0f, TEXT("Holding exhausted sprint does not restart or recover"));
    Fire(Sprint, ETriggerEvent::Completed);
    Character->Tick(2.0f);
    Expect(FMath::IsNearlyEqual(Character->Stamina, 50.0f), TEXT("Release permits recovery at 25 per second"));
    Fire(Sprint, ETriggerEvent::Started);
    Expect(Character->bIsSprinting, TEXT("Sprint can restart after recovery"));
    Fire(Sprint, ETriggerEvent::Canceled);
    Expect(!Character->bIsSprinting, TEXT("Canceled input also stops sprint"));

    FInteractionCapture Capture;
    GLog->AddOutputDevice(&Capture);
    Fire(Interact, ETriggerEvent::Started);
    GLog->RemoveOutputDevice(&Capture);
    Expect(Capture.Count == 1, TEXT("E broadcasts delegate and Blueprint prints Interaction triggered! exactly once"));

    UAnimInstance* Anim = Character->GetMesh()->GetAnimInstance();
    Expect(Anim && Anim->GetClass() == AnimBP->GeneratedClass, TEXT("Character uses the configured standard Animation Blueprint"));
    if (Anim)
    {
        auto* SprintProperty = FindFProperty<FBoolProperty>(Anim->GetClass(), TEXT("IsSprinting"));
        Character->StartSprint();
        Anim->BlueprintUpdateAnimation(0.016f);
        Expect(SprintProperty && SprintProperty->GetPropertyValue_InContainer(Anim), TEXT("Animation Blueprint reads bIsSprinting=true from C++"));
        Character->StopSprint();
        Anim->BlueprintUpdateAnimation(0.016f);
        Expect(SprintProperty && !SprintProperty->GetPropertyValue_InContainer(Anim), TEXT("Animation Blueprint reads bIsSprinting=false from C++"));

        // Evaluate the actual animation graph, with movement supplied by the test world.
        auto* Movement = Character->GetCharacterMovement();
        Movement->SetMovementMode(MOVE_Walking);
        Movement->Velocity = FVector(WalkSpeed, 0, 0);
        auto* Acceleration = FindFProperty<FStructProperty>(Movement->GetClass(), TEXT("Acceleration"));
        if (Acceleration) *Acceleration->ContainerPtrToValuePtr<FVector>(Movement) = FVector(2048, 0, 0);
        auto AdvanceAnimation = [Character]() {
            for (int32 I = 0; I < 30; ++I)
            {
                ++GFrameCounter;
                Character->GetMesh()->TickAnimation(1.0f / 60.0f, false);
                Character->GetMesh()->RefreshBoneTransforms();
            }
        };
        auto InState = [Anim](FName MachineName, FName StateName) {
            const FAnimNode_StateMachine* Machine = Anim->GetStateMachineInstanceFromName(MachineName);
            return Machine && Machine->GetCurrentStateName() == StateName;
        };
        AdvanceAnimation();
        Expect(InState(TEXT("Locomotion"), TEXT("Walk / Run")), TEXT("Animation evaluates normal movement"));
        Character->StartSprint();
        AdvanceAnimation();
        Expect(InState(TEXT("Locomotion"), TEXT("Sprint")), TEXT("Animation transitions from movement to Sprint"));
        Character->StopSprint();
        AdvanceAnimation();
        Expect(InState(TEXT("Locomotion"), TEXT("Walk / Run")), TEXT("Animation transitions back after release"));
        Character->StartSprint();
        Character->Tick(10.0f);
        AdvanceAnimation();
        Expect(InState(TEXT("Locomotion"), TEXT("Walk / Run")), TEXT("Exhaustion also exits Sprint animation"));
        Movement->SetMovementMode(MOVE_Falling);
        Movement->Velocity.Z = 300.0f;
        AdvanceAnimation();
        Expect(!InState(TEXT("Main States"), TEXT("Locomotion")), TEXT("Template airborne animation remains active"));
    }
    TArray<UEdGraph*> Graphs;
    AnimBP->GetAllGraphs(Graphs);
    bool bSprintState = false;
    for (UEdGraph* Graph : Graphs)
        for (UEdGraphNode* Node : Graph->Nodes)
            if (auto* State = Cast<UAnimStateNode>(Node)) bSprintState |= State->GetStateName() == TEXT("Sprint");
    Expect(bSprintState, TEXT("Animation state machine contains Sprint"));

    Character->Destroy();
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    UE_LOG(LogTemp, Display, TEXT("HW1 VALIDATION: %d checks, %d failures"), Checks, Failures);
    return Failures == 0;
}
}
