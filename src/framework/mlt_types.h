/*
 * mlt_types.h -- provides forward definitions of all public types
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _MLT_TYPES_H_
#define _MLT_TYPES_H_

#include <stdint.h>

typedef enum
{
	mlt_whence_relative_start,
	mlt_whence_relative_current,
	mlt_whence_relative_end
}
mlt_whence;

typedef double mlt_timecode;
typedef struct mlt_frame_s *mlt_frame, **mlt_frame_ptr;
typedef struct mlt_properties_s *mlt_properties;
typedef struct mlt_service_s *mlt_service;
typedef struct mlt_producer_s *mlt_producer;
typedef struct mlt_manager_s *mlt_manager;
typedef struct mlt_playlist_s *mlt_playlist;
typedef struct mlt_multitrack_s *mlt_multitrack;
typedef struct mlt_filter_s *mlt_filter;
typedef struct mlt_transition_s *mlt_transition;
typedef struct mlt_tractor_s *mlt_tractor;
typedef struct mlt_field_s *mlt_field;
typedef struct mlt_consumer_s *mlt_consumer;

typedef void ( *mlt_destructor )( void * );
typedef char *( *mlt_serialiser )( void *, int length );


#endif
