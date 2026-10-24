#include "Interaction/FHSkillCheckComponent.h"
#include "Interaction/FHInteractable.h"
#include "Interaction/FHInteractionComponent.h"
#include "Characters/FHCharacterBase.h"
#include "Engine/World.h"

EFHSkillCheckResult FFHSkillCheckParams::Evaluate(float Needle) const
{
	if (Needle >= GoodStart && Needle < GoodStart + GreatSize)
	{
		return EFHSkillCheckResult::Great;
	}
	if (Needle >= GoodStart && Needle < GoodStart + GoodSize)
	{
		return EFHSkillCheckResult::Good;
	}
	return EFHSkillCheckResult::Fail;
}

UFHSkillCheckComponent::UFHSkillCheckComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

AFHCharacterBase* UFHSkillCheckComponent::GetFHOwner() const
{
	return Cast<AFHCharacterBase>(GetOwner());
}

bool UFHSkillCheckComponent::IsLocallyControlled() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return Pawn && Pawn->IsLocallyControlled();
}

double UFHSkillCheckComponent::Now() const
{
	return GetWorld()->GetTimeSeconds();
}

IFHInteractable* UFHSkillCheckComponent::GetSkillCheckInteractable() const
{
	const AFHCharacterBase* FHOwner = GetFHOwner();
	const UFHInteractionComponent* Interaction = FHOwner ? FHOwner->GetInteractionComponent() : nullptr;
	IFHInteractable* Interactable = Interaction ? Cast<IFHInteractable>(Interaction->GetCurrentInteractable()) : nullptr;
	return (Interactable && Interactable->WantsSkillChecks(FHOwner)) ? Interactable : nullptr;
}

void UFHSkillCheckComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwner()->HasAuthority())
	{
		TickServer(DeltaTime);
	}
}

void UFHSkillCheckComponent::TickServer(float DeltaTime)
{
	const bool bEligible = GetSkillCheckInteractable() != nullptr;

	if (bServerActive)
	{
		if (!bEligible)
		{
			CancelSkillCheck();
		}
		else if (Now() > ServerNeedleStartTime + ServerParams.Duration + MaxLatencyCompensation)
		{
			// Needle ran off the end without a press: a miss counts as a fail.
			ResolveSkillCheck(EFHSkillCheckResult::Fail);
		}
		return;
	}

	if (!bEligible)
	{
		RollAccumulator = 0.f;
		return;
	}
	if (Now() - LastCheckEndTime < MinTimeBetweenChecks)
	{
		return;
	}

	RollAccumulator += DeltaTime;
	if (RollAccumulator >= RollInterval)
	{
		RollAccumulator -= RollInterval;
		if (FMath::FRand() < TriggerChance)
		{
			StartSkillCheck();
		}
	}
}

void UFHSkillCheckComponent::StartSkillCheck()
{
	ServerParams = Template;
	ServerParams.Id = NextId;
	NextId = (NextId == MAX_uint8) ? 1 : static_cast<uint8>(NextId + 1);
	ServerParams.GoodStart = static_cast<float>(FMath::FRandRange(GoodStartRange.X, GoodStartRange.Y));

	ServerNeedleStartTime = Now() + ServerParams.WarningTime;
	bServerActive = true;

	Client_StartSkillCheck(ServerParams);
}

void UFHSkillCheckComponent::Server_SubmitSkillCheck_Implementation(uint8 Id, float ClientNeedle)
{
	if (!bServerActive || Id != ServerParams.Id)
	{
		return;
	}

	// The client's needle trails the server's by about one round trip. Accept that lag up to a
	// limit, but never a press later than the server has seen or earlier than the limit allows.
	const float ServerNeedle = static_cast<float>((Now() - ServerNeedleStartTime) / ServerParams.Duration);
	const float MaxLag = MaxLatencyCompensation / ServerParams.Duration;
	const float Needle = FMath::Clamp(ClientNeedle, ServerNeedle - MaxLag, ServerNeedle + 0.02f);

	ResolveSkillCheck(ServerParams.Evaluate(Needle));
}

void UFHSkillCheckComponent::ResolveSkillCheck(EFHSkillCheckResult Result)
{
	bServerActive = false;
	LastCheckEndTime = Now();
	Client_EndSkillCheck(Result);

	// May end the interaction (e.g. a failed check kicks the survivor off the generator).
	if (IFHInteractable* Interactable = GetSkillCheckInteractable())
	{
		Interactable->HandleSkillCheckResult(GetFHOwner(), Result);
	}
}

void UFHSkillCheckComponent::CancelSkillCheck()
{
	bServerActive = false;
	LastCheckEndTime = Now();
	Client_EndSkillCheck(EFHSkillCheckResult::None);
}

void UFHSkillCheckComponent::Client_StartSkillCheck_Implementation(const FFHSkillCheckParams& Params)
{
	LocalParams = Params;
	LocalNeedleStartTime = Now() + Params.WarningTime;
	bLocalVisible = true;
	bLocalAwaitingResult = false;

	OnSkillCheckStarted.Broadcast(Params);
}

void UFHSkillCheckComponent::Client_EndSkillCheck_Implementation(EFHSkillCheckResult Result)
{
	bLocalVisible = false;
	bLocalAwaitingResult = false;
	LastLocalResult = Result;
	LastLocalResultTime = Now();

	OnSkillCheckEnded.Broadcast(Result);
}

float UFHSkillCheckComponent::GetLocalNeedle() const
{
	if (!bLocalVisible)
	{
		return -1.f;
	}
	if (bLocalAwaitingResult)
	{
		return LocalFrozenNeedle;
	}
	return static_cast<float>((Now() - LocalNeedleStartTime) / LocalParams.Duration);
}

void UFHSkillCheckComponent::PressSkillCheck()
{
	if (!bLocalVisible || bLocalAwaitingResult)
	{
		return;
	}

	const float Needle = GetLocalNeedle();
	if (Needle < 0.f)
	{
		// Still in the warning phase.
		return;
	}

	LocalFrozenNeedle = Needle;
	bLocalAwaitingResult = true;
	Server_SubmitSkillCheck(LocalParams.Id, Needle);
}
