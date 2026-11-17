#include "JoineryWorkbench.h"

#include "Common/Constant/JNWorkbenchConstants.h"
#include "JNWorkbenchStyle.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "JoineryWorkbench"

void FJoineryWorkbenchModule::StartupModule()
{
	FJNWorkbenchStyle::Initialize();
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FJoineryWorkbenchModule::RegisterMenus));
}

void FJoineryWorkbenchModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(JNWorkbenchConstants::MenuOwnerName);
	FJNWorkbenchStyle::Shutdown();
}

void FJoineryWorkbenchModule::RegisterMenus()
{
	UToolMenu* MainMenu = UToolMenus::Get()->ExtendMenu(JNWorkbenchConstants::LevelEditorMainMenuName);
	if (!IsValid(MainMenu)) return;

	UToolMenu* JoineryMenu = MainMenu->AddSubMenu(
		FToolMenuOwner(JNWorkbenchConstants::MenuOwnerName),
		NAME_None,
		JNWorkbenchConstants::JoineryMenuEntryName,
		LOCTEXT("JoineryMenuLabel", "Joinery"),
		LOCTEXT("JoineryMenuToolTip", "Joinery editor tools"));
	if (!IsValid(JoineryMenu)) return;

	JoineryMenu->FindOrAddSection(JNWorkbenchConstants::JoineryToolsSectionName, LOCTEXT("JoineryToolsSection", "Tools"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FJoineryWorkbenchModule, JoineryWorkbench)
