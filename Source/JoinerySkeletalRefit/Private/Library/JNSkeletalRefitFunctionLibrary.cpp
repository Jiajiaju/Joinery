#include "Library/JNSkeletalRefitFunctionLibrary.h"

#include "Animation/Skeleton.h"
#include "Asset/JNSkeletalRefitJig.h"
#include "AssetToolsModule.h"
#include "BoneWeights.h"
#include "Common/Constant/JNSkeletalRefitConstants.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "IAssetTools.h"
#include "JNSkeletalRefitLog.h"
#include "MeshDescription.h"
#include "Misc/PackageName.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "ReferenceSkeleton.h"
#include "RenderingThread.h"
#include "SkeletalMeshAttributes.h"
#include "Struct/JNSkeletalRefitPlan.h"
#include "Struct/JNSkeletalRefitResult.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JNSkeletalRefitFunctionLibrary)

#define LOCTEXT_NAMESPACE "JNSkeletalRefit"

int32 UJNSkeletalRefitFunctionLibrary::AutoFillJig(UJNSkeletalRefitJig* Jig, const USkeletalMesh* Mesh)
{
	if (!IsValid(Jig) || !IsValid(Jig->TargetSkeleton) || !IsValid(Mesh)) return 0;

	TSet<int32> WeightedBones;
	CollectWeightedBones(Mesh, WeightedBones);

	const FReferenceSkeleton& Source = Mesh->GetRefSkeleton();
	const FReferenceSkeleton& Target = Jig->TargetSkeleton->GetReferenceSkeleton();

	Jig->Modify();
	int32 AddedCount = 0;
	for (int32 SourceIndex = 0; SourceIndex < Source.GetRawBoneNum(); ++SourceIndex)
	{
		const FName SourceBone = Source.GetBoneName(SourceIndex);
		if (Jig->FindEntry(SourceBone) != nullptr) continue;

		Jig->Bones.Add(MakeAutoEntry(Target, SourceBone, WeightedBones.Contains(SourceIndex)));
		++AddedCount;
	}

	if (AddedCount > 0) Jig->MarkPackageDirty();
	return AddedCount;
}

