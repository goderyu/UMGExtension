// Fill out your copyright notice in the Description page of Project Settings.


#include "MenuAnchorEx.h"
#include "SMenuAnchorEx.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Blueprint/UserWidget.h"

#define LOCTEXT_NAMESPACE "UMGExtension"

/////////////////////////////////////////////////////
// UMenuAnchorEx

UMenuAnchorEx::UMenuAnchorEx(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ShouldDeferPaintingAfterWindowContent(true)
	, UseApplicationMenuStack(true)
{
	Placement = MenuPlacement_ComboBox;
	bFitInWindow = true;
}

void UMenuAnchorEx::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MyMenuAnchor.Reset();
}

TSharedRef<SWidget> UMenuAnchorEx::RebuildWidget()
{
	MyMenuAnchor = SNew(SMenuAnchorEx)
		.Placement(Placement)
		.FitInWindow(bFitInWindow)
		.OnGetMenuContent(BIND_UOBJECT_DELEGATE(FOnGetContent, HandleGetMenuContent))
		.OnMenuOpenChanged(BIND_UOBJECT_DELEGATE(FOnIsOpenChanged, HandleMenuOpenChanged))
		.ShouldDeferPaintingAfterWindowContent(ShouldDeferPaintingAfterWindowContent)
		.UseApplicationMenuStack(UseApplicationMenuStack);

	if (GetChildrenCount() > 0)
	{
		MyMenuAnchor->SetContent(GetContentSlot()->Content ? GetContentSlot()->Content->TakeWidget() : SNullWidget::NullWidget);
	}

	return MyMenuAnchor.ToSharedRef();
}

void UMenuAnchorEx::OnSlotAdded(UPanelSlot* InSlot)
{
	// Add the child to the live slot if it already exists
	if (MyMenuAnchor.IsValid())
	{
		MyMenuAnchor->SetContent(InSlot->Content ? InSlot->Content->TakeWidget() : SNullWidget::NullWidget);
	}
}

void UMenuAnchorEx::OnSlotRemoved(UPanelSlot* InSlot)
{
	// Remove the widget from the live slot if it exists.
	if (MyMenuAnchor.IsValid())
	{
		MyMenuAnchor->SetContent(SNullWidget::NullWidget);
	}
}

void UMenuAnchorEx::HandleMenuOpenChanged(bool bIsOpen)
{
	OnMenuOpenChanged.Broadcast(bIsOpen);
}

TSharedRef<SWidget> UMenuAnchorEx::HandleGetMenuContent()
{
	TSharedPtr<SWidget> SlateMenuWidget;

	if (OnGetUserMenuContentEvent.IsBound())
	{
		UWidget* MenuWidget = OnGetUserMenuContentEvent.Execute();
		if (MenuWidget)
		{
			SlateMenuWidget = MenuWidget->TakeWidget();
		}
	}
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	else if (OnGetMenuContentEvent.IsBound())
	{
		// Remove when OnGetMenuContentEvent is fully deprecated.
		UWidget* MenuWidget = OnGetMenuContentEvent.Execute();
		if (MenuWidget)
		{
			SlateMenuWidget = MenuWidget->TakeWidget();
		}
	}
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	else
	{
		if (MenuClass != nullptr && !MenuClass->HasAnyClassFlags(CLASS_Abstract))
		{
			if (UWorld* World = GetWorld())
			{
				if (UUserWidget* MenuWidget = CreateWidget(World, MenuClass))
				{
					SlateMenuWidget = MenuWidget->TakeWidget();
				}
			}
		}
	}

	return SlateMenuWidget.IsValid() ? SlateMenuWidget.ToSharedRef() : SNullWidget::NullWidget;
}

void UMenuAnchorEx::ToggleOpen(bool bFocusOnOpen)
{
	if (MyMenuAnchor.IsValid())
	{
		MyMenuAnchor->SetIsOpen(!MyMenuAnchor->IsOpen(), bFocusOnOpen);
	}
}

void UMenuAnchorEx::Open(bool bFocusMenu)
{
	if (MyMenuAnchor.IsValid() && !MyMenuAnchor->IsOpen())
	{
		MyMenuAnchor->SetIsOpen(true, bFocusMenu);
	}
}

void UMenuAnchorEx::Close()
{
	if (MyMenuAnchor.IsValid())
	{
		return MyMenuAnchor->SetIsOpen(false, false);
	}
}

bool UMenuAnchorEx::IsOpen() const
{
	if (MyMenuAnchor.IsValid())
	{
		return MyMenuAnchor->IsOpen();
	}

	return false;
}

void UMenuAnchorEx::SetPlacement(TEnumAsByte<EMenuPlacement> InPlacement)
{
	Placement = InPlacement;
	if (MyMenuAnchor.IsValid())
	{
		return MyMenuAnchor->SetMenuPlacement(Placement);
	}
}

void UMenuAnchorEx::FitInWindow(bool bFit)
{
	bFitInWindow = bFit;
	if (MyMenuAnchor.IsValid())
	{
		return MyMenuAnchor->SetFitInWindow(bFitInWindow);
	}
}

bool UMenuAnchorEx::ShouldOpenDueToClick() const
{
	if (MyMenuAnchor.IsValid())
	{
		return MyMenuAnchor->ShouldOpenDueToClick();
	}

	return false;
}

FVector2D UMenuAnchorEx::GetMenuPosition() const
{
	if (MyMenuAnchor.IsValid())
	{
		return MyMenuAnchor->GetMenuPosition();
	}

	return FVector2D(0, 0);
}

bool UMenuAnchorEx::HasOpenSubMenus() const
{
	if (MyMenuAnchor.IsValid())
	{
		return MyMenuAnchor->HasOpenSubMenus();
	}

	return false;
}

#if WITH_EDITOR

const FText UMenuAnchorEx::GetPaletteCategory()
{
	return LOCTEXT("Primitive", "Primitive");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
