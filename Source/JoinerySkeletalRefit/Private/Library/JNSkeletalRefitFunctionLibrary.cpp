#include "Library/JNSkeletalRefitFunctionLibrary.h"

#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Animation/AnimInstance.h"
#include "Animation/Skeleton.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetToolsModule.h"
#include "BoneWeights.h"
#include "ClothingAsset.h"
#include "Common/Constant/JNSkeletalRefitConstants.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GroomBindingAsset.h"
#include "IAssetTools.h"
#include "JNSkeletalRefitLog.h"
#include "MeshDescription.h"
#include "Misc/PackageName.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "ReferenceSkeleton.h"
#include "RenderingThread.h"
#include "SkeletalMeshAttributes.h"
#include "Struct/JNSkeletalRefitPlan.h"
#include "Struct/JNSkeletalRefitResult.h"
#include "Struct/JNSkeletalRefitSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JNSkeletalRefitFunctionLibrary)

#define LOCTEXT_NAMESPACE "JNSkeletalRefit"

int32 UJNSkeletalRefitFunctionLibrary::FillBones(FJNSkeletalRefitSettings& Settings, const USkeletalMesh* Mesh)
{
	if (!IsValid(Settings.TargetSkeleton) || !IsValid(Mesh)) return 0;

	TSet<int32> WeightedBones;
	CollectWeightedBones(Mesh, WeightedBones);

	const FReferenceSkeleton& Source = Mesh->GetRefSkeleton();
	const FReferenceSkeleton& Target = Settings.TargetSkeleton->GetReferenceSkeleton();

	int32 AddedCount = 0;
	for (int32 SourceIndex = 0; SourceIndex < Source.GetRawBoneNum(); ++SourceIndex)
	{
		const FName SourceBone = Source.GetBoneName(SourceIndex);
		if (Settings.FindEntry(SourceBone) != nullptr) continue;

		Settings.Bones.Add(MakeAutoEntry(Target, SourceBone, WeightedBones.Contains(SourceIndex)));
		++AddedCount;
	}

	return AddedCount;
}

bool UJNSkeletalRefitFunctionLibrary::BuildPlan(const FJNSkeletalRefitSettings& Settings, const USkeletalMesh* SourceMesh, FJNSkeletalRefitPlan& OutPlan, FJNSkeletalRefitResult& OutResult)
{
	if (!IsValid(Settings.TargetSkeleton))
	{
		OutResult.Error = LOCTEXT("NoTargetSkeleton", "No target skeleton.");
		return false;
	}
	if (!IsValid(SourceMesh))
	{
		OutResult.Error = LOCTEXT("NoSourceMesh", "No source mesh.");
		return false;
	}
	if (SourceMesh->GetSkeleton() == Settings.TargetSkeleton)
	{
		OutResult.Error = FText::Format(LOCTEXT("AlreadyOnTarget", "'{0}' already uses the target skeleton."), FText::FromString(SourceMesh->GetName()));
		return false;
	}

	TSet<int32> WeightedBones;
	CollectWeightedBones(SourceMesh, WeightedBones);
	OutPlan.TargetSkeleton = Settings.TargetSkeleton;
	OutPlan.bCopySkeletonSockets = Settings.bCopySkeletonSockets;
	return BuildBonePlan(Settings, SourceMesh->GetRefSkeleton(), WeightedBones, OutPlan, OutResult);
}

