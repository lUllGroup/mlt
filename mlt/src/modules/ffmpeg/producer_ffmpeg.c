/*
 * producer_ffmpeg.c -- simple ffmpeg test case
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

#include "producer_ffmpeg.h"

#include <framework/mlt_frame.h>
#include <framework/mlt_factory.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct producer_ffmpeg_s *producer_ffmpeg;

struct producer_ffmpeg_s
{
	struct mlt_producer_s parent;
	FILE *video;
	FILE *audio;
	uint64_t expected;
	uint8_t *buffer;
	int open;
	int width;
	int height;
	int end_of_video;
	int end_of_audio;
};

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index );
static void producer_close( mlt_producer parent );

/** Consutruct an ffmpeg producer.
*/

mlt_producer producer_ffmpeg_init( char *file )
{
	producer_ffmpeg this = calloc( sizeof( struct producer_ffmpeg_s ), 1 );
	if ( this != NULL && mlt_producer_init( &this->parent, this ) == 0 )
	{
		// Get the producer
		mlt_producer producer = &this->parent;

		// Get the properties of the producer
		mlt_properties properties = mlt_producer_properties( producer );

		// Override get_frame and close methods
		producer->get_frame = producer_get_frame;
		producer->close = producer_close;

		// Set the properties
		mlt_properties_set( properties, "mlt_type", "producer_ffmpeg" );

		if ( file != NULL && !strcmp( file, "v4l" ) )
		{
			mlt_properties_set( properties, "video_type", "v4l" );
			mlt_properties_set( properties, "video_file", "/dev/video0" );
			mlt_properties_set( properties, "video_size", "640x480" );
			mlt_properties_set( properties, "audio_type", "dsp" );
			mlt_properties_set( properties, "audio_file", "/dev/dsp" );
		}
		else
		{
			mlt_properties_set( properties, "video_type", "file" );
			mlt_properties_set( properties, "video_file", file );
			mlt_properties_set( properties, "video_size", "" );
			mlt_properties_set( properties, "audio_type", "file" );
			mlt_properties_set( properties, "audio_file", file );
		}

		mlt_properties_set_int( properties, "audio_rate", 48000 );
		mlt_properties_set_int( properties, "audio_channels", 2 );
		mlt_properties_set_int( properties, "audio_track", 0 );

		this->buffer = malloc( 1024 * 1024 * 2 );

		return producer;
	}
	free( this );
	return NULL;
}

static int producer_get_image( mlt_frame this, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the frames properties
	mlt_properties properties = mlt_frame_properties( this );

	if ( mlt_properties_get_int( properties, "has_image" ) )
	{
		// Get width and height
		*format = mlt_image_yuv422;
		*width = mlt_properties_get_int( properties, "width" );
		*height = mlt_properties_get_int( properties, "height" );

		// Specify format and image
		*buffer = mlt_properties_get_data( properties, "image", NULL );
	}
	else
	{
		mlt_frame_get_image( this, buffer, format, width, height, writable );
	}

	return 0;
}

FILE *producer_ffmpeg_run_video( producer_ffmpeg this )
{
	if ( this->video == NULL )
	{
		// Get the producer
		mlt_producer producer = &this->parent;

		// Get the properties of the producer
		mlt_properties properties = mlt_producer_properties( producer );

		// Get the video loop property
		int video_loop = mlt_properties_get_int( properties, "video_loop" );

		if ( !this->open || video_loop )
		{
			const char *mlt_prefix = mlt_factory_prefix( );
			char *video_type = mlt_properties_get( properties, "video_type" );
			char *video_file = mlt_properties_get( properties, "video_file" );
			float video_rate = mlt_properties_get_double( properties, "fps" );
			char *video_size = mlt_properties_get( properties, "video_size" );
			char command[ 1024 ] = "";
			float position = mlt_producer_position( &this->parent );

			if ( video_loop || position < 0 ) position = 0;

			sprintf( command, "%s/ffmpeg/video.sh \"%s\" \"%s\" \"%s\" %f %f 2>/dev/null",
							  mlt_prefix,
							  video_type,
							  video_file,
							  video_size,
							  video_rate,
							  position );

			this->video = popen( command, "r" );
		}
	}
	return this->video;
}

