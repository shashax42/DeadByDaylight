#include "Characters/FHCharacterMovementComponent.h"
#include "Characters/FHCharacterBase.h"
#include "GameFramework/Character.h"

namespace
{
	/** Saved move that records the sprint input so the server replays it exactly. */
	class FFHSavedMove : public FSavedMove_Character
	{
	public:
		typedef FSavedMove_Character Super;

		bool bSavedWantsToSprint = false;

		virtual void Clear() override
		{
			Super::Clear();
			bSavedWantsToSprint = false;
		}

		virtual uint8 GetCompressedFlags() const override
		{
			uint8 Result = Super::GetCompressedFlags();
			if (bSavedWantsToSprint)
			{
				Result |= FLAG_Custom_0;
			}
			return Result;
		}

		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override
		{
			if (bSavedWantsToSprint != static_cast<const FFHSavedMove*>(NewMove.Get())->bSavedWantsToSprint)
			{
				return false;
			}
			return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
		}

		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override
		{
			Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
			if (const UFHCharacterMovementComponent* Movement = Cast<UFHCharacterMovementComponent>(C->GetCharacterMovement()))
			{
				bSavedWantsToSprint = Movement->WantsToSprint();
			}
		}

		virtual void PrepMoveFor(ACharacter* C) override
		{
			Super::PrepMoveFor(C);
			if (UFHCharacterMovementComponent* Movement = Cast<UFHCharacterMovementComponent>(C->GetCharacterMovement()))
			{
				Movement->SetWantsToSprint(bSavedWantsToSprint);
			}
		}
	};

	class FFHNetworkPredictionData_Client : public FNetworkPredictionData_Client_Character
	{
	public:
		typedef FNetworkPredictionData_Client_Character Super;

		explicit FFHNetworkPredictionData_Client(const UCharacterMovementComponent& ClientMovement)
			: Super(ClientMovement)
		{
		}

		virtual FSavedMovePtr AllocateNewMove() override
		{
			return FSavedMovePtr(new FFHSavedMove());
		}
	};
}

bool UFHCharacterMovementComponent::IsSprinting() const
{
	return bCanSprint && bWantsToSprint && IsMovingOnGround() && !IsCrouching() && Velocity.SizeSquared2D() > KINDA_SMALL_NUMBER;
}

float UFHCharacterMovementComponent::GetMaxSpeed() const
{
	if (IsMovementLocked())
	{
		return 0.f;
	}
	if (bCanSprint && bWantsToSprint && IsMovingOnGround() && !IsCrouching())
	{
		return MaxSprintSpeed;
	}
	// Handles walking and MaxWalkSpeedCrouched.
	return Super::GetMaxSpeed();
}

void UFHCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

FNetworkPredictionData_Client* UFHCharacterMovementComponent::GetPredictionData_Client() const
{
	if (ClientPredictionData == nullptr)
	{
		UFHCharacterMovementComponent* MutableThis = const_cast<UFHCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FFHNetworkPredictionData_Client(*this);
	}
	return ClientPredictionData;
}

bool UFHCharacterMovementComponent::IsMovementLocked() const
{
	const AFHCharacterBase* FHOwner = Cast<AFHCharacterBase>(GetCharacterOwner());
	return FHOwner && FHOwner->IsMovementLocked();
}