void UJNSkeletalRefitFunctionLibrary::RebuildGroomBindings(const USkeletalMesh* SourceMesh, USkeletalMesh* NewMesh, const FString& OutputSuffix, FJNSkeletalRefitResult& OutResult)
{
	if (!IsValid(SourceMesh) || !IsValid(NewMesh)) return;

	// 找出目标网格是来源网格的毛发 Binding，各复制一份指向新网格并重建
	const IAssetRegistry& AssetRegistry = IAssetRegistry::GetChecked();
	TArray<FName> ReferencerPackages;
	AssetRegistry.GetReferencers(SourceMesh->GetOutermost()->GetFName(), ReferencerPackages);

	IAssetTools& AssetTools = FAssetToolsModule::GetModule().Get();
	for (const FName ReferencerPackage : ReferencerPackages)
	{
		TArray<FAssetData> ReferencerAssets;
		AssetRegistry.GetAssetsByPackageName(ReferencerPackage, ReferencerAssets);
		for (const FAssetData& ReferencerAsset : ReferencerAssets)
		{
			if (ReferencerAsset.AssetClassPath != UGroomBindingAsset::StaticClass()->GetClassPathName()) continue;

			UGroomBindingAsset* Binding = Cast<UGroomBindingAsset>(ReferencerAsset.GetAsset());
			if (!IsValid(Binding) || Binding->GetTargetSkeletalMesh() != SourceMesh) continue;

			const FString PackagePath = FPackageName::GetLongPackagePath(Binding->GetOutermost()->GetName());
			const FString AssetName = Binding->GetName() + OutputSuffix;
			if (FPackageName::DoesPackageExist(PackagePath / AssetName))
			{
				OutResult.Warnings.Add(FText::Format(LOCTEXT("GroomBindingExists", "Groom binding '{0}' already exists and was left as is."), FText::FromString(PackagePath / AssetName)));
				continue;
			}

			UGroomBindingAsset* NewBinding = Cast<UGroomBindingAsset>(AssetTools.DuplicateAsset(AssetName, PackagePath, Binding));
			if (!IsValid(NewBinding))
			{
				OutResult.Warnings.Add(FText::Format(LOCTEXT("GroomBindingFailed", "Failed to duplicate groom binding '{0}'. Rebuild it for the refitted mesh by hand."), FText::FromString(Binding->GetName())));
				continue;
			}

			NewBinding->SetTargetSkeletalMesh(NewMesh);
			NewBinding->Build();
			NewBinding->MarkPackageDirty();
			OutResult.Warnings.Add(FText::Format(LOCTEXT("GroomBindingRebuilt", "Groom binding '{0}' was created for the refitted mesh. Use it in place of '{1}'."), FText::FromString(NewBinding->GetName()), FText::FromString(Binding->GetName())));
		}
	}
}

bool UJNSkeletalRefitFunctionLibrary::IsPhysicsAssetCompatible(const UPhysicsAsset* PhysicsAsset, const FReferenceSkeleton& RefSkeleton)
{
	if (!IsValid(PhysicsAsset)) return false;

	for (const USkeletalBodySetup* BodySetup : PhysicsAsset->SkeletalBodySetups)
	{
		if (IsValid(BodySetup) && RefSkeleton.FindRawBoneIndex(BodySetup->BoneName) == INDEX_NONE) return false;
	}
	return true;
}

