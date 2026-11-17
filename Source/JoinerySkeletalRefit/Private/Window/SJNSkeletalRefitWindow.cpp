#include "Window/SJNSkeletalRefitWindow.h"

#include "Animation/AnimationAsset.h"
#include "Animation/Skeleton.h"
#include "Asset/JNSkeletalRefitProfile.h"
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
	const TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
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
				DetailsView
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
						LOCTEXT("AutoFillLabel", "Auto Fill Profile"),
						LOCTEXT("AutoFillToolTip", "Add a row to the profile for every bone of the first mesh that is not listed yet. Same-named bones are mapped, other weighted bones are appended, the rest are discarded."),
						&SJNSkeletalRefitWindow::HandleAutoFillClicked)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, JNSkeletalRefitConstants::WindowPadding, 0.0f)
				[
					MakeButton(
						FJNCoreStyle::GetBrush(JNCoreConstants::SkeletalRefitIconName),
						LOCTEXT("PreviewLabel", "Preview"),
						LOCTEXT("PreviewToolTip", "Refit temporary copies of the meshes with the current profile and show them in the viewport. No asset is created or changed."),
						&SJNSkeletalRefitWindow::HandlePreviewClicked)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					MakeButton(
						FJNCoreStyle::GetBrush(JNCoreConstants::ConvertIconName),
						LOCTEXT("RefitLabel", "Refit"),
						LOCTEXT("RefitToolTip", "Duplicate every mesh and rebind the copy to the target skeleton of the profile. Source meshes are left untouched; new assets are not saved automatically."),
						&SJNSkeletalRefitWindow::HandleRefitClicked)
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

void SJNSkeletalRefitWindow::HandleRequestChanged(const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (IsValid(Request) && Viewport.IsValid()) Viewport->SetShowBones(Request->bShowBones);
}

FReply SJNSkeletalRefitWindow::HandleAutoFillClicked()
{
	if (!IsValid(Request) || !IsValid(Request->Profile) || Request->Meshes.IsEmpty() || !IsValid(Request->Meshes[0]))
	{
		ResultText = LOCTEXT("AutoFillNeedsInput", "Pick a profile and at least one mesh first.");
		return FReply::Handled();
	}
	if (!IsValid(Request->Profile->TargetSkeleton))
	{
		ResultText = LOCTEXT("AutoFillNeedsTarget", "The profile has no target skeleton.");
		return FReply::Handled();
	}

	const int32 AddedCount = UJNSkeletalRefitFunctionLibrary::AutoFillProfile(Request->Profile, Request->Meshes[0]);
	ResultText = FText::Format(LOCTEXT("AutoFillDone", "Added {0} row(s) to '{1}' from '{2}'. Review them in the profile asset."), FText::AsNumber(AddedCount), FText::FromString(Request->Profile->GetName()), FText::FromString(Request->Meshes[0]->GetName()));
	return FReply::Handled();
}

FReply SJNSkeletalRefitWindow::HandlePreviewClicked()
{
	if (!IsValid(Request) || !IsValid(Request->Profile) || Request->Meshes.IsEmpty() || !Viewport.IsValid())
	{
		ResultText = LOCTEXT("PreviewNeedsInput", "Pick a profile and at least one mesh first.");
		return FReply::Handled();
	}

	USkeleton* PreviewSkeleton = UJNSkeletalRefitFunctionLibrary::MakePreviewSkeleton(Request->Profile);
	if (!IsValid(PreviewSkeleton))
	{
		ResultText = LOCTEXT("PreviewNeedsTarget", "The profile has no target skeleton.");
		return FReply::Handled();
	}

	// 同批网格共用一份临时骨架，各自追加的骨骼都并进它
	FTextBuilder Builder;
	TArray<USkeletalMesh*> PreviewMeshes;
	for (USkeletalMesh* Mesh : Request->Meshes)
	{
		if (!IsValid(Mesh)) continue;

		FJNSkeletalRefitResult Result;
		USkeletalMesh* PreviewMesh = UJNSkeletalRefitFunctionLibrary::MakePreviewMesh(Request->Profile, Mesh, PreviewSkeleton, Result);
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
	return FReply::Handled();
}

FReply SJNSkeletalRefitWindow::HandleRefitClicked()
{
	if (!IsValid(Request) || !IsValid(Request->Profile) || Request->Meshes.IsEmpty())
	{
		ResultText = LOCTEXT("RefitNeedsInput", "Pick a profile and at least one mesh first.");
		return FReply::Handled();
	}

	FTextBuilder Builder;
	for (USkeletalMesh* Mesh : Request->Meshes)
	{
		if (!IsValid(Mesh)) continue;

		FJNSkeletalRefitResult Result;
		const USkeletalMesh* NewMesh = UJNSkeletalRefitFunctionLibrary::RefitMesh(Request->Profile, Mesh, Request->OutputFolder.Path, Request->OutputSuffix, Result);
		if (Result.bSucceeded && IsValid(NewMesh))
		{
			Builder.AppendLineFormat(LOCTEXT("RefitSucceeded", "{0}: refitted as {1}. Save the new mesh and the target skeleton."), FText::FromString(Mesh->GetName()), FText::FromString(NewMesh->GetPathName()));
		}
		else
		{
			Builder.AppendLineFormat(LOCTEXT("RefitFailed", "{0}: failed. {1}"), FText::FromString(Mesh->GetName()), Result.Error);
		}

		for (const FText& Warning : Result.Warnings)
		{
			Builder.AppendLineFormat(LOCTEXT("RefitWarning", "    Warning: {0}"), Warning);
		}
	}

	ResultText = Builder.ToText();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
