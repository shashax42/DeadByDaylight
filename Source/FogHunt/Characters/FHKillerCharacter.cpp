#include "Characters/FHKillerCharacter.h"
#include "Characters/FHCharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"

AFHKillerCharacter::AFHKillerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetCapsuleComponent()->InitCapsuleSize(40.f, 100.f);

	// First person: the body always faces where the camera looks.
	bUseControllerRotationYaw = true;

	UFHCharacterMovementComponent* Movement = GetFHMovement();
	Movement->bOrientRotationToMovement = false;
	Movement->MaxWalkSpeed = 460.f;
	Movement->bCanSprint = false;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(15.f, 0.f, 75.f));
	FirstPersonCamera->bUsePawnControlRotation = true;

	// The killer never sees their own body; survivors do. First-person arms go on a separate OnlyOwnerSee mesh later.
	GetMesh()->SetOwnerNoSee(true);
	PlaceholderBody->SetOwnerNoSee(true);
	PlaceholderBody->SetRelativeScale3D(FVector(0.75f, 0.75f, 2.0f));
	PlaceholderColor = FLinearColor(0.85f, 0.1f, 0.1f);
}