USkeletalMesh* UJNSkeletalRefitFunctionLibrary::RefitMesh(const UJNSkeletalRefitJig* Jig, USkeletalMesh* SourceMesh, const FString& OutputFolder, const FString& OutputSuffix, FJNSkeletalRefitResult& OutResult)
{
	OutResult = FJNSkeletalRefitResult();
	if (!IsValid(Jig) || !IsValid(Jig->TargetSkeleton))
	{
		OutResult.Error = LOCTEXT("NoTargetSkeleton", "The jig has no target skeleton.");
		return nullptr;
	}
	if (!IsValid(SourceMesh))
	{
		OutResult.Error = LOCTEXT("NoSourceMesh", "No source mesh.");
		return nullptr;
	}
	if (SourceMesh->GetSkeleton() == Jig->TargetSkeleton)
	{
		OutResult.Error = FText::Format(LOCTEXT("AlreadyOnTarget", "'{0}' already uses the target skeleton."), FText::FromString(SourceMesh->GetName()));
		return nullptr;
	}

	// 先算方案，方案不成立就不产生任何资产
	TSet<int32> WeightedBones;
	CollectWeightedBones(SourceMesh, WeightedBones);

	FJNSkeletalRefitPlan Plan;
	if (!BuildPlan(Jig, SourceMesh->GetRefSkeleton(), WeightedBones, Plan, OutResult)) return nullptr;

	const FString PackagePath = OutputFolder.IsEmpty() ? FPackageName::GetLongPackagePath(SourceMesh->GetOutermost()->GetName()) : OutputFolder;
	const FString AssetName = SourceMesh->GetName() + OutputSuffix;
	if (FPackageName::DoesPackageExist(PackagePath / AssetName))
	{
		OutResult.Error = FText::Format(LOCTEXT("OutputExists", "'{0}' already exists. Delete it or use another suffix."), FText::FromString(PackagePath / AssetName));
		return nullptr;
	}

	IAssetTools& AssetTools = FAssetToolsModule::GetModule().Get();
	USkeletalMesh* NewMesh = Cast<USkeletalMesh>(AssetTools.DuplicateAsset(AssetName, PackagePath, SourceMesh));
	if (!IsValid(NewMesh))
	{
		OutResult.Error = FText::Format(LOCTEXT("DuplicateFailed", "Failed to duplicate '{0}'."), FText::FromString(SourceMesh->GetName()));
		return nullptr;
	}

	const int32 SourceMorphCount = SourceMesh->GetMorphTargets().Num();

	// 换骨骼层级并重算绑定：顶点不动，只有骨骼的静止姿态变了
	FlushRenderingCommands();
	NewMesh->Modify();
	NewMesh->SetRefSkeleton(Plan.RefSkeleton);
	NewMesh->GetRefBasesInvMatrix().Reset();
	NewMesh->CalculateInvRefMatrices();

	for (int32 LODIndex = 0; LODIndex < NewMesh->GetLODNum(); ++LODIndex)
	{
		if (!NewMesh->HasMeshDescription(LODIndex)) continue;

		FMeshDescription MeshDescription;
		if (!NewMesh->CloneMeshDescription(LODIndex, MeshDescription)) continue;

		ApplyPlanToMeshDescription(Plan, MeshDescription);
		NewMesh->ModifyMeshDescription(LODIndex);
		NewMesh->CreateMeshDescription(LODIndex, MoveTemp(MeshDescription));
		NewMesh->CommitMeshDescription(LODIndex);
	}

	// 网格自己的插槽跟着骨骼改名；挂在已不保留的骨骼上的插槽留给人工处理
	for (USkeletalMeshSocket* Socket : NewMesh->GetMeshOnlySocketList())
	{
		if (!IsValid(Socket)) continue;

		const FName* NewBoneName = Plan.SourceToNewName.Find(Socket->BoneName);
		if (NewBoneName != nullptr)
		{
			Socket->BoneName = *NewBoneName;
			continue;
		}

		OutResult.Warnings.Add(FText::Format(LOCTEXT("SocketBoneRemoved", "Socket '{0}' is attached to bone '{1}', which was not kept."), FText::FromName(Socket->SocketName), FText::FromName(Socket->BoneName)));
	}

	// 物理资产与后处理动画蓝图是按来源骨架做的，不能带过来
	if (IsValid(NewMesh->GetPhysicsAsset()))
	{
		NewMesh->SetPhysicsAsset(nullptr);
		OutResult.Warnings.Add(LOCTEXT("PhysicsAssetCleared", "The physics asset was cleared. Create one for the refitted mesh."));
	}
	if (NewMesh->GetPostProcessAnimBlueprint() != nullptr)
	{
		NewMesh->SetPostProcessAnimBlueprint(nullptr);
		OutResult.Warnings.Add(LOCTEXT("PostProcessCleared", "The post process anim blueprint was cleared. Assign one made for the target skeleton."));
	}

	USkeleton* TargetSkeleton = Jig->TargetSkeleton;
	TargetSkeleton->Modify();
	NewMesh->SetSkeleton(TargetSkeleton);
	const bool bMerged = TargetSkeleton->MergeAllBonesToBoneTree(NewMesh, false);
	NewMesh->PostEditChange();
	NewMesh->MarkPackageDirty();
	if (!bMerged)
	{
		OutResult.Error = FText::Format(LOCTEXT("MergeFailed", "Failed to merge the bones of '{0}' into the target skeleton. The new mesh asset is left for inspection."), FText::FromString(NewMesh->GetName()));
		return nullptr;
	}
	TargetSkeleton->MarkPackageDirty();

	if (NewMesh->GetMorphTargets().Num() != SourceMorphCount)
	{
		OutResult.Error = FText::Format(LOCTEXT("MorphMismatch", "Morph target count changed from {0} to {1}. The new mesh asset is left for inspection."), FText::AsNumber(SourceMorphCount), FText::AsNumber(NewMesh->GetMorphTargets().Num()));
		return nullptr;
	}

	UE_LOG(LogJNSkeletalRefit, Log, TEXT("Refitted '%s' to skeleton '%s' as '%s' with %d warning(s)."), *SourceMesh->GetPathName(), *TargetSkeleton->GetPathName(), *NewMesh->GetPathName(), OutResult.Warnings.Num());
	OutResult.bSucceeded = true;
	return NewMesh;
}

