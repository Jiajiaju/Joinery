#pragma once

#include "CoreMinimal.h"
#include "EditorViewportClient.h"
#include "SEditorViewport.h"

class FAdvancedPreviewScene;
class UAnimationAsset;
class USkeletalMesh;
class USkeletalMeshComponent;

/** Skeletal Refit 预览视口的客户端：推进预览世界，并在网格上叠画骨骼连线。 */
class FJNSkeletalRefitViewportClient : public FEditorViewportClient
{
private:
	/** 当前预览的网格组件，由视口控件设置。 */
	TArray<TWeakObjectPtr<USkeletalMeshComponent>> MeshComponents;

	bool bShowBones = true;

public:
	FJNSkeletalRefitViewportClient(FPreviewScene& InPreviewScene, const TSharedRef<SEditorViewport>& InViewport);

	void SetMeshComponents(const TArray<TWeakObjectPtr<USkeletalMeshComponent>>& InMeshComponents);
	void SetShowBones(const bool bInShowBones);

	//~ Begin FEditorViewportClient Interface.
	virtual void Tick(float DeltaSeconds) override;
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	//~ End FEditorViewportClient Interface.
};

/**
 * Skeletal Refit 的预览视口：显示按对应表临时转换出的网格，可以播放目标骨架上的动画、叠画骨骼。
 * 同批的网格一起显示，第一个网格播放动画，其余网格跟随它的姿态，用来检查身体与头这类分开的网格的接缝。
 */
class SJNSkeletalRefitViewport : public SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SJNSkeletalRefitViewport) {}
	SLATE_END_ARGS()

private:
	TSharedPtr<FAdvancedPreviewScene> PreviewScene;
	TSharedPtr<FJNSkeletalRefitViewportClient> ViewportClient;

	/** 当前预览的网格组件，第一个是播放动画的主组件。 */
	TArray<TWeakObjectPtr<USkeletalMeshComponent>> MeshComponents;

public:
	void Construct(const FArguments& InArgs);

	/** 换成显示这些网格，并在第一个网格上循环播放动画；动画为空时保持静止姿态。 */
	void SetPreviewMeshes(const TArray<USkeletalMesh*>& Meshes, UAnimationAsset* Animation);

	void SetShowBones(const bool bShowBones);

protected:
	//~ Begin SEditorViewport Interface.
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	//~ End SEditorViewport Interface.

private:
	/** 把当前的预览组件从场景里拿掉。 */
	void ClearPreviewMeshes();
};
