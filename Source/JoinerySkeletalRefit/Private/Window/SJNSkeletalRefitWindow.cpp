#include "Window/SJNSkeletalRefitWindow.h"

#include "Animation/AnimationAsset.h"
#include "Animation/Skeleton.h"
#include "Common/Constant/JNCoreConstants.h"
#include "Common/Constant/JNSkeletalRefitConstants.h"
#include "Engine/SkeletalMesh.h"
#include "IDetailsView.h"
#include "JNCoreStyle.h"
#include "Library/JNSkeletalRefitFunctionLibrary.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Struct/JNSkeletalRefitResult.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Window/JNSkeletalRefitRequest.h"
#include "Window/SJNSkeletalRefitViewport.h"

#define LOCTEXT_NAMESPACE "SJNSkeletalRefitWindow"

void SJNSkeletalRefitWindow::Construct(const FArguments& InArgs)
{
	Request = NewObject<UJNSkeletalRefitRequest>(GetTransientPackage(), NAME_None, RF_Transient);
	Request->OutputSuffix = JNSkeletalRefitConstants::DefaultOutputSuffix;

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(JNSkeletalRefitConstants::PropertyEditorModuleName);
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(Request);
	DetailsView->OnFinishedChangingProperties().AddSP(this, &SJNSkeletalRefitWindow::HandleRequestChanged);

	ChildSlot
	[
		SNew(SSplitter)
		.Orientation(Orient_Horizontal)

		+ SSplitter::Slot()
		.Value(JNSkeletalRefitConstants::WindowParameterRatio)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(JNSkeletalRefitConstants::WindowPadding)
			[
				DetailsView.ToSharedRef()
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(JNSkeletalRefitConstants::WindowPadding)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, JNSkeletalRefitConstants::WindowPadding, 0.0f)
				[
					MakeButton(
						FJNCoreStyle::GetBrush(JNCoreConstants::AutoFillIconName),
						LOCTEXT("ResetBonesLabel", "Reset Bones"),
						LOCTEXT("ResetBonesToolTip", "Rebuild the bone list from the meshes with the automatic rule, discarding manual edits. Same-named bones are mapped, other weighted bones are appended, the rest are discarded."),
						&SJNSkeletalRefitWindow::HandleResetBonesClicked)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, JNSkeletalRefitConstants::WindowPadding, 0.0f)
				[
					MakeButton(
						FJNCoreStyle::GetBrush(JNCoreConstants::SkeletalRefitIconName),
						LOCTEXT("PreviewLabel", "Preview"),
						LOCTEXT("PreviewToolTip", "Refit temporary copies of the meshes with the current settings and show them in the viewport. No asset is created or changed."),
						&SJNSkeletalRefitWindow::HandlePreviewClicked)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					MakeButton(
						FJNCoreStyle::GetBrush(JNCoreConstants::ConvertIconName),
						LOCTEXT("ExportLabel", "Export"),
						LOCTEXT("ExportToolTip", "Duplicate every mesh and rebind the copy to the target skeleton. Source meshes are left untouched; new assets are not saved automatically."),
						&SJNSkeletalRefitWindow::HandleExportClicked)
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.MaxHeight(JNSkeletalRefitConstants::ResultMaxHeight)
			.Padding(JNSkeletalRefitConstants::WindowPadding)
			[
				SNew(SScrollBox)

				+ SScrollBox::Slot()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text(this, &SJNSkeletalRefitWindow::GetResultText)
				]
			]
		]

		+ SSplitter::Slot()
		[
			SAssignNew(Viewport, SJNSkeletalRefitViewport)
		]
	];
}

void SJNSkeletalRefitWindow::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(Request);
}

FString SJNSkeletalRefitWindow::GetReferencerName() const
{
	return TEXT("SJNSkeletalRefitWindow");
}

FText SJNSkeletalRefitWindow::GetResultText() const
{
	return ResultText;
}

TSharedRef<SWidget> SJNSkeletalRefitWindow::MakeButton(const FSlateBrush* Icon, const FText& Label, const FText& ToolTip, FReply (SJNSkeletalRefitWindow::*Handler)())
{
	return SNew(SButton)
		.ToolTipText(ToolTip)
		.OnClicked(this, Handler)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, JNSkeletalRefitConstants::WindowPadding, 0.0f)
			[
				SNew(SImage)
				.Image(Icon)
				.ColorAndOpacity(FSlateColor::UseForeground())
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Label)
			]
		];
}

void SJNSkeletalRefitWindow::FillBones(const bool bReset)
{
	if (!IsValid(Request)) return;

	int32 AddedCount = 0;
	if (bReset)
	{
		AddedCount -= Request->Settings.Bones.Num();
		Request->Settings.Bones.Reset();
	}
	for (const USkeletalMesh* Mesh : Request->Meshes)
	{
		AddedCount += UJNSkeletalRefitFunctionLibrary::FillBones(Request->Settings, Mesh);
	}

	// 骨骼处理表是在面板之外改的，改了才需要让面板重建
	if (AddedCount != 0 && DetailsView.IsValid()) DetailsView->ForceRefresh();
}

