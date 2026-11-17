#pragma once

#include "CoreMinimal.h"

class FSlateStyleSet;
class ISlateStyle;
struct FSlateBrush;

/**
 * Joinery 的 Slate 样式：插件各工具共用的一套图标。
 * 图片是插件 Resources/Icons 目录下的 SVG，图标名见 JNWorkbenchConstants。
 */
class JOINERYWORKBENCH_API FJNWorkbenchStyle
{
private:
	static TSharedPtr<FSlateStyleSet> StyleSet;

public:
	static void Initialize();
	static void Shutdown();

	static const ISlateStyle& Get();
	static FName GetStyleSetName();

	/** 按图标名取笔刷，样式未初始化时返回空。 */
	static const FSlateBrush* GetBrush(const FName IconName);

private:
	/** 用插件 Resources/Icons 下的 SVG 登记一个图标。 */
	static void RegisterIcon(const FName IconName, const TCHAR* FileName, const FVector2f& Size);
};
