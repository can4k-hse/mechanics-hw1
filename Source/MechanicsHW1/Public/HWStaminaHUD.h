#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HWStaminaHUD.generated.h"

UCLASS()
class MECHANICSHW1_API AHWStaminaHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
};
