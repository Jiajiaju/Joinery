#include "Asset/JNSkeletalRefitProfileFactory.h"

#include "Asset/JNSkeletalRefitProfile.h"
#include "Common/Constant/JNSkeletalRefitConstants.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JNSkeletalRefitProfileFactory)

#define LOCTEXT_NAMESPACE "JNSkeletalRefitProfileFactory"

UJNSkeletalRefitProfileFactory::UJNSkeletalRefitProfileFactory()
{
	SupportedClass = UJNSkeletalRefitProfile::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UJNSkeletalRefitProfileFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UJNSkeletalRefitProfile>(InParent, InClass, InName, Flags);
}

FText UJNSkeletalRefitProfileFactory::GetDisplayName() const
{
	return LOCTEXT("DisplayName", "Skeletal Refit Profile");
}

FString UJNSkeletalRefitProfileFactory::GetDefaultNewAssetName() const
{
	return JNSkeletalRefitConstants::NewProfileAssetName;
}

#undef LOCTEXT_NAMESPACE
