# Third-Party Licenses

MBootCore uses the following third-party dependencies. Their licenses are
acknowledged below.

---

## Qt 6 — GNU Lesser General Public License v2.1

Qt is a cross-platform application framework used under the terms of the
**GNU Lesser General Public License v2.1** (LGPL-2.1).

You may obtain a copy of the LGPL-2.1 at:
<https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html>

Qt is Copyright (C) The Qt Company Ltd and other contributors.
For the full list of Qt copyright holders, see:
<https://doc.qt.io/qt-6/licenses.html>

### LGPL-2.1 Overview

The LGPL-2.1 allows you to use the licensed library in your own projects
without releasing your source code, provided that:

- You do not modify the library itself (or if you do, you release those
  modifications under the LGPL).
- You allow users to replace the library with a modified version (dynamic
  linking satisfies this).
- You provide a copy of the LGPL-2.1 with your distribution.

In this project, Qt is used as a **static library** with the LGPL-2.1
linking exception, meaning the application must allow users to re-link
against a modified Qt build. Object files sufficient for re-linking must
be provided.

---

## zlib — zlib License

zlib is a general-purpose compression library used for CRC32 computation.

The full text of the zlib license is reproduced below:

> Copyright (c) 1995-2024 Jean-loup Gailly and Mark Adler
>
> This software is provided 'as-is', without any express or implied
> warranty. In no event will the authors be held liable for any damages
> arising from the use of this software.
>
> Permission is granted to anyone to use this software for any purpose,
> including commercial applications, and to alter it and redistribute it
> freely, subject to the following restrictions:
>
> 1. The origin of this software must not be misrepresented; you must not
>    claim that you wrote the original software. If you use this software
>    in a product, an acknowledgment in the product documentation would be
>    appreciated but is not required.
>
> 2. Altered source versions must be plainly marked as such, and must not be
>    misrepresented as being the original software.
>
> 3. This notice may not be removed or altered from any source distribution.

---

## MinGW-w64 — GCC Runtime Library Exception

MinGW-w64 provides the GCC compiler toolchain and runtime libraries used
to build this project. The GCC runtime libraries are covered by the
**GNU General Public License v2** with the **GCC Runtime Library Exception**.

The exception permits the use of GCC's runtime libraries (libgcc, libstdc++,
libgomp, etc.) in compiled binaries without requiring the entire binary to
be licensed under the GPL.

A copy of the GCC Runtime Library Exception can be found at:
<https://gcc.gnu.org/onlinedocs/libstdc++/manual/license.html>

Specifically, the exception states:

> "As a special exception, you may use the free software of this project
> without restriction, and may link compiled code from this project into
> your proprietary programs without restriction."

MinGW-w64 is Copyright (C) The MinGW-w64 Project.

---

## Additional Notes

- All third-party components are used in compliance with their respective
  licenses. No proprietary code is included in any form.
- For questions regarding licensing, please contact the MBootCore project
  maintainers.
