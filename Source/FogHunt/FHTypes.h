#pragma once

#include "CoreMinimal.h"
#include "FHTypes.generated.h"

UENUM(BlueprintType)
enum class EFHRole : uint8
{
	None,
	Survivor,
	Killer
};

UENUM(BlueprintType)
enum class EFHSkillCheckResult : uint8
{
	None,
	Great,
	Good,
	Fail
};
