#include "Window/SJNSkeletalRefitViewport.h"

#include "AdvancedPreviewScene.h"
#include "Animation/AnimationAsset.h"
#include "Common/Constant/JNSkeletalRefitConstants.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "ReferenceSkeleton.h"
#include "SceneManagement.h"

FJNSkeletalRefitViewportClient::FJNSkeletalRefitViewportClient(FPreviewScene& InPreviewScene, const TSharedRef<SEditorViewport>& InViewport)
	: FEditorViewportClient(nullptr, &InPreviewScene, InViewport)
{
	SetRealtime(true);
}

void FJNSkeletalRefitViewportClient::SetMeshComponents(const TArray<TWeakObjectPtr<USkeletalMeshComponent>>& InMeshComponents)
{
	MeshComponents = InMeshComponents;
}

void FJNSkeletalRefitViewportClient::SetShowBones(const bool bInShowBones)
{
	bShowBones = bInShowBones;
	Invalidate();
}

void FJNSkeletalRefitViewportClient::Tick(float DeltaSeconds)
{
	FEditorViewportClient::Tick(DeltaSeconds);

	// 预览世界不在引擎的世界列表里，要自己推进，动画才会播
	UWorld* World = PreviewScene != nullptr ? PreviewScene->GetWorld() : nullptr;
	if (IsValid(World)) World->Tick(LEVELTICK_All, DeltaSeconds);
}

void FJNSkeletalRefitViewportClient::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	FEditorViewportClient::Draw(View, PDI);
	if (!bShowBones) return;

	for (const TWeakObjectPtr<USkeletalMeshComponent>& WeakComponent : MeshComponents)
	{
		// 跟随主组件姿态的组件自己不求值，骨骼只画自己求值的组件
		const USkeletalMeshComponent* Component = WeakComponent.Get();
		if (!IsValid(Component) || !IsValid(Component->GetSkeletalMeshAsset()) || Component->LeaderPoseComponent.IsValid()) continue;

		const FReferenceSkeleton& RefSkeleton = Component->GetSkeletalMeshAsset()->GetRefSkeleton();
		const TArray<FTransform>& BoneTransforms = Component->GetComponentSpaceTransforms();
		const FTransform& ComponentTransform = Component->GetComponentTransform();
		for (int32 BoneIndex = 0; BoneIndex < BoneTransforms.Num() && BoneIndex < RefSkeleton.GetNum(); ++BoneIndex)
		{
			const int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);
			if (!BoneTransforms.IsValidIndex(ParentIndex)) continue;

			const FVector Start = ComponentTransform.TransformPosition(BoneTransforms[ParentIndex].GetLocation());
			const FVector End = ComponentTransform.TransformPosition(BoneTransforms[BoneIndex].GetLocation());
			PDI->DrawLine(Start, End, JNSkeletalRefitConstants::PreviewBoneColor, SDPG_Foreground, JNSkeletalRefitConstants::PreviewBoneThickness, 0.0f, true);
		}
	}
}

void SJNSkeletalRefitViewport::Construct(const FArguments& InArgs)
{
	// 预览世界默认不跑物理，布料也就不模拟；和引擎的动画编辑器一样把它打开
	PreviewScene = MakeShared<FAdvancedPreviewScene>(FPreviewScene::ConstructionValues().ShouldSimulatePhysics(true));
	SEditorViewport::Construct(SEditorViewport::FArguments());
}

void SJNSkeletalRefitViewport::SetPreviewMeshes(const TArray<USkeletalMesh*>& Meshes, UAnimationAsset* Animation)
{
	ClearPreviewMeshes();
	if (!PreviewScene.IsValid()) return;

	USkeletalMeshComponent* LeaderComponent = nullptr;
	FBox PreviewBox(ForceInit);
	for (USkeletalMesh* Mesh : Meshes)
	{
		if (!IsValid(Mesh)) continue;

		USkeletalMeshComponent* Component = NewObject<USkeletalMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient);
		Component->SetSkeletalMesh(Mesh);
		Component->SetUpdateAnimationInEditor(true);
		Component->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		PreviewScene->AddComponent(Component, FTransform::Identity);

		// 第一个网格播放动画，其余的跟随它的姿态
		if (!IsValid(LeaderComponent))
		{
			LeaderComponent = Component;
			if (IsValid(Animation)) Component->PlayAnimation(Animation, true);
		}
		else
		{
			Component->SetLeaderPoseComponent(LeaderComponent);
		}

		MeshComponents.Add(Component);
		PreviewBox += Component->Bounds.GetBox();
	}

	if (!ViewportClient.IsValid()) return;

	ViewportClient->SetMeshComponents(MeshComponents);
	if (PreviewBox.IsValid) ViewportClient->FocusViewportOnBox(PreviewBox, true);
	ViewportClient->Invalidate();
}

void SJNSkeletalRefitViewport::SetPreviewAnimation(UAnimationAsset* Animation)
{
	if (MeshComponents.IsEmpty()) return;

	// 动画为空时回到静止姿态
	USkeletalMeshComponent* LeaderComponent = MeshComponents[0].Get();
	if (IsValid(LeaderComponent)) LeaderComponent->PlayAnimation(Animation, true);
	if (ViewportClient.IsValid()) ViewportClient->Invalidate();
}

void SJNSkeletalRefitViewport::SetShowBones(const bool bShowBones)
{
	if (ViewportClient.IsValid()) ViewportClient->SetShowBones(bShowBones);
}

TSharedRef<FEditorViewportClient> SJNSkeletalRefitViewport::MakeEditorViewportClient()
{
	ViewportClient = MakeShared<FJNSkeletalRefitViewportClient>(*PreviewScene, SharedThis(this));
	return ViewportClient.ToSharedRef();
}

void SJNSkeletalRefitViewport::ClearPreviewMeshes()
{
	for (const TWeakObjectPtr<USkeletalMeshComponent>& WeakComponent : MeshComponents)
	{
		USkeletalMeshComponent* Component = WeakComponent.Get();
		if (IsValid(Component) && PreviewScene.IsValid()) PreviewScene->RemoveComponent(Component);
	}
	MeshComponents.Reset();

	if (ViewportClient.IsValid()) ViewportClient->SetMeshComponents(MeshComponents);
}
