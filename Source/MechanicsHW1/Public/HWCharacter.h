#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HWCharacter.generated.h"

class UInputAction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInteractionDelegate);

UCLASS()
class MECHANICSHW1_API AHWCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AHWCharacter();
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsSprinting = false;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FInteractionDelegate OnInteraction;

	UPROPERTY(BlueprintReadOnly, Category = "Stamina")
	float Stamina = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "1.0"))
	float MaxStamina = 100.0f;

	// Public C++ handlers also allow testing the same logic used by Enhanced Input.
	void StartSprint();
	void StopSprint();
	void Interact();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float SprintSpeed = 900.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
	float StaminaDrainPerSecond = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
	float StaminaRecoveryPerSecond = 25.0f;

private:
	float DefaultWalkSpeed = 0.0f;
	bool bSprintHeld = false;
	bool bExhaustedUntilRelease = false;
	void SetSprinting(bool bEnabled);
};
