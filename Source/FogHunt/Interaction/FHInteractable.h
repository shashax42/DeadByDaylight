#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FHTypes.h"
#include "FHInteractable.generated.h"

class AFHCharacterBase;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UFHInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Anything a character can hold the interact key on: generators now; heal, unhook,
 * exit gate, kick and pick-up later. Begin/End run on the server only; the
 * UFHInteractionComponent handles input, RPCs and range validation.
 */
class FOGHUNT_API IFHInteractable
{
	GENERATED_BODY()

public:
	/** Can Instigator start interacting now? Runs on the owning client (prompt) and the server (validation). */
	virtual bool CanInteract(const AFHCharacterBase* Instigator) const = 0;

	/** Server: return false to kick an instigator who is already interacting. */
	virtual bool CanContinueInteract(const AFHCharacterBase* Instigator) const { return true; }

	virtual void BeginInteract(AFHCharacterBase* Instigator) = 0;
	virtual void EndInteract(AFHCharacterBase* Instigator) = 0;

	virtual FText GetInteractPrompt(const AFHCharacterBase* Instigator) const { return FText::GetEmpty(); }

	/** 0..1 progress for the HUD bar, or negative when the interaction has no progress. */
	virtual float GetInteractionProgress() const { return -1.f; }

	/** Whether holding this interaction rolls skill checks (generators now, healing later). */
	virtual bool WantsSkillChecks(const AFHCharacterBase* Instigator) const { return false; }

	/** Server: apply the outcome of a skill check the instigator just resolved. */
	virtual void HandleSkillCheckResult(AFHCharacterBase* Instigator, EFHSkillCheckResult Result) {}
};
