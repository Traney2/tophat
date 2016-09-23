/*
Copyright_License {

  Top Hat Soaring Glide Computer - http://www.tophatsoaring.org/
  Copyright (C) 2000-2016 The Top Hat Soaring Project
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

#ifndef XCSOAR_SCREEN_DOUBLE_BUFFER_WINDOW_HXX
#define XCSOAR_SCREEN_DOUBLE_BUFFER_WINDOW_HXX

#ifdef ENABLE_OPENGL

#include "BufferWindow.hpp"

class DoubleBufferWindow : public BufferWindow {
  /* there's no DrawThread on OpenGL, so this is just a normal
     BufferWindow */
};

#else

#include "Screen/PaintWindow.hpp"
#include "Screen/BufferCanvas.hpp"
#include "Thread/Mutex.hpp"

/**
 * A #PaintWindow with double buffered painting, to avoid flickering.
 * Having two buffers, one thread may render, while the main threadd
 * copies the other buffer to the screen.
 */
class DoubleBufferWindow : public PaintWindow {
  static constexpr unsigned INVALIDATE = 4242;

  BufferCanvas buffers[2];

  /**
   * The buffer currently drawn into.  This buffer may only be
   * accessed by the drawing thread.  The other buffer (current^1) may
   * only be accessed by the main thread.
   */
  unsigned current;

protected:
  /**
   * This mutex protects the variable "current".
   */
  Mutex mutex;

public:
  DoubleBufferWindow()
    :current(0) {}

private:
  /**
   * Returns the Canvas which is currently used for rendering.  This
   * method may only be called within the drawing thread.
   */
  Canvas &GetPaintCanvas() {
    return buffers[current];
  }

  /**
   * Marks the hidden Canvas as "done" and schedules it for painting
   * to the Window.
   */
  void Flip();

protected:
  /**
   * Returns the Canvas which is currently visible.  A call to this
   * method must be protected with the Mutex.
   */
  const Canvas &GetVisibleCanvas() const {
    return buffers[current ^ 1];
  }

protected:
  virtual void OnCreate() override;
  virtual void OnDestroy() override;
  virtual bool OnUser(unsigned id) override;
  virtual void OnPaint(Canvas &canvas) override;
  virtual void OnPaintBuffer(Canvas &canvas) = 0;

public:
  void Repaint() {
    OnPaintBuffer(GetPaintCanvas());
    Flip();
  }
};

#endif

#endif
