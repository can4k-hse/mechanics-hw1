#include "HWStaminaHUD.h"
#include "HWCharacter.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

void AHWStaminaHUD::DrawHUD()
{
	Super::DrawHUD();
	const AHWCharacter* Character = PlayerOwner ? Cast<AHWCharacter>(PlayerOwner->GetPawn()) : nullptr;
	if (!Canvas || !Character)
	{
		return;
	}
	const float Ratio = FMath::Clamp(Character->Stamina / FMath::Max(1.0f, Character->MaxStamina), 0.0f, 1.0f);
	const float Y = FMath::Max(24.0f, Canvas->ClipY - 110.0f);
	DrawRect(FLinearColor(0.03f, 0.03f, 0.03f, 0.85f), 24.0f, Y, 310.0f, 80.0f);
	DrawText(FString::Printf(TEXT("Stamina: %.0f / %.0f"), Character->Stamina, Character->MaxStamina),
		FLinearColor::White, 36.0f, Y + 8.0f, GEngine->GetMediumFont());
	DrawRect(FLinearColor(0.15f, 0.15f, 0.15f), 36.0f, Y + 34.0f, 280.0f, 12.0f);
	DrawRect(Ratio > 0.2f ? FLinearColor(0.1f, 0.8f, 0.3f) : FLinearColor(0.9f, 0.2f, 0.1f),
		36.0f, Y + 34.0f, 280.0f * Ratio, 12.0f);
	DrawText(TEXT("Shift: sprint   E: interact"), FLinearColor::White,
		36.0f, Y + 53.0f, GEngine->GetSmallFont());
}
