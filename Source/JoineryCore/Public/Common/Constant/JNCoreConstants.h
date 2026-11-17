#pragma once

#include "CoreMinimal.h"

/** Joinery 各工具共用的固定约定：插件名、样式名、主菜单名、图标名与图标尺寸。 */
namespace JNCoreConstants
{
	/** 插件名，用来定位插件 Resources 目录。 */
	inline constexpr const TCHAR* PluginName = TEXT("Joinery");

	/** 图标所在的目录，相对插件根目录。 */
	inline constexpr const TCHAR* ResourcesDirectoryName = TEXT("Resources");
	inline constexpr const TCHAR* IconsDirectoryName = TEXT("Icons");
	inline constexpr const TCHAR* IconFileExtension = TEXT(".svg");

	inline const FName StyleName(TEXT("JNCoreStyle"));

	/** 关卡编辑器主菜单栏，以及挂在它上面的 Joinery 菜单。各工具把自己的入口加进 Joinery 菜单。 */
	inline const FName LevelEditorMainMenuName(TEXT("LevelEditor.MainMenu"));
	inline const FName JoineryMenuEntryName(TEXT("Joinery"));
	inline const FName JoineryMenuName(TEXT("LevelEditor.MainMenu.Joinery"));
	inline const FName JoineryToolsSectionName(TEXT("JoineryTools"));

	/** 向 ToolMenus 登记菜单时用的所有者名，模块关闭时据此注销。 */
	inline const FName MenuOwnerName(TEXT("JoineryCore"));

	inline const FVector2f Icon16(16.0f, 16.0f);
	inline const FVector2f Icon64(64.0f, 64.0f);

	/** 图标名，对应 Resources/Icons 下的同名 SVG。 */
	inline const FName JoineryIconName(TEXT("Joinery.Joinery"));
	inline const FName SkeletalRefitIconName(TEXT("Joinery.SkeletalRefit"));
	inline const FName MapIconName(TEXT("Joinery.Map"));
	inline const FName AppendIconName(TEXT("Joinery.Append"));
	inline const FName MergeIconName(TEXT("Joinery.Merge"));
	inline const FName DiscardIconName(TEXT("Joinery.Discard"));
	inline const FName ConvertIconName(TEXT("Joinery.Convert"));
	inline const FName AutoFillIconName(TEXT("Joinery.AutoFill"));

	inline constexpr const TCHAR* JoineryIconFile = TEXT("JNJoinery_16");
	inline constexpr const TCHAR* SkeletalRefitIconFile = TEXT("JNSkeletalRefit_16");
	inline constexpr const TCHAR* MapIconFile = TEXT("JNMap_16");
	inline constexpr const TCHAR* AppendIconFile = TEXT("JNAppend_16");
	inline constexpr const TCHAR* MergeIconFile = TEXT("JNMerge_16");
	inline constexpr const TCHAR* DiscardIconFile = TEXT("JNDiscard_16");
	inline constexpr const TCHAR* ConvertIconFile = TEXT("JNConvert_16");
	inline constexpr const TCHAR* AutoFillIconFile = TEXT("JNAutoFill_16");
}
