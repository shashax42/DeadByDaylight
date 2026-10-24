#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "FHGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFHGeneratorsChanged, int32, GeneratorsRemaining);

/** Match-wide state every client needs: objective progress, exit gate power. */
UCLASS()
class FOGHUNT_API AFHGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "FogHunt")
	int32 GetGeneratorsRemaining() const { return GeneratorsRemaining; }

	UFUNCTION(BlueprintPure, Category = "FogHunt")
	bool AreExitGatesPowered() const { return bExitGatesPowered; }

	/** Server only. */
	void InitGenerators(int32 Required);

	/** Server only. Returns true if this completion powered the exit gates. */
	bool NotifyGeneratorCompleted();

	UPROPERTY(BlueprintAssignable, Category = "FogHunt")
	FFHGeneratorsChanged OnGeneratorsChanged;

protected:
	UFUNCTION()
	void OnRep_GeneratorsRemaining();

	UPROPERTY(ReplicatedUsing = OnRep_GeneratorsRemaining)
	int32 GeneratorsRemaining = 0;

	UPROPERTY(Replicated)
	bool bExitGatesPowered = false;
};
