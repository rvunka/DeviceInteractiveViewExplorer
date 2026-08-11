// Copyright (c) 2026. All Rights Reserved.

#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "DIVEActionBinding.h"

struct FDIVEActionBinding;
struct FDIVEMenuSection;
class FDataValidationContext;

/**
 * Shared binding/section validation logic.
 *
 * Called from UDIVEActionCatalogAsset::IsDataValid, UDIVEInspectableComponent::IsDataValid and
 * DIVEDeviceScan so that all three produce the same errors against the same contract.
 *
 * Contract:
 *  - kSectionStandard and kSectionAdmin are always implicitly valid SectionIds, even if absent
 *    from the Sections array.
 *  - A Binding with SectionId == NAME_None falls back to kSectionStandard (no error).
 */
namespace DIVEActionBindingValidation
{
	/**
	 * Validates Sections for empty/duplicate IDs.
	 * Populates KnownSectionIds (includes built-in IDs).
	 * Returns false if any error was added.
	 */
	DIVECORE_API bool ValidateSections(
		const TArray<FDIVEMenuSection>& Sections,
		TSet<FName>& OutKnownSectionIds,
		FDataValidationContext& Context);

	/**
	 * Validates a flat list of bindings against the provided known section IDs.
	 * Returns false if any error was added.
	 */
	DIVECORE_API bool ValidateBindings(
		const TArray<const FDIVEActionBinding*>& Bindings,
		const TSet<FName>& KnownSectionIds,
		FDataValidationContext& Context);
}

#endif // WITH_EDITOR
