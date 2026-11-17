#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class FSpawnTabArgs;
class SDockTab;

/** Skeletal Refit 工具模块：登记转换窗口、Joinery 菜单里的入口，以及骨骼处理表条目的 Details 显示。 */
class FJoinerySkeletalRefitModule : public IModuleInterface
{
public:
	//~ Begin IModuleInterface Interface.
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~ End IModuleInterface Interface.

private:
	/** ToolMenus 就绪后把入口加进主菜单栏的 Joinery 菜单。 */
	void RegisterMenus();

	TSharedRef<SDockTab> SpawnWindowTab(const FSpawnTabArgs& Args);

	/** 打开或聚焦转换窗口。 */
	void OpenWindow();
};
