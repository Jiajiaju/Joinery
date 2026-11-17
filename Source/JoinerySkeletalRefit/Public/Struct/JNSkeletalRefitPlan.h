#pragma once

#include "CoreMinimal.h"
#include "ReferenceSkeleton.h"

/** 一次转换的骨骼方案：转换后网格的骨骼层级，以及来源骨骼到新骨骼的去向。 */
struct FJNSkeletalRefitPlan
{
	/** 转换后网格的骨骼层级与静止姿态。 */
	FReferenceSkeleton RefSkeleton;

	/** 来源骨骼下标到新骨骼下标，权重按它转移；每根来源骨骼都有去向。 */
	TArray<int32> SourceToNewIndex;

	/** 仍然保留的来源骨骼（Map 与 Append）到新骨骼名，用来改写网格上插槽所挂的骨骼。 */
	TMap<FName, FName> SourceToNewName;
};
