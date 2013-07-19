/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "GlueMapWindow.hpp"
#include "Message.hpp"
#include "Input/InputEvents.hpp"
#include "Screen/Key.h"
#include "Screen/Layout.hpp"
#include "Simulator.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Components.hpp"
#include "Protection.hpp"
#include "Math/FastMath.h"
#include "Compiler.h"
#include "Interface.hpp"
#include "Pan.hpp"
#include "Util/Clamp.hpp"
#include "Asset.hpp"

#ifdef ENABLE_SDL
#include <SDL_keyboard.h>
#endif

#include <algorithm>

bool
GlueMapWindow::OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys)
{
  /* allow a bigger threshold on touch screens */
  const unsigned threshold = Layout::Scale(IsEmbedded() ? 50 : 10);
  if (drag_mode != DRAG_NONE && arm_mapitem_list &&
      (manhattan_distance(drag_start, RasterPoint{x, y}) > threshold ||
       mouse_down_clock.Elapsed() > 200))
    arm_mapitem_list = false;

  switch (drag_mode) {
  case DRAG_NONE:
    break;
  case DRAG_GESTURE:
    if ((abs(drag_start.x - x) + abs(drag_start.y - y)) < Layout::Scale(threshold)
        && follow_mode != FOLLOW_PAN)
      break;
    drag_projection = visible_projection;
    follow_mode = FOLLOW_PAN;

#ifdef HAVE_MULTI_TOUCH
  case DRAG_MULTI_TOUCH_PAN:
#endif
  case DRAG_PAN:
    visible_projection.SetGeoLocation(drag_projection.GetGeoLocation()
                                      + drag_start_geopoint
                                      - drag_projection.ScreenToGeo(x, y));
    QuickRedraw();

#ifdef ENABLE_OPENGL
    kinetic_x.MouseMove(x);
    kinetic_y.MouseMove(y);
#endif
    return true;

  case DRAG_SIMULATOR:
    return true;
  }

  return MapWindow::OnMouseMove(x, y, keys);
}

gcc_pure
static bool
IsCtrlKeyPressed()
{
#ifdef ENABLE_SDL
  return SDL_GetModState() & (KMOD_LCTRL|KMOD_RCTRL);
#elif defined(USE_GDI)
  return GetKeyState(VK_CONTROL) & 0x8000;
#else
  return false;
#endif
}

bool
GlueMapWindow::OnMouseDown(PixelScalar x, PixelScalar y)
{

#ifndef ENABLE_OPENGL
  if (ButtonOverlaysOnMouseDown(x, y))
    return true;
#endif

  map_item_timer.Cancel();

#ifdef ENABLE_OPENGL
  kinetic_timer.Cancel();
#endif

  // Ignore single click event if double click detected
  if (ignore_single_click || drag_mode != DRAG_NONE)
    return true;

  if (is_simulator() && IsCtrlKeyPressed() && visible_projection.IsValid()) {
    /* clicking with Alt key held moves the simulator to the click
       location instantly */
    const GeoPoint location = visible_projection.ScreenToGeo(x, y);
    device_blackboard->SetSimulatorLocation(location);
    return true;
  }

  mouse_down_clock.Update();
  arm_mapitem_list = HasFocus();

  SetFocus();

  if (!visible_projection.IsValid()) {
    gestures.Start(x, y, Layout::Scale(20));
    drag_mode = DRAG_GESTURE;
    SetCapture();
    return true;
  }

  drag_start.x = x;
  drag_start.y = y;
  drag_start_geopoint = visible_projection.ScreenToGeo(x, y);

  switch (follow_mode) {
  case FOLLOW_SELF:
    break;

  case FOLLOW_PAN:
    drag_mode = DRAG_PAN;
    drag_projection = visible_projection;

#ifdef ENABLE_OPENGL
    kinetic_x.MouseDown(x);
    kinetic_y.MouseDown(y);
#endif

    break;
  }

  if (CommonInterface::Basic().gps.simulator && drag_mode == DRAG_NONE)
    if (compare_squared(visible_projection.GetScreenOrigin().x - x,
                        visible_projection.GetScreenOrigin().y - y,
                        Layout::Scale(30)) != 1)
        drag_mode = DRAG_SIMULATOR;
  if (drag_mode == DRAG_NONE ) {
    drag_mode = DRAG_GESTURE;
  }

  if (drag_mode != DRAG_NONE)
    SetCapture();

  return true;
}

