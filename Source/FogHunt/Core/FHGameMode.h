#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FHTypes.h"
#include "FHGameMode.generated.h"

class AFHGenerator;
class AFHKillerCharacter;
class AFHSurvivorCharacter;

/**
 * Server-only match rules: who plays which role, where they spawn,
 * and what happens when objectives complete.
 */
UCLASS()
class FOGHUNT_API AFHGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFHGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void StartPlay() override;

	void NotifyGeneratorCompleted(AFHGenerator* Generator);

protected:
	/** Phase 1 rule: the first player to join is the killer, everyone after is a survivor. */
	EFHRole PickRoleForNewPlayer() const;

	UPROPERTY(EditDefaultsOnly, Category = "FogHunt|Classes")
	TSubclassOf<AFHSurvivorCharacter> SurvivorClass;

	UPROPERTY(EditDefaultsOnly, Category = "FogHunt|Classes")
	TSubclassOf<AFHKillerCharacter> KillerClass;

	/** How many generators must be repaired to power the exit gates (clamped to what the map has). */
	UPROPERTY(EditDefaultsOnly, Category = "FogHunt|Rules", meta = (ClampMin = "1"))
	int32 GeneratorsToPower = 5;
};
