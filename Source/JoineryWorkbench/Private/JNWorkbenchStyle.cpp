#include "JNWorkbenchStyle.h"

#include "Brushes/SlateImageBrush.h"
#include "Common/Constant/JNWorkbenchConstants.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FSlateStyleSet> FJNWorkbenchStyle::StyleSet;

void FJNWorkbenchStyle::Initialize()
{
	if (StyleSet.IsValid()) return;

	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(JNWorkbenchConstants::PluginName);
	if (!Plugin.IsValid()) return;

	StyleSet = MakeShared<FSlateStyleSet>(JNWorkbenchConstants::StyleName);
	StyleSet->SetContentRoot(Plugin->GetBaseDir() / JNWorkbenchConstants::ResourcesDirectoryName / JNWorkbenchConstants::IconsDirectoryName);

	RegisterIcon(JNWorkbenchConstants::JoineryIconName, JNWorkbenchConstants::JoineryIconFile, JNWorkbenchConstants::Icon16);
	RegisterIcon(JNWorkbenchConstants::SkeletalRefitIconName, JNWorkbenchConstants::SkeletalRefitIconFile, JNWorkbenchConstants::Icon16);
	RegisterIcon(JNWorkbenchConstants::JigIconName, JNWorkbenchConstants::JigIconFile, JNWorkbenchConstants::Icon16);
	RegisterIcon(JNWorkbenchConstants::JigThumbnailName, JNWorkbenchConstants::JigIconFile, JNWorkbenchConstants::Icon64);
	RegisterIcon(JNWorkbenchConstants::MapIconName, JNWorkbenchConstants::MapIconFile, JNWorkbenchConstants::Icon16);
	RegisterIcon(JNWorkbenchConstants::AppendIconName, JNWorkbenchConstants::AppendIconFile, JNWorkbenchConstants::Icon16);
	RegisterIcon(JNWorkbenchConstants::MergeIconName, JNWorkbenchConstants::MergeIconFile, JNWorkbenchConstants::Icon16);
	RegisterIcon(JNWorkbenchConstants::DiscardIconName, JNWorkbenchConstants::DiscardIconFile, JNWorkbenchConstants::Icon16);
	RegisterIcon(JNWorkbenchConstants::ConvertIconName, JNWorkbenchConstants::ConvertIconFile, JNWorkbenchConstants::Icon16);
	RegisterIcon(JNWorkbenchConstants::AutoFillIconName, JNWorkbenchConstants::AutoFillIconFile, JNWorkbenchConstants::Icon16);

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
}

void FJNWorkbenchStyle::Shutdown()
{
	if (!StyleSet.IsValid()) return;

	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
	StyleSet.Reset();
}

const ISlateStyle& FJNWorkbenchStyle::Get()
{
	return *StyleSet;
}

FName FJNWorkbenchStyle::GetStyleSetName()
{
	return JNWorkbenchConstants::StyleName;
}

const FSlateBrush* FJNWorkbenchStyle::GetBrush(const FName IconName)
{
	if (!StyleSet.IsValid()) return nullptr;

	return StyleSet->GetBrush(IconName);
}

void FJNWorkbenchStyle::RegisterIcon(const FName IconName, const TCHAR* FileName, const FVector2f& Size)
{
	StyleSet->Set(IconName, new FSlateVectorImageBrush(StyleSet->RootToContentDir(FileName, JNWorkbenchConstants::IconFileExtension), Size));
}
