#include "Struct/JNSkeletalRefitSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JNSkeletalRefitSettings)

const FJNBoneRefitEntry* FJNSkeletalRefitSettings::FindEntry(const FName SourceBone) const
{
	for (const FJNBoneRefitEntry& Entry : Bones)
	{
		if (Entry.SourceBone == SourceBone) return &Entry;
	}
	return nullptr;
}
