#pragma once

#include "CoreMinimal.h"

class USkeletalMesh;

/** 合并导出时并进第一个网格的另一个来源网格，以及它的骨骼在合并后的去向。 */
struct FJNSkeletalRefitMergePart
{
	/** 被并入的来源网格。 */
	TWeakObjectPtr<const USkeletalMesh> Mesh;

	/** 这个网格的来源骨骼下标到合并后骨骼层级的下标，蒙皮权重按它转移。 */
	TArray<int32> SourceToNewIndex;

	/** 这个网格的每根来源骨骼最终落到的新骨骼名，插槽按它改写所挂的骨骼。 */
	TMap<FName, FName> SourceToFinalName;
};
