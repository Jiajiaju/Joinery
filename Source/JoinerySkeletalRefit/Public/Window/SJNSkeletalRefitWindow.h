#pragma once

#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "Widgets/SCompoundWidget.h"

class UJNSkeletalRefitRequest;

/**
 * Skeletal Refit 的转换窗口：上方填写对应表、网格与输出位置，中间两个按钮分别按同名自动填表与执行转换，
 * 最下面只读显示上一次操作的结果。对应表的逐行内容在对应表资产自己的编辑器里改。
 */
class SJNSkeletalRefitWindow : public SCompoundWidget, public FGCObject
{
private:
	/** 窗口里填写的转换请求，由本窗口持有。 */
	TObjectPtr<UJNSkeletalRefitRequest> Request = nullptr;

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

	/** 用第一个网格给对应表补上还没有列出的骨骼。 */
	FReply HandleAutoFillClicked();

	/** 逐个转换请求里的网格，并汇总结果。 */
	FReply HandleRefitClicked();
};
