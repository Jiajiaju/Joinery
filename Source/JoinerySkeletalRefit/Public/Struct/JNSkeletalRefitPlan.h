#pragma once

#include "CoreMinimal.h"
#include "ReferenceSkeleton.h"

class USkeleton;

/** 一次转换的骨骼方案：转换后网格的骨骼层级，以及来源骨骼到新骨骼的去向。 */
struct FJNSkeletalRefitPlan
{
	/** 转换后网格的骨骼层级与静止姿态。 */
	FReferenceSkeleton RefSkeleton;

	/** 来源骨骼下标到新骨骼下标，权重按它转移；每根来源骨骼都有去向。 */
	TArray<int32> SourceToNewIndex;

	/**
	 * 每根来源骨骼最终落到的新骨骼名：保留的骨骼（Map 与 Append）是它自己的新名字，
	 * 不保留的骨骼（Merge 与 Discard）是它权重去向的骨骼。插槽、布料数据按它改写所引用的骨骼。
	 */
	TMap<FName, FName> SourceToFinalName;

	/** 转换设置里的目标骨架。预览时网格指向的是它的临时副本，判断资产是否属于目标骨架要用这个。 */
	TWeakObjectPtr<USkeleton> TargetSkeleton;

	/** 是否把来源骨架上的插槽复制成新网格自己的插槽。 */
	bool bCopySkeletonSockets = true;
};
