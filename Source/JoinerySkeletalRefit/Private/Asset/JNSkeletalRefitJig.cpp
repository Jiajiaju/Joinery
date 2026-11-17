#include "Asset/JNSkeletalRefitJig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JNSkeletalRefitJig)

const FJNBoneRefitEntry* UJNSkeletalRefitJig::FindEntry(const FName SourceBone) const
{
	for (const FJNBoneRefitEntry& Entry : Bones)
	{
		if (Entry.SourceBone == SourceBone) return &Entry;
	}
	return nullptr;
}
