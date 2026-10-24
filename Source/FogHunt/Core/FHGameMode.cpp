#include "Core/FHGameMode.h"
#include "Core/FHGameState.h"
#include "Core/FHPlayerState.h"
#include "Characters/FHKillerCharacter.h"
#include "Characters/FHSurvivorCharacter.h"
#include "Objectives/FHGenerator.h"
#include "UI/FHHUD.h"
#include "FogHunt.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

namespace FHPlayerStartTags
{
	static const FName Killer(TEXT("Killer"));
	static const FName Survivor(TEXT("Survivor"));
}

AFHGameMode::AFHGameMode()
{
	GameStateClass = AFHGameState::StaticClass();
	PlayerStateClass = AFHPlayerState::StaticClass();
	HUDClass = AFHHUD::StaticClass();
	DefaultPawnClass = AFHSurvivorCharacter::StaticClass();

	SurvivorClass = AFHSurvivorCharacter::StaticClass();
	KillerClass = AFHKillerCharacter::StaticClass();
}

void AFHGameMode::PostLogin(APlayerController* NewPlayer)
{
	// Super::PostLogin spawns the pawn, so the role has to be decided first.
	if (AFHPlayerState* PS = NewPlayer ? NewPlayer->GetPlayerState<AFHPlayerState>() : nullptr)
	{
		PS->SetPlayerRole(PickRoleForNewPlayer());
		UE_LOG(LogFogHunt, Log, TEXT("%s joined as %s"), *PS->GetPlayerName(), *UEnum::GetValueAsString(PS->GetPlayerRole()));
	}

	Super::PostLogin(NewPlayer);
}

EFHRole AFHGameMode::PickRoleForNewPlayer() const
{
	for (const APlayerState* Other : GameState->PlayerArray)
	{
		const AFHPlayerState* FHOther = Cast<AFHPlayerState>(Other);
		if (FHOther && FHOther->GetPlayerRole() == EFHRole::Killer)
		{
			return EFHRole::Survivor;
		}
	}
	return EFHRole::Killer;
}

UClass* AFHGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (const AFHPlayerState* PS = InController ? InController->GetPlayerState<AFHPlayerState>() : nullptr)
	{
		if (PS->GetPlayerRole() == EFHRole::Killer && KillerClass)
		{
			return KillerClass;
		}
		if (PS->GetPlayerRole() == EFHRole::Survivor && SurvivorClass)
		{
			return SurvivorClass;
		}
	}
	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

AActor* AFHGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// Player Starts tagged "Killer" / "Survivor" in the level keep the two sides apart.
	const AFHPlayerState* PS = Player ? Player->GetPlayerState<AFHPlayerState>() : nullptr;
	const FName WantedTag = (PS && PS->GetPlayerRole() == EFHRole::Killer) ? FHPlayerStartTags::Killer : FHPlayerStartTags::Survivor;

	TArray<APlayerStart*> Candidates;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		if (It->PlayerStartTag == WantedTag)
		{
			Candidates.Add(*It);
		}
	}

	if (Candidates.Num() > 0)
	{
		return Candidates[FMath::RandHelper(Candidates.Num())];
	}
	return Super::ChoosePlayerStart_Implementation(Player);
}

void AFHGameMode::StartPlay()
{
	Super::StartPlay();

	int32 GeneratorsInLevel = 0;
	for (TActorIterator<AFHGenerator> It(GetWorld()); It; ++It)
	{
		++GeneratorsInLevel;
	}

	if (GeneratorsInLevel < GeneratorsToPower)
	{
		UE_LOG(LogFogHunt, Warning, TEXT("Level has %d generators but %d are required; requiring %d."),
			GeneratorsInLevel, GeneratorsToPower, GeneratorsInLevel);
	}

	if (AFHGameState* GS = GetGameState<AFHGameState>())
	{
		GS->InitGenerators(FMath::Min(GeneratorsToPower, GeneratorsInLevel));
	}
}

void AFHGameMode::NotifyGeneratorCompleted(AFHGenerator* Generator)
{
	AFHGameState* GS = GetGameState<AFHGameState>();
	if (!GS)
	{
		return;
	}

	const bool bPowered = GS->NotifyGeneratorCompleted();
	UE_LOG(LogFogHunt, Log, TEXT("Generator %s completed, %d remaining"), *GetNameSafe(Generator), GS->GetGeneratorsRemaining());

	if (bPowered)
	{
		// Phase 3: power the exit gates and start the end-game collapse here.
		UE_LOG(LogFogHunt, Log, TEXT("All required generators repaired: exit gates powered"));
	}
}