void UJNSkeletalRefitFunctionLibrary::CollectWeightedBones(const USkeletalMesh* Mesh, TSet<int32>& OutWeightedBones)
{
	OutWeightedBones.Reset();
	if (!IsValid(Mesh) || !Mesh->HasMeshDescription(JNSkeletalRefitConstants::SourceLODIndex)) return;

	const FMeshDescription* MeshDescription = Mesh->GetMeshDescription(JNSkeletalRefitConstants::SourceLODIndex);
	if (MeshDescription == nullptr) return;

	const FSkeletalMeshConstAttributes MeshAttributes(*MeshDescription);
	const FSkinWeightsVertexAttributesConstRef SkinWeights = MeshAttributes.GetVertexSkinWeights();
	if (!SkinWeights.IsValid()) return;

	for (const FVertexID VertexID : MeshDescription->Vertices().GetElementIDs())
	{
		const FVertexBoneWeightsConst BoneWeights = SkinWeights.Get(VertexID);
		for (int32 WeightIndex = 0; WeightIndex < BoneWeights.Num(); ++WeightIndex)
		{
			if (BoneWeights[WeightIndex].GetRawWeight() > 0) OutWeightedBones.Add(BoneWeights[WeightIndex].GetBoneIndex());
		}
	}
}

FJNBoneRefitEntry UJNSkeletalRefitFunctionLibrary::MakeAutoEntry(const FReferenceSkeleton& Target, const FName SourceBone, const bool bWeighted)
{
	FJNBoneRefitEntry Entry;
	Entry.SourceBone = SourceBone;
	if (Target.FindRawBoneIndex(SourceBone) != INDEX_NONE)
	{
		Entry.Action = EJNBoneRefitAction::Map;
		Entry.TargetBone = SourceBone;
		return Entry;
	}

	Entry.Action = bWeighted ? EJNBoneRefitAction::Append : EJNBoneRefitAction::Discard;
	return Entry;
}

void UJNSkeletalRefitFunctionLibrary::ComputeGlobalPose(const FReferenceSkeleton& RefSkeleton, TArray<FTransform>& OutGlobals)
{
	const TArray<FTransform>& Locals = RefSkeleton.GetRawRefBonePose();
	OutGlobals.SetNum(RefSkeleton.GetRawBoneNum());
	for (int32 BoneIndex = 0; BoneIndex < OutGlobals.Num(); ++BoneIndex)
	{
		const int32 ParentIndex = RefSkeleton.GetRawParentIndex(BoneIndex);
		const FTransform Global = ParentIndex != INDEX_NONE ? Locals[BoneIndex] * OutGlobals[ParentIndex] : Locals[BoneIndex];
		OutGlobals[BoneIndex] = FTransform(Global.GetRotation(), Global.GetLocation());
	}
}

int32 UJNSkeletalRefitFunctionLibrary::FindMappedBelow(const FReferenceSkeleton& Target, const TArray<int32>& MappedSourceOfTarget, const int32 TargetIndex)
{
	if (MappedSourceOfTarget[TargetIndex] != INDEX_NONE) return TargetIndex;

	// 子骨骼的下标一定比父骨骼大
	for (int32 ChildIndex = TargetIndex + 1; ChildIndex < Target.GetRawBoneNum(); ++ChildIndex)
	{
		if (Target.GetRawParentIndex(ChildIndex) != TargetIndex) continue;

		const int32 Found = FindMappedBelow(Target, MappedSourceOfTarget, ChildIndex);
		if (Found != INDEX_NONE) return Found;
	}
	return INDEX_NONE;
}

