#pragma once

#include "CoreMinimal.h"
#include "Characters/FHCharacterBase.h"
#include "FHSurvivorCharacter.generated.h"

class UCameraComponent;
class UFHSkillCheckComponent;
class USpringArmComponent;

/** Third-person survivor: walks by default, sprints while Shift is held, crouches while Ctrl is held. */
UCLASS()
class FOGHUNT_API AFHSurvivorCharacter : public AFHCharacterBase
{
	GENERATED_BODY()

public:
	AFHSurvivorCharacter(const FObjectInitializer& ObjectInitializer);

	virtual EFHRole GetPlayerRole() const override { return EFHRole::Survivor; }

	UFUNCTION(BlueprintPure, Category = "FogHunt")
	UFHSkillCheckComponent* GetSkillCheckComponent() const { return SkillCheckComponent; }

protected:
	virtual void BindRoleInput(UEnhancedInputComponent* EnhancedInput) override;
	virtual void AddDefaultKeyMappings(UInputMappingContext* Context) override;
	virtual FKey GetDefaultInteractKey() const override { return EKeys::LeftMouseButton; }

	void StartSprint();
	void StopSprint();
	void StartCrouch();
	void StopCrouch();
	void PressSkillCheck();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FogHunt|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FogHunt|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FogHunt")
	TObjectPtr<UFHSkillCheckComponent> SkillCheckComponent;

	UPROPERTY(EditDefaultsOnly, Category = "FogHunt|Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, Category = "FogHunt|Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditDefaultsOnly, Category = "FogHunt|Input")
	TObjectPtr<UInputAction> SkillCheckAction;
};
