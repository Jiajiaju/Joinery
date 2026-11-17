#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JNSkeletalRefitRequest.generated.h"

class UJNSkeletalRefitJig;
class USkeletalMesh;

/** 转换窗口里填写的一次转换请求：用哪份对应表、转换哪些网格、结果放哪。只在窗口存续期间存在，不存盘。 */
UCLASS(Transient)
class UJNSkeletalRefitRequest : public UObject
{
	GENERATED_BODY()

public:
	/** 骨骼对应表，目标骨架由它指定。 */
	UPROPERTY(EditAnywhere, Category = "Skeletal Refit")
	TObjectPtr<UJNSkeletalRefitJig> Jig = nullptr;

	/** 要转换的网格。身体与头这类共用骨骼的网格放在一起，用同一份对应表转换，接缝处的骨骼结果一致。 */
	UPROPERTY(EditAnywhere, Category = "Skeletal Refit")
	TArray<TObjectPtr<USkeletalMesh>> Meshes;

	/** 新网格所在的内容目录，留空时与来源网格同目录。 */
	UPROPERTY(EditAnywhere, Category = "Output", meta = (ContentDir))
	FDirectoryPath OutputFolder;

	/** 新网格名在来源网格名后追加的后缀。 */
	UPROPERTY(EditAnywhere, Category = "Output")
	FString OutputSuffix;
};
