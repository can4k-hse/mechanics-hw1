#include "HW1SetupCommandlet.h"
#include "HWCharacter.h"
#include "HWStaminaHUD.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"
#include "AnimationStateMachineGraph.h"
#include "AnimationTransitionGraph.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_StateMachine.h"
#include "AnimGraphNode_StateResult.h"
#include "AnimGraphNode_TransitionResult.h"
#include "AnimStateEntryNode.h"
#include "AnimStateNode.h"
#include "AnimStateTransitionNode.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "K2Node_AddDelegate.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "KismetCompiler.h"
#include "Misc/PackageName.h"
#include "Misc/OutputDevice.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

namespace HW1
{
const TCHAR* CharacterPath = TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter");
const TCHAR* AnimPath = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed");

template<class T, class F>
T* Node(UEdGraph* Graph, int32 X, int32 Y, F Configure)
{
    T* Result = NewObject<T>(Graph);
    Graph->AddNode(Result, false, false);
    Result->CreateNewGuid();
    Result->NodePosX = X;
    Result->NodePosY = Y;
    Configure(Result);
    Result->PostPlacedNewNode();
    Result->AllocateDefaultPins();
    return Result;
}

template<class T>
T* Node(UEdGraph* Graph, int32 X, int32 Y)
{
    return Node<T>(Graph, X, Y, [](T*) {});
}

void Connect(UEdGraphPin* A, UEdGraphPin* B)
{
    checkf(A && B, TEXT("Missing graph pin"));
    checkf(A->GetOwningNode()->GetGraph()->GetSchema()->TryCreateConnection(A, B),
        TEXT("Cannot connect %s.%s to %s.%s"), *A->GetOwningNode()->GetName(), *A->PinName.ToString(),
        *B->GetOwningNode()->GetName(), *B->PinName.ToString());
}

UEdGraphPin* Pin(UEdGraphNode* Node, const TCHAR* Name)
{
    return Node->FindPinChecked(FName(Name));
}

UK2Node_CallFunction* Call(UEdGraph* Graph, UClass* Class, const TCHAR* Name, int32 X, int32 Y)
{
    UFunction* Function = Class->FindFunctionByName(FName(Name));
    check(Function);
    return Node<UK2Node_CallFunction>(Graph, X, Y, [Function](auto* N) { N->SetFromFunction(Function); });
}

UK2Node_Event* Event(UEdGraph* Graph, UClass* Class, const TCHAR* Name, int32 X, int32 Y)
{
    for (UEdGraphNode* N : Graph->Nodes)
        if (auto* E = Cast<UK2Node_Event>(N))
            if (E->EventReference.GetMemberName() == FName(Name)) return E;
    return Node<UK2Node_Event>(Graph, X, Y, [Class, Name](auto* N) {
        N->EventReference.SetExternalMember(FName(Name), Class);
        N->bOverrideFunction = true;
    });
}

bool Save(UObject* Asset)
{
    UPackage* Package = Asset->GetOutermost();
    Package->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Package, Asset, *Filename, Args);
}

bool Compile(UBlueprint* BP)
{
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FCompilerResultsLog Results;
    FKismetEditorUtilities::CompileBlueprint(BP, EBlueprintCompileOptions::None, &Results);
    UE_LOG(LogTemp, Display, TEXT("HW1 compile %s: %d errors, %d warnings"), *BP->GetName(), Results.NumErrors, Results.NumWarnings);
    return Results.NumErrors == 0;
}

UInputAction* Action(const TCHAR* Name)
{
    const FString Path = FString(TEXT("/Game/Input/Actions/")) + Name;
    UInputAction* Result = LoadObject<UInputAction>(nullptr, *Path, nullptr, LOAD_NoWarn);
    if (!Result)
    {
        Result = NewObject<UInputAction>(CreatePackage(*Path), FName(Name), RF_Public | RF_Standalone);
        FAssetRegistryModule::AssetCreated(Result);
    }
    Result->ValueType = EInputActionValueType::Boolean;
    check(Save(Result));
    return Result;
}

void Assign(UObject* Object, const TCHAR* PropertyName, UObject* Value)
{
    auto* Property = FindFProperty<FObjectPropertyBase>(Object->GetClass(), PropertyName);
    check(Property);
    Property->SetObjectPropertyValue_InContainer(Object, Value);
}

