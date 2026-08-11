// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "IPropertyTypeCustomization.h"
#include "Templates/SharedPointer.h"
#include "Widgets/Input/SComboBox.h"

class IPropertyHandle;
class SWidget;

/** Details UI for FDIVEActionBinding: PrimaryActionIndex as a combo of Actions display names. */
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

	TSharedPtr<IPropertyHandle> StructHandle;
	TSharedPtr<IPropertyHandle> PrimaryIndexHandle;
	TSharedPtr<IPropertyHandle> ActionsHandle;
	TArray<TSharedPtr<FPrimaryOption>> PrimaryOptions;
	TWeakPtr<SComboBox<TSharedPtr<FPrimaryOption>>> PrimaryComboWeak;
};
