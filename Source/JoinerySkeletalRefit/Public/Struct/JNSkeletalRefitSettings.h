#pragma once

#include "CoreMinimal.h"
#include "Struct/JNBoneRefitEntry.h"
#include "JNSkeletalRefitSettings.generated.h"

class USkeleton;

/**
 * 一次 Skeletal Refit 的转换设置：转到哪套目标骨架，以及来源骨架的每根骨骼怎么处理。
 * 没有列在骨骼处理表里的来源骨骼按自动规则处理：目标骨架上有同名骨骼则对应过去，否则带权重的追加、不带权重的丢弃。
 */
USTRUCT()
struct JOINERYSKELETALREFIT_API FJNSkeletalRefitSettings
{
	GENERATED_BODY()

	/** 目标骨架：转换后的网格使用它，追加的骨骼写进它。 */
	UPROPERTY(EditAnywhere, Category = "Skeletal Refit")
	TObjectPtr<USkeleton> TargetSkeleton = nullptr;

	/** 追加骨骼的名字前缀，避免不同角色的私有骨骼在目标骨架上重名。单行 NewName 另有指定时不加。 */
	UPROPERTY(EditAnywhere, Category = "Bones")
	FString AppendPrefix;

	/**
	 * 不带进转换后网格的目标骨骼，连同其子骨骼一起排除；被对应到的骨骼及其祖先不受影响。
	 * 用来排除目标骨架上其他角色追加的私有骨骼。
	 */
	UPROPERTY(EditAnywhere, Category = "Bones")
	TArray<FName> ExcludedTargetBones;

	/** 来源骨骼的处理方式，一根骨骼一行。 */
	UPROPERTY(EditAnywhere, Category = "Bones", meta = (TitleProperty = "SourceBone"))
	TArray<FJNBoneRefitEntry> Bones;

	/** 取某根来源骨骼的行，没有时返回空。 */
	const FJNBoneRefitEntry* FindEntry(const FName SourceBone) const;
};
