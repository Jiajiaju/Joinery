#include "Customization/JNBoneRefitEntryCustomization.h"

#include "Common/Constant/JNSkeletalRefitConstants.h"
#include "Common/Constant/JNCoreConstants.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "JNCoreStyle.h"
#include "PropertyHandle.h"
#include "Struct/JNBoneRefitEntry.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"

TSharedRef<IPropertyTypeCustomization> FJNBoneRefitEntryCustomization::MakeInstance()
{
	return MakeShared<FJNBoneRefitEntryCustomization>();
}

void FJNBoneRefitEntryCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	ActionHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJNBoneRefitEntry, Action));
	const TSharedPtr<IPropertyHandle> SourceBoneHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJNBoneRefitEntry, SourceBone));
	const TSharedRef<SWidget> SourceBoneWidget = SourceBoneHandle.IsValid() ? SourceBoneHandle->CreatePropertyValueWidget() : SNullWidget::NullWidget;

	HeaderRow
	.NameContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.0f, 0.0f, JNSkeletalRefitConstants::EntryIconPadding, 0.0f)
		[
			SNew(SImage)
			.Image(this, &FJNBoneRefitEntryCustomization::GetActionBrush)
			.ColorAndOpacity(FSlateColor::UseForeground())
		]

		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			SourceBoneWidget
		]
	];
}

void FJNBoneRefitEntryCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// 来源骨骼名已经在行首，这里只列其余字段
	uint32 ChildCount = 0;
	PropertyHandle->GetNumChildren(ChildCount);
	for (uint32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
	{
		const TSharedPtr<IPropertyHandle> ChildHandle = PropertyHandle->GetChildHandle(ChildIndex);
		if (!ChildHandle.IsValid() || ChildHandle->GetProperty() == nullptr) continue;
		if (ChildHandle->GetProperty()->GetFName() == GET_MEMBER_NAME_CHECKED(FJNBoneRefitEntry, SourceBone)) continue;

		ChildBuilder.AddProperty(ChildHandle.ToSharedRef());
	}
}

const FSlateBrush* FJNBoneRefitEntryCustomization::GetActionBrush() const
{
	uint8 ActionValue = 0;
	if (!ActionHandle.IsValid() || ActionHandle->GetValue(ActionValue) != FPropertyAccess::Success) return nullptr;

	switch (static_cast<EJNBoneRefitAction>(ActionValue))
	{
		case EJNBoneRefitAction::Map:
		{
			return FJNCoreStyle::GetBrush(JNCoreConstants::MapIconName);
		}
		case EJNBoneRefitAction::Append:
		{
			return FJNCoreStyle::GetBrush(JNCoreConstants::AppendIconName);
		}
		case EJNBoneRefitAction::Merge:
		{
			return FJNCoreStyle::GetBrush(JNCoreConstants::MergeIconName);
		}
		case EJNBoneRefitAction::Discard:
		{
			return FJNCoreStyle::GetBrush(JNCoreConstants::DiscardIconName);
		}
		default:
		{
			return nullptr;
		}
	}
}
