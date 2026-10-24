#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "FHTypes.h"
#include "FHPlayerState.generated.h"

UCLASS()
class FOGHUNT_API AFHPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "FogHunt")
	EFHRole GetPlayerRole() const { return PlayerRole; }

	/** Server only. Assigned by the game mode before the pawn is spawned. */
	void SetPlayerRole(EFHRole NewRole) { PlayerRole = NewRole; }

protected:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FogHunt")
	EFHRole PlayerRole = EFHRole::None;
};