bool UJNSkeletalRefitFunctionLibrary::ApplyPlan(const FJNSkeletalRefitPlan& Plan, USkeletalMesh* Mesh, USkeleton* Skeleton, FJNSkeletalRefitResult& OutResult)
{
	if (!IsValid(Mesh) || !IsValid(Skeleton))
	{
		OutResult.Error = LOCTEXT("NoMeshOrSkeleton", "No mesh or skeleton to apply the plan to.");
		return false;
	}

	const int32 MorphCountBefore = Mesh->GetMorphTargets().Num();

	// 换骨骼之前先记下旧层级的模型空间姿态与旧骨架，插槽的相对位置要靠它们换算
	const FReferenceSkeleton OldRefSkeleton = Mesh->GetRefSkeleton();
	const USkeleton* OldSkeleton = Mesh->GetSkeleton();
	TArray<FTransform> OldGlobals;
	TArray<FTransform> NewGlobals;
	ComputeGlobalPose(OldRefSkeleton, OldGlobals);
	ComputeGlobalPose(Plan.RefSkeleton, NewGlobals);

	// 换骨骼层级并重算绑定：顶点不动，只有骨骼的静止姿态变了
	FlushRenderingCommands();
	Mesh->Modify();
	Mesh->SetRefSkeleton(Plan.RefSkeleton);
	Mesh->GetRefBasesInvMatrix().Reset();
	Mesh->CalculateInvRefMatrices();

	// 布料数据内嵌在网格里，按名字记着用到的骨骼并缓存了下标：名字跟着处理表改，下标按新层级重建
	for (UClothingAssetBase* ClothingAsset : Mesh->GetMeshClothingAssets())
	{
		UClothingAssetCommon* Cloth = Cast<UClothingAssetCommon>(ClothingAsset);
		if (!IsValid(Cloth)) continue;

		for (FName& UsedBoneName : Cloth->UsedBoneNames)
		{
			const FName* FinalName = Plan.SourceToFinalName.Find(UsedBoneName);
			if (FinalName != nullptr) UsedBoneName = *FinalName;
		}
		Cloth->RefreshBoneMapping(Mesh);
		Cloth->CalculateReferenceBoneIndex();
		Cloth->InvalidateAllCachedData();

		if (IsValid(Cloth->PhysicsAsset) && !IsPhysicsAssetCompatible(Cloth->PhysicsAsset, Plan.RefSkeleton))
		{
			OutResult.Warnings.Add(FText::Format(LOCTEXT("ClothCollisionCleared", "Cloth '{0}': its collision physics asset '{1}' uses bones that were renamed or not kept, so it was cleared."), FText::FromString(Cloth->GetName()), FText::FromString(Cloth->PhysicsAsset->GetName())));
			Cloth->PhysicsAsset = nullptr;
		}
	}

	for (int32 LODIndex = 0; LODIndex < Mesh->GetLODNum(); ++LODIndex)
	{
		if (!Mesh->HasMeshDescription(LODIndex)) continue;

		FMeshDescription MeshDescription;
		if (!Mesh->CloneMeshDescription(LODIndex, MeshDescription)) continue;

		ApplyPlanToMeshDescription(Plan, MeshDescription);
		Mesh->ModifyMeshDescription(LODIndex);
		Mesh->CreateMeshDescription(LODIndex, MoveTemp(MeshDescription));
		Mesh->CommitMeshDescription(LODIndex);
	}

	// 来源骨架上的插槽不跟网格走，复制成网格自己的插槽；网格上已有同名插槽的不覆盖
	if (Plan.bCopySkeletonSockets && IsValid(OldSkeleton))
	{
		for (const USkeletalMeshSocket* SkeletonSocket : OldSkeleton->Sockets)
		{
			if (!IsValid(SkeletonSocket)) continue;

			const bool bAlreadyOnMesh = Mesh->GetMeshOnlySocketList().ContainsByPredicate([SkeletonSocket](const USkeletalMeshSocket* MeshSocket) { return IsValid(MeshSocket) && MeshSocket->SocketName == SkeletonSocket->SocketName; });
			if (bAlreadyOnMesh) continue;

			Mesh->AddSocket(DuplicateObject<USkeletalMeshSocket>(SkeletonSocket, Mesh));
		}
	}

	// 插槽是相对骨骼记录的，骨骼换了名字和朝向：保持插槽在模型空间的位置不变，换算成相对新骨骼的值
	for (USkeletalMeshSocket* Socket : Mesh->GetMeshOnlySocketList())
	{
		if (!IsValid(Socket)) continue;

		const int32 OldBoneIndex = OldRefSkeleton.FindRawBoneIndex(Socket->BoneName);
		const FName* FinalName = Plan.SourceToFinalName.Find(Socket->BoneName);
		const int32 NewBoneIndex = FinalName != nullptr ? Plan.RefSkeleton.FindRawBoneIndex(*FinalName) : INDEX_NONE;
		if (OldBoneIndex == INDEX_NONE || NewBoneIndex == INDEX_NONE)
		{
			OutResult.Warnings.Add(FText::Format(LOCTEXT("SocketBoneMissing", "Socket '{0}' is attached to bone '{1}', which the mesh does not have; it was left as is."), FText::FromName(Socket->SocketName), FText::FromName(Socket->BoneName)));
			continue;
		}

		const FTransform SocketGlobal = Socket->GetSocketLocalTransform() * OldGlobals[OldBoneIndex];
		const FTransform SocketLocal = SocketGlobal.GetRelativeTransform(NewGlobals[NewBoneIndex]);
		Socket->BoneName = *FinalName;
		Socket->RelativeLocation = SocketLocal.GetLocation();
		Socket->RelativeRotation = SocketLocal.Rotator();

		// 插槽与骨骼重名时，引擎按名字查找先取插槽，这里只做提示
		if (Plan.RefSkeleton.FindRawBoneIndex(Socket->SocketName) != INDEX_NONE)
		{
			OutResult.Warnings.Add(FText::Format(LOCTEXT("SocketNameIsBone", "Socket '{0}' has the same name as a bone; lookups by name return the socket."), FText::FromName(Socket->SocketName)));
		}
	}
	Mesh->RebuildSocketMap();

	// 物理资产按骨骼名引用：用到的骨骼都还在就沿用，否则清掉
	if (IsValid(Mesh->GetPhysicsAsset()) && !IsPhysicsAssetCompatible(Mesh->GetPhysicsAsset(), Plan.RefSkeleton))
	{
		OutResult.Warnings.Add(FText::Format(LOCTEXT("PhysicsAssetCleared", "The physics asset '{0}' uses bones that were renamed or not kept, so it was cleared. Create one for the refitted mesh."), FText::FromString(Mesh->GetPhysicsAsset()->GetName())));
		Mesh->SetPhysicsAsset(nullptr);
	}

	// 后处理动画蓝图绑定在骨架上：本来就是目标骨架的才留
	const UAnimBlueprintGeneratedClass* PostProcessClass = Cast<UAnimBlueprintGeneratedClass>(Mesh->GetPostProcessAnimBlueprint().Get());
	if (Mesh->GetPostProcessAnimBlueprint() != nullptr && (PostProcessClass == nullptr || PostProcessClass->TargetSkeleton != Plan.TargetSkeleton.Get()))
	{
		Mesh->SetPostProcessAnimBlueprint(nullptr);
		OutResult.Warnings.Add(LOCTEXT("PostProcessCleared", "The post process anim blueprint was made for another skeleton, so it was cleared. Assign one made for the target skeleton."));
	}

	Skeleton->Modify();
	Mesh->SetSkeleton(Skeleton);
	const bool bMerged = Skeleton->MergeAllBonesToBoneTree(Mesh, false);
	Mesh->PostEditChange();
	Mesh->MarkPackageDirty();
	if (!bMerged)
	{
		OutResult.Error = FText::Format(LOCTEXT("MergeFailed", "Failed to merge the bones of '{0}' into the skeleton."), FText::FromString(Mesh->GetName()));
		return false;
	}
	Skeleton->MarkPackageDirty();

	if (Mesh->GetMorphTargets().Num() != MorphCountBefore)
	{
		OutResult.Error = FText::Format(LOCTEXT("MorphMismatch", "Morph target count changed from {0} to {1}."), FText::AsNumber(MorphCountBefore), FText::AsNumber(Mesh->GetMorphTargets().Num()));
		return false;
	}
	return true;
}

