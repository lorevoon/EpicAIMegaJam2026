#include "DCTitleGameMode.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/SlateApplication.h"

ADCTitleGameMode::ADCTitleGameMode()
{
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
}

void ADCTitleGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (!GetWorld()->GetGameViewport()) return;
	Menu = SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.012f, 0.018f, 0.027f))
		.HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
			[
				SNew(SBox).WidthOverride(820).HeightOverride(700)
				[
					SAssignNew(Page, SVerticalBox)
				]
			]
		];
	GetWorld()->GetGameViewport()->AddViewportWidgetContent(Menu.ToSharedRef(), 100);
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->bShowMouseCursor = true;
		FInputModeUIOnly Input;
		Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Input);
	}
	ShowPage(false);
}

void ADCTitleGameMode::ShowPage(bool bCredits)
{
	Page->ClearChildren();
	const FLinearColor Gold(0.83f, 0.61f, 0.30f);
	auto Text = [this](const TCHAR* Label, int32 Size, FLinearColor Color, float Bottom)
	{
		Page->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, Bottom)
		[
			SNew(STextBlock).Text(FText::FromString(Label))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", Size)).ColorAndOpacity(Color)
		];
	};
	Page->AddSlot().FillHeight(1);
	Text(TEXT("A FILM BY YOU"), 16, Gold, 22);
	Text(TEXT("DIRECTOR'S CUT"), 64, FLinearColor::White, 14);
	Text(bCredits ? TEXT("C R E D I T S") : TEXT("Every frame tells your story."), 20, Gold, 50);
	if (bCredits)
	{
		Text(TEXT("Created by"), 18, FLinearColor(0.6f, 0.65f, 0.7f), 20);
		Text(TEXT("SeYeon Jung"), 32, FLinearColor::White, 14);
		Text(TEXT("Nayoung Lim"), 32, FLinearColor::White, 40);
	}
	auto Button = [this](const TCHAR* Label, TFunction<FReply()> Action)
	{
		TSharedPtr<SButton> Result;
		Page->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 16)
		[
			SNew(SBox).WidthOverride(320).HeightOverride(60)
			[
				SAssignNew(Result, SButton).HAlign(HAlign_Center).VAlign(VAlign_Center)
				.ButtonColorAndOpacity(FLinearColor(0.65f, 0.44f, 0.19f))
				.OnClicked_Lambda([Action]() { return Action(); })
				[
					SNew(STextBlock).Text(FText::FromString(Label))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
					.ColorAndOpacity(FLinearColor::White)
				]
			]
		];
		return Result;
	};
	TSharedPtr<SButton> Focus;
	if (bCredits)
	{
		Focus = Button(TEXT("Back"), [this]() { ShowPage(false); return FReply::Handled(); });
	}
	else
	{
		Focus = Button(TEXT("Start"), [this]()
		{
			if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
			{
				PC->SetInputMode(FInputModeGameOnly());
				PC->bShowMouseCursor = false;
			}
			UGameplayStatics::OpenLevel(this, TEXT("/Game/DirectorsCut/Levels/L_DC_Act1_Promise"));
			return FReply::Handled();
		});
		Button(TEXT("Credits"), [this]() { ShowPage(true); return FReply::Handled(); });
	}
	Page->AddSlot().FillHeight(1);
	FSlateApplication::Get().SetKeyboardFocus(Focus, EFocusCause::SetDirectly);
}

void ADCTitleGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Menu.IsValid() && GetWorld()->GetGameViewport())
	{
		GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(Menu.ToSharedRef());
	}
	Page.Reset();
	Menu.Reset();
	Super::EndPlay(EndPlayReason);
}
