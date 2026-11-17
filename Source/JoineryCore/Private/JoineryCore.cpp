#include "JoineryCore.h"

#include "Common/Constant/JNCoreConstants.h"
#include "JNCoreStyle.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "JoineryCore"

void FJoineryCoreModule::StartupModule()
{
	FJNCoreStyle::Initialize();
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FJoineryCoreModule::RegisterMenus));
}

void FJoineryCoreModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(JNCoreConstants::MenuOwnerName);
	FJNCoreStyle::Shutdown();
}

void FJoineryCoreModule::RegisterMenus()
{
	UToolMenu* MainMenu = UToolMenus::Get()->ExtendMenu(JNCoreConstants::LevelEditorMainMenuName);
	if (!IsValid(MainMenu)) return;

	UToolMenu* JoineryMenu = MainMenu->AddSubMenu(
		FToolMenuOwner(JNCoreConstants::MenuOwnerName),
		NAME_None,
		JNCoreConstants::JoineryMenuEntryName,
		LOCTEXT("JoineryMenuLabel", "Joinery"),
		LOCTEXT("JoineryMenuToolTip", "Joinery editor tools"));
	if (!IsValid(JoineryMenu)) return;

	JoineryMenu->FindOrAddSection(JNCoreConstants::JoineryToolsSectionName, LOCTEXT("JoineryToolsSection", "Tools"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FJoineryCoreModule, JoineryCore)