int32 UJNSkeletalRefitFunctionLibrary::FindResolvedAncestor(const FReferenceSkeleton& Source, const TArray<int32>& SourceToNewIndex, const int32 SourceIndex)
{
	int32 AncestorIndex = Source.GetRawParentIndex(SourceIndex);
	while (AncestorIndex != INDEX_NONE)
	{
		if (SourceToNewIndex[AncestorIndex] != INDEX_NONE) return SourceToNewIndex[AncestorIndex];

		AncestorIndex = Source.GetRawParentIndex(AncestorIndex);
	}
	return INDEX_NONE;
}

FQuat UJNSkeletalRefitFunctionLibrary::ComputeAlignment(const FReferenceSkeleton& Target, const TArray<FTransform>& TargetGlobals, const TArray<FTransform>& SourceGlobals, const TArray<int32>& MappedSourceOfTarget, const int32 TargetIndex, const FQuat& ParentAlignment)
{
	const FVector TargetOrigin = TargetGlobals[TargetIndex].GetLocation();
	const FVector SourceOrigin = SourceGlobals[MappedSourceOfTarget[TargetIndex]].GetLocation();

	// 每个有对应的子分支给出一对方向：目标骨架上的与网格上的
	TArray<FVector> TargetDirections;
	TArray<FVector> SourceDirections;
	int32 PrimaryIndex = INDEX_NONE;
	double PrimaryLength = 0.0;
	for (int32 ChildIndex = TargetIndex + 1; ChildIndex < Target.GetRawBoneNum(); ++ChildIndex)
	{
		if (Target.GetRawParentIndex(ChildIndex) != TargetIndex) continue;

		const int32 MappedIndex = FindMappedBelow(Target, MappedSourceOfTarget, ChildIndex);
		if (MappedIndex == INDEX_NONE) continue;

		const FVector TargetOffset = TargetGlobals[MappedIndex].GetLocation() - TargetOrigin;
		const FVector SourceOffset = SourceGlobals[MappedSourceOfTarget[MappedIndex]].GetLocation() - SourceOrigin;
		if (TargetOffset.Size() < JNSkeletalRefitConstants::MinBoneLength || SourceOffset.Size() < JNSkeletalRefitConstants::MinBoneLength) continue;

		if (TargetOffset.Size() > PrimaryLength)
		{
			PrimaryLength = TargetOffset.Size();
			PrimaryIndex = TargetDirections.Num();
		}
		TargetDirections.Add(TargetOffset.GetSafeNormal());
		SourceDirections.Add(SourceOffset.GetSafeNormal());
	}
	if (PrimaryIndex == INDEX_NONE) return ParentAlignment;

	// 摆动：在父骨骼对齐的基础上，把主方向转到网格上的方向
	const FVector PrimaryAxis = SourceDirections[PrimaryIndex];
	const FQuat Swing = FQuat::FindBetweenNormals(ParentAlignment.RotateVector(TargetDirections[PrimaryIndex]), PrimaryAxis);
	const FQuat Alignment = (Swing * ParentAlignment).GetNormalized();

	// 扭转：取与主方向最不共线的另一个方向，绕主方向转到位
	int32 SecondaryIndex = INDEX_NONE;
	double SecondaryCollinearity = JNSkeletalRefitConstants::MaxSecondaryCollinearity;
	for (int32 DirectionIndex = 0; DirectionIndex < TargetDirections.Num(); ++DirectionIndex)
	{
		if (DirectionIndex == PrimaryIndex) continue;

		const double Collinearity = FMath::Abs(FVector::DotProduct(TargetDirections[DirectionIndex], TargetDirections[PrimaryIndex]));
		if (Collinearity >= SecondaryCollinearity) continue;

		SecondaryCollinearity = Collinearity;
		SecondaryIndex = DirectionIndex;
	}
	if (SecondaryIndex == INDEX_NONE) return Alignment;

	const FVector TwistFrom = FVector::VectorPlaneProject(Alignment.RotateVector(TargetDirections[SecondaryIndex]), PrimaryAxis);
	const FVector TwistTo = FVector::VectorPlaneProject(SourceDirections[SecondaryIndex], PrimaryAxis);
	if (TwistFrom.Size() < UE_KINDA_SMALL_NUMBER || TwistTo.Size() < UE_KINDA_SMALL_NUMBER) return Alignment;

	const FQuat Twist = FQuat::FindBetweenNormals(TwistFrom.GetSafeNormal(), TwistTo.GetSafeNormal());
	return (Twist * Alignment).GetNormalized();
}

