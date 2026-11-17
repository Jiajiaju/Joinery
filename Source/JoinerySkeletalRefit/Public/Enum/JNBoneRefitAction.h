#pragma once

#include "CoreMinimal.h"
#include "JNBoneRefitAction.generated.h"

/** 来源骨架上的一根骨骼在转换到目标骨架时怎么处理。 */
UENUM()
enum class EJNBoneRefitAction : uint8
{
	// 对应到目标骨架上的一根骨骼：权重转给它，位置沿用网格原来的关节位置
	Map,

	// 目标骨架上没有，追加为新骨骼：保持原来的位置与朝向，挂到指定的父骨骼下
	Append,

	// 不保留这根骨骼，权重并入另一根骨骼
	Merge,

	// 不保留这根骨骼；万一带权重，权重落到最近的保留祖先上
	Discard
};
