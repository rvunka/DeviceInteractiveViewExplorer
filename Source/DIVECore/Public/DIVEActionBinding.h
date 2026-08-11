// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DIVEDeviceAction.h"

#include "DIVEActionBinding.generated.h"

UENUM(BlueprintType)
enum class EDIVETargetMatchMode : uint8
{
	ComponentTag UMETA(DisplayName = "Component Tag"),
	ComponentName UMETA(DisplayName = "Component Name"),
	PartId UMETA(DisplayName = "Part Id"),
	/** MatchValues ignored. Default Focus/Isolate/Admin bindings. */
	AnyPrimitive UMETA(DisplayName = "Any Primitive")
};

USTRUCT(BlueprintType)
struct DIVECORE_API FDIVETargetQuery
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	EDIVETargetMatchMode MatchMode = EDIVETargetMatchMode::ComponentTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target", meta = (
		ToolTip = "Tags, names, or PartIds by Match Mode. Hidden for Any Primitive.",
		EditCondition = "MatchMode != EDIVETargetMatchMode::AnyPrimitive",
		EditConditionHides))
	TArray<FName> MatchValues;

	/** Higher wins on duplicate labels; sorts rows within a section. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	int32 Priority = 0;
};

USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEMenuSection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Section")
	FName SectionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Section", meta = (
		ToolTip = "Optional label above the section separator."))
	FText Header;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Section")
	int32 SortOrder = 0;
};

USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEActionBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding", meta = (
		ToolTip = "Optional id for Dump/Scan/FindActionInstance."))
	FName BindingId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding")
	FDIVETargetQuery Targets;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Binding")
	TArray<TObjectPtr<UDIVEDeviceAction>> Actions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding", meta = (
		ToolTip = "Index into Actions for primary click. INDEX_NONE = none."))
	int32 PrimaryActionIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding", meta = (
		ToolTip = "Must match a Sections SectionId (catalog or component). Built-in: Standard, Admin.",
		GetOptions = "GetAvailableSectionIds"))
	FName SectionId = NAME_None;

	UDIVEDeviceAction* GetPrimaryAction() const
	{
		if (PrimaryActionIndex == INDEX_NONE || !Actions.IsValidIndex(PrimaryActionIndex))
		{
			return nullptr;
		}

		return Actions[PrimaryActionIndex];
	}
};
