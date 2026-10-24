#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FHHUD.generated.h"

class AFHCharacterBase;
class AFHSurvivorCharacter;

/**
 * Debug HUD drawn with Canvas so phase 1 is testable without any UMG assets:
 * role, generators left, interaction prompt and progress, and the skill check bar.
 * Replace with UMG widgets bound to the same getters once the art pass starts.
 */
UCLASS()
class FOGHUNT_API AFHHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	void DrawStatus(const AFHCharacterBase* Character);
	void DrawInteraction(const AFHCharacterBase* Character);
	void DrawSkillCheck(const AFHSurvivorCharacter* Survivor);
	void DrawProgressBar(float CenterX, float Y, float Width, float Height, float Fill, const FLinearColor& Color);
};
