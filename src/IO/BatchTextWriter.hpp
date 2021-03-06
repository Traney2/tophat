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

#ifndef XCSOAR_BATCH_TEXT_WRITER_HPP
#define XCSOAR_BATCH_TEXT_WRITER_HPP

#include "Util/BatchBuffer.hpp"
#include "Util/tstring.hpp"

#include <string>

/**
 * A wrapper for #TextWriter which collects 256 lines, and writes them
 * in a one batch.
 */
class BatchTextWriter {
  tstring path;
  bool append;
  BatchBuffer<std::string,256> buffer;

public:
  BatchTextWriter(tstring::const_pointer _path, bool _append=false)
    :path(_path), append(_append) {}
  ~BatchTextWriter() {
    Flush();
  }

  bool WriteLine(const char *line);
  bool Flush();
};

#endif