USkeletalMesh* UJNSkeletalRefitFunctionLibrary::RefitMesh(const FJNSkeletalRefitSettings& Settings, USkeletalMesh* SourceMesh, const FString& OutputFolder, const FString& OutputSuffix, FJNSkeletalRefitResult& OutResult)
{
	OutResult = FJNSkeletalRefitResult();

	// 先算方案，方案不成立就不产生任何资产
	FJNSkeletalRefitPlan Plan;
	if (!BuildPlan(Settings, SourceMesh, Plan, OutResult)) return nullptr;

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

	// 应用失败时新网格资产留着供检查，不算成功
	if (!ApplyPlan(Plan, NewMesh, Settings.TargetSkeleton.Get(), OutResult)) return nullptr;

	if (Settings.bRebuildGroomBindings) RebuildGroomBindings(SourceMesh, NewMesh, OutputSuffix, OutResult);

	UE_LOG(LogJNSkeletalRefit, Log, TEXT("Refitted '%s' to skeleton '%s' as '%s' with %d warning(s)."), *SourceMesh->GetPathName(), *Settings.TargetSkeleton->GetPathName(), *NewMesh->GetPathName(), OutResult.Warnings.Num());
	OutResult.bSucceeded = true;
	return NewMesh;
}

USkeleton* UJNSkeletalRefitFunctionLibrary::MakePreviewSkeleton(const FJNSkeletalRefitSettings& Settings)
{
	if (!IsValid(Settings.TargetSkeleton)) return nullptr;

	// 目标骨架的临时副本：承载预览里追加的骨骼，并登记为与目标骨架兼容，目标骨架上的动画可以直接播
	USkeleton* PreviewSkeleton = DuplicateObject<USkeleton>(Settings.TargetSkeleton.Get(), GetTransientPackage());
	if (!IsValid(PreviewSkeleton)) return nullptr;

	PreviewSkeleton->ClearFlags(RF_Public | RF_Standalone);
	PreviewSkeleton->SetFlags(RF_Transient);
	PreviewSkeleton->AddCompatibleSkeleton(Settings.TargetSkeleton.Get());
	return PreviewSkeleton;
}