void SJNSkeletalRefitWindow::HandleRequestChanged(const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (!IsValid(Request)) return;

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	const FName MemberName = PropertyChangedEvent.GetMemberPropertyName();

	// 只影响显示或输出位置的选项不用重新转换
	if (MemberName == GET_MEMBER_NAME_CHECKED(UJNSkeletalRefitRequest, bShowBones))
	{
		if (Viewport.IsValid()) Viewport->SetShowBones(Request->bShowBones);
		return;
	}
	if (MemberName == GET_MEMBER_NAME_CHECKED(UJNSkeletalRefitRequest, PreviewAnimation))
	{
		if (Viewport.IsValid()) Viewport->SetPreviewAnimation(Request->PreviewAnimation);
		return;
	}
	if (MemberName == GET_MEMBER_NAME_CHECKED(UJNSkeletalRefitRequest, OutputFolder) || MemberName == GET_MEMBER_NAME_CHECKED(UJNSkeletalRefitRequest, OutputSuffix)) return;

	// 换了目标骨架，同名规则的结果全变，整表重来；网格变了只补缺的行
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FJNSkeletalRefitSettings, TargetSkeleton))
	{
		FillBones(true);
	}
	else if (MemberName == GET_MEMBER_NAME_CHECKED(UJNSkeletalRefitRequest, Meshes))
	{
		FillBones(false);
	}

	// 其余都是会改变转换结果的设置，预览跟着重做
	RefreshPreview();
}

void SJNSkeletalRefitWindow::RefreshPreview()
{
	if (!IsValid(Request) || !Viewport.IsValid()) return;

	// 设置还没填全时清空视口
	TArray<USkeletalMesh*> PreviewMeshes;
	const bool bHasMesh = Request->Meshes.ContainsByPredicate([](const TObjectPtr<USkeletalMesh>& Mesh) { return IsValid(Mesh); });
	if (!IsValid(Request->Settings.TargetSkeleton) || !bHasMesh)
	{
		Viewport->SetPreviewMeshes(PreviewMeshes, nullptr);
		ResultText = LOCTEXT("NeedsInput", "Pick a target skeleton and at least one mesh first.");
		return;
	}

	// 同批网格共用一份临时骨架，各自追加的骨骼都并进它
	USkeleton* PreviewSkeleton = UJNSkeletalRefitFunctionLibrary::MakePreviewSkeleton(Request->Settings);
	if (!IsValid(PreviewSkeleton))
	{
		Viewport->SetPreviewMeshes(PreviewMeshes, nullptr);
		ResultText = LOCTEXT("PreviewSkeletonFailed", "Failed to prepare the preview skeleton.");
		return;
	}

	FTextBuilder Builder;
	for (USkeletalMesh* Mesh : Request->Meshes)
	{
		if (!IsValid(Mesh)) continue;

		FJNSkeletalRefitResult Result;
		USkeletalMesh* PreviewMesh = UJNSkeletalRefitFunctionLibrary::MakePreviewMesh(Request->Settings, Mesh, PreviewSkeleton, Result);
		if (Result.bSucceeded && IsValid(PreviewMesh))
		{
			PreviewMeshes.Add(PreviewMesh);
			Builder.AppendLineFormat(LOCTEXT("PreviewSucceeded", "{0}: preview ready."), FText::FromString(Mesh->GetName()));
		}
		else
		{
			Builder.AppendLineFormat(LOCTEXT("PreviewFailed", "{0}: preview failed. {1}"), FText::FromString(Mesh->GetName()), Result.Error);
		}

		for (const FText& Warning : Result.Warnings)
		{
			Builder.AppendLineFormat(LOCTEXT("PreviewWarning", "    Warning: {0}"), Warning);
		}
	}

	Viewport->SetShowBones(Request->bShowBones);
	Viewport->SetPreviewMeshes(PreviewMeshes, Request->PreviewAnimation);
	ResultText = Builder.ToText();
}

FReply SJNSkeletalRefitWindow::HandleResetBonesClicked()
{
	if (!IsValid(Request) || !IsValid(Request->Settings.TargetSkeleton) || Request->Meshes.IsEmpty())
	{
		ResultText = LOCTEXT("NeedsInput", "Pick a target skeleton and at least one mesh first.");
		return FReply::Handled();
	}

	FillBones(true);
	RefreshPreview();
	return FReply::Handled();
}

FReply SJNSkeletalRefitWindow::HandlePreviewClicked()
{
	RefreshPreview();
	return FReply::Handled();
}

FReply SJNSkeletalRefitWindow::HandleExportClicked()
{
	if (!IsValid(Request) || !IsValid(Request->Settings.TargetSkeleton) || Request->Meshes.IsEmpty())
	{
		ResultText = LOCTEXT("NeedsInput", "Pick a target skeleton and at least one mesh first.");
		return FReply::Handled();
	}

	FTextBuilder Builder;
	for (USkeletalMesh* Mesh : Request->Meshes)
	{
		if (!IsValid(Mesh)) continue;

		FJNSkeletalRefitResult Result;
		const USkeletalMesh* NewMesh = UJNSkeletalRefitFunctionLibrary::RefitMesh(Request->Settings, Mesh, Request->OutputFolder.Path, Request->OutputSuffix, Result);
		if (Result.bSucceeded && IsValid(NewMesh))
		{
			Builder.AppendLineFormat(LOCTEXT("ExportSucceeded", "{0}: exported as {1}. Save the new mesh and the target skeleton."), FText::FromString(Mesh->GetName()), FText::FromString(NewMesh->GetPathName()));
		}
		else
		{
			Builder.AppendLineFormat(LOCTEXT("ExportFailed", "{0}: failed. {1}"), FText::FromString(Mesh->GetName()), Result.Error);
		}

		for (const FText& Warning : Result.Warnings)
		{
			Builder.AppendLineFormat(LOCTEXT("ExportWarning", "    Warning: {0}"), Warning);
		}
	}

	ResultText = Builder.ToText();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
