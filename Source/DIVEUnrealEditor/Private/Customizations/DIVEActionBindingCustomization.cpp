// Copyright (c) 2026. All Rights Reserved.

#include "Customizations/DIVEActionBindingCustomization.h"

#include "DIVEActionBinding.h"
#include "DIVEDeviceAction.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DIVEActionBindingCustomization"

TSharedRef<IPropertyTypeCustomization> FDIVEActionBindingCustomization::MakeInstance()
{
	return MakeShared<FDIVEActionBindingCustomization>();
}

void FDIVEActionBindingCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	(void)StructCustomizationUtils;

	HeaderRow
		.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			StructPropertyHandle->CreatePropertyValueWidget()
		];
}

void FDIVEActionBindingCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& ChildBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	(void)StructCustomizationUtils;

	StructHandle = StructPropertyHandle;
	PrimaryIndexHandle = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FDIVEActionBinding, PrimaryActionIndex));
	ActionsHandle = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FDIVEActionBinding, Actions));

	if (ActionsHandle.IsValid())
	{
		ActionsHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FDIVEActionBindingCustomization::RebuildPrimaryOptions));
		ActionsHandle->SetOnChildPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FDIVEActionBindingCustomization::RebuildPrimaryOptions));
	}

	RebuildPrimaryOptions();

	int32 InitialPrimaryIndex = INDEX_NONE;
	if (PrimaryIndexHandle.IsValid())
	{
		PrimaryIndexHandle->GetValue(InitialPrimaryIndex);
	}

	uint32 NumChildren = 0;
	StructPropertyHandle->GetNumChildren(NumChildren);
	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		TSharedPtr<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex);
		if (!ChildHandle.IsValid() || !ChildHandle->GetProperty())
		{
			continue;
		}

		if (ChildHandle->GetProperty()->GetFName()
			== GET_MEMBER_NAME_CHECKED(FDIVEActionBinding, PrimaryActionIndex)
			&& PrimaryIndexHandle.IsValid())
		{
			TSharedRef<SComboBox<TSharedPtr<FPrimaryOption>>> PrimaryCombo =
				SNew(SComboBox<TSharedPtr<FPrimaryOption>>)
				.OptionsSource(&PrimaryOptions)
				.OnGenerateWidget(this, &FDIVEActionBindingCustomization::MakePrimaryOptionWidget)
				.OnSelectionChanged(this, &FDIVEActionBindingCustomization::OnPrimaryOptionSelected)
				.InitiallySelectedItem(FindOptionByIndex(InitialPrimaryIndex))
				[
					SNew(STextBlock)
					.Text(TAttribute<FText>::Create(
						TAttribute<FText>::FGetter::CreateSP(
							this,
							&FDIVEActionBindingCustomization::GetPrimarySelectionLabel)))
				];
			PrimaryComboWeak = PrimaryCombo;

			ChildBuilder.AddCustomRow(PrimaryIndexHandle->GetPropertyDisplayName())
				.NameContent()
				[
					PrimaryIndexHandle->CreatePropertyNameWidget()
				]
				.ValueContent()
				.MinDesiredWidth(240.f)
				[
					PrimaryCombo
				];
		}
		else
		{
			ChildBuilder.AddProperty(ChildHandle.ToSharedRef());
		}
	}
}

void FDIVEActionBindingCustomization::RebuildPrimaryOptions()
{
	PrimaryOptions.Reset();

	TSharedPtr<FPrimaryOption> NoneOption = MakeShared<FPrimaryOption>();
	NoneOption->Index = INDEX_NONE;
	NoneOption->Label = LOCTEXT("PrimaryNone", "None");
	PrimaryOptions.Add(NoneOption);

	if (ActionsHandle.IsValid())
	{
		uint32 NumActions = 0;
		ActionsHandle->GetNumChildren(NumActions);
		for (uint32 ActionIndex = 0; ActionIndex < NumActions; ++ActionIndex)
		{
			TSharedPtr<IPropertyHandle> ElementHandle = ActionsHandle->GetChildHandle(ActionIndex);
			UObject* ActionObject = nullptr;
			if (ElementHandle.IsValid())
			{
				if (ElementHandle->GetValue(ActionObject) != FPropertyAccess::Success)
				{
					TArray<void*> RawData;
					ElementHandle->AccessRawData(RawData);
					if (RawData.Num() > 0 && RawData[0])
					{
						ActionObject = static_cast<TObjectPtr<UObject>*>(RawData[0])->Get();
					}
				}
			}

			TSharedPtr<FPrimaryOption> Option = MakeShared<FPrimaryOption>();
			Option->Index = static_cast<int32>(ActionIndex);
			if (const UDIVEDeviceAction* Action = Cast<UDIVEDeviceAction>(ActionObject))
			{
				Option->Label = FText::Format(
					LOCTEXT("PrimaryNamed", "[{0}] {1}"),
					FText::AsNumber(Option->Index),
					Action->GetResolvedDisplayName());
			}
			else
			{
				Option->Label = FText::Format(
					LOCTEXT("PrimaryNull", "[{0}] (null)"),
					FText::AsNumber(Option->Index));
			}
			PrimaryOptions.Add(Option);
		}
	}

	if (TSharedPtr<SComboBox<TSharedPtr<FPrimaryOption>>> Combo = PrimaryComboWeak.Pin())
	{
		Combo->RefreshOptions();

		int32 Current = INDEX_NONE;
		if (PrimaryIndexHandle.IsValid())
		{
			PrimaryIndexHandle->GetValue(Current);
		}
		Combo->SetSelectedItem(FindOptionByIndex(Current));
	}
}

FText FDIVEActionBindingCustomization::GetPrimarySelectionLabel() const
{
	int32 Current = INDEX_NONE;
	if (PrimaryIndexHandle.IsValid())
	{
		PrimaryIndexHandle->GetValue(Current);
	}

	if (const TSharedPtr<FPrimaryOption> Option = FindOptionByIndex(Current))
	{
		return Option->Label;
	}

	return FText::Format(
		LOCTEXT("PrimaryOutOfRange", "[{0}] (out of range)"),
		FText::AsNumber(Current));
}

TSharedRef<SWidget> FDIVEActionBindingCustomization::MakePrimaryOptionWidget(
	TSharedPtr<FPrimaryOption> Option) const
{
	return SNew(STextBlock)
		.Text(Option.IsValid() ? Option->Label : LOCTEXT("PrimaryInvalid", "(invalid)"));
}

void FDIVEActionBindingCustomization::OnPrimaryOptionSelected(
	TSharedPtr<FPrimaryOption> Option,
	ESelectInfo::Type SelectInfo)
{
	(void)SelectInfo;
	if (!Option.IsValid() || !PrimaryIndexHandle.IsValid())
	{
		return;
	}

	PrimaryIndexHandle->SetValue(Option->Index);
}

TSharedPtr<FDIVEActionBindingCustomization::FPrimaryOption>
FDIVEActionBindingCustomization::FindOptionByIndex(const int32 Index) const
{
	for (const TSharedPtr<FPrimaryOption>& Option : PrimaryOptions)
	{
		if (Option.IsValid() && Option->Index == Index)
		{
			return Option;
		}
	}
	return nullptr;
}

#undef LOCTEXT_NAMESPACE
