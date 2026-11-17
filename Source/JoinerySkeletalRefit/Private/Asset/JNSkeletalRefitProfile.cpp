#include "Asset/JNSkeletalRefitProfile.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JNSkeletalRefitProfile)

const FJNBoneRefitEntry* UJNSkeletalRefitProfile::FindEntry(const FName SourceBone) const
{
	for (const FJNBoneRefitEntry& Entry : Bones)
	{
		if (Entry.SourceBone == SourceBone) return &Entry;
	}
	return nullptr;
}
