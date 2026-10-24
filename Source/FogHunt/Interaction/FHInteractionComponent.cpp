#include "Interaction/FHInteractionComponent.h"
#include "Interaction/FHInteractable.h"
#include "Characters/FHCharacterBase.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

UFHInteractionComponent::UFHInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UFHInteractionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UFHInteractionComponent, CurrentInteractable);
}

AFHCharacterBase* UFHInteractionComponent::GetFHOwner() const
{
	return Cast<AFHCharacterBase>(GetOwner());
}

bool UFHInteractionComponent::IsLocallyControlled() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return Pawn && Pawn->IsLocallyControlled();
}

bool UFHInteractionComponent::IsInteracting() const
{
	if (bPendingInteract)
	{
		return true;
	}
	return CurrentInteractable != nullptr && !bLocalReleased;
}

void UFHInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsLocallyControlled())
	{
		FocusedInteractable = IsInteracting() ? nullptr : FindBestInteractable();
	}

	if (GetOwner()->HasAuthority() && CurrentInteractable)
	{
		const IFHInteractable* Interactable = Cast<IFHInteractable>(CurrentInteractable.Get());
		if (!Interactable || !IsInReach(CurrentInteractable, ServerRangeTolerance) || !Interactable->CanContinueInteract(GetFHOwner()))
		{
			EndInteractionInternal();
		}
	}
}

void UFHInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		EndInteractionInternal();
	}
	Super::EndPlay(EndPlayReason);
}

AActor* UFHInteractionComponent::FindBestInteractable() const
{
	const AFHCharacterBase* FHOwner = GetFHOwner();
	UWorld* World = GetWorld();
	if (!FHOwner || !World)
	{
		return nullptr;
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FHInteraction), false, FHOwner);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, FHOwner->GetActorLocation(), FQuat::Identity, ObjectParams,
		FCollisionShape::MakeSphere(InteractionRadius), QueryParams);

	// Prefer what the camera is pointed at, like DBD's prompt selection.
	const FVector ViewForward = FHOwner->GetControlRotation().Vector().GetSafeNormal2D();
	AActor* Best = nullptr;
	float BestScore = -1.f;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		const IFHInteractable* Interactable = Cast<IFHInteractable>(Candidate);
		if (!Interactable || !Interactable->CanInteract(FHOwner))
		{
			continue;
		}

		const FVector ToCandidate = (Candidate->GetActorLocation() - FHOwner->GetActorLocation()).GetSafeNormal2D();
		const float Score = FVector::DotProduct(ViewForward, ToCandidate);
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = Candidate;
		}
	}
	return Best;
}

bool UFHInteractionComponent::IsInReach(const AActor* Target, float Tolerance) const
{
	const AFHCharacterBase* FHOwner = GetFHOwner();
	if (!FHOwner || !Target)
	{
		return false;
	}

	const FBox Bounds = Target->GetComponentsBoundingBox();
	const float MaxDistance = InteractionRadius + Tolerance;
	return Bounds.ComputeSquaredDistanceToPoint(FHOwner->GetActorLocation()) <= FMath::Square(MaxDistance);
}

void UFHInteractionComponent::StartInteract()
{
	if (IsInteracting() || !FocusedInteractable)
	{
		return;
	}

	bPendingInteract = true;
	bLocalReleased = false;
	Server_BeginInteract(FocusedInteractable);
}

void UFHInteractionComponent::StopInteract()
{
	if (!bPendingInteract && !CurrentInteractable)
	{
		return;
	}

	bPendingInteract = false;
	bLocalReleased = true;
	Server_EndInteract();
}

void UFHInteractionComponent::ForceEndInteraction()
{
	if (GetOwner()->HasAuthority())
	{
		EndInteractionInternal();
	}
}

void UFHInteractionComponent::Server_BeginInteract_Implementation(AActor* Target)
{
	AFHCharacterBase* FHOwner = GetFHOwner();
	IFHInteractable* Interactable = Cast<IFHInteractable>(Target);

	const bool bValid = FHOwner && Interactable && !CurrentInteractable
		&& IsInReach(Target, ServerRangeTolerance) && Interactable->CanInteract(FHOwner);
	if (!bValid)
	{
		Client_InteractRejected();
		return;
	}

	CurrentInteractable = Target;
	bPendingInteract = false;
	bLocalReleased = false;
	Interactable->BeginInteract(FHOwner);
}

void UFHInteractionComponent::Server_EndInteract_Implementation()
{
	EndInteractionInternal();
}

void UFHInteractionComponent::Client_InteractRejected_Implementation()
{
	bPendingInteract = false;
}

void UFHInteractionComponent::EndInteractionInternal()
{
	bPendingInteract = false;
	if (!CurrentInteractable)
	{
		return;
	}

	AActor* Previous = CurrentInteractable;
	CurrentInteractable = nullptr;
	if (IFHInteractable* Interactable = Cast<IFHInteractable>(Previous))
	{
		Interactable->EndInteract(GetFHOwner());
	}
}

void UFHInteractionComponent::OnRep_CurrentInteractable()
{
	// The server's answer replaces any local prediction.
	bPendingInteract = false;
	if (CurrentInteractable == nullptr)
	{
		bLocalReleased = false;
	}
}
