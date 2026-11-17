#include "Window/SJNSkeletalRefitWindow.h"

#include "Asset/JNSkeletalRefitJig.h"
#include "Common/Constant/JNSkeletalRefitConstants.h"
#include "Common/Constant/JNWorkbenchConstants.h"
#include "Engine/SkeletalMesh.h"
#include "IDetailsView.h"
#include "JNWorkbenchStyle.h"
#include "Library/JNSkeletalRefitFunctionLibrary.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Struct/JNSkeletalRefitResult.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Window/JNSkeletalRefitRequest.h"

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

	ChildSlot
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
				SNew(SButton)
				.ToolTipText(LOCTEXT("AutoFillToolTip", "Add a row to the jig for every bone of the first mesh that is not listed yet. Same-named bones are mapped, other weighted bones are appended, the rest are discarded."))
				.OnClicked(this, &SJNSkeletalRefitWindow::HandleAutoFillClicked)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, JNSkeletalRefitConstants::WindowPadding, 0.0f)
					[
						SNew(SImage)
						.Image(FJNWorkbenchStyle::GetBrush(JNWorkbenchConstants::AutoFillIconName))
						.ColorAndOpacity(FSlateColor::UseForeground())
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("AutoFillLabel", "Auto Fill Jig"))
					]
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.ToolTipText(LOCTEXT("RefitToolTip", "Duplicate every mesh and rebind the copy to the target skeleton of the jig. Source meshes are left untouched; new assets are not saved automatically."))
				.OnClicked(this, &SJNSkeletalRefitWindow::HandleRefitClicked)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, JNSkeletalRefitConstants::WindowPadding, 0.0f)
					[
						SNew(SImage)
						.Image(FJNWorkbenchStyle::GetBrush(JNWorkbenchConstants::ConvertIconName))
						.ColorAndOpacity(FSlateColor::UseForeground())
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("RefitLabel", "Refit"))
					]
				]
			]
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
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

FReply SJNSkeletalRefitWindow::HandleAutoFillClicked()
{
	if (!IsValid(Request) || !IsValid(Request->Jig) || Request->Meshes.IsEmpty() || !IsValid(Request->Meshes[0]))
	{
		ResultText = LOCTEXT("AutoFillNeedsInput", "Pick a jig and at least one mesh first.");
		return FReply::Handled();
	}
	if (!IsValid(Request->Jig->TargetSkeleton))
	{
		ResultText = LOCTEXT("AutoFillNeedsTarget", "The jig has no target skeleton.");
		return FReply::Handled();
	}

	const int32 AddedCount = UJNSkeletalRefitFunctionLibrary::AutoFillJig(Request->Jig, Request->Meshes[0]);
	ResultText = FText::Format(LOCTEXT("AutoFillDone", "Added {0} row(s) to '{1}' from '{2}'. Review them in the jig asset."), FText::AsNumber(AddedCount), FText::FromString(Request->Jig->GetName()), FText::FromString(Request->Meshes[0]->GetName()));
	return FReply::Handled();
}

FReply SJNSkeletalRefitWindow::HandleRefitClicked()
{
	if (!IsValid(Request) || !IsValid(Request->Jig) || Request->Meshes.IsEmpty())
	{
		ResultText = LOCTEXT("RefitNeedsInput", "Pick a jig and at least one mesh first.");
		return FReply::Handled();
	}

	FTextBuilder Builder;
	for (USkeletalMesh* Mesh : Request->Meshes)
	{
		if (!IsValid(Mesh)) continue;

		FJNSkeletalRefitResult Result;
		const USkeletalMesh* NewMesh = UJNSkeletalRefitFunctionLibrary::RefitMesh(Request->Jig, Mesh, Request->OutputFolder.Path, Request->OutputSuffix, Result);
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
