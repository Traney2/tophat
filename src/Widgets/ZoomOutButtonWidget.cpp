/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "ZoomOutButtonWidget.hpp"
#include "UIGlobals.hpp"
#include "Look/ButtonLook.hpp"
#include "Look/IconLook.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Layout.hpp"
#include "Input/InputEvents.hpp"
#include "MainWindow.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Look/Look.hpp"
#include "UIState.hpp"
#include "Input/InputEvents.hpp"
#include "Widgets/MapOverlayButton.hpp"
#include "Screen/Canvas.hpp"
#include "Interface.hpp"

void
ZoomOutButtonWidget::Prepare(ContainerWindow &parent,
                              const PixelRect &rc)
{
  const IconLook &icon_look = CommonInterface::main_window->GetLook().icon;
  SetBitmap(&icon_look.hBmpZoomOutButton);
  OverlayButtonWidget::Prepare(parent, rc);
}

void
ZoomOutButtonWidget::Move(const PixelRect &rc_map)
{
  UPixelScalar clear_border_width = Layout::Scale(2);
  PixelRect rc;
  if (Layout::landscape) {
    rc.left = zoom_in->GetPosition().right;
    rc.right = rc.left + GetWidth() + 2 * clear_border_width;
    rc.bottom = rc_map.bottom;
    rc.top = rc.bottom - (GetHeight() + 2 * clear_border_width);
  } else {
    rc.left = rc_map.left;
    rc.right = rc.left + GetWidth() + 2 * clear_border_width;
    rc.bottom = rc_map.bottom;
    rc.top = rc.bottom - GetHeight() - 2 * clear_border_width;
  }

  WindowWidget::Move(rc);
}

UPixelScalar
ZoomOutButtonWidget::GetWidth() const
{
  return MapOverlayButton::GetStandardButtonHeight() *
      MapOverlayButton::GetScale();
}

UPixelScalar
ZoomOutButtonWidget::GetHeight() const
{
  return GetWidth();
}

void
ZoomOutButtonWidget::OnAction(int id)
{
  UISettings &ui_settings = CommonInterface::SetUISettings();
  ui_settings.clear_gesture_help = true;
  InputEvents::eventZoom(_T("-"));
  InputEvents::HideMenu();
}

void
ZoomOutButtonWidget::UpdateVisibility(const PixelRect &rc,
                                       bool is_panning,
                                       bool is_main_window_widget,
                                       bool is_map)
{
  if (is_map && !is_main_window_widget)
    Show(rc);
  else
    Hide();
}

UPixelScalar
ZoomOutButtonWidget::HeightFromBottomLeft()
{
  return GetHeight() + Layout::Scale(3);
}
