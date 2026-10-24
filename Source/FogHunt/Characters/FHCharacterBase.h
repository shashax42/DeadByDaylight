#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "FHTypes.h"
#include "FHCharacterBase.generated.h"

class UEnhancedInputComponent;
class UFHCharacterMovementComponent;
class UFHInteractionComponent;
class UInputAction;
class UInputMappingContext;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/**
 * Shared base for killer and survivor: movement, look, the interaction
 * component and Enhanced Input wiring.
 *
 * Input assets can be assigned in a Blueprint child. If DefaultMappingContext is
 * left empty, default actions and keys are built at runtime so the C++ project
 * is playable before any editor assets exist.
 */
UCLASS(Abstract)
class FOGHUNT_API AFHCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AFHCharacterBase(const FObjectInitializer& ObjectInitializer);

	virtual EFHRole GetPlayerRole() const { return EFHRole::None; }

	/** True while the character must not move (repairing now; carried or hooked later). */
	virtual bool IsMovementLocked() const;

	UFUNCTION(BlueprintPure, Category = "FogHunt")
	UFHCharacterMovementComponent* GetFHMovement() const;

	UFUNCTION(BlueprintPure, Category = "FogHunt")
	UFHInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }

protected:
	virtual void BeginPlay() override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Subclasses bind their role-specific actions here. */
	virtual void BindRoleInput(UEnhancedInputComponent* EnhancedInput) {}

	/** Subclasses add their role-specific actions and keys to the runtime default context. */
	virtual void AddDefaultKeyMappings(UInputMappingContext* Context);

	virtual FKey GetDefaultInteractKey() const { return EKeys::SpaceBar; }

	void CreateDefaultInputIfMissing();
	UInputAction* MakeRuntimeAction(const TCHAR* Name, EInputActionValueType ValueType);

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartInteract();
	void StopInteract();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FogHunt")
	TObjectPtr<UFHInteractionComponent> InteractionComponent;

	UPROPERTY(EditDefaultsOnly, Category = "FogHunt|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "FogHunt|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "FogHunt|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "FogHunt|Input")
	TObjectPtr<UInputAction> InteractAction;

	/** Stand-in body so players can see each other before real meshes exist. Hide it once a skeletal mesh is set. */
	UPROPERTY(VisibleAnywhere, Category = "FogHunt|Placeholder")
	TObjectPtr<UStaticMeshComponent> PlaceholderBody;

	UPROPERTY(EditDefaultsOnly, Category = "FogHunt|Placeholder")
	FLinearColor PlaceholderColor = FLinearColor::White;
};
