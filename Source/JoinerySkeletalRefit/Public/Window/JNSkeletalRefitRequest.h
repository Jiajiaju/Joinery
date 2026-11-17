#pragma once

#include "CoreMinimal.h"
#include "Struct/JNSkeletalRefitSettings.h"
#include "UObject/Object.h"
#include "JNSkeletalRefitRequest.generated.h"

class UAnimationAsset;
class USkeletalMesh;

/** 转换窗口里填写的一次转换请求：转到哪套骨架、转换哪些网格、每根骨骼怎么处理、结果放哪。只在窗口存续期间存在，不存盘。 */
UCLASS(Transient)
class UJNSkeletalRefitRequest : public UObject
{
	GENERATED_BODY()

public:
	/** 目标骨架与骨骼处理表。 */
	UPROPERTY(EditAnywhere, Category = "Skeletal Refit", meta = (ShowOnlyInnerProperties))
	FJNSkeletalRefitSettings Settings;

	/**
	 * 要转换的网格，导出时合成一个。只放身体就只导出身体，头之后还可以单独换；
	 * 把头也放进来就并进同一个网格，之后不能单独换。第一个是底，新网格的名字、布料与物理资产都跟它。
	 */
	UPROPERTY(EditAnywhere, Category = "Skeletal Refit")
	TArray<TObjectPtr<USkeletalMesh>> Meshes;

	/** 新网格所在的内容目录，留空时与来源网格同目录。 */
	UPROPERTY(EditAnywhere, Category = "Output", meta = (ContentDir))
	FDirectoryPath OutputFolder;

	/** 新网格名在来源网格名后追加的后缀。 */
	UPROPERTY(EditAnywhere, Category = "Output")
	FString OutputSuffix;

	/** 预览时播放的动画，要用目标骨架上的动画；留空时预览保持静止姿态。 */
	UPROPERTY(EditAnywhere, Category = "Preview")
	TObjectPtr<UAnimationAsset> PreviewAnimation = nullptr;

	/** 预览视口里是否叠画骨骼连线。 */
	UPROPERTY(EditAnywhere, Category = "Preview")
	bool bShowBones = true;
};
