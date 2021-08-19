#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SMenuAnchor.h"
/**
 * A PopupAnchor summons a Popup relative to its content.
 * Summoning a popup relative to the cursor is accomplished via the application.
 */
class UMGEXTENSION_API SMenuAnchorEx : public SMenuAnchor
{
public:
	//DECLARE_MULTICAST_DELEGATE_
	/**
	 * Open or close the popup
	 *
	 * @param InIsOpen    If true, open the popup. Otherwise close it.
	 * @param bFocusMenu  Should we focus the popup as soon as it opens?
	 */
	virtual void SetIsOpen(bool InIsOpen, const bool bFocusMenu = true, const int32 FocusUserIndex = 0) override;
private:
	FPopupMethodReply QueryPopupMethod(const FWidgetPath& PathToQuery);
};
