#include "Objectives/FHGenerator.h"
#include "Characters/FHCharacterBase.h"
#include "Core/FHGameMode.h"
#include "Interaction/FHInteractionComponent.h"
#include "FogHunt.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "FogHuntGenerator"

AFHGenerator::AFHGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// Placeholder box about the size of a DBD generator (1m x 1.6m x 1.2m), resting on the actor origin.
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	Mesh->SetRelativeScale3D(FVector(1.f, 1.6f, 1.2f));
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}
	if (ShapeMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, ShapeMaterial.Object);
	}

	CoopEfficiency = { 1.f, 0.85f, 0.7f, 0.55f };
}

void AFHGenerator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFHGenerator, Progress);
	DOREPLIFETIME(AFHGenerator, bCompleted);
	DOREPLIFETIME(AFHGenerator, Repairers);
}

void AFHGenerator::BeginPlay()
{
	Super::BeginPlay();

	MeshMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0);
	if (MeshMaterial)
	{
		MeshMaterial->SetVectorParameterValue(TEXT("Color"), bCompleted ? CompletedColor : IdleColor);
	}
}

void AFHGenerator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority() || bCompleted || Repairers.IsEmpty())
	{
		return;
	}

	const int32 NumRepairers = Repairers.Num();
	const float Efficiency = CoopEfficiency.IsValidIndex(NumRepairers - 1) ? CoopEfficiency[NumRepairers - 1] : 1.f;
	AddProgress(NumRepairers * Efficiency * DeltaSeconds / RepairTimeSolo);
}

void AFHGenerator::AddProgress(float Delta)
{
	if (!HasAuthority() || bCompleted)
	{
		return;
	}

	Progress = FMath::Clamp(Progress + Delta, 0.f, 1.f);
	if (Progress >= 1.f)
	{
		Complete();
	}
}

void AFHGenerator::Complete()
{
	bCompleted = true;

	// Copy first: ending an interaction calls back into EndInteract, which edits Repairers.
	const TArray<TObjectPtr<AFHCharacterBase>> ToRelease = Repairers;
	for (AFHCharacterBase* Repairer : ToRelease)
	{
		if (Repairer && Repairer->GetInteractionComponent())
		{
			Repairer->GetInteractionComponent()->ForceEndInteraction();
		}
	}
	Repairers.Reset();

	ApplyCompletedVisuals();

	if (AFHGameMode* GameMode = GetWorld()->GetAuthGameMode<AFHGameMode>())
	{
		GameMode->NotifyGeneratorCompleted(this);
	}
}

void AFHGenerator::OnRep_Completed()
{
	if (bCompleted)
	{
		ApplyCompletedVisuals();
	}
}

void AFHGenerator::ApplyCompletedVisuals()
{
	if (MeshMaterial)
	{
		MeshMaterial->SetVectorParameterValue(TEXT("Color"), CompletedColor);
	}
	BP_OnCompleted();
}

bool AFHGenerator::CanInteract(const AFHCharacterBase* Instigator) const
{
	if (bCompleted || !Instigator || Instigator->GetPlayerRole() != EFHRole::Survivor)
	{
		return false;
	}

	const bool bAlreadyRepairing = Repairers.ContainsByPredicate([Instigator](const TObjectPtr<AFHCharacterBase>& Repairer)
	{
		return Repairer.Get() == Instigator;
	});
	return !bAlreadyRepairing && Repairers.Num() < MaxRepairers;
}

bool AFHGenerator::CanContinueInteract(const AFHCharacterBase* Instigator) const
{
	return !bCompleted;
}

void AFHGenerator::BeginInteract(AFHCharacterBase* Instigator)
{
	Repairers.AddUnique(Instigator);
}

void AFHGenerator::EndInteract(AFHCharacterBase* Instigator)
{
	Repairers.Remove(Instigator);
}

FText AFHGenerator::GetInteractPrompt(const AFHCharacterBase* Instigator) const
{
	return LOCTEXT("RepairPrompt", "Repair");
}

void AFHGenerator::HandleSkillCheckResult(AFHCharacterBase* Instigator, EFHSkillCheckResult Result)
{
	switch (Result)
	{
	case EFHSkillCheckResult::Great:
		AddProgress(GreatSkillCheckBonus);
		break;

	case EFHSkillCheckResult::Fail:
		AddProgress(-FailedSkillCheckPenalty);
		Multicast_LoudNoise();
		// The survivor flinches off the generator and has to press again.
		if (Instigator && Instigator->GetInteractionComponent())
		{
			Instigator->GetInteractionComponent()->ForceEndInteraction();
		}
		break;

	default:
		break;
	}
}

void AFHGenerator::Multicast_LoudNoise_Implementation()
{
	UE_LOG(LogFogHunt, Log, TEXT("%s: failed skill check, loud noise"), *GetName());
	BP_OnLoudNoise();
}

#undef LOCTEXT_NAMESPACE
