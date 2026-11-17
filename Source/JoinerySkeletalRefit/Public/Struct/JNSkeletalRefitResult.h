#pragma once

#include "CoreMinimal.h"

/** 一次转换的结果：是否成功、失败原因，以及需要人工留意的事项。 */
struct FJNSkeletalRefitResult
{
	bool bSucceeded = false;

	/** 失败原因，成功时为空。 */
	FText Error;

	/** 不影响转换完成、但需要人工留意或后续处理的事项。 */
	TArray<FText> Warnings;
};