FILE *producer_ffmpeg_run_audio( producer_ffmpeg this )
{
	// Get the producer
	mlt_producer producer = &this->parent;

	// Get the properties of the producer
	mlt_properties properties = mlt_producer_properties( producer );

	if ( this->audio == NULL )
	{
		int audio_loop = mlt_properties_get_int( properties, "audio_loop" );

		if ( !this->open || audio_loop )
		{
			const char *mlt_prefix = mlt_factory_prefix( );
			char *audio_type = mlt_properties_get( properties, "audio_type" );
			char *audio_file = mlt_properties_get( properties, "audio_file" );
			int frequency = mlt_properties_get_int( properties, "audio_rate" );
			int channels = mlt_properties_get_int( properties, "audio_channels" );
			int track = mlt_properties_get_int( properties, "audio_track" );
			char command[ 1024 ] = "";
			float position = mlt_producer_position( &this->parent );

			if ( audio_loop || position < 0 ) position = 0;

			sprintf( command, "%s/ffmpeg/audio.sh \"%s\" \"%s\" %f %d %d %d 2>/dev/null",
							  mlt_prefix,
							  audio_type,
							  audio_file,
							  position,
							  frequency,
							  channels,
							  track );

			this->audio = popen( command, "r" );
		}
	}
	return this->audio;
}

static void producer_ffmpeg_position( producer_ffmpeg this, uint64_t requested, int *skip )
{
	*skip = 0;

	if ( this->open && requested > this->expected )
	{
		// Skip the following n frames
		*skip = requested - this->expected;
	}
	else if ( requested != this->expected )
	{
		// Close the video pipe
		if ( this->video != NULL )
			pclose( this->video );
		this->video = NULL;

		// Close the audio pipe
		if ( this->audio != NULL )
			pclose( this->audio );
		this->audio = NULL;
	
		// We should not be open now
		this->open = 0;
		this->end_of_video = 0;
		this->end_of_audio = 0;
	}

	// This is the next frame we expect
	this->expected = mlt_producer_frame( &this->parent ) + 1;

	// Open the pipe
	this->video = producer_ffmpeg_run_video( this );

	// Open the audio pipe
	this->audio = producer_ffmpeg_run_audio( this );

	// We should be open now
	this->open = 1;
}

static int sample_calculator( float fps, int frequency, int64_t position )
{
	int samples = 0;

	if ( fps > 29 && fps <= 30 )
	{
		samples = frequency / 30;

		switch ( frequency )
		{
			case 48000:
				if ( position % 5 != 0 )
					samples += 2;
				break;
			case 44100:
				if ( position % 300 == 0 )
					samples = 1471;
				else if ( position % 30 == 0 )
					samples = 1470;
				else if ( position % 2 == 0 )
					samples = 1472;
				else
					samples = 1471;
				break;
			case 32000:
				if ( position % 30 == 0 )
					samples = 1068;
				else if ( position % 29 == 0 )
					samples = 1067;
				else if ( position % 4 == 2 )
					samples = 1067;
				else
					samples = 1068;
				break;
			default:
				samples = 0;
		}
	}
	else if ( fps != 0 )
	{
		samples = frequency / fps;
	}

	return samples;
}

static int producer_get_audio( mlt_frame this, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the frames properties
	mlt_properties properties = mlt_frame_properties( this );

	producer_ffmpeg producer = mlt_properties_get_data( properties, "producer_ffmpeg", NULL );
	mlt_properties producer_properties = mlt_producer_properties( &producer->parent );

	int64_t target = mlt_properties_get_double( properties, "target" );
	int skip = mlt_properties_get_int( properties, "skip" );

	float fps = mlt_properties_get_double( producer_properties, "fps" );
	*frequency = mlt_properties_get_int( producer_properties, "audio_rate" );
	*channels = mlt_properties_get_int( producer_properties, "audio_channels" );

	// Maximum Size (?)
	int size = ( *frequency / 25 ) * *channels * 2;

	// Allocate an image
	*buffer = malloc( size );
		
	// Read it
	if ( producer->audio != NULL )
	{
		do
		{
			*samples = sample_calculator( fps, *frequency, target - skip );
			if ( fread( *buffer, *samples * *channels * 2, 1, producer->audio ) != 1 )
			{
				pclose( producer->audio );
				producer->audio = NULL;
				producer->end_of_audio = 1;
			}
		}
		while( producer->audio != NULL && skip -- );
	}
	else
	{
		*samples = sample_calculator( fps, *frequency, target );
		memset( *buffer, 0, size );
	}

	// Pass the data on the frame properties
	mlt_properties_set_data( properties, "audio", *buffer, size, free, NULL );

	// Set the producer properties
	mlt_properties_set_int( producer_properties, "end_of_clip", producer->end_of_video && producer->end_of_audio );

	return 0;
}