bool UJNSkeletalRefitFunctionLibrary::BuildPlan(const UJNSkeletalRefitJig* Jig, const FReferenceSkeleton& Source, const TSet<int32>& WeightedBones, FJNSkeletalRefitPlan& OutPlan, FJNSkeletalRefitResult& OutResult)
{
	const FReferenceSkeleton& Target = Jig->TargetSkeleton->GetReferenceSkeleton();
	const int32 SourceBoneCount = Source.GetRawBoneNum();
	const int32 TargetBoneCount = Target.GetRawBoneNum();

	TArray<FTransform> SourceGlobals;
	TArray<FTransform> TargetGlobals;
	ComputeGlobalPose(Source, SourceGlobals);
	ComputeGlobalPose(Target, TargetGlobals);

	// 每根来源骨骼的处理方式，以及每根目标骨骼被哪根来源骨骼对应
	TArray<FJNBoneRefitEntry> Entries;
	TArray<int32> MappedSourceOfTarget;
	Entries.SetNum(SourceBoneCount);
	MappedSourceOfTarget.Init(INDEX_NONE, TargetBoneCount);
	for (int32 SourceIndex = 0; SourceIndex < SourceBoneCount; ++SourceIndex)
	{
		const FName SourceBone = Source.GetBoneName(SourceIndex);
		const FJNBoneRefitEntry* JigEntry = Jig->FindEntry(SourceBone);
		FJNBoneRefitEntry Entry = JigEntry != nullptr ? *JigEntry : MakeAutoEntry(Target, SourceBone, WeightedBones.Contains(SourceIndex));
		if (Entry.Action == EJNBoneRefitAction::Map)
		{
			const int32 TargetIndex = Target.FindRawBoneIndex(Entry.TargetBone);
			if (TargetIndex == INDEX_NONE)
			{
				OutResult.Error = FText::Format(LOCTEXT("MapTargetMissing", "Bone '{0}' is mapped to '{1}', which is not on the target skeleton."), FText::FromName(SourceBone), FText::FromName(Entry.TargetBone));
				return false;
			}

			if (MappedSourceOfTarget[TargetIndex] == INDEX_NONE)
			{
				MappedSourceOfTarget[TargetIndex] = SourceIndex;
			}
			else
			{
				// 一根目标骨骼只能由一根来源骨骼定位置，后来的改为并入
				Entry.Action = EJNBoneRefitAction::Merge;
				OutResult.Warnings.Add(FText::Format(LOCTEXT("DuplicateMap", "Bone '{0}' is also mapped to '{1}'; its weights were merged instead."), FText::FromName(SourceBone), FText::FromName(Entry.TargetBone)));
			}
		}
		Entries[SourceIndex] = Entry;
	}

	// 带进新网格的目标骨骼：排除表里的连同子骨骼去掉，但被对应到的骨骼及其祖先必须在
	TArray<bool> IncludedTargets;
	IncludedTargets.Init(true, TargetBoneCount);
	for (int32 TargetIndex = 0; TargetIndex < TargetBoneCount; ++TargetIndex)
	{
		const int32 ParentIndex = Target.GetRawParentIndex(TargetIndex);
		const bool bParentExcluded = ParentIndex != INDEX_NONE && !IncludedTargets[ParentIndex];
		IncludedTargets[TargetIndex] = !bParentExcluded && !Jig->ExcludedTargetBones.Contains(Target.GetBoneName(TargetIndex));
	}
	for (int32 TargetIndex = 0; TargetIndex < TargetBoneCount; ++TargetIndex)
	{
		if (MappedSourceOfTarget[TargetIndex] == INDEX_NONE) continue;

		for (int32 AncestorIndex = TargetIndex; AncestorIndex != INDEX_NONE; AncestorIndex = Target.GetRawParentIndex(AncestorIndex))
		{
			IncludedTargets[AncestorIndex] = true;
		}
	}

	// 目标骨骼按原顺序进新层级。有对应的：位置取网格原关节位置，朝向取目标骨架的规范并对齐到网格的肢体方向；
	// 没有对应的：按目标骨架上相对父骨骼的位置补上
	TArray<FName> NewNames;
	TArray<int32> NewParents;
	TArray<FTransform> NewGlobals;
	TArray<int32> TargetToNewIndex;
	TArray<FQuat> Alignments;
	TargetToNewIndex.Init(INDEX_NONE, TargetBoneCount);
	Alignments.Init(FQuat::Identity, TargetBoneCount);
	OutPlan.SourceToNewIndex.Init(INDEX_NONE, SourceBoneCount);
	OutPlan.SourceToNewName.Reset();
	for (int32 TargetIndex = 0; TargetIndex < TargetBoneCount; ++TargetIndex)
	{
		if (!IncludedTargets[TargetIndex]) continue;

		const int32 ParentIndex = Target.GetRawParentIndex(TargetIndex);
		const int32 NewParentIndex = ParentIndex != INDEX_NONE ? TargetToNewIndex[ParentIndex] : INDEX_NONE;
		const FQuat ParentAlignment = ParentIndex != INDEX_NONE ? Alignments[ParentIndex] : FQuat::Identity;
		const int32 SourceIndex = MappedSourceOfTarget[TargetIndex];

		FTransform NewGlobal = TargetGlobals[TargetIndex];
		if (SourceIndex != INDEX_NONE)
		{
			Alignments[TargetIndex] = ComputeAlignment(Target, TargetGlobals, SourceGlobals, MappedSourceOfTarget, TargetIndex, ParentAlignment);
			NewGlobal = FTransform((Alignments[TargetIndex] * TargetGlobals[TargetIndex].GetRotation()).GetNormalized(), SourceGlobals[SourceIndex].GetLocation());

			OutPlan.SourceToNewIndex[SourceIndex] = NewNames.Num();
			OutPlan.SourceToNewName.Add(Source.GetBoneName(SourceIndex), Target.GetBoneName(TargetIndex));
		}
		else
		{
			Alignments[TargetIndex] = ParentAlignment;
			if (NewParentIndex != INDEX_NONE)
			{
				const FTransform& TargetLocal = Target.GetRawRefBonePose()[TargetIndex];
				NewGlobal = FTransform(TargetLocal.GetRotation(), TargetLocal.GetLocation()) * NewGlobals[NewParentIndex];
			}
		}

		TargetToNewIndex[TargetIndex] = NewNames.Num();
		NewNames.Add(Target.GetBoneName(TargetIndex));
		NewParents.Add(NewParentIndex);
		NewGlobals.Add(NewGlobal);
	}
	if (NewNames.IsEmpty())
	{
		OutResult.Error = LOCTEXT("NoBones", "No bone of the target skeleton is kept.");
		return false;
	}

	// 追加的骨骼排在目标骨骼之后，保持原来的位置与朝向。来源骨骼是父先子后，父骨骼若也是追加的，此时已有去向
	for (int32 SourceIndex = 0; SourceIndex < SourceBoneCount; ++SourceIndex)
	{
		const FJNBoneRefitEntry& Entry = Entries[SourceIndex];
		if (Entry.Action != EJNBoneRefitAction::Append) continue;

		const FName SourceBone = Source.GetBoneName(SourceIndex);
		const FName NewName = Entry.NewName.IsNone() ? FName(*(Jig->AppendPrefix + SourceBone.ToString())) : Entry.NewName;
		if (NewNames.Contains(NewName))
		{
			OutResult.Error = FText::Format(LOCTEXT("AppendNameTaken", "Bone '{0}' cannot be appended as '{1}': the name is already used."), FText::FromName(SourceBone), FText::FromName(NewName));
			return false;
		}

		int32 NewParentIndex = Entry.TargetBone.IsNone() ? FindResolvedAncestor(Source, OutPlan.SourceToNewIndex, SourceIndex) : NewNames.IndexOfByKey(Entry.TargetBone);
		if (NewParentIndex == INDEX_NONE && !Entry.TargetBone.IsNone())
		{
			OutResult.Error = FText::Format(LOCTEXT("AppendParentMissing", "Bone '{0}' cannot be appended under '{1}': no such bone."), FText::FromName(SourceBone), FText::FromName(Entry.TargetBone));
			return false;
		}
		if (NewParentIndex == INDEX_NONE) NewParentIndex = JNSkeletalRefitConstants::RootBoneIndex;

		OutPlan.SourceToNewIndex[SourceIndex] = NewNames.Num();
		OutPlan.SourceToNewName.Add(SourceBone, NewName);
		NewNames.Add(NewName);
		NewParents.Add(NewParentIndex);
		NewGlobals.Add(SourceGlobals[SourceIndex]);
	}

	// 不保留的骨骼：权重落到指定的骨骼，或最近的保留祖先
	for (int32 SourceIndex = 0; SourceIndex < SourceBoneCount; ++SourceIndex)
	{
		if (OutPlan.SourceToNewIndex[SourceIndex] != INDEX_NONE) continue;

		const FJNBoneRefitEntry& Entry = Entries[SourceIndex];
		const FName SourceBone = Source.GetBoneName(SourceIndex);
		const bool bUseNamedBone = Entry.Action == EJNBoneRefitAction::Merge && !Entry.TargetBone.IsNone();
		int32 NewIndex = bUseNamedBone ? NewNames.IndexOfByKey(Entry.TargetBone) : FindResolvedAncestor(Source, OutPlan.SourceToNewIndex, SourceIndex);
		if (NewIndex == INDEX_NONE && bUseNamedBone)
		{
			OutResult.Error = FText::Format(LOCTEXT("MergeTargetMissing", "Bone '{0}' cannot be merged into '{1}': no such bone."), FText::FromName(SourceBone), FText::FromName(Entry.TargetBone));
			return false;
		}
		if (NewIndex == INDEX_NONE) NewIndex = JNSkeletalRefitConstants::RootBoneIndex;

		OutPlan.SourceToNewIndex[SourceIndex] = NewIndex;
		if (Entry.Action == EJNBoneRefitAction::Discard && WeightedBones.Contains(SourceIndex))
		{
			OutResult.Warnings.Add(FText::Format(LOCTEXT("DiscardedWeighted", "Discarded bone '{0}' has skin weights; they were moved to '{1}'."), FText::FromName(SourceBone), FText::FromName(NewNames[NewIndex])));
		}
	}

	// 由模型空间姿态换回各骨骼相对父骨骼的姿态，建出新层级
	OutPlan.RefSkeleton.Empty();
	{
		FReferenceSkeletonModifier Modifier(OutPlan.RefSkeleton, nullptr);
		for (int32 NewIndex = 0; NewIndex < NewNames.Num(); ++NewIndex)
		{
			const int32 NewParentIndex = NewParents[NewIndex];
			const FTransform Local = NewParentIndex != INDEX_NONE ? NewGlobals[NewIndex].GetRelativeTransform(NewGlobals[NewParentIndex]) : NewGlobals[NewIndex];
			Modifier.Add(FMeshBoneInfo(NewNames[NewIndex], NewNames[NewIndex].ToString(), NewParentIndex), Local);
		}
	}
	return true;
}

