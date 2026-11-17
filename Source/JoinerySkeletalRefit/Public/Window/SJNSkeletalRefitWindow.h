#pragma once

#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "Widgets/SCompoundWidget.h"

class FUICommandList;
class IDetailsView;
class SJNSkeletalRefitViewport;
class UJNSkeletalRefitRequest;
struct FPropertyChangedEvent;
struct FSlateBrush;

/**
 * Skeletal Refit 的转换窗口。左边直接选目标骨架与网格，骨骼处理表随之按自动规则填好并可逐行修改；
 * 下面三个按钮分别是重置骨骼处理表、刷新预览与导出，再下面只读显示上一次操作的结果。
 * 右边是预览视口，显示按当前设置临时转换出的网格。
 */
class SJNSkeletalRefitWindow : public SCompoundWidget, public FGCObject
{
private:
	/** 窗口里填写的转换请求，由本窗口持有。 */
	TObjectPtr<UJNSkeletalRefitRequest> Request = nullptr;

	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<SJNSkeletalRefitViewport> Viewport;

	/** 上一次操作的结果文字。 */
	FText ResultText;

	/** 引擎的全局编辑器快捷键（打开内容抽屉、打开资产等），焦点在本窗口里时由本窗口处理。 */
	TSharedPtr<FUICommandList> GlobalCommands;

public:
	SLATE_BEGIN_ARGS(SJNSkeletalRefitWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	//~ Begin FGCObject Interface.
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;
	//~ End FGCObject Interface.

	//~ Begin SWidget Interface.
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	//~ End SWidget Interface.

private:
	FText GetResultText() const;

	/** 建一个带图标的按钮。 */
	TSharedRef<SWidget> MakeButton(const FSlateBrush* Icon, const FText& Label, const FText& ToolTip, FReply (SJNSkeletalRefitWindow::*Handler)());

	/** 按自动规则给骨骼处理表补上各网格里还没有列出的骨骼；bReset 为真时先清空，丢掉手改的行。 */
	void FillBones(const bool bReset);

	/** 请求里的属性改了：会改变转换结果的设置变了就补骨骼处理表并重做预览，动画与骨骼显示只同步给视口。 */
	void HandleRequestChanged(const FPropertyChangedEvent& PropertyChangedEvent);

	/** 按当前设置临时转换请求里的网格并显示在视口里，不产生资产；设置没填全时清空视口。 */
	void RefreshPreview();

	/** 按自动规则重新生成骨骼处理表。 */
	FReply HandleResetBonesClicked();

	/** 手动重做一次预览。 */
	FReply HandlePreviewClicked();

	/** 逐个转换请求里的网格，生成新的网格资产，并汇总结果。 */
	FReply HandleExportClicked();
};
