#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "HW1SetupCommandlet.generated.h"

UCLASS()
class UHW1SetupCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UHW1SetupCommandlet();
    virtual int32 Main(const FString& Params) override;
};
