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
	// Prevent redundant opens/closes
	if (IsOpen() != InIsOpen)
	{
		if (InIsOpen)
		{
			//Super::SetIsOpen(InIsOpen, bFocusMenu, FocusUserIndex);
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