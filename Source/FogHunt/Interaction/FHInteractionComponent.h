#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FHInteractionComponent.generated.h"

class AFHCharacterBase;

/**
 * Finds the interactable in reach (owning client), asks the server to start or stop
 * interacting with it, and keeps the server-side interaction valid while it lasts.
 *
 * Flow: input pressed -> StartInteract (client, predicted) -> Server_BeginInteract
 * (validated) -> CurrentInteractable replicates back. Input released -> StopInteract
 * -> Server_EndInteract.
 */
UCLASS(ClassGroup = (FogHunt), meta = (BlueprintSpawnableComponent))
class FOGHUNT_API UFHInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFHInteractionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Owning client: interact input pressed / released. */
	void StartInteract();
	void StopInteract();

	/** Server: end the current interaction regardless of input (completed, failed skill check, hit...). */
	void ForceEndInteraction();

	UFUNCTION(BlueprintPure, Category = "FogHunt|Interaction")
	bool IsInteracting() const;

	UFUNCTION(BlueprintPure, Category = "FogHunt|Interaction")
	AActor* GetCurrentInteractable() const { return CurrentInteractable; }

	/** Owning client only: best interactable in reach, used for the prompt. */
	UFUNCTION(BlueprintPure, Category = "FogHunt|Interaction")
	AActor* GetFocusedInteractable() const { return FocusedInteractable; }

protected:
	UFUNCTION(Server, Reliable)
	void Server_BeginInteract(AActor* Target);

	UFUNCTION(Server, Reliable)
	void Server_EndInteract();

	UFUNCTION(Client, Reliable)
	void Client_InteractRejected();

	UFUNCTION()
	void OnRep_CurrentInteractable();

	AActor* FindBestInteractable() const;
	bool IsInReach(const AActor* Target, float Tolerance) const;
	void EndInteractionInternal();
	AFHCharacterBase* GetFHOwner() const;
	bool IsLocallyControlled() const;

	/** Distance from the character's center to the target's bounds. */
	UPROPERTY(EditDefaultsOnly, Category = "FogHunt|Interaction")
	float InteractionRadius = 150.f;

	/** Extra distance the server allows, for latency. */
	UPROPERTY(EditDefaultsOnly, Category = "FogHunt|Interaction")
	float ServerRangeTolerance = 75.f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentInteractable)
	TObjectPtr<AActor> CurrentInteractable;

	UPROPERTY(Transient)
	TObjectPtr<AActor> FocusedInteractable;

	/** Owning client: pressed, waiting for the server to confirm. */
	bool bPendingInteract = false;

	/** Owning client: released, so movement unlocks immediately instead of after a round trip. */
	bool bLocalReleased = false;
};