static int read_ffmpeg_header( producer_ffmpeg this, int *width, int *height )
{
	int count = 0;
	char temp[ 132 ];
	FILE *video = this->video;

	if ( fgets( temp, 132, video ) )
	{
		if ( strncmp( temp, "FRAME", 5 ) )
		{
			if ( strstr( temp, " W" ) != NULL )
				*width = atoi( strstr( temp, " W" ) + 2 );
			if ( strstr( temp, " H" ) != NULL )
				*height = atoi( strstr( temp, " H" ) + 2 );
			count = 2;
			fgets( temp, 132, video );
			this->width = *width;
			this->height = *height;
		}
		else
		{
			*width = this->width;
			*height = this->height;
			count = 2;
		}
	}
	return count;
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	producer_ffmpeg this = producer->child;
	int width;
	int height;
	int skip;

	// Construct a test frame
	*frame = mlt_frame_init( );

	// Are we at the position expected?
	producer_ffmpeg_position( this, mlt_producer_frame( producer ), &skip );

	// Get properties objects
	mlt_properties producer_properties = mlt_producer_properties( &this->parent );

	// Get the frames properties
	mlt_properties properties = mlt_frame_properties( *frame );

	FILE *video = this->video;

	mlt_properties_set_double( properties, "target", mlt_producer_frame( producer ) );
	mlt_properties_set_int( properties, "skip", skip );

	// Read the video
	if ( video != NULL && read_ffmpeg_header( this, &width, &height ) == 2 )
	{
		// Allocate an image
		uint8_t *image = malloc( width * height * 2 );
		
		// Read it
		while( skip -- )
		{
			if ( fread( this->buffer, width * height * 3 / 2, 1, video ) == 1 )
				read_ffmpeg_header( this, &width, &height );
			else
				skip = 0;
		}

		fread( this->buffer, width * height * 3 / 2, 1, video );

		// Convert it
		mlt_convert_yuv420p_to_yuv422( this->buffer, width, height, width, image );

		// Pass the data on the frame properties
		mlt_properties_set_data( properties, "image", image, width * height * 2, free, NULL );
		mlt_properties_set_int( properties, "width", width );
		mlt_properties_set_int( properties, "height", height );
		mlt_properties_set_int( properties, "has_image", 1 );

		// Push the image callback
		mlt_frame_push_get_image( *frame, producer_get_image );

	}
	else
	{
		// Clean up
		if ( this->video != NULL )
		{
			int video_loop = mlt_properties_get_int( producer_properties, "video_loop" );

			// Inform caller that end of clip is reached
			this->end_of_video = !video_loop;
			pclose( this->video );
			this->video = NULL;
		}

		// Push the image callback
		mlt_frame_push_get_image( *frame, producer_get_image );
	}

	// Set the audio pipe
	mlt_properties_set_data( properties, "producer_ffmpeg", this, 0, NULL, NULL );
	mlt_properties_set_int( properties, "end_of_clip", this->end_of_video && this->end_of_audio );

	// Hmm - register audio callback
	( *frame )->get_audio = producer_get_audio;

	// Get the additional properties
	double aspect_ratio = mlt_properties_get_double( producer_properties, "aspect_ratio" );
	double speed = mlt_properties_get_double( producer_properties, "speed" );

	// Set them on the frame
	mlt_properties_set_double( properties, "aspect_ratio", aspect_ratio );
	mlt_properties_set_double( properties, "speed", speed );

	// Set the out point on the producer
	if ( this->end_of_video && this->end_of_audio )
		mlt_producer_set_in_and_out( &this->parent, mlt_producer_get_in( &this->parent ), mlt_producer_position( &this->parent ) );

	// Update timecode on the frame we're creating
	mlt_frame_set_timecode( *frame, mlt_producer_position( producer ) );

	// Calculate the next timecode
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_close( mlt_producer parent )
{
	producer_ffmpeg this = parent->child;
	if ( this->video )
		pclose( this->video );
	if ( this->audio )
		pclose( this->audio );
	parent->close = NULL;
	mlt_producer_close( parent );
	free( this->buffer );
	free( this );
}

