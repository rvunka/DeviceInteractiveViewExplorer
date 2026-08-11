// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DIVEActionBinding.h"
#include "Engine/DataAsset.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include "DIVEActionCatalogAsset.generated.h"

/**
 * Shared bindings/sections for a device family.
 * Prefer over component Bindings for reusable authored ops.
 */
UCLASS(BlueprintType)
class DIVECORE_API UDIVEActionCatalogAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sections")
	TArray<FDIVEMenuSection> Sections;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bindings")
	TArray<FDIVEActionBinding> Bindings;

	/**
	 * Returns available SectionId values for the GetOptions dropdown on Binding.SectionId.
	 * Includes built-in IDs (Standard, Admin) and all authored section IDs.
	 */
	UFUNCTION()
	TArray<FName> GetAvailableSectionIds() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
