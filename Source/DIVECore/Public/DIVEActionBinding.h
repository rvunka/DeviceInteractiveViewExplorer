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

/** LMB primary ranking: Name > PartId > Tag > AnyPrimitive. Ties / menu / section order = array index. */
inline int32 GetTargetMatchSpecificity(const EDIVETargetMatchMode Mode)
{
	switch (Mode)
	{
	case EDIVETargetMatchMode::ComponentName:
		return 3;
	case EDIVETargetMatchMode::PartId:
		return 2;
	case EDIVETargetMatchMode::ComponentTag:
		return 1;
	case EDIVETargetMatchMode::AnyPrimitive:
	default:
		return 0;
	}
}

USTRUCT(BlueprintType)
struct DIVECORE_API FDIVETargetQuery
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target", meta = (
		ToolTip = "How Match Values identify the pick. Part Id = DIVE Anchor PartId inherited by attached children (not the component name)."))
	EDIVETargetMatchMode MatchMode = EDIVETargetMatchMode::ComponentTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target", meta = (
		ToolTip = "Tags, component names, or Anchor PartIds by Match Mode. PartId is not the Box/mesh component name — attach the primitive under a DIVE Anchor. Hidden for Any Primitive.",
		EditCondition = "MatchMode != EDIVETargetMatchMode::AnyPrimitive",
		EditConditionHides))
	TArray<FName> MatchValues;
};

USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEMenuSection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Section")
	FName SectionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Section", meta = (
		ToolTip = "Optional label above the section separator. Section order in the menu follows the Sections array (component, then catalog)."))
	FText Header;
};

USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEActionBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding", meta = (
		ToolTip = "Optional slot id. Used by Dump/Scan, FindActionInstance, and DIVE Action Event filter."))
	FName BindingId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding")
	FDIVETargetQuery Targets;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Binding")
	TArray<TObjectPtr<UDIVEDeviceAction>> Actions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding", meta = (
		ToolTip = "Index into Actions for LMB primary. Among matching bindings, the most specific Match Mode wins (Name > PartId > Tag > Any); equal specificity keeps the earlier binding in the Bindings array. INDEX_NONE = none."))
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

/**
 * Among matched bindings that define a primary action, pick the highest match specificity.
 * Ties keep the first entry in Matched (authored Bindings order: component, then catalog).
 */
inline const FDIVEActionBinding* SelectPrimaryBinding(const TArray<const FDIVEActionBinding*>& Matched)
{
	const FDIVEActionBinding* Best = nullptr;
	int32 BestSpecificity = INDEX_NONE;

	for (const FDIVEActionBinding* Binding : Matched)
	{
		if (!Binding || !Binding->GetPrimaryAction())
		{
			continue;
		}

		const int32 Specificity = GetTargetMatchSpecificity(Binding->Targets.MatchMode);
		if (!Best || Specificity > BestSpecificity)
		{
			Best = Binding;
			BestSpecificity = Specificity;
		}
	}

	return Best;
}