bool
GlueMapWindow::OnMouseUp(PixelScalar x, PixelScalar y)
{
  if (drag_mode != DRAG_NONE)
    ReleaseCapture();

  // Ignore single click event if double click detected
  if (ignore_single_click) {
    ignore_single_click = false;
    return true;
  }

  int click_time = mouse_down_clock.Elapsed();
  mouse_down_clock.Reset();

  DragMode old_drag_mode = drag_mode;
  drag_mode = DRAG_NONE;

  switch (old_drag_mode) {
  case DRAG_NONE:
    /* skip the arm_mapitem_list check below */
    return false;

#ifdef HAVE_MULTI_TOUCH
  case DRAG_MULTI_TOUCH_PAN:
    follow_mode = FOLLOW_SELF;
    EnterPan();
    return true;
#endif

  case DRAG_PAN:
#ifndef ENABLE_OPENGL
    /* allow the use of the stretched last buffer for the next two
       redraws */
    scale_buffer = 2;
#endif

#ifdef ENABLE_OPENGL
    kinetic_x.MouseUp(x);
    kinetic_y.MouseUp(y);
    kinetic_timer.Schedule(30);
#endif
    break;

  case DRAG_SIMULATOR:
    if (click_time > 50 &&
        compare_squared(drag_start.x - x, drag_start.y - y,
                        Layout::Scale(36)) == 1) {
      GeoPoint location = visible_projection.ScreenToGeo(x, y);

      double distance = hypot(drag_start.x - x, drag_start.y - y);

      // This drag moves the aircraft (changes speed and direction)
      const Angle old_bearing = CommonInterface::Basic().track;
      const fixed min_speed = fixed(1.1) *
        CommonInterface::GetComputerSettings().polar.glide_polar_task.GetVMin();
      const Angle new_bearing = drag_start_geopoint.Bearing(location);
      if (((new_bearing - old_bearing).AsDelta().AbsoluteDegrees() < fixed(30)) ||
          (CommonInterface::Basic().ground_speed < min_speed))
        device_blackboard->SetSpeed(Clamp(fixed(distance) / Layout::FastScale(3),
                                          min_speed, fixed(100)));

      device_blackboard->SetTrack(new_bearing);
      // change bearing without changing speed if direction change > 30
      // 20080815 JMW prevent dragging to stop glider

      return true;
    }

    break;

  case DRAG_GESTURE:
  {
    /* allow a bigger threshold on touch screens */
    const int threshold = IsEmbedded() ? 50 : 10;
    if ((abs(drag_start.x - x) + abs(drag_start.y - y)) > Layout::Scale(threshold)) {
      follow_mode = FOLLOW_SELF;
      EnterPan();
    } else
      LeavePan();

    break;
  }
  }

  if (!InputEvents::IsDefault() && !IsPanning()) {
    InputEvents::HideMenu();
    return true;
  }

  if (arm_mapitem_list) {
    map_item_timer.Schedule(200);
    return true;
  }

  return false;
}

bool
GlueMapWindow::OnMouseWheel(PixelScalar x, PixelScalar y, int delta)
{
  map_item_timer.Cancel();

#ifdef ENABLE_OPENGL
  kinetic_timer.Cancel();
#endif

  if (drag_mode != DRAG_NONE)
    return true;

  if (delta > 0)
    // zoom in
    InputEvents::sub_ScaleZoom(1);
  else if (delta < 0)
    // zoom out
    InputEvents::sub_ScaleZoom(-1);

  return true;
}

#ifdef HAVE_MULTI_TOUCH

bool
GlueMapWindow::OnMultiTouchDown()
{
  if (drag_mode == DRAG_GESTURE)
    gestures.Finish();
  else if (follow_mode != FOLLOW_SELF)
    return false;

  /* start panning on MultiTouch event */

  drag_mode = DRAG_MULTI_TOUCH_PAN;
  drag_projection = visible_projection;
  follow_mode = FOLLOW_PAN;
  return true;
}

#endif /* HAVE_MULTI_TOUCH */

bool
GlueMapWindow::OnMouseGesture(const TCHAR* gesture)
{
  return InputEvents::processGesture(gesture);
}

bool
GlueMapWindow::OnKeyDown(unsigned key_code)
{
  map_item_timer.Cancel();

#ifdef ENABLE_OPENGL
  kinetic_timer.Cancel();
#endif

  if (InputEvents::processKey(key_code)) {
    return true; // don't go to default handler
  }

  return false;
}

void
GlueMapWindow::OnCancelMode()
{
  MapWindow::OnCancelMode();

  if (drag_mode != DRAG_NONE) {
#ifdef HAVE_MULTI_TOUCH
    if (drag_mode == DRAG_MULTI_TOUCH_PAN)
      follow_mode = FOLLOW_SELF;
#endif

    if (drag_mode == DRAG_GESTURE)
      gestures.Finish();

    ReleaseCapture();
    drag_mode = DRAG_NONE;
  }

#ifdef ENABLE_OPENGL
  kinetic_timer.Cancel();
#endif

  map_item_timer.Cancel();
}

