#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Struct/JNBoneRefitEntry.h"
#include "JNSkeletalRefitFunctionLibrary.generated.h"

class UPhysicsAsset;
class USkeletalMesh;
class USkeletalMeshSocket;
class USkeleton;
struct FJNSkeletalRefitMergePart;
struct FJNSkeletalRefitPlan;
struct FJNSkeletalRefitSettings;
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
	 * 把一组来源网格转换到目标骨架并导出成一个新网格，来源网格不动。新资产与目标骨架只标脏，不保存。
	 * 只有一个来源网格时就是转换它；有多个时以第一个为底，其余的（比如头）并进去，导出后不能再单独更换。
	 * @param OutputFolder 新网格所在的内容目录，留空时与第一个来源网格同目录。
	 * @param OutputSuffix 新网格名在第一个来源网格名后追加的后缀。
	 * @return 导出的网格，失败时为空，原因在 OutResult。
	 */
	static USkeletalMesh* RefitMeshes(const FJNSkeletalRefitSettings& Settings, const TArray<USkeletalMesh*>& SourceMeshes, const FString& OutputFolder, const FString& OutputSuffix, FJNSkeletalRefitResult& OutResult);

	/** 按骨骼处理表算出一个来源网格转换后的骨骼层级与权重去向，不改任何资产。 */
	static bool BuildPlan(const FJNSkeletalRefitSettings& Settings, const USkeletalMesh* SourceMesh, FJNSkeletalRefitPlan& OutPlan, FJNSkeletalRefitResult& OutResult);

	/** 算出一组来源网格合并导出的方案：以第一个网格的方案为底，其余网格多出来的骨骼补进去，各自的权重去向换成合并后的下标。 */
	static bool BuildMergedPlan(const FJNSkeletalRefitSettings& Settings, const TArray<USkeletalMesh*>& SourceMeshes, FJNSkeletalRefitPlan& OutPlan, FJNSkeletalRefitResult& OutResult);

	/**
	 * 把骨骼方案就地应用到一个网格：换骨骼层级、重算绑定、转移权重，并入方案里的其他来源网格，
	 * 然后把网格指到 Skeleton 并把骨骼并进它。布料、物理资产、插槽一并处理。
	 * 正式转换与预览共用：正式转换传网格副本与目标骨架，预览传临时网格与临时骨架。
	 */
	static bool ApplyPlan(const FJNSkeletalRefitPlan& Plan, USkeletalMesh* Mesh, USkeleton* Skeleton, FJNSkeletalRefitResult& OutResult);

	/** 建一份预览用的临时骨架：目标骨架的副本，与目标骨架兼容，不存盘。预览里追加的骨骼写进它，目标骨架不动。 */
	static USkeleton* MakePreviewSkeleton(const FJNSkeletalRefitSettings& Settings);

	/** 建一个预览用的临时网格：按与导出相同的方案转换并合并来源网格，结果落在预览骨架上，不存盘。失败时为空，原因在 OutResult。 */
	static USkeletalMesh* MakePreviewMesh(const FJNSkeletalRefitSettings& Settings, const TArray<USkeletalMesh*>& SourceMeshes, USkeleton* PreviewSkeleton, FJNSkeletalRefitResult& OutResult);

private:
	/** 收集网格上的变形目标名。 */
	static void CollectMorphTargetNames(const USkeletalMesh* Mesh, TSet<FName>& OutNames);

	/**
	 * 把一个插槽换算到新骨骼层级：所挂的骨骼换成它最终落到的骨骼，相对位置按"在模型空间里不动"重算。
	 * OldRefSkeleton 是插槽原来所在网格的骨骼层级。
	 */
	static void ConvertSocket(USkeletalMeshSocket* Socket, const FReferenceSkeleton& OldRefSkeleton, const TMap<FName, FName>& SourceToFinalName, const FReferenceSkeleton& NewRefSkeleton, const TArray<FTransform>& NewGlobals, FJNSkeletalRefitResult& OutResult);

	/** 把一批插槽复制成网格自己的插槽并换算到新骨骼层级；网格上已有同名插槽的不覆盖。 */
	static void CopySockets(const TArray<USkeletalMeshSocket*>& Sockets, const FReferenceSkeleton& OldRefSkeleton, const TMap<FName, FName>& SourceToFinalName, const FReferenceSkeleton& NewRefSkeleton, const TArray<FTransform>& NewGlobals, USkeletalMesh* Mesh, FJNSkeletalRefitResult& OutResult);

	/**
	 * 把一个并入网格某一级 LOD 的几何、材质分段、蒙皮权重与变形目标追加进底的网格描述。
	 * SlotNames 是并入网格的材质槽名到合并后槽名的对应。
	 */
	static void AppendPartToMeshDescription(const FJNSkeletalRefitMergePart& Part, const TMap<FName, FName>& SlotNames, const int32 LODIndex, FMeshDescription& MeshDescription, FJNSkeletalRefitResult& OutResult);

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
