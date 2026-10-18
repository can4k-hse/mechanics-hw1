#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "aaiakovlev.generated.h"

class UInputAction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInteractionDelegate);

UCLASS()
class AAIAKOVLEV_GAME_API Aaaiakovlev : public ACharacter
{
    GENERATED_BODY()

public:
    Aaaiakovlev();

    virtual void Tick(float DeltaTime) override;

    virtual void SetupPlayerInputComponent(
        UInputComponent* PlayerInputComponent) override;

    UPROPERTY(BlueprintAssignable, Category = "Interaction")
    FInteractionDelegate OnInteraction;

    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsSprinting = false;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> SprintAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> InteractAction;

    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    float SprintSpeed = 900.0f;

private:
    float DefaultWalkSpeed = 0.0f;

    void StartSprint();
    void StopSprint();
    void Interact();
};