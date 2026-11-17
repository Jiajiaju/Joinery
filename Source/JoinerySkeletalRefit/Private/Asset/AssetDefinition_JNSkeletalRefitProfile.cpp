#include "Asset/AssetDefinition_JNSkeletalRefitProfile.h"

#include "Asset/JNSkeletalRefitProfile.h"
#include "Common/Constant/JNSkeletalRefitConstants.h"
#include "Common/Constant/JNCoreConstants.h"
#include "JNCoreStyle.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AssetDefinition_JNSkeletalRefitProfile)

#define LOCTEXT_NAMESPACE "AssetDefinition_JNSkeletalRefitProfile"

FText UAssetDefinition_JNSkeletalRefitProfile::GetAssetDisplayName() const
{
	return LOCTEXT("DisplayName", "Skeletal Refit Profile");
}

FLinearColor UAssetDefinition_JNSkeletalRefitProfile::GetAssetColor() const
{
	return JNSkeletalRefitConstants::ProfileAssetColor;
}

TSoftClassPtr<UObject> UAssetDefinition_JNSkeletalRefitProfile::GetAssetClass() const
{
	return UJNSkeletalRefitProfile::StaticClass();
}

TConstArrayView<FAssetCategoryPath> UAssetDefinition_JNSkeletalRefitProfile::GetAssetCategories() const
{
	static const FAssetCategoryPath Categories[] = { FAssetCategoryPath(LOCTEXT("JoineryCategory", "Joinery")) };
	return Categories;
}

const FSlateBrush* UAssetDefinition_JNSkeletalRefitProfile::GetThumbnailBrush(const FAssetData& InAssetData, const FName InClassName) const
{
	return FJNCoreStyle::GetBrush(JNCoreConstants::ProfileThumbnailName);
}

const FSlateBrush* UAssetDefinition_JNSkeletalRefitProfile::GetIconBrush(const FAssetData& InAssetData, const FName InClassName) const
{
	return FJNCoreStyle::GetBrush(JNCoreConstants::ProfileIconName);
}

#undef LOCTEXT_NAMESPACE
