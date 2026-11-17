#pragma once

#include "CoreMinimal.h"
#include "Enum/JNBoneRefitAction.h"
#include "JNBoneRefitEntry.generated.h"

/** 骨骼处理表里的一行：来源骨架上的一根骨骼，以及它转换到目标骨架时的处理方式。 */
USTRUCT()
struct FJNBoneRefitEntry
{
	GENERATED_BODY()

	/** 来源骨架上的骨骼名。 */
	UPROPERTY(EditAnywhere, Category = "Bone")
	FName SourceBone = NAME_None;

	/** 这根骨骼怎么处理。 */
	UPROPERTY(EditAnywhere, Category = "Bone")
	EJNBoneRefitAction Action = EJNBoneRefitAction::Map;

	/**
	 * Map：目标骨架上对应的骨骼名。
	 * Append：新骨骼的父骨骼名（目标骨架上的骨骼，或另一根追加骨骼的新名字）；留空时取来源骨架上最近的保留祖先。
	 * Merge：权重并入的骨骼名；留空时取来源骨架上最近的保留祖先。
	 * Discard：不使用。
	 */
	UPROPERTY(EditAnywhere, Category = "Bone", meta = (EditCondition = "Action != EJNBoneRefitAction::Discard"))
	FName TargetBone = NAME_None;

	/** Append 时新骨骼的名字；留空时用骨骼处理表的追加前缀加来源骨骼名。 */
	UPROPERTY(EditAnywhere, Category = "Bone", meta = (EditCondition = "Action == EJNBoneRefitAction::Append"))
	FName NewName = NAME_None;
};
