#include "Asset/AssetDefinition_JNSkeletalRefitJig.h"

#include "Asset/JNSkeletalRefitJig.h"
#include "Common/Constant/JNSkeletalRefitConstants.h"
#include "Common/Constant/JNWorkbenchConstants.h"
#include "JNWorkbenchStyle.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AssetDefinition_JNSkeletalRefitJig)

#define LOCTEXT_NAMESPACE "AssetDefinition_JNSkeletalRefitJig"

FText UAssetDefinition_JNSkeletalRefitJig::GetAssetDisplayName() const
{
	return LOCTEXT("DisplayName", "Skeletal Refit Jig");
}

FLinearColor UAssetDefinition_JNSkeletalRefitJig::GetAssetColor() const
{
	return JNSkeletalRefitConstants::JigAssetColor;
}

TSoftClassPtr<UObject> UAssetDefinition_JNSkeletalRefitJig::GetAssetClass() const
{
	return UJNSkeletalRefitJig::StaticClass();
}

TConstArrayView<FAssetCategoryPath> UAssetDefinition_JNSkeletalRefitJig::GetAssetCategories() const
{
	static const FAssetCategoryPath Categories[] = { FAssetCategoryPath(LOCTEXT("JoineryCategory", "Joinery")) };
	return Categories;
}

const FSlateBrush* UAssetDefinition_JNSkeletalRefitJig::GetThumbnailBrush(const FAssetData& InAssetData, const FName InClassName) const
{
	return FJNWorkbenchStyle::GetBrush(JNWorkbenchConstants::JigThumbnailName);
}

const FSlateBrush* UAssetDefinition_JNSkeletalRefitJig::GetIconBrush(const FAssetData& InAssetData, const FName InClassName) const
{
	return FJNWorkbenchStyle::GetBrush(JNWorkbenchConstants::JigIconName);
}

#undef LOCTEXT_NAMESPACE
