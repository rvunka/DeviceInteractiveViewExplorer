// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "DIVEActionBinding.h"
#include "IPropertyTypeCustomization.h"
#include "Input/Reply.h"
#include "Templates/SharedPointer.h"
#include "Widgets/Input/SComboBox.h"

class IPropertyHandle;
class SWidget;
class UDIVEInspectableComponent;
class UPrimitiveComponent;

/** Details UI for FDIVEActionBinding: PrimaryActionIndex combo + matched-primitive preview. */
class FDIVEActionBindingCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

	virtual void CustomizeChildren(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		IDetailChildrenBuilder& ChildBuilder,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
	struct FPrimaryOption
	{
		int32 Index = INDEX_NONE;
		FText Label;
	};

	void RebuildPrimaryOptions();
	FText GetPrimarySelectionLabel() const;
	TSharedRef<SWidget> MakePrimaryOptionWidget(TSharedPtr<FPrimaryOption> Option) const;
	void OnPrimaryOptionSelected(TSharedPtr<FPrimaryOption> Option, ESelectInfo::Type SelectInfo);
	TSharedPtr<FPrimaryOption> FindOptionByIndex(int32 Index) const;

	const FDIVETargetQuery* GetTargetQuery() const;
	UDIVEInspectableComponent* ResolvePreviewInspectable() const;
	void InvalidateMatchedPreview();
	void EnsureMatchedPreview() const;
	void CollectMatchingPrimitives(TArray<UPrimitiveComponent*>& OutPrimitives) const;
	FText GetTargetsPreviewText() const;
	bool CanSelectMatchingPrimitives() const;
	FReply OnSelectMatchingPrimitives();

	TSharedPtr<IPropertyHandle> StructHandle;
	TSharedPtr<IPropertyHandle> PrimaryIndexHandle;
	TSharedPtr<IPropertyHandle> ActionsHandle;
	TSharedPtr<IPropertyHandle> TargetsHandle;
	TArray<TSharedPtr<FPrimaryOption>> PrimaryOptions;
	TWeakPtr<SComboBox<TSharedPtr<FPrimaryOption>>> PrimaryComboWeak;

	struct FMatchedPreview
	{
		TWeakObjectPtr<UDIVEInspectableComponent> Inspectable;
		EDIVETargetMatchMode MatchMode = EDIVETargetMatchMode::ComponentTag;
		TArray<FName> MatchValues;
		TArray<TWeakObjectPtr<UPrimitiveComponent>> Primitives;
		bool bValid = false;
	};

	mutable FMatchedPreview MatchedPreview;
};
