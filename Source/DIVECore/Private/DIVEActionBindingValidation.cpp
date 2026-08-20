// Copyright (c) 2026. All Rights Reserved.

#if WITH_EDITOR

#include "DIVEActionBindingValidation.h"

#include "DIVEConvention.h"
#include "Misc/DataValidation.h"

namespace DIVEActionBindingValidation
{

bool ValidateSections(
	const TArray<FDIVEMenuSection>& Sections,
	TSet<FName>& OutKnownSectionIds,
	FDataValidationContext& Context)
{
	// Built-ins are implicitly valid for bindings even when absent from Sections.
	// Do not pre-seed them into the duplicate set: Inspectable / catalogs may author Standard/Admin rows.
	bool bValid = true;
	for (int32 SectionIndex = 0; SectionIndex < Sections.Num(); ++SectionIndex)
	{
		const FDIVEMenuSection& Section = Sections[SectionIndex];
		if (Section.SectionId.IsNone())
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("Sections[%d] has an empty SectionId."), SectionIndex)));
			bValid = false;
			continue;
		}

		if (OutKnownSectionIds.Contains(Section.SectionId))
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("Duplicate SectionId '%s'."), *Section.SectionId.ToString())));
			bValid = false;
		}
		else
		{
			OutKnownSectionIds.Add(Section.SectionId);
		}
	}

	OutKnownSectionIds.Add(DIVE::kSectionStandard);
	OutKnownSectionIds.Add(DIVE::kSectionAdmin);
	return bValid;
}

bool ValidateBindings(
	const TArray<const FDIVEActionBinding*>& Bindings,
	const TSet<FName>& KnownSectionIds,
	FDataValidationContext& Context)
{
	bool bValid = true;
	TSet<FName> SeenBindingIds;

	for (int32 BindingIndex = 0; BindingIndex < Bindings.Num(); ++BindingIndex)
	{
		const FDIVEActionBinding* Binding = Bindings[BindingIndex];
		if (!Binding)
		{
			continue;
		}

		const FString BindingLabel = Binding->BindingId.IsNone()
			? FString::FromInt(BindingIndex)
			: Binding->BindingId.ToString();

		if (!Binding->BindingId.IsNone())
		{
			if (SeenBindingIds.Contains(Binding->BindingId))
			{
				Context.AddError(FText::FromString(FString::Printf(
					TEXT("Duplicate BindingId '%s'."), *BindingLabel)));
				bValid = false;
			}
			else
			{
				SeenBindingIds.Add(Binding->BindingId);
			}
		}
		else
		{
			Context.AddWarning(FText::FromString(FString::Printf(
				TEXT("Bindings[%d] has an empty BindingId (diagnostics harder)."), BindingIndex)));
		}

		if (Binding->Targets.MatchMode != EDIVETargetMatchMode::AnyPrimitive
			&& Binding->Targets.MatchValues.IsEmpty())
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("Binding '%s' has empty MatchValues."), *BindingLabel)));
			bValid = false;
		}

		if (Binding->Actions.IsEmpty())
		{
			Context.AddWarning(FText::FromString(FString::Printf(
				TEXT("Binding '%s' has no actions."), *BindingLabel)));
		}

		for (int32 ActionIndex = 0; ActionIndex < Binding->Actions.Num(); ++ActionIndex)
		{
			if (!Binding->Actions[ActionIndex])
			{
				Context.AddError(FText::FromString(FString::Printf(
					TEXT("Binding '%s' Actions[%d] is null."), *BindingLabel, ActionIndex)));
				bValid = false;
			}
		}

		if (Binding->PrimaryActionIndex != INDEX_NONE)
		{
			if (!Binding->Actions.IsValidIndex(Binding->PrimaryActionIndex)
				|| !Binding->Actions[Binding->PrimaryActionIndex])
			{
				Context.AddError(FText::FromString(FString::Printf(
					TEXT("Binding '%s' PrimaryActionIndex %d is out of range or null."),
					*BindingLabel,
					Binding->PrimaryActionIndex)));
				bValid = false;
			}
		}

		// NAME_None SectionId falls back to kSectionStandard at runtime — not an error.
		if (!Binding->SectionId.IsNone() && !KnownSectionIds.Contains(Binding->SectionId))
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("Binding '%s' SectionId '%s' does not match any Sections entry."),
				*BindingLabel,
				*Binding->SectionId.ToString())));
			bValid = false;
		}
	}

	return bValid;
}

} // namespace DIVEActionBindingValidation

#endif // WITH_EDITOR
