#pragma once

#include "CoreMinimal.h"
#include "AssetDefinitionDefault.h"
#include "AssetDefinition_JNSkeletalRefitJig.generated.h"

/** Skeletal Refit 对应表在内容浏览器里的类型定义：显示为 Skeletal Refit Jig，归在 Joinery 分类下。 */
UCLASS()
class UAssetDefinition_JNSkeletalRefitJig : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:
	//~ Begin UAssetDefinition Interface.
	virtual FText GetAssetDisplayName() const override;
	virtual FLinearColor GetAssetColor() const override;
	virtual TSoftClassPtr<UObject> GetAssetClass() const override;
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override;
	virtual const FSlateBrush* GetThumbnailBrush(const FAssetData& InAssetData, const FName InClassName) const override;
	virtual const FSlateBrush* GetIconBrush(const FAssetData& InAssetData, const FName InClassName) const override;
	//~ End UAssetDefinition Interface.
};