bool ConfigureCharacter()
{
    UInputAction* Sprint = Action(TEXT("IA_Sprint"));
    UInputAction* Interact = Action(TEXT("IA_Interact"));
    auto* Mapping = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_Default"));
    if (!Mapping) return false;
    Mapping->UnmapKey(Sprint, EKeys::LeftShift);
    Mapping->UnmapKey(Interact, EKeys::E);
    Mapping->MapKey(Sprint, EKeys::LeftShift);
    Mapping->MapKey(Interact, EKeys::E);
    if (!Save(Mapping)) return false;

    auto* BP = LoadObject<UBlueprint>(nullptr, CharacterPath);
    if (!BP || BP->ParentClass != AHWCharacter::StaticClass()) return false;
    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(BP);
    if (!Graph) return false;
    // Idempotence: never add another subscription when the setup is rerun.
    bool bAlreadyBound = false;
    for (UEdGraphNode* N : Graph->Nodes)
        if (auto* Add = Cast<UK2Node_AddDelegate>(N))
            bAlreadyBound |= Add->DelegateReference.GetMemberName() == TEXT("OnInteraction");
    if (!bAlreadyBound)
    {
        auto* Begin = Event(Graph, AActor::StaticClass(), TEXT("ReceiveBeginPlay"), 0, -600);
        auto* Bind = Node<UK2Node_AddDelegate>(Graph, Begin->NodePosX + 300, Begin->NodePosY, [](auto* N) {
            N->SetFromProperty(FindFProperty<FMulticastDelegateProperty>(AHWCharacter::StaticClass(), TEXT("OnInteraction")), true, AHWCharacter::StaticClass());
        });
        const TArray<UEdGraphPin*> OldLinks = Pin(Begin, TEXT("then"))->LinkedTo;
        Pin(Begin, TEXT("then"))->BreakAllPinLinks();
        Connect(Pin(Begin, TEXT("then")), Pin(Bind, TEXT("execute")));
        for (UEdGraphPin* Old : OldLinks) Connect(Pin(Bind, TEXT("then")), Old);
        auto* Handler = UK2Node_CustomEvent::CreateFromFunction(FVector2D(Begin->NodePosX, Begin->NodePosY + 240),
            Graph, TEXT("ShowInteraction"), Bind->GetDelegateSignature(), false);
        Connect(Handler->GetDelegatePin(), Bind->GetDelegatePin());
        auto* Print = Call(Graph, UKismetSystemLibrary::StaticClass(), TEXT("PrintString"), Begin->NodePosX + 300, Begin->NodePosY + 260);
        Pin(Print, TEXT("InString"))->DefaultValue = TEXT("Interaction triggered!");
        Pin(Print, TEXT("bPrintToScreen"))->DefaultValue = TEXT("true");
        Pin(Print, TEXT("bPrintToLog"))->DefaultValue = TEXT("true");
        Pin(Print, TEXT("Duration"))->DefaultValue = TEXT("2.0");
        Connect(Pin(Handler, TEXT("then")), Pin(Print, TEXT("execute")));
    }
    if (!Compile(BP)) return false;
    Assign(BP->GeneratedClass->GetDefaultObject(), TEXT("SprintAction"), Sprint);
    Assign(BP->GeneratedClass->GetDefaultObject(), TEXT("InteractAction"), Interact);
    if (!Save(BP)) return false;

    auto* GM = LoadObject<UBlueprint>(nullptr, TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode"));
    if (!GM) return false;
    auto* Defaults = CastChecked<AGameModeBase>(GM->GeneratedClass->GetDefaultObject());
    Defaults->HUDClass = AHWStaminaHUD::StaticClass();
    return Save(GM);
}

void ConfigureSprintVariable(UAnimBlueprint* BP)
{
    const FName Variable(TEXT("IsSprinting"));
    if (FindFProperty<FBoolProperty>(BP->GeneratedClass, Variable)) return;
    FEdGraphPinType BoolType;
    BoolType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    check(FBlueprintEditorUtils::AddMemberVariable(BP, Variable, BoolType));
    check(Compile(BP));
    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(BP);
    auto* Update = Event(Graph, UAnimInstance::StaticClass(), TEXT("BlueprintUpdateAnimation"), -1000, -600);
    auto* Owner = Call(Graph, UAnimInstance::StaticClass(), TEXT("TryGetPawnOwner"), -1000, -360);
    auto* Cast = Node<UK2Node_DynamicCast>(Graph, -700, -600, [](auto* N) { N->TargetType = AHWCharacter::StaticClass(); });
    Cast->SetPurity(false);
    Connect(Pin(Owner, TEXT("ReturnValue")), Cast->GetCastSourcePin());
    auto* Read = Node<UK2Node_VariableGet>(Graph, -400, -360, [](auto* N) {
        N->VariableReference.SetExternalMember(TEXT("bIsSprinting"), AHWCharacter::StaticClass());
    });
    Connect(Cast->GetCastResultPin(), Pin(Read, TEXT("self")));
    auto* Set = Node<UK2Node_VariableSet>(Graph, -100, -600, [Variable](auto* N) { N->VariableReference.SetSelfMember(Variable); });
    Connect(Pin(Read, TEXT("bIsSprinting")), Pin(Set, TEXT("IsSprinting")));
    const TArray<UEdGraphPin*> OldLinks = Pin(Update, TEXT("then"))->LinkedTo;
    Pin(Update, TEXT("then"))->BreakAllPinLinks();
    Connect(Pin(Update, TEXT("then")), Pin(Cast, TEXT("execute")));
    Connect(Cast->GetValidCastPin(), Pin(Set, TEXT("execute")));
    for (UEdGraphPin* Old : OldLinks)
    {
        Connect(Pin(Set, TEXT("then")), Old);
        Connect(Cast->GetInvalidCastPin(), Old);
    }
}

void SprintRule(UAnimStateTransitionNode* Transition, bool bEntering)
{
    UEdGraph* Graph = Transition->BoundGraph;
    auto* Read = Node<UK2Node_VariableGet>(Graph, -400, 0, [](auto* N) { N->VariableReference.SetSelfMember(TEXT("IsSprinting")); });
    UEdGraphPin* Value = Pin(Read, TEXT("IsSprinting"));
    if (!bEntering)
    {
        auto* Not = Call(Graph, UKismetMathLibrary::StaticClass(), TEXT("Not_PreBool"), -200, 0);
        Connect(Value, Pin(Not, TEXT("A")));
        Value = Pin(Not, TEXT("ReturnValue"));
    }
    auto* Result = CastChecked<UAnimationTransitionGraph>(Graph)->GetResultNode();
    Connect(Value, Pin(Result, TEXT("bCanEnterTransition")));
    Transition->CrossfadeDuration = 0.15f;
}

bool ConfigureAnimation()
{
    auto* BP = LoadObject<UAnimBlueprint>(nullptr, AnimPath);
    if (!BP) return false;
    ConfigureSprintVariable(BP);
    TArray<UEdGraph*> Graphs;
    BP->GetAllGraphs(Graphs);
    UAnimationStateMachineGraph* Machine = nullptr;
    for (UEdGraph* Graph : Graphs)
        if (Graph->GetName() == TEXT("Locomotion"))
            if (auto* Candidate = Cast<UAnimationStateMachineGraph>(Graph)) Machine = Candidate;
    if (!Machine) return false;
    UAnimStateNode* WalkRun = nullptr;
    UAnimStateNode* Idle = nullptr;
    for (UEdGraphNode* N : Machine->Nodes)
    {
        if (auto* State = Cast<UAnimStateNode>(N))
        {
            if (State->GetStateName() == TEXT("Sprint")) return Compile(BP) && Save(BP);
            if (State->GetStateName() == TEXT("Walk / Run")) WalkRun = State;
            if (State->GetStateName() == TEXT("Idle")) Idle = State;
        }
    }
    if (!WalkRun || !Idle) return false;
    auto* Sprint = Node<UAnimStateNode>(Machine, WalkRun->NodePosX + 360, WalkRun->NodePosY);
    FBlueprintEditorUtils::RenameGraph(Sprint->BoundGraph, TEXT("Sprint"));
    auto* Sequence = LoadObject<UAnimSequence>(nullptr,
        TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd"));
    if (!Sequence) return false;
    auto* Player = Node<UAnimGraphNode_SequencePlayer>(Sprint->BoundGraph, -300, 0, [Sequence](auto* N) {
        N->Node.SetSequence(Sequence);
        N->Node.SetLoopAnimation(true);
        N->Node.SetPlayRate(1.5f);
    });
    Connect(Pin(Player, TEXT("Pose")), Sprint->GetPoseSinkPinInsideState());
    auto* Enter = Node<UAnimStateTransitionNode>(Machine, WalkRun->NodePosX + 200, WalkRun->NodePosY - 80);
    Enter->CreateConnections(WalkRun, Sprint);
    SprintRule(Enter, true);
    auto* Exit = Node<UAnimStateTransitionNode>(Machine, WalkRun->NodePosX + 200, WalkRun->NodePosY + 80);
    Exit->CreateConnections(Sprint, WalkRun);
    SprintRule(Exit, false);
    Exit->PriorityOrder = 2;

    // Preserve the template's idle behavior when Shift is held without movement.
    auto* StopMoving = Node<UAnimStateTransitionNode>(Machine, Sprint->NodePosX, Sprint->NodePosY + 180);
    StopMoving->CreateConnections(Sprint, Idle);
    StopMoving->PriorityOrder = 1;
    StopMoving->CrossfadeDuration = 0.15f;
    auto* Moving = Node<UK2Node_VariableGet>(StopMoving->BoundGraph, -400, 0, [](auto* N) {
        N->VariableReference.SetSelfMember(TEXT("ShouldMove"));
    });
    auto* Not = Call(StopMoving->BoundGraph, UKismetMathLibrary::StaticClass(), TEXT("Not_PreBool"), -200, 0);
    Connect(Pin(Moving, TEXT("ShouldMove")), Pin(Not, TEXT("A")));
    Connect(Pin(Not, TEXT("ReturnValue")), Pin(CastChecked<UAnimationTransitionGraph>(StopMoving->BoundGraph)->GetResultNode(), TEXT("bCanEnterTransition")));
    return Compile(BP) && Save(BP);
}

bool Validate();
}

UHW1SetupCommandlet::UHW1SetupCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UHW1SetupCommandlet::Main(const FString& Params)
{
    if (Params.Contains(TEXT("validate"))) return HW1::Validate() ? 0 : 1;
    if (!HW1::ConfigureCharacter() || !HW1::ConfigureAnimation()) return 1;
    return HW1::Validate() ? 0 : 1;
}
