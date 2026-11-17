#pragma once

#include "CoreMinimal.h"

/** Skeletal Refit 的固定约定：菜单与标签页名、默认命名、几何容差。 */
namespace JNSkeletalRefitConstants
{
	/** 转换窗口标签页底部状态栏的名字，内容抽屉挂在它上面。 */
	inline const FName StatusBarName(TEXT("JoinerySkeletalRefit.StatusBar"));

	/** 转换窗口的标签页名。 */
	inline const FName WindowTabName(TEXT("JoinerySkeletalRefit"));

	/** Joinery 菜单里本工具入口的名字，以及登记菜单时用的所有者名。 */
	inline const FName MenuEntryName(TEXT("SkeletalRefit"));
	inline const FName MenuOwnerName(TEXT("JoinerySkeletalRefit"));

	inline const FName PropertyEditorModuleName(TEXT("PropertyEditor"));

	/** 骨骼处理表条目结构的名字，登记属性定制时用。 */
	inline const FName BoneRefitEntryStructName(TEXT("JNBoneRefitEntry"));

	/** 转换后网格名在来源网格名后追加的默认后缀。 */
	inline constexpr const TCHAR* DefaultOutputSuffix = TEXT("_Refit");

	/** 读取骨骼与权重所用的 LOD。 */
	inline constexpr int32 SourceLODIndex = 0;

	/** 没有可用的保留祖先时，权重与追加骨骼落到的骨骼下标（根骨骼）。 */
	inline constexpr int32 RootBoneIndex = 0;

	/** 短于这个长度的骨骼方向不参与朝向对齐（厘米）。 */
	inline constexpr double MinBoneLength = 0.01;

	/** 第二参考方向与主方向夹角余弦的绝对值上限，超过视为共线、不用来定扭转。 */
	inline constexpr double MaxSecondaryCollinearity = 0.95;

	/** 并入网格的材质槽名与已有槽名重复时，在槽名后面接上这个分隔符与并入网格的名字。 */
	inline constexpr const TCHAR* MergedSlotSeparator = TEXT("_");

	/** 窗口内各控件之间的间距（Slate 单位）。 */
	inline constexpr float WindowPadding = 4.0f;

	/** 窗口左侧参数区占整个窗口宽度的比例，其余给预览视口。 */
	inline constexpr float WindowParameterRatio = 0.4f;

	/** 窗口左侧结果文字区的最大高度（Slate 单位）。 */
	inline constexpr float ResultMaxHeight = 160.0f;

	/** 预览视口里骨骼连线的颜色与屏幕空间线宽。 */
	inline const FLinearColor PreviewBoneColor(1.0f, 0.85f, 0.2f);
	inline constexpr float PreviewBoneThickness = 1.0f;

	/** 骨骼处理表行首图标与骨骼名之间的间距（Slate 单位）。 */
	inline constexpr float EntryIconPadding = 4.0f;
}