USkeletalMesh* UJNSkeletalRefitFunctionLibrary::MakePreviewMesh(const FJNSkeletalRefitSettings& Settings, USkeletalMesh* SourceMesh, USkeleton* PreviewSkeleton, FJNSkeletalRefitResult& OutResult)
{
	OutResult = FJNSkeletalRefitResult();

	FJNSkeletalRefitPlan Plan;
	if (!BuildPlan(Settings, SourceMesh, Plan, OutResult)) return nullptr;

	USkeletalMesh* PreviewMesh = DuplicateObject<USkeletalMesh>(SourceMesh, GetTransientPackage());
	if (!IsValid(PreviewMesh))
	{
		OutResult.Error = FText::Format(LOCTEXT("PreviewDuplicateFailed", "Failed to duplicate '{0}' for preview."), FText::FromString(SourceMesh->GetName()));
		return nullptr;
	}

	PreviewMesh->ClearFlags(RF_Public | RF_Standalone);
	PreviewMesh->SetFlags(RF_Transient);
	if (!ApplyPlan(Plan, PreviewMesh, PreviewSkeleton, OutResult)) return nullptr;

	OutResult.bSucceeded = true;
	return PreviewMesh;
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

bool UJNSkeletalRefitFunctionLibrary::BuildBonePlan(const FJNSkeletalRefitSettings& Settings, const FReferenceSkeleton& Source, const TSet<int32>& WeightedBones, FJNSkeletalRefitPlan& OutPlan, FJNSkeletalRefitResult& OutResult)
{
	const FReferenceSkeleton& Target = Settings.TargetSkeleton->GetReferenceSkeleton();
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
		const FJNBoneRefitEntry* ListedEntry = Settings.FindEntry(SourceBone);
		FJNBoneRefitEntry Entry = ListedEntry != nullptr ? *ListedEntry : MakeAutoEntry(Target, SourceBone, WeightedBones.Contains(SourceIndex));
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
		IncludedTargets[TargetIndex] = !bParentExcluded && !Settings.ExcludedTargetBones.Contains(Target.GetBoneName(TargetIndex));
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
	OutPlan.SourceToFinalName.Reset();
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
		const FName NewName = Entry.NewName.IsNone() ? FName(*(Settings.AppendPrefix + SourceBone.ToString())) : Entry.NewName;
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

	// 每根来源骨骼最终落到的新骨骼名：保留的是自己的新名字，不保留的是权重去向的骨骼
	for (int32 SourceIndex = 0; SourceIndex < SourceBoneCount; ++SourceIndex)
	{
		OutPlan.SourceToFinalName.Add(Source.GetBoneName(SourceIndex), NewNames[OutPlan.SourceToNewIndex[SourceIndex]]);
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
