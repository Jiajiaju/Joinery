#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/**
 * Joinery 的公共模块：登记共用图标，并在关卡编辑器主菜单栏上建 Joinery 菜单。
 * 各工具模块依赖本模块，把自己的入口加进 JNCoreConstants::JoineryMenuName。
 */
class FJoineryCoreModule : public IModuleInterface
{
public:
	//~ Begin IModuleInterface Interface.
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~ End IModuleInterface Interface.

private:
	/** ToolMenus 就绪后在主菜单栏上建 Joinery 菜单。 */
	void RegisterMenus();
};
