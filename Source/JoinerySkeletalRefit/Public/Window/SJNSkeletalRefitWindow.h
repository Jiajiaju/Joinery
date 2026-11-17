#pragma once

#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "Widgets/SCompoundWidget.h"

class SJNSkeletalRefitViewport;
class UJNSkeletalRefitRequest;
struct FPropertyChangedEvent;
struct FSlateBrush;

/**
 * Skeletal Refit 的转换窗口。左边填写对应表、网格、输出位置与预览选项，下面三个按钮分别是按同名自动填表、
 * 刷新预览与执行转换，再下面只读显示上一次操作的结果；右边是预览视口，显示按当前对应表临时转换出的网格。
 * 对应表的逐行内容在对应表资产自己的编辑器里改，改完点 Preview 刷新。
 */
class SJNSkeletalRefitWindow : public SCompoundWidget, public FGCObject
{
private:
	/** 窗口里填写的转换请求，由本窗口持有。 */
	TObjectPtr<UJNSkeletalRefitRequest> Request = nullptr;

	TSharedPtr<SJNSkeletalRefitViewport> Viewport;

	/** 上一次操作的结果文字。 */
	FText ResultText;

public:
	SLATE_BEGIN_ARGS(SJNSkeletalRefitWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	//~ Begin FGCObject Interface.
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;
	//~ End FGCObject Interface.

private:
	FText GetResultText() const;

	/** 建一个带图标的按钮。 */
	TSharedRef<SWidget> MakeButton(const FSlateBrush* Icon, const FText& Label, const FText& ToolTip, FReply (SJNSkeletalRefitWindow::*Handler)());

	/** 请求里的属性改了：把骨骼显示开关同步给视口。 */
	void HandleRequestChanged(const FPropertyChangedEvent& PropertyChangedEvent);

	/** 用第一个网格给对应表补上还没有列出的骨骼。 */
	FReply HandleAutoFillClicked();

	/** 按当前对应表临时转换请求里的网格并显示在视口里，不产生资产。 */
	FReply HandlePreviewClicked();

	/** 逐个转换请求里的网格，并汇总结果。 */
	FReply HandleRefitClicked();
};
