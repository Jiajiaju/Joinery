#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Struct/JNBoneRefitEntry.h"
#include "JNSkeletalRefitFunctionLibrary.generated.h"

struct FJNSkeletalRefitSettings;
class UPhysicsAsset;
class USkeletalMesh;
class USkeleton;
struct FJNSkeletalRefitPlan;
struct FJNSkeletalRefitResult;
struct FMeshDescription;
struct FReferenceSkeleton;

/**
 * Skeletal Refit 的转换逻辑：按骨骼处理表把蒙在来源骨架上的网格重新绑定到目标骨架。
 * 顶点与变形目标不动，只重建骨骼层级、重算绑定并转移权重；目标骨架上没有的骨骼追加进目标骨架。
 */
UCLASS()
class JOINERYSKELETALREFIT_API UJNSkeletalRefitFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 按自动规则给骨骼处理表补上网格里还没有列出的骨骼，已有的行不动。
	 * @return 新增的行数。
	 */
	static int32 FillBones(FJNSkeletalRefitSettings& Settings, const USkeletalMesh* Mesh);

	/**
	 * 复制来源网格并把副本转换到骨骼处理表的目标骨架，来源网格不动。新资产与目标骨架只标脏，不保存。
	 * @param OutputFolder 新网格所在的内容目录，留空时与来源网格同目录。
	 * @param OutputSuffix 新网格名在来源网格名后追加的后缀。
	 * @return 转换后的网格，失败时为空，原因在 OutResult。
	 */
	static USkeletalMesh* RefitMesh(const FJNSkeletalRefitSettings& Settings, USkeletalMesh* SourceMesh, const FString& OutputFolder, const FString& OutputSuffix, FJNSkeletalRefitResult& OutResult);

	/** 按骨骼处理表算出来源网格转换后的骨骼层级与权重去向，不改任何资产。 */
	static bool BuildPlan(const FJNSkeletalRefitSettings& Settings, const USkeletalMesh* SourceMesh, FJNSkeletalRefitPlan& OutPlan, FJNSkeletalRefitResult& OutResult);

	/**
	 * 把骨骼方案就地应用到一个网格：换骨骼层级、重算绑定、转移权重，然后把网格指到 Skeleton 并把骨骼并进它。
	 * 正式转换与预览共用：正式转换传网格副本与目标骨架，预览传临时网格与临时骨架。
	 */
	static bool ApplyPlan(const FJNSkeletalRefitPlan& Plan, USkeletalMesh* Mesh, USkeleton* Skeleton, FJNSkeletalRefitResult& OutResult);

	/** 建一份预览用的临时骨架：目标骨架的副本，与目标骨架兼容，不存盘。预览里追加的骨骼写进它，目标骨架不动。 */
	static USkeleton* MakePreviewSkeleton(const FJNSkeletalRefitSettings& Settings);

	/** 建一个预览用的临时网格：来源网格的副本按骨骼处理表转换到预览骨架，不存盘。失败时为空，原因在 OutResult。 */
	static USkeletalMesh* MakePreviewMesh(const FJNSkeletalRefitSettings& Settings, USkeletalMesh* SourceMesh, USkeleton* PreviewSkeleton, FJNSkeletalRefitResult& OutResult);

private:
	/** 收集网格里真正带权重的骨骼下标。 */
	static void CollectWeightedBones(const USkeletalMesh* Mesh, TSet<int32>& OutWeightedBones);

	/** 没有列在骨骼处理表里的骨骼的处理方式：同名则对应，否则带权重的追加、不带权重的丢弃。 */
	static FJNBoneRefitEntry MakeAutoEntry(const FReferenceSkeleton& Target, const FName SourceBone, const bool bWeighted);

	/** 由各骨骼的局部静止姿态算出模型空间的静止姿态，不带缩放。 */
	static void ComputeGlobalPose(const FReferenceSkeleton& RefSkeleton, TArray<FTransform>& OutGlobals);

	/** 从一根目标骨骼往下找第一根有来源骨骼对应的骨骼（含自身），没有时返回 INDEX_NONE。 */
	static int32 FindMappedBelow(const FReferenceSkeleton& Target, const TArray<int32>& MappedSourceOfTarget, const int32 TargetIndex);

	/** 从一根来源骨骼往上找最近的、已经有去向的祖先，返回它的新骨骼下标，没有时返回 INDEX_NONE。 */
	static int32 FindResolvedAncestor(const FReferenceSkeleton& Source, const TArray<int32>& SourceToNewIndex, const int32 SourceIndex);

	/**
	 * 算一根被对应到的目标骨骼的对齐旋转：把目标骨架上"本骨骼指向子骨骼"的方向转到网格上对应的方向。
	 * 先继承父骨骼的对齐，再用最长的子方向定摆动，有第二个不共线的子方向时再定扭转；没有可用子方向时沿用父骨骼的对齐。
	 */
	static FQuat ComputeAlignment(const FReferenceSkeleton& Target, const TArray<FTransform>& TargetGlobals, const TArray<FTransform>& SourceGlobals, const TArray<int32>& MappedSourceOfTarget, const int32 TargetIndex, const FQuat& ParentAlignment);

	/** 由来源骨骼层级与带权重的骨骼，按骨骼处理表算出转换后的骨骼层级与权重去向。 */
	static bool BuildBonePlan(const FJNSkeletalRefitSettings& Settings, const FReferenceSkeleton& Source, const TSet<int32>& WeightedBones, FJNSkeletalRefitPlan& OutPlan, FJNSkeletalRefitResult& OutResult);

	/** 物理资产用到的骨骼在这套骨骼层级里是否都在。 */
	static bool IsPhysicsAssetCompatible(const UPhysicsAsset* PhysicsAsset, const FReferenceSkeleton& RefSkeleton);

	/** 找出目标网格是来源网格的毛发 Binding，各复制一份指向新网格并重建；做了什么记在 OutResult 的提示里。 */
	static void RebuildGroomBindings(const USkeletalMesh* SourceMesh, USkeletalMesh* NewMesh, const FString& OutputSuffix, FJNSkeletalRefitResult& OutResult);

	/** 把骨骼方案写进一份网格描述：替换骨骼表，并按去向转移各权重配置里的权重。 */
	static void ApplyPlanToMeshDescription(const FJNSkeletalRefitPlan& Plan, FMeshDescription& MeshDescription);
};
