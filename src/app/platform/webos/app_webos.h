/*
 * Copyright (c) 2024 Mariotaku <https://github.com/mariotaku>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

typedef struct session_t session_t;

void app_webos_open_ribbon();

/* Remote Home. With a stream open, disconnect first (the game keeps running on
 * the PC) and open the LG home once the stream has closed; otherwise open it now.
 * The TV sends no background event when its home screen opens over Aurora, so a
 * stream left running behind it came back with a corrupted picture. */
void app_webos_go_home(session_t *session);

/* Call when a stream has finished; opens the LG home if Home asked for it. */
void app_webos_session_finished(void);