void UJNSkeletalRefitFunctionLibrary::ApplyPlanToMeshDescription(const FJNSkeletalRefitPlan& Plan, FMeshDescription& MeshDescription)
{
	const TArray<FMeshBoneInfo>& BoneInfos = Plan.RefSkeleton.GetRawRefBoneInfo();
	const TArray<FTransform>& BonePoseTransforms = Plan.RefSkeleton.GetRawRefBonePose();

	FSkeletalMeshAttributes MeshAttributes(MeshDescription);
	if (!MeshAttributes.HasBones()) MeshAttributes.Register(true);

	// 骨骼表整体换成新层级
	MeshAttributes.Bones().Reset(BoneInfos.Num());
	FSkeletalMeshAttributes::FBoneNameAttributesRef BoneNames = MeshAttributes.GetBoneNames();
	FSkeletalMeshAttributes::FBoneParentIndexAttributesRef BoneParentIndices = MeshAttributes.GetBoneParentIndices();
	FSkeletalMeshAttributes::FBonePoseAttributesRef BonePoses = MeshAttributes.GetBonePoses();
	for (int32 BoneIndex = 0; BoneIndex < BoneInfos.Num(); ++BoneIndex)
	{
		const FBoneID BoneID = MeshAttributes.CreateBone();
		BoneNames.Set(BoneID, BoneInfos[BoneIndex].Name);
		BoneParentIndices.Set(BoneID, BoneInfos[BoneIndex].ParentIndex);
		BonePoses.Set(BoneID, BonePoseTransforms[BoneIndex]);
	}

	// 权重按去向转移；多根来源骨骼落到同一根新骨骼时相加。不重新归一化，总和保持不变
	UE::AnimationCore::FBoneWeightsSettings WeightSettings;
	WeightSettings.SetNormalizeType(UE::AnimationCore::EBoneWeightNormalizeType::None);
	for (const FName ProfileName : MeshAttributes.GetSkinWeightProfileNames())
	{
		FSkinWeightsVertexAttributesRef SkinWeights = MeshAttributes.GetVertexSkinWeights(ProfileName);
		if (!SkinWeights.IsValid()) continue;

		for (const FVertexID VertexID : MeshDescription.Vertices().GetElementIDs())
		{
			const FVertexBoneWeights OldWeights = SkinWeights.Get(VertexID);
			if (OldWeights.Num() == 0) continue;

			TArray<UE::AnimationCore::FBoneWeight> NewWeights;
			for (int32 WeightIndex = 0; WeightIndex < OldWeights.Num(); ++WeightIndex)
			{
				const int32 SourceIndex = OldWeights[WeightIndex].GetBoneIndex();
				const int32 NewIndex = Plan.SourceToNewIndex.IsValidIndex(SourceIndex) ? Plan.SourceToNewIndex[SourceIndex] : JNSkeletalRefitConstants::RootBoneIndex;
				const uint32 RawWeight = OldWeights[WeightIndex].GetRawWeight();

				UE::AnimationCore::FBoneWeight* Existing = NewWeights.FindByPredicate([NewIndex](const UE::AnimationCore::FBoneWeight& Weight) { return Weight.GetBoneIndex() == NewIndex; });
				if (Existing != nullptr)
				{
					Existing->SetRawWeight(static_cast<uint16>(FMath::Min<uint32>(Existing->GetRawWeight() + RawWeight, UE::AnimationCore::FBoneWeight::GetMaxRawWeight())));
				}
				else
				{
					NewWeights.Add(UE::AnimationCore::FBoneWeight(static_cast<FBoneIndexType>(NewIndex), static_cast<uint16>(RawWeight)));
				}
			}
			SkinWeights.Set(VertexID, UE::AnimationCore::FBoneWeights::Create(NewWeights, WeightSettings));
		}
	}
}

#undef LOCTEXT_NAMESPACE
