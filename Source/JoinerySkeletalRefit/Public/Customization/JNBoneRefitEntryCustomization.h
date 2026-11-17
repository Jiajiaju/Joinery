#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

struct FSlateBrush;

/** 骨骼处理表条目在 Details 面板里的显示：行首按处理方式显示图标，后面是来源骨骼名；展开后是其余字段。 */
class FJNBoneRefitEntryCustomization : public IPropertyTypeCustomization
{
private:
	TSharedPtr<IPropertyHandle> ActionHandle;

public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	//~ Begin IPropertyTypeCustomization Interface.
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	//~ End IPropertyTypeCustomization Interface.

private:
	/** 当前处理方式对应的图标。 */
	const FSlateBrush* GetActionBrush() const;
};
