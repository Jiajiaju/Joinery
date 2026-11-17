#pragma once

#include "CoreMinimal.h"

/** Skeletal Refit 的固定约定：菜单与标签页名、默认命名、几何容差。 */
namespace JNSkeletalRefitConstants
{
	/** 转换窗口的标签页名。 */
	inline const FName WindowTabName(TEXT("JoinerySkeletalRefit"));

	/** Joinery 菜单里本工具入口的名字，以及登记菜单时用的所有者名。 */
	inline const FName MenuEntryName(TEXT("SkeletalRefit"));
	inline const FName MenuOwnerName(TEXT("JoinerySkeletalRefit"));

	inline const FName PropertyEditorModuleName(TEXT("PropertyEditor"));

	/** 对应表条目结构的名字，登记属性定制时用。 */
	inline const FName BoneRefitEntryStructName(TEXT("JNBoneRefitEntry"));

	/** 新建对应表资产的默认名。 */
	inline constexpr const TCHAR* NewJigAssetName = TEXT("JIG_NewSkeletalRefit");

	/** 转换后网格名在来源网格名后追加的默认后缀。 */
	inline constexpr const TCHAR* DefaultOutputSuffix = TEXT("_Refit");

	/** 对应表资产在内容浏览器里的颜色。 */
	inline const FLinearColor JigAssetColor(0.72f, 0.52f, 0.30f);

	/** 读取骨骼与权重所用的 LOD。 */
	inline constexpr int32 SourceLODIndex = 0;

	/** 没有可用的保留祖先时，权重与追加骨骼落到的骨骼下标（根骨骼）。 */
	inline constexpr int32 RootBoneIndex = 0;

	/** 短于这个长度的骨骼方向不参与朝向对齐（厘米）。 */
	inline constexpr double MinBoneLength = 0.01;

	/** 第二参考方向与主方向夹角余弦的绝对值上限，超过视为共线、不用来定扭转。 */
	inline constexpr double MaxSecondaryCollinearity = 0.95;

	/** 窗口内各控件之间的间距（Slate 单位）。 */
	inline constexpr float WindowPadding = 4.0f;

	/** 对应表行首图标与骨骼名之间的间距（Slate 单位）。 */
	inline constexpr float EntryIconPadding = 4.0f;
}
