#include "Core/FHPlayerState.h"
#include "Net/UnrealNetwork.h"

void AFHPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFHPlayerState, PlayerRole);
}
