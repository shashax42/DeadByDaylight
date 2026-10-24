#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/FHInteractable.h"
#include "FHGenerator.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/**
 * Survivors hold interact to repair. Progress is server-authoritative and replicated;
 * several survivors repair faster together, with diminishing efficiency.
 */
UCLASS()
class FOGHUNT_API AFHGenerator : public AActor, public IFHInteractable
{
	GENERATED_BODY()

public:
	AFHGenerator();

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IFHInteractable
	virtual bool CanInteract(const AFHCharacterBase* Instigator) const override;
	virtual bool CanContinueInteract(const AFHCharacterBase* Instigator) const override;
	virtual void BeginInteract(AFHCharacterBase* Instigator) override;
	virtual void EndInteract(AFHCharacterBase* Instigator) override;
	virtual FText GetInteractPrompt(const AFHCharacterBase* Instigator) const override;
	virtual float GetInteractionProgress() const override { return Progress; }
	virtual bool WantsSkillChecks(const AFHCharacterBase* Instigator) const override { return !bCompleted; }
	virtual void HandleSkillCheckResult(AFHCharacterBase* Instigator, EFHSkillCheckResult Result) override;

	UFUNCTION(BlueprintPure, Category = "FogHunt|Generator")
	float GetProgress() const { return Progress; }

	UFUNCTION(BlueprintPure, Category = "FogHunt|Generator")
	bool IsCompleted() const { return bCompleted; }

	UFUNCTION(BlueprintPure, Category = "FogHunt|Generator")
	int32 GetNumRepairers() const { return Repairers.Num(); }

	/** Server only. Adds progress as a 0..1 fraction; negative values regress. */
	void AddProgress(float Delta);

protected:
	virtual void BeginPlay() override;

	void Complete();
	void ApplyCompletedVisuals();

	UFUNCTION()
	void OnRep_Completed();

	/** Explosion on a failed skill check. Phase 2: also notify the killer. */
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_LoudNoise();

	UFUNCTION(BlueprintImplementableEvent, Category = "FogHunt|Generator")
	void BP_OnCompleted();

	UFUNCTION(BlueprintImplementableEvent, Category = "FogHunt|Generator")
	void BP_OnLoudNoise();

	UPROPERTY(VisibleAnywhere, Category = "FogHunt|Generator")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Seconds for one survivor to finish alone (DBD: 90 charges at 1 charge/s). */
	UPROPERTY(EditAnywhere, Category = "FogHunt|Generator", meta = (ClampMin = "1"))
	float RepairTimeSolo = 90.f;

	/** Per-survivor efficiency by repairer count (index 0 = alone). Approximate values for tuning. */
	UPROPERTY(EditAnywhere, Category = "FogHunt|Generator")
	TArray<float> CoopEfficiency;

	UPROPERTY(EditAnywhere, Category = "FogHunt|Generator", meta = (ClampMin = "1"))
	int32 MaxRepairers = 4;

	UPROPERTY(EditAnywhere, Category = "FogHunt|Generator")
	float GreatSkillCheckBonus = 0.01f;

	UPROPERTY(EditAnywhere, Category = "FogHunt|Generator")
	float FailedSkillCheckPenalty = 0.10f;

	UPROPERTY(EditDefaultsOnly, Category = "FogHunt|Placeholder")
	FLinearColor IdleColor = FLinearColor(0.25f, 0.25f, 0.25f);

	UPROPERTY(EditDefaultsOnly, Category = "FogHunt|Placeholder")
	FLinearColor CompletedColor = FLinearColor(1.f, 0.8f, 0.2f);

	UPROPERTY(Replicated)
	float Progress = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_Completed)
	bool bCompleted = false;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<AFHCharacterBase>> Repairers;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MeshMaterial;
};
