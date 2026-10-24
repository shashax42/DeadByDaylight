#pragma once

#include "CoreMinimal.h"
#include "Characters/FHCharacterBase.h"
#include "FHKillerCharacter.generated.h"

class UCameraComponent;

/** First-person killer: always moves at 115% of survivor run speed, no sprint. */
UCLASS()
class FOGHUNT_API AFHKillerCharacter : public AFHCharacterBase
{
	GENERATED_BODY()

public:
	AFHKillerCharacter(const FObjectInitializer& ObjectInitializer);

	virtual EFHRole GetPlayerRole() const override { return EFHRole::Killer; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FogHunt|Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;
};
