/*
 * Copyright (c) 2024 Andreas Signer <asigner@gmail.com>
 *
 * This file is part of AmiMines.
 *
 * AmiMines is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of 
 * the License, or (at your option) any later version.
 *
 * AmiMines is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with AmiMines.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG

#define debug_print(fmt, ...) \
        do { dbg_print("%s:%d:%s(): " fmt "\n", __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)

#define debug_init() dbg_init()
#define debug_shutdown() dbg_shutdown()

void dbg_print(const char *fmt, ...);
void dbg_init();
void dbg_shutdown();

# else 

#define debug_print(fmt, ...) {}
#define debug_init() {}
#define debug_shutdown() {}

#endif

#endif