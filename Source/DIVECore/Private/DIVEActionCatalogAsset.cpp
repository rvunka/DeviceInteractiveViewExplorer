// Copyright (c) 2026. All Rights Reserved.

#include "DIVEActionCatalogAsset.h"

#include "DIVEConvention.h"

TArray<FName> UDIVEActionCatalogAsset::GetAvailableSectionIds() const
{
	TArray<FName> Result = {DIVE::kSectionStandard, DIVE::kSectionAdmin};
	for (const FDIVEMenuSection& Section : Sections)
	{
		if (!Section.SectionId.IsNone())
		{
			Result.AddUnique(Section.SectionId);
		}
	}
	return Result;
}

#if WITH_EDITOR

#include "DIVEActionBindingValidation.h"
#include "Misc/DataValidation.h"

EDataValidationResult UDIVEActionCatalogAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (Result == EDataValidationResult::NotValidated)
	{
		Result = EDataValidationResult::Valid;
	}

	TSet<FName> KnownSectionIds;
	if (!DIVEActionBindingValidation::ValidateSections(Sections, KnownSectionIds, Context))
	{
		Result = EDataValidationResult::Invalid;
	}

	TArray<const FDIVEActionBinding*> BindingPtrs;
	BindingPtrs.Reserve(Bindings.Num());
	for (const FDIVEActionBinding& B : Bindings)
	{
		BindingPtrs.Add(&B);
	}

	if (!DIVEActionBindingValidation::ValidateBindings(BindingPtrs, KnownSectionIds, Context))
	{
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}

#endif // WITH_EDITOR
