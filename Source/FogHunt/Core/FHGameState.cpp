#include "Core/FHGameState.h"
#include "Net/UnrealNetwork.h"

void AFHGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFHGameState, GeneratorsRemaining);
	DOREPLIFETIME(AFHGameState, bExitGatesPowered);
}

void AFHGameState::InitGenerators(int32 Required)
{
	check(HasAuthority());
	GeneratorsRemaining = FMath::Max(Required, 0);
	bExitGatesPowered = GeneratorsRemaining == 0;
	OnRep_GeneratorsRemaining();
}

bool AFHGameState::NotifyGeneratorCompleted()
{
	check(HasAuthority());
	if (GeneratorsRemaining <= 0)
	{
		return false;
	}

	--GeneratorsRemaining;
	OnRep_GeneratorsRemaining();

	if (GeneratorsRemaining == 0)
	{
		bExitGatesPowered = true;
		return true;
	}
	return false;
}

void AFHGameState::OnRep_GeneratorsRemaining()
{
	OnGeneratorsChanged.Broadcast(GeneratorsRemaining);
}
