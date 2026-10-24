#include "Characters/FHSurvivorCharacter.h"
#include "Characters/FHCharacterMovementComponent.h"
#include "Interaction/FHSkillCheckComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"

AFHSurvivorCharacter::AFHSurvivorCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Body turns toward movement; the camera orbits freely.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UFHCharacterMovementComponent* Movement = GetFHMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 540.f, 0.f);
	// Survivor speeds from DBD: walk 2.26 m/s, run 4.0 m/s, crouch 1.13 m/s.
	Movement->MaxWalkSpeed = 226.f;
	Movement->MaxSprintSpeed = 400.f;
	Movement->MaxWalkSpeedCrouched = 113.f;
	Movement->bCanSprint = true;
	Movement->GetNavAgentPropertiesRef().bCanCrouch = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 320.f;
	CameraBoom->SocketOffset = FVector(0.f, 40.f, 70.f);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	SkillCheckComponent = CreateDefaultSubobject<UFHSkillCheckComponent>(TEXT("SkillCheck"));

	PlaceholderColor = FLinearColor(0.2f, 0.55f, 1.f);
}

void AFHSurvivorCharacter::AddDefaultKeyMappings(UInputMappingContext* Context)
{
	Super::AddDefaultKeyMappings(Context);

	SprintAction = SprintAction ? SprintAction.Get() : MakeRuntimeAction(TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	Context->MapKey(SprintAction, EKeys::LeftShift);

	CrouchAction = CrouchAction ? CrouchAction.Get() : MakeRuntimeAction(TEXT("IA_Crouch"), EInputActionValueType::Boolean);
	Context->MapKey(CrouchAction, EKeys::LeftControl);

	SkillCheckAction = SkillCheckAction ? SkillCheckAction.Get() : MakeRuntimeAction(TEXT("IA_SkillCheck"), EInputActionValueType::Boolean);
	Context->MapKey(SkillCheckAction, EKeys::SpaceBar);
}

void AFHSurvivorCharacter::BindRoleInput(UEnhancedInputComponent* EnhancedInput)
{
	if (SprintAction)
	{
		EnhancedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &AFHSurvivorCharacter::StartSprint);
		EnhancedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &AFHSurvivorCharacter::StopSprint);
	}
	if (CrouchAction)
	{
		EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Started, this, &AFHSurvivorCharacter::StartCrouch);
		EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AFHSurvivorCharacter::StopCrouch);
	}
	if (SkillCheckAction)
	{
		EnhancedInput->BindAction(SkillCheckAction, ETriggerEvent::Started, this, &AFHSurvivorCharacter::PressSkillCheck);
	}
}

void AFHSurvivorCharacter::StartSprint()
{
	GetFHMovement()->SetWantsToSprint(true);
}

void AFHSurvivorCharacter::StopSprint()
{
	GetFHMovement()->SetWantsToSprint(false);
}

void AFHSurvivorCharacter::StartCrouch()
{
	Crouch();
}

void AFHSurvivorCharacter::StopCrouch()
{
	UnCrouch();
}

void AFHSurvivorCharacter::PressSkillCheck()
{
	SkillCheckComponent->PressSkillCheck();
}
