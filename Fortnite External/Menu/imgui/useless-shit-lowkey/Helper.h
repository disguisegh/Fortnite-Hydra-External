#pragma once
#include <rpcndr.h>
#include <WinUser.h>
#include <rpcasync.h>
#include <wtypes.h>

boolean bIsInRectangle(double centerX, double centerY, double radius, double x, double y) {
	return x >= centerX - radius && x <= centerX + radius &&
		y >= centerY - radius && y <= centerY + radius;
}

void HideSystemCursor()
{

	SetCursor(NULL);
	SetCursor(LoadCursor(NULL, IDC_NO));
	while (ShowCursor(FALSE) >= 0);
}

void ShowSystemCursor()
{

	SetCursor(LoadCursor(NULL, IDC_ARROW));
	while (ShowCursor(TRUE) < 0);
}
