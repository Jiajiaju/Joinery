#include "Asset/JNSkeletalRefitJigFactory.h"

#include "Asset/JNSkeletalRefitJig.h"
#include "Common/Constant/JNSkeletalRefitConstants.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JNSkeletalRefitJigFactory)

#define LOCTEXT_NAMESPACE "JNSkeletalRefitJigFactory"

UJNSkeletalRefitJigFactory::UJNSkeletalRefitJigFactory()
{
	SupportedClass = UJNSkeletalRefitJig::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UJNSkeletalRefitJigFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UJNSkeletalRefitJig>(InParent, InClass, InName, Flags);
}

FText UJNSkeletalRefitJigFactory::GetDisplayName() const
{
	return LOCTEXT("DisplayName", "Skeletal Refit Jig");
}

FString UJNSkeletalRefitJigFactory::GetDefaultNewAssetName() const
{
	return JNSkeletalRefitConstants::NewJigAssetName;
}

#undef LOCTEXT_NAMESPACE
