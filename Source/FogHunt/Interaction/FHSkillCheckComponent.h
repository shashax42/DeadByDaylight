#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FHTypes.h"
#include "FHSkillCheckComponent.generated.h"

class AFHCharacterBase;
class IFHInteractable;

/** One skill check. Positions are fractions of the needle's sweep (0 = start, 1 = end). */
USTRUCT(BlueprintType)
struct FOGHUNT_API FFHSkillCheckParams
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "SkillCheck")
	uint8 Id = 0;

	/** Warning sound plays this long before the needle starts moving. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillCheck")
	float WarningTime = 0.5f;

	/** Seconds for the needle to sweep from 0 to 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillCheck")
	float Duration = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillCheck")
	float GoodStart = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillCheck")
	float GoodSize = 0.13f;

	/** The great zone sits at the very start of the good zone, as in DBD. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillCheck")
	float GreatSize = 0.03f;

	EFHSkillCheckResult Evaluate(float Needle) const;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFHSkillCheckStarted, const FFHSkillCheckParams&, Params);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFHSkillCheckEnded, EFHSkillCheckResult, Result);

/**
 * Server rolls skill checks while the owner holds an interaction that wants them,
 * the owning client draws the needle and reports where it was when the key was hit,
 * and the server judges the result with latency limits.
 */
UCLASS(ClassGroup = (FogHunt), meta = (BlueprintSpawnableComponent))
class FOGHUNT_API UFHSkillCheckComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFHSkillCheckComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Owning client: skill check key pressed. */
	void PressSkillCheck();

	// Owning-client UI state, read by the HUD or a UMG widget.

	UFUNCTION(BlueprintPure, Category = "FogHunt|SkillCheck")
	bool IsSkillCheckVisible() const { return bLocalVisible; }

	/** Needle position 0..1; negative during the warning before it starts moving. */
	UFUNCTION(BlueprintPure, Category = "FogHunt|SkillCheck")
	float GetLocalNeedle() const;

	UFUNCTION(BlueprintPure, Category = "FogHunt|SkillCheck")
	FFHSkillCheckParams GetLocalParams() const { return LocalParams; }

	UFUNCTION(BlueprintPure, Category = "FogHunt|SkillCheck")
	EFHSkillCheckResult GetLastResult() const { return LastLocalResult; }

	/** World time when the last result arrived, for flashing "GREAT" etc. */
	double GetLastResultTime() const { return LastLocalResultTime; }

	UPROPERTY(BlueprintAssignable, Category = "FogHunt|SkillCheck")
	FFHSkillCheckStarted OnSkillCheckStarted;

	UPROPERTY(BlueprintAssignable, Category = "FogHunt|SkillCheck")
	FFHSkillCheckEnded OnSkillCheckEnded;

protected:
	UFUNCTION(Client, Reliable)
	void Client_StartSkillCheck(const FFHSkillCheckParams& Params);

	UFUNCTION(Client, Reliable)
	void Client_EndSkillCheck(EFHSkillCheckResult Result);

	UFUNCTION(Server, Reliable)
	void Server_SubmitSkillCheck(uint8 Id, float ClientNeedle);

	void TickServer(float DeltaTime);
	void StartSkillCheck();
	void ResolveSkillCheck(EFHSkillCheckResult Result);
	void CancelSkillCheck();

	/** Server: the interaction the owner is holding, if it wants skill checks. */
	IFHInteractable* GetSkillCheckInteractable() const;
	AFHCharacterBase* GetFHOwner() const;
	bool IsLocallyControlled() const;
	double Now() const;

	/** How often the server rolls for a new skill check while eligible. */
	UPROPERTY(EditDefaultsOnly, Category = "FogHunt|SkillCheck")
	float RollInterval = 1.f;

	/** Chance per roll. */
	UPROPERTY(EditDefaultsOnly, Category = "FogHunt|SkillCheck", meta = (ClampMin = "0", ClampMax = "1"))
	float TriggerChance = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "FogHunt|SkillCheck")
	float MinTimeBetweenChecks = 2.f;

	/** How far (seconds) the client's reported needle may lag the server's before it is clamped. */
	UPROPERTY(EditDefaultsOnly, Category = "FogHunt|SkillCheck")
	float MaxLatencyCompensation = 0.3f;

	/** Base parameters for each check; GoodStart is randomized within GoodStartRange. */
	UPROPERTY(EditDefaultsOnly, Category = "FogHunt|SkillCheck")
	FFHSkillCheckParams Template;

	UPROPERTY(EditDefaultsOnly, Category = "FogHunt|SkillCheck")
	FVector2D GoodStartRange = FVector2D(0.35f, 0.85f);

	// Server state
	bool bServerActive = false;
	FFHSkillCheckParams ServerParams;
	double ServerNeedleStartTime = 0.0;
	double LastCheckEndTime = -1000.0;
	float RollAccumulator = 0.f;
	uint8 NextId = 1;

	// Owning-client state
	bool bLocalVisible = false;
	bool bLocalAwaitingResult = false;
	FFHSkillCheckParams LocalParams;
	double LocalNeedleStartTime = 0.0;
	float LocalFrozenNeedle = 0.f;
	EFHSkillCheckResult LastLocalResult = EFHSkillCheckResult::None;
	double LastLocalResultTime = -1000.0;
};
