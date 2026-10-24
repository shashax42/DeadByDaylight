#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "FHCharacterMovementComponent.generated.h"

/**
 * Adds a client-predicted sprint (sent to the server as a saved-move flag, so no
 * rubber-banding) and a movement lock while the owner is busy interacting.
 */
UCLASS()
class FOGHUNT_API UFHCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	/** Ground speed while the sprint input is held (cm/s). Ignored unless bCanSprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FogHunt|Movement")
	float MaxSprintSpeed = 400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FogHunt|Movement")
	bool bCanSprint = false;

	void SetWantsToSprint(bool bNewWantsToSprint) { bWantsToSprint = bNewWantsToSprint; }
	bool WantsToSprint() const { return bWantsToSprint; }

	UFUNCTION(BlueprintPure, Category = "FogHunt|Movement")
	bool IsSprinting() const;

	virtual float GetMaxSpeed() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

protected:
	bool IsMovementLocked() const;

	bool bWantsToSprint = false;
};
