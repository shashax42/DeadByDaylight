#include "Characters/FHCharacterBase.h"
#include "Characters/FHCharacterMovementComponent.h"
#include "Interaction/FHInteractionComponent.h"
#include "FogHunt.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AFHCharacterBase::AFHCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UFHCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionComponent = CreateDefaultSubobject<UFHInteractionComponent>(TEXT("Interaction"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
	PlaceholderBody->SetupAttachment(GetCapsuleComponent());
	PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Engine cylinder is 100cm x 100cm with a centered pivot; fit it to the default 34 x 88 capsule.
	PlaceholderBody->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.76f));
	if (CylinderMesh.Succeeded())
	{
		PlaceholderBody->SetStaticMesh(CylinderMesh.Object);
	}
	if (ShapeMaterial.Succeeded())
	{
		PlaceholderBody->SetMaterial(0, ShapeMaterial.Object);
	}
}

UFHCharacterMovementComponent* AFHCharacterBase::GetFHMovement() const
{
	return CastChecked<UFHCharacterMovementComponent>(GetCharacterMovement());
}

bool AFHCharacterBase::IsMovementLocked() const
{
	return InteractionComponent && InteractionComponent->IsInteracting();
}

void AFHCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (UMaterialInstanceDynamic* Material = PlaceholderBody->CreateAndSetMaterialInstanceDynamic(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), PlaceholderColor);
	}
}

void AFHCharacterBase::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	const APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	CreateDefaultInputIfMissing();
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}

void AFHCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	CreateDefaultInputIfMissing();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogFogHunt, Error, TEXT("Input component is not an EnhancedInputComponent. Check Config/DefaultInput.ini."));
		return;
	}

	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFHCharacterBase::Move);
	}
	if (LookAction)
	{
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFHCharacterBase::Look);
	}
	if (InteractAction)
	{
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AFHCharacterBase::StartInteract);
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Completed, this, &AFHCharacterBase::StopInteract);
	}

	BindRoleInput(EnhancedInput);
}

void AFHCharacterBase::CreateDefaultInputIfMissing()
{
	if (DefaultMappingContext)
	{
		return;
	}

	DefaultMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_RuntimeDefault"));
	AddDefaultKeyMappings(DefaultMappingContext);
}

UInputAction* AFHCharacterBase::MakeRuntimeAction(const TCHAR* Name, EInputActionValueType ValueType)
{
	UInputAction* Action = NewObject<UInputAction>(this, Name);
	Action->ValueType = ValueType;
	return Action;
}

void AFHCharacterBase::AddDefaultKeyMappings(UInputMappingContext* Context)
{
	// Move: WASD -> 2D axis (X = right, Y = forward), same layout as the UE third person template.
	MoveAction = MoveAction ? MoveAction.Get() : MakeRuntimeAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	{
		FEnhancedActionKeyMapping& W = Context->MapKey(MoveAction, EKeys::W);
		W.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Context));
	}
	{
		FEnhancedActionKeyMapping& S = Context->MapKey(MoveAction, EKeys::S);
		S.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Context));
		S.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
	}
	{
		FEnhancedActionKeyMapping& A = Context->MapKey(MoveAction, EKeys::A);
		A.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
	}
	Context->MapKey(MoveAction, EKeys::D);

	// Look: mouse delta, Y inverted so moving the mouse up looks up.
	LookAction = LookAction ? LookAction.Get() : MakeRuntimeAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	{
		FEnhancedActionKeyMapping& Mouse = Context->MapKey(LookAction, EKeys::Mouse2D);
		UInputModifierNegate* NegateY = NewObject<UInputModifierNegate>(Context);
		NegateY->bX = false;
		NegateY->bZ = false;
		Mouse.Modifiers.Add(NegateY);
	}

	InteractAction = InteractAction ? InteractAction.Get() : MakeRuntimeAction(TEXT("IA_Interact"), EInputActionValueType::Boolean);
	Context->MapKey(InteractAction, GetDefaultInteractKey());
}

void AFHCharacterBase::Move(const FInputActionValue& Value)
{
	if (!Controller || IsMovementLocked())
	{
		return;
	}

	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FRotationMatrix YawMatrix(YawRotation);

	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), Axis.X);
}

void AFHCharacterBase::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void AFHCharacterBase::StartInteract()
{
	InteractionComponent->StartInteract();
}

void AFHCharacterBase::StopInteract()
{
	InteractionComponent->StopInteract();
}
