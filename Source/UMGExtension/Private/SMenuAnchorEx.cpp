#include "SMenuAnchorEx.h"
#include "Layout/ArrangedChildren.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SWindow.h"
#include "Layout/WidgetPath.h"
#include "Slate/Private/Framework/Application/Menu.h"
#include "Framework/Application/MenuStack.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/LayoutUtils.h"

#if WITH_EDITOR
#include "Framework/MultiBox/MultiBox.h"
#include "Framework/MultiBox/ToolMenuBase.h"
#endif

FPopupMethodReply SMenuAnchorEx::QueryPopupMethod(const FWidgetPath& PathToQuery)
{
	for (int32 WidgetIndex = PathToQuery.Widgets.Num() - 1; WidgetIndex >= 0; --WidgetIndex)
	{
		FPopupMethodReply PopupMethod = PathToQuery.Widgets[WidgetIndex].Widget->OnQueryPopupMethod();
		if (PopupMethod.IsEventHandled())
		{
			return PopupMethod;
		}
	}

	return FPopupMethodReply::UseMethod(EPopupMethod::CreateNewWindow);
}


void SMenuAnchorEx::SetIsOpen(bool InIsOpen, const bool bFocusMenu, const int32 FocusUserIndex)
{
	UE_LOG(LogTemp, Log, TEXT("SetIsOpen"));
	// Prevent redundant opens/closes
	if (IsOpen() != InIsOpen)
	{
		if (InIsOpen)
		{
			//Super::SetIsOpen(InIsOpen, bFocusMenu, FocusUserIndex);
			if (OnGetMenuContent.IsBound())
			{
				SetMenuContent(OnGetMenuContent.Execute());
			}

			if (MenuContent.IsValid())
			{
				// OPEN POPUP
				if (OnMenuOpenChanged.IsBound())
				{
					OnMenuOpenChanged.Execute(true);
				}

				// Figure out where the menu anchor is on the screen, so we can set the initial position of our pop-up window
				// This can be called at any time so we use the push menu override that explicitly allows us to specify our parent
				// NOTE: Careful, GeneratePathToWidget can be reentrant in that it can call visibility delegates and such
				FWidgetPath MyWidgetPath;
				FSlateApplication::Get().GeneratePathToWidgetUnchecked(AsShared(), MyWidgetPath);
				if (MyWidgetPath.IsValid())
				{
					const FGeometry& MyGeometry = MyWidgetPath.Widgets.Last().Geometry;
					const float LayoutScaleMultiplier = MyGeometry.GetAccumulatedLayoutTransform().GetScale();

					SlatePrepass(LayoutScaleMultiplier);

					// Figure out how big the content widget is so we can set the window's initial size properly
					TSharedRef< SWidget > MenuContentRef = MenuContent.ToSharedRef();
					MenuContentRef->SlatePrepass(LayoutScaleMultiplier);

					// Combo-boxes never size down smaller than the widget that spawned them, but all
					// other pop-up menus are currently auto-sized
					const FVector2D DesiredContentSize = MenuContentRef->GetDesiredSize();  // @todo slate: This is ignoring any window border size!
					const EMenuPlacement PlacementMode = Placement.Get();

					const FVector2D NewPosition = MyGeometry.AbsolutePosition;
					FVector2D NewWindowSize = DesiredContentSize;

					FPopupTransitionEffect TransitionEffect(FPopupTransitionEffect::None);
					if (PlacementMode == MenuPlacement_ComboBox || PlacementMode == MenuPlacement_ComboBoxRight)
					{
						TransitionEffect = FPopupTransitionEffect(FPopupTransitionEffect::ComboButton);
						NewWindowSize = FVector2D(FMath::Max(MyGeometry.Size.X, DesiredContentSize.X), DesiredContentSize.Y);
					}
					else if (PlacementMode == MenuPlacement_BelowAnchor)
					{
						TransitionEffect = FPopupTransitionEffect(FPopupTransitionEffect::TopMenu);
					}
					else if (PlacementMode == MenuPlacement_MenuRight)
					{
						TransitionEffect = FPopupTransitionEffect(FPopupTransitionEffect::SubMenu);
						NewWindowSize = MyGeometry.GetAbsoluteSize();
					}

					MethodInUse = Method.IsSet()
						? FPopupMethodReply::UseMethod(Method.GetValue())
						: QueryPopupMethod(MyWidgetPath);

					// "Normal" menus are created and managed by the application's menu stack functions
					if (bUseApplicationMenuStack)
					{
						if (MethodInUse.GetPopupMethod() == EPopupMethod::CreateNewWindow)
						{
							// Open the pop-up
							TSharedPtr<IMenu> NewMenu = FSlateApplication::Get().PushMenu(AsShared(), MyWidgetPath, MenuContentRef, NewPosition, TransitionEffect, bFocusMenu, NewWindowSize, MethodInUse.GetPopupMethod(), bIsCollapsedByParent);

							if (ensure(NewMenu.IsValid()))
							{
#if WITH_EDITOR
								// Reporting more information for UE-81655
								if (!NewMenu->GetOwnedWindow().IsValid())
								{
									UE_LOG(LogSlate, Error, TEXT("!NewMenu->GetOwnedWindow().IsValid(), WidgetPath:\n%s"), *MyWidgetPath.ToString());

									static const FName SMultiBoxWidgetTypeName = "SMultiBoxWidget";
									for (int32 WidgetIndex = MyWidgetPath.Widgets.Num() - 1; WidgetIndex >= 0; --WidgetIndex)
									{
										if (MyWidgetPath.Widgets[WidgetIndex].Widget->GetType() == SMultiBoxWidgetTypeName)
										{
											TSharedRef<SMultiBoxWidget> MultiBoxWidget = StaticCastSharedRef<SMultiBoxWidget>(MyWidgetPath.Widgets[WidgetIndex].Widget);
											TSharedRef<const FMultiBox> MultiBox = MultiBoxWidget->GetMultiBox();
											FString BlockText;
											const TArray< TSharedRef< const FMultiBlock > >& Blocks = MultiBox->GetBlocks();
											for (int32 BlockIndex = 0; BlockIndex < Blocks.Num(); ++BlockIndex)
											{
												const bool bIsFinalBlock = (BlockIndex == (Blocks.Num() - 1));
												const TSharedRef< const FMultiBlock >& Block = Blocks[BlockIndex];
												BlockText += FString::Printf(TEXT("%s (%d)%s"), *Block->GetExtensionHook().ToString(), (int32)Block->GetType(), bIsFinalBlock ? TEXT("") : TEXT(", "));
											}
											UE_LOG(LogSlate, Error, TEXT(" Blocks: %s"), *BlockText);
											break;
										}
									}

									UE_LOG(LogSlate, Error, TEXT(" MenuContentRef: %s"), *MenuContentRef->ToString());
								}
#endif
								if (NewMenu->GetOwnedWindow().IsValid())
								{
									PopupMenuPtr = NewMenu;
									NewMenu->GetOnMenuDismissed().AddSP(this, &SMenuAnchorEx::OnMenuClosed);
									PopupWindowPtr = NewMenu->GetOwnedWindow();
								}
								else
								{
									UE_LOG(LogSlate, Error, TEXT(" Menu '%s' could not open '%s'"), *ToString(), *MenuContentRef->ToString());
									if (TSharedPtr<IMenu> Pinned = PopupMenuPtr.Pin())
									{
										Pinned->Dismiss();
									}

									ResetPopupMenuContent();
								}
							}
						}
						else
						{
							// We are re-using the current window instead of creating a new one.
							// The popup will be presented as a child of this widget.
							ensure(MethodInUse.GetPopupMethod() == EPopupMethod::UseCurrentWindow);
							// We get the deepest window so that it works correctly inside a widget component
							// though we may need to come up with a more complex setup if we ever need
							// parents to be in a virtual window, but not the popup.
							PopupWindowPtr = MyWidgetPath.GetDeepestWindow();

							if (bFocusMenu)
							{
								FSlateApplication::Get().ReleaseAllPointerCapture(FocusUserIndex);
							}

							TSharedRef<SMenuAnchor> SharedThis = StaticCastSharedRef<SMenuAnchor>(AsShared());

							TSharedPtr<IMenu> NewMenu = FSlateApplication::Get().PushHostedMenu(
								SharedThis, MyWidgetPath, SharedThis, MenuContentRef, WrappedContent, TransitionEffect, MethodInUse.GetShouldThrottle(), bIsCollapsedByParent);

							PopupMenuPtr = NewMenu;
							check(NewMenu.IsValid());
							//check(NewMenu->GetParentWindow().ToSharedRef() == PopupWindow);
							check(WrappedContent.IsValid());

							Children[1]
								[
									WrappedContent.ToSharedRef()
								];

							if (bFocusMenu)
							{
								FSlateApplication::Get().SetUserFocus(FocusUserIndex, MenuContentRef, EFocusCause::SetDirectly);
							}
						}
					}
					else // !bUseApplicationMenuStack
					{
						//// Anchor's menu doesn't participate in the application's menu stack.
						//// Lifetime is managed by this anchor
						//if (MethodInUse.GetPopupMethod() == EPopupMethod::CreateNewWindow)
						//{
						//	// Start pop-up windows out transparent, then fade them in over time
						//	const EWindowTransparency Transparency(EWindowTransparency::PerWindow);

						//	FSlateRect Anchor(NewPosition, NewPosition + MyGeometry.GetLocalSize());
						//	EOrientation Orientation = (TransitionEffect.SlideDirection == FPopupTransitionEffect::SubMenu) ? Orient_Horizontal : Orient_Vertical;

						//	// @todo slate: Assumes that popup is not Scaled up or down from application scale.
						//	MenuContentRef->SlatePrepass(FSlateApplication::Get().GetApplicationScale());
						//	// @todo slate: Doesn't take into account potential window border size
						//	FVector2D ExpectedSize = MenuContentRef->GetDesiredSize();

						//	// already handled
						//	const bool bAutoAdjustForDPIScale = false;

						//	const FVector2D ScreenPosition = FSlateApplication::Get().CalculatePopupWindowPosition(Anchor, ExpectedSize, bAutoAdjustForDPIScale, FVector2D::ZeroVector, Orientation);

						//	// Release the mouse so that context can be properly restored upon closing menus.  See CL 1411833 before changing this.
						//	if (bFocusMenu)
						//	{
						//		FSlateApplication::Get().ReleaseAllPointerCapture(FocusUserIndex);
						//	}

						//	// Create a new window for the menu
						//	TSharedRef<SWindow> NewMenuWindow = SNew(SWindow)
						//		.Type(EWindowType::Menu)
						//		.IsPopupWindow(true)
						//		.SizingRule(ESizingRule::Autosized)
						//		.ScreenPosition(ScreenPosition)
						//		.AutoCenter(EAutoCenter::None)
						//		.ClientSize(ExpectedSize)
						//		.InitialOpacity(1.0f)
						//		.SupportsTransparency(Transparency)
						//		.FocusWhenFirstShown(bFocusMenu)
						//		.ActivationPolicy(bFocusMenu ? EWindowActivationPolicy::Always : EWindowActivationPolicy::Never)
						//		[
						//			MenuContentRef
						//		];

						//	if (bFocusMenu)
						//	{
						//		// Focus the unwrapped content rather than just the window
						//		NewMenuWindow->SetWidgetToFocusOnActivate(MenuContentRef);
						//	}

						//	TSharedPtr<IMenu> NewMenu = MakeShareable(new FMenuInWindow(NewMenuWindow, MenuContentRef, bIsCollapsedByParent));
						//	FSlateApplication::Get().AddWindowAsNativeChild(NewMenuWindow, MyWidgetPath.GetWindow(), true);

						//	PopupMenuPtr = OwnedMenuPtr = NewMenu;
						//	check(NewMenu.IsValid());
						//	NewMenu->GetOnMenuDismissed().AddSP(this, &SMenuAnchor::OnMenuClosed);
						//	PopupWindowPtr = NewMenuWindow;
						//}
						//else
						//{
						//	// We are re-using the current window instead of creating a new one.
						//	// The popup will be presented as a child of this widget.
						//	ensure(MethodInUse.GetPopupMethod() == EPopupMethod::UseCurrentWindow);
						//	PopupWindowPtr = MyWidgetPath.GetWindow();

						//	if (bFocusMenu)
						//	{
						//		FSlateApplication::Get().ReleaseAllPointerCapture(FocusUserIndex);
						//	}

						//	TSharedRef<SMenuAnchor> SharedThis = StaticCastSharedRef<SMenuAnchor>(AsShared());
						//	TSharedPtr<IMenu> NewMenu = MakeShareable(new FMenuInHostWidget(SharedThis, MenuContentRef, bIsCollapsedByParent));

						//	PopupMenuPtr = OwnedMenuPtr = NewMenu;
						//	check(NewMenu.IsValid());
						//	//check(NewMenu->GetParentWindow().ToSharedRef() == PopupWindow);

						//	Children[1]
						//		[
						//			MenuContentRef
						//		];

						//	if (bFocusMenu)
						//	{
						//		FSlateApplication::Get().SetUserFocus(FocusUserIndex, MenuContentRef, EFocusCause::SetDirectly);
						//	}

						//	OpenApplicationMenus.Add(NewMenu);
						//}
					}
				}
			}

			Invalidate(EInvalidateWidget::ChildOrder | EInvalidateWidget::Volatility);
		}
		else
		{
			// CLOSE POPUP
			if (PopupMenuPtr.IsValid())
			{
				PopupMenuPtr.Pin()->Dismiss();
			}

			ResetPopupMenuContent();
		}
	}
}