void
GlueMapWindow::OnPaint(Canvas &canvas)
{
  MapWindow::OnPaint(canvas);

  // Draw center screen cross hair in pan mode
  if (IsPanning())
    DrawCrossHairs(canvas);


  DrawGesture(canvas);
}

void
GlueMapWindow::OnPaintBuffer(Canvas &canvas)
{
#ifdef ENABLE_OPENGL
  ExchangeBlackboard();

  EnterDrawThread();

  /* update terrain, topography, ... */
  Idle();
#endif

  MapWindow::OnPaintBuffer(canvas);

  DrawMapScale(canvas, GetClientRect(), render_projection);
  if (IsPanning())
    DrawPanInfo(canvas);

#ifndef ENABLE_OPENGL
  if (!IsPanning())
    DrawMainMenuButtonOverlay(canvas);
  DrawZoomButtonOverlays(canvas);
  if (!HasDraggableScreen())
    DrawTaskNavSliderShape(canvas);
#endif

#ifdef ENABLE_OPENGL
  LeaveDrawThread();
#endif
}

bool
GlueMapWindow::OnTimer(WindowTimer &timer)
{
  if (timer == map_item_timer) {
    map_item_timer.Cancel();
    if (!InputEvents::IsDefault() && !IsPanning()) {
      InputEvents::HideMenu();
      return true;
    }
    ShowMapItems(drag_start_geopoint, false);
    return true;
#ifdef ENABLE_OPENGL
  } else if (timer == kinetic_timer) {
    if (kinetic_x.IsSteady() && kinetic_y.IsSteady()) {
      kinetic_timer.Cancel();
    } else {
      auto location = drag_projection.ScreenToGeo(kinetic_x.GetPosition(),
                                                  kinetic_y.GetPosition());
      location = drag_projection.GetGeoLocation() +
          drag_start_geopoint - location;

      visible_projection.SetGeoLocation(location);
      QuickRedraw();
    }

    return true;
#endif
  } else
    return MapWindow::OnTimer(timer);
}

void
GlueMapWindow::Render(Canvas &canvas, const PixelRect &rc)
{
  MapWindow::Render(canvas, rc);

  if (IsNearSelf()) {
    draw_sw.Mark("DrawGlueMisc");
    if (GetMapSettings().show_thermal_profile)
      DrawThermalBand(canvas, rc);
    DrawStallRatio(canvas, rc);
    DrawFlightMode(canvas, rc);
    DrawFinalGlide(canvas, rc);
    DrawVario(canvas, rc);
    DrawGPSStatus(canvas, rc, Basic());
  }
}

#ifndef ENABLE_OPENGL
bool
GlueMapWindow::ButtonOverlaysOnMouseDown(PixelScalar x, PixelScalar y)
{
  RasterPoint p {x, y};

  if (rc_main_menu_button.IsInside(p)) {
    StaticString<20> menu;
    StaticString<20> menu_1;
    StaticString<20> menu_2;
    StaticString<20> menu_last;

    menu = _T("Menu");
    menu_1 = _T("Menu1");
    menu_2 = _T("Menu2");
    menu_last = _T("MenuLast");

    if (InputEvents::IsMode(menu.buffer()))
      InputEvents::setMode(menu_1.buffer());

    else if (InputEvents::IsMode(menu_1.buffer()))
      InputEvents::setMode(menu_2.buffer());

    else if (InputEvents::IsMode(menu_2.buffer()))
      InputEvents::setMode(menu_last.buffer());

    else if (InputEvents::IsMode(menu_last.buffer()))
      InputEvents::HideMenu();

    else InputEvents::setMode(menu.buffer());

    return true;
  }
  if (rc_zoom_out_button.IsInside(p)) {
    InputEvents::eventZoom(_T("-"));
    InputEvents::HideMenu();
    return true;
  }
  if (rc_zoom_in_button.IsInside(p)) {
    InputEvents::eventZoom(_T("+"));
    InputEvents::HideMenu();
    return true;
  }
  if (!HasDraggableScreen() && slider_shape.GetInnerRect().IsInside(p)) {
    StaticString<20> menu_ordered(_T("NavOrdered"));
    StaticString<20> menu_goto(_T("NavGoto"));
    TaskType task_mode;
    {
      ProtectedTaskManager::Lease task_manager(*protected_task_manager);
      task_mode = task_manager->GetMode();
    }

    if (InputEvents::IsMode(menu_ordered.buffer())
        || InputEvents::IsMode(menu_goto.buffer()))
      InputEvents::HideMenu();
    else if (task_mode == TaskType::GOTO
        || task_mode == TaskType::ABORT)
      InputEvents::setMode(menu_goto.buffer());
    else
      InputEvents::setMode(menu_ordered.buffer());

    return true;
  }

  return false;
}
#endif
