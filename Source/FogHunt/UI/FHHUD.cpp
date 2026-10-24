#include "UI/FHHUD.h"
#include "Characters/FHCharacterBase.h"
#include "Characters/FHSurvivorCharacter.h"
#include "Core/FHGameState.h"
#include "Interaction/FHInteractable.h"
#include "Interaction/FHInteractionComponent.h"
#include "Interaction/FHSkillCheckComponent.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"

void AFHHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	const AFHCharacterBase* Character = Cast<AFHCharacterBase>(GetOwningPawn());
	DrawStatus(Character);
	if (!Character)
	{
		return;
	}

	DrawInteraction(Character);

	if (const AFHSurvivorCharacter* Survivor = Cast<AFHSurvivorCharacter>(Character))
	{
		DrawSkillCheck(Survivor);
	}
	else
	{
		// Killer crosshair.
		DrawRect(FLinearColor::White, Canvas->ClipX * 0.5f - 2.f, Canvas->ClipY * 0.5f - 2.f, 4.f, 4.f);
	}
}

void AFHHUD::DrawStatus(const AFHCharacterBase* Character)
{
	FString RoleText = TEXT("SPECTATING");
	if (Character)
	{
		RoleText = Character->GetPlayerRole() == EFHRole::Killer ? TEXT("KILLER") : TEXT("SURVIVOR");
	}
	DrawText(RoleText, FLinearColor::White, 20.f, 20.f, nullptr, 1.5f);

	if (const AFHGameState* GS = GetWorld()->GetGameState<AFHGameState>())
	{
		const FString Objective = GS->AreExitGatesPowered()
			? FString(TEXT("Exit gates powered"))
			: FString::Printf(TEXT("Generators remaining: %d"), GS->GetGeneratorsRemaining());
		DrawText(Objective, FLinearColor(1.f, 0.85f, 0.4f), 20.f, 50.f);
	}
}

void AFHHUD::DrawInteraction(const AFHCharacterBase* Character)
{
	const UFHInteractionComponent* Interaction = Character->GetInteractionComponent();
	if (!Interaction)
	{
		return;
	}

	const float CenterX = Canvas->ClipX * 0.5f;
	const float BarY = Canvas->ClipY * 0.78f;
	const FLinearColor BarColor(0.95f, 0.8f, 0.25f);

	if (const IFHInteractable* Current = Cast<IFHInteractable>(Interaction->GetCurrentInteractable()))
	{
		const float Progress = Current->GetInteractionProgress();
		if (Progress >= 0.f)
		{
			DrawProgressBar(CenterX, BarY, 320.f, 14.f, Progress, BarColor);
		}
		return;
	}

	AActor* FocusedActor = Interaction->GetFocusedInteractable();
	if (const IFHInteractable* Focused = Cast<IFHInteractable>(FocusedActor))
	{
		const TCHAR* Key = Character->GetPlayerRole() == EFHRole::Survivor ? TEXT("LMB") : TEXT("SPACE");
		const FString Prompt = FString::Printf(TEXT("[Hold %s] %s"), Key, *Focused->GetInteractPrompt(Character).ToString());

		float TextWidth = 0.f;
		float TextHeight = 0.f;
		GetTextSize(Prompt, TextWidth, TextHeight);
		DrawText(Prompt, FLinearColor::White, CenterX - TextWidth * 0.5f, BarY - 28.f);

		const float Progress = Focused->GetInteractionProgress();
		if (Progress >= 0.f)
		{
			DrawProgressBar(CenterX, BarY, 320.f, 14.f, Progress, BarColor * 0.6f);
		}
	}
}

void AFHHUD::DrawSkillCheck(const AFHSurvivorCharacter* Survivor)
{
	const UFHSkillCheckComponent* SkillCheck = Survivor->GetSkillCheckComponent();
	if (!SkillCheck)
	{
		return;
	}

	const float Width = 420.f;
	const float Height = 22.f;
	const float X = Canvas->ClipX * 0.5f - Width * 0.5f;
	const float Y = Canvas->ClipY * 0.5f + 90.f;

	if (SkillCheck->IsSkillCheckVisible())
	{
		const FFHSkillCheckParams Params = SkillCheck->GetLocalParams();
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), X, Y, Width, Height);
		DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.45f), X + Width * Params.GoodStart, Y, Width * Params.GoodSize, Height);
		DrawRect(FLinearColor::White, X + Width * Params.GoodStart, Y, Width * Params.GreatSize, Height);

		const float Needle = SkillCheck->GetLocalNeedle();
		if (Needle >= 0.f)
		{
			DrawRect(FLinearColor::Red, X + Width * FMath::Clamp(Needle, 0.f, 1.f) - 2.f, Y - 5.f, 4.f, Height + 10.f);
		}
		DrawText(TEXT("[SPACE]"), FLinearColor::White, X + Width + 12.f, Y + 2.f);
		return;
	}

	// Briefly flash the last result.
	const double SinceResult = GetWorld()->GetTimeSeconds() - SkillCheck->GetLastResultTime();
	if (SinceResult < 0.8)
	{
		switch (SkillCheck->GetLastResult())
		{
		case EFHSkillCheckResult::Great:
			DrawText(TEXT("GREAT!"), FLinearColor(0.4f, 1.f, 0.4f), X, Y, nullptr, 1.5f);
			break;
		case EFHSkillCheckResult::Good:
			DrawText(TEXT("GOOD"), FLinearColor::White, X, Y, nullptr, 1.5f);
			break;
		case EFHSkillCheckResult::Fail:
			DrawText(TEXT("FAILED"), FLinearColor::Red, X, Y, nullptr, 1.5f);
			break;
		default:
			break;
		}
	}
}

void AFHHUD::DrawProgressBar(float CenterX, float Y, float Width, float Height, float Fill, const FLinearColor& Color)
{
	const float X = CenterX - Width * 0.5f;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), X, Y, Width, Height);
	DrawRect(Color, X, Y, Width * FMath::Clamp(Fill, 0.f, 1.f), Height);
}
