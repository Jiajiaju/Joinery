#include "JoinerySkeletalRefit.h"

#include "Common/Constant/JNSkeletalRefitConstants.h"
#include "Common/Constant/JNCoreConstants.h"
#include "Customization/JNBoneRefitEntryCustomization.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "JNCoreStyle.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "StatusBarSubsystem.h"
#include "Textures/SlateIcon.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/SBoxPanel.h"
#include "Window/SJNSkeletalRefitWindow.h"

#define LOCTEXT_NAMESPACE "JoinerySkeletalRefit"

void FJoinerySkeletalRefitModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(JNSkeletalRefitConstants::WindowTabName, FOnSpawnTab::CreateRaw(this, &FJoinerySkeletalRefitModule::SpawnWindowTab))
		.SetDisplayName(LOCTEXT("WindowTitle", "Skeletal Refit"))
		.SetTooltipText(LOCTEXT("WindowToolTip", "Rebind skeletal meshes from any humanoid skeleton to a target skeleton."))
		.SetIcon(FSlateIcon(FJNCoreStyle::GetStyleSetName(), JNCoreConstants::SkeletalRefitIconName))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(JNSkeletalRefitConstants::PropertyEditorModuleName);
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(JNSkeletalRefitConstants::BoneRefitEntryStructName, FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FJNBoneRefitEntryCustomization::MakeInstance));

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FJoinerySkeletalRefitModule::RegisterMenus));
}

void FJoinerySkeletalRefitModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(JNSkeletalRefitConstants::MenuOwnerName);

	FPropertyEditorModule* PropertyEditorModule = FModuleManager::GetModulePtr<FPropertyEditorModule>(JNSkeletalRefitConstants::PropertyEditorModuleName);
	if (PropertyEditorModule != nullptr) PropertyEditorModule->UnregisterCustomPropertyTypeLayout(JNSkeletalRefitConstants::BoneRefitEntryStructName);

	if (FSlateApplication::IsInitialized()) FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(JNSkeletalRefitConstants::WindowTabName);
}

void FJoinerySkeletalRefitModule::RegisterMenus()
{
	UToolMenu* JoineryMenu = UToolMenus::Get()->ExtendMenu(JNCoreConstants::JoineryMenuName);
	if (!IsValid(JoineryMenu)) return;

	const FToolMenuOwnerScoped OwnerScoped(JNSkeletalRefitConstants::MenuOwnerName);
	FToolMenuSection& Section = JoineryMenu->FindOrAddSection(JNCoreConstants::JoineryToolsSectionName);
	Section.AddMenuEntry(
		JNSkeletalRefitConstants::MenuEntryName,
		LOCTEXT("MenuEntryLabel", "Skeletal Refit"),
		LOCTEXT("MenuEntryToolTip", "Rebind skeletal meshes from any humanoid skeleton to a target skeleton."),
		FSlateIcon(FJNCoreStyle::GetStyleSetName(), JNCoreConstants::SkeletalRefitIconName),
		FUIAction(FExecuteAction::CreateRaw(this, &FJoinerySkeletalRefitModule::OpenWindow)));
}

TSharedRef<SDockTab> FJoinerySkeletalRefitModule::SpawnWindowTab(const FSpawnTabArgs& Args)
{
	const TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab);

	// 内容抽屉（Ctrl+空格）是从前台标签页自己的状态栏里弹出的，所以标签页底部要带一条状态栏
	TSharedRef<SWidget> StatusBar = SNullWidget::NullWidget;
	UStatusBarSubsystem* StatusBarSubsystem = GEditor != nullptr ? GEditor->GetEditorSubsystem<UStatusBarSubsystem>() : nullptr;
	if (IsValid(StatusBarSubsystem)) StatusBar = StatusBarSubsystem->MakeStatusBarWidget(JNSkeletalRefitConstants::StatusBarName, Tab);

	Tab->SetContent(
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SJNSkeletalRefitWindow)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			StatusBar
		]);
	return Tab;
}

void FJoinerySkeletalRefitModule::OpenWindow()
{
	FGlobalTabmanager::Get()->TryInvokeTab(JNSkeletalRefitConstants::WindowTabName);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FJoinerySkeletalRefitModule, JoinerySkeletalRefit)
