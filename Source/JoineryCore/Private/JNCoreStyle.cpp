#include "JNCoreStyle.h"

#include "Brushes/SlateImageBrush.h"
#include "Common/Constant/JNCoreConstants.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FSlateStyleSet> FJNCoreStyle::StyleSet;

void FJNCoreStyle::Initialize()
{
	if (StyleSet.IsValid()) return;

	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(JNCoreConstants::PluginName);
	if (!Plugin.IsValid()) return;

	StyleSet = MakeShared<FSlateStyleSet>(JNCoreConstants::StyleName);
	StyleSet->SetContentRoot(Plugin->GetBaseDir() / JNCoreConstants::ResourcesDirectoryName / JNCoreConstants::IconsDirectoryName);

	RegisterIcon(JNCoreConstants::JoineryIconName, JNCoreConstants::JoineryIconFile, JNCoreConstants::Icon16);
	RegisterIcon(JNCoreConstants::SkeletalRefitIconName, JNCoreConstants::SkeletalRefitIconFile, JNCoreConstants::Icon16);
	RegisterIcon(JNCoreConstants::ProfileIconName, JNCoreConstants::ProfileIconFile, JNCoreConstants::Icon16);
	RegisterIcon(JNCoreConstants::ProfileThumbnailName, JNCoreConstants::ProfileIconFile, JNCoreConstants::Icon64);
	RegisterIcon(JNCoreConstants::MapIconName, JNCoreConstants::MapIconFile, JNCoreConstants::Icon16);
	RegisterIcon(JNCoreConstants::AppendIconName, JNCoreConstants::AppendIconFile, JNCoreConstants::Icon16);
	RegisterIcon(JNCoreConstants::MergeIconName, JNCoreConstants::MergeIconFile, JNCoreConstants::Icon16);
	RegisterIcon(JNCoreConstants::DiscardIconName, JNCoreConstants::DiscardIconFile, JNCoreConstants::Icon16);
	RegisterIcon(JNCoreConstants::ConvertIconName, JNCoreConstants::ConvertIconFile, JNCoreConstants::Icon16);
	RegisterIcon(JNCoreConstants::AutoFillIconName, JNCoreConstants::AutoFillIconFile, JNCoreConstants::Icon16);

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
}

void FJNCoreStyle::Shutdown()
{
	if (!StyleSet.IsValid()) return;

	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
	StyleSet.Reset();
}

const ISlateStyle& FJNCoreStyle::Get()
{
	return *StyleSet;
}

FName FJNCoreStyle::GetStyleSetName()
{
	return JNCoreConstants::StyleName;
}

const FSlateBrush* FJNCoreStyle::GetBrush(const FName IconName)
{
	if (!StyleSet.IsValid()) return nullptr;

	return StyleSet->GetBrush(IconName);
}

void FJNCoreStyle::RegisterIcon(const FName IconName, const TCHAR* FileName, const FVector2f& Size)
{
	StyleSet->Set(IconName, new FSlateVectorImageBrush(StyleSet->RootToContentDir(FileName, JNCoreConstants::IconFileExtension), Size));
}
