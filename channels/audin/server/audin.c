/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * Server Audio Input Virtual Channel
 *
 * Copyright 2012 Vic Lee
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <winpr/crt.h>

#include <freerdp/codec/dsp.h>
#include <freerdp/utils/thread.h>
#include <freerdp/utils/stream.h>
#include <freerdp/channels/wtsvc.h>
#include <freerdp/server/audin.h>

#define MSG_SNDIN_VERSION       0x01
#define MSG_SNDIN_FORMATS       0x02
#define MSG_SNDIN_OPEN          0x03
#define MSG_SNDIN_OPEN_REPLY    0x04
#define MSG_SNDIN_DATA_INCOMING 0x05
#define MSG_SNDIN_DATA          0x06
#define MSG_SNDIN_FORMATCHANGE  0x07

typedef struct _audin_server
{
	audin_server_context context;

	void* audin_channel;
	freerdp_thread* audin_channel_thread;

	FREERDP_DSP_CONTEXT* dsp_context;

	BOOL opened;
} audin_server;

static void audin_server_select_format(audin_server_context* context, int client_format_index)
{
	audin_server* audin = (audin_server*) context;

	if (client_format_index >= context->num_client_formats)
		return;
	context->selected_client_format = client_format_index;
	if (audin->opened)
	{
		/* TODO: send MSG_SNDIN_FORMATCHANGE */
	}
}

static void audin_server_send_version(audin_server* audin, STREAM* s)
{
	stream_write_BYTE(s, MSG_SNDIN_VERSION);
	stream_write_UINT32(s, 1); /* Version (4 bytes) */
	WTSVirtualChannelWrite(audin->audin_channel, stream_get_head(s), stream_get_length(s), NULL);
}

static BOOL audin_server_recv_version(audin_server* audin, STREAM* s, UINT32 length)
{
	UINT32 Version;

	if (length < 4)
		return FALSE;
	stream_read_UINT32(s, Version);
	if (Version < 1)
		return FALSE;

	return TRUE;
}

static void audin_server_send_formats(audin_server* audin, STREAM* s)
{
	int i;
	UINT32 nAvgBytesPerSec;

	stream_set_pos(s, 0);
	stream_write_BYTE(s, MSG_SNDIN_FORMATS);
	stream_write_UINT32(s, audin->context.num_server_formats); /* NumFormats (4 bytes) */
	stream_write_UINT32(s, 0); /* cbSizeFormatsPacket (4 bytes), client-to-server only */

	for (i = 0; i < audin->context.num_server_formats; i++)
	{
		nAvgBytesPerSec = audin->context.server_formats[i].nSamplesPerSec *
			audin->context.server_formats[i].nChannels *
			audin->context.server_formats[i].wBitsPerSample / 8;
		stream_check_size(s, 18);
		stream_write_UINT16(s, audin->context.server_formats[i].wFormatTag);
		stream_write_UINT16(s, audin->context.server_formats[i].nChannels);
		stream_write_UINT32(s, audin->context.server_formats[i].nSamplesPerSec);
		stream_write_UINT32(s, nAvgBytesPerSec);
		stream_write_UINT16(s, audin->context.server_formats[i].nBlockAlign);
		stream_write_UINT16(s, audin->context.server_formats[i].wBitsPerSample);
		stream_write_UINT16(s, audin->context.server_formats[i].cbSize);
		if (audin->context.server_formats[i].cbSize)
		{
			stream_check_size(s, audin->context.server_formats[i].cbSize);
			stream_write(s, audin->context.server_formats[i].data,
				audin->context.server_formats[i].cbSize);
		}
	}

	WTSVirtualChannelWrite(audin->audin_channel, stream_get_head(s), stream_get_length(s), NULL);
}

static BOOL audin_server_recv_formats(audin_server* audin, STREAM* s, UINT32 length)
{
	int i;

	if (length < 8)
		return FALSE;

	stream_read_UINT32(s, audin->context.num_client_formats); /* NumFormats (4 bytes) */
	stream_seek_UINT32(s); /* cbSizeFormatsPacket (4 bytes) */
	length -= 8;

	if (audin->context.num_client_formats <= 0)
		return FALSE;

	audin->context.client_formats = malloc(audin->context.num_client_formats * sizeof(AUDIO_FORMAT));
	ZeroMemory(audin->context.client_formats, audin->context.num_client_formats * sizeof(AUDIO_FORMAT));

	for (i = 0; i < audin->context.num_client_formats; i++)
	{
		if (length < 18)
		{
			free(audin->context.client_formats);
			audin->context.client_formats = NULL;
			return FALSE;
		}

		stream_read_UINT16(s, audin->context.client_formats[i].wFormatTag);
		stream_read_UINT16(s, audin->context.client_formats[i].nChannels);
		stream_read_UINT32(s, audin->context.client_formats[i].nSamplesPerSec);
		stream_seek_UINT32(s); /* nAvgBytesPerSec */
		stream_read_UINT16(s, audin->context.client_formats[i].nBlockAlign);
		stream_read_UINT16(s, audin->context.client_formats[i].wBitsPerSample);
		stream_read_UINT16(s, audin->context.client_formats[i].cbSize);
		if (audin->context.client_formats[i].cbSize > 0)
		{
			stream_seek(s, audin->context.client_formats[i].cbSize);
		}
	}

	IFCALL(audin->context.Opening, &audin->context);

	return TRUE;
}

static void audin_server_send_open(audin_server* audin, STREAM* s)
{
	if (audin->context.selected_client_format < 0)
		return;

	audin->opened = TRUE;

	stream_set_pos(s, 0);
	stream_write_BYTE(s, MSG_SNDIN_OPEN);
	stream_write_UINT32(s, audin->context.frames_per_packet); /* FramesPerPacket (4 bytes) */
	stream_write_UINT32(s, audin->context.selected_client_format); /* initialFormat (4 bytes) */
	/*
	 * [MS-RDPEAI] 3.2.5.1.6
	 * The second format specify the format that SHOULD be used to capture data from
	 * the actual audio input device.
	 */
	stream_write_UINT16(s, 1); /* wFormatTag = PCM */
	stream_write_UINT16(s, 2); /* nChannels */
	stream_write_UINT32(s, 44100); /* nSamplesPerSec */
	stream_write_UINT32(s, 44100 * 2 * 2); /* nAvgBytesPerSec */
	stream_write_UINT16(s, 4); /* nBlockAlign */
	stream_write_UINT16(s, 16); /* wBitsPerSample */
	stream_write_UINT16(s, 0); /* cbSize */

	WTSVirtualChannelWrite(audin->audin_channel, stream_get_head(s), stream_get_length(s), NULL);
}

static BOOL audin_server_recv_open_reply(audin_server* audin, STREAM* s, UINT32 length)
{
	UINT32 Result;

	if (length < 4)
		return FALSE;
	stream_read_UINT32(s, Result);

	IFCALL(audin->context.OpenResult, &audin->context, Result);

	return TRUE;
}

static BOOL audin_server_recv_data(audin_server* audin, STREAM* s, UINT32 length)
{
	AUDIO_FORMAT* format;
	int sbytes_per_sample;
	int sbytes_per_frame;
	BYTE* src;
	int size;
	int frames;

	if (audin->context.selected_client_format < 0)
		return FALSE;

	format = &audin->context.client_formats[audin->context.selected_client_format];

	if (format->wFormatTag == 2)
	{
		audin->dsp_context->decode_ms_adpcm(audin->dsp_context,
			stream_get_tail(s), length, format->nChannels, format->nBlockAlign);
		size = audin->dsp_context->adpcm_size;
		src = audin->dsp_context->adpcm_buffer;
		sbytes_per_sample = 2;
		sbytes_per_frame = format->nChannels * 2;
	}
	else if (format->wFormatTag == 0x11)
	{
		audin->dsp_context->decode_ima_adpcm(audin->dsp_context,
			stream_get_tail(s), length, format->nChannels, format->nBlockAlign);
		size = audin->dsp_context->adpcm_size;
		src = audin->dsp_context->adpcm_buffer;
		sbytes_per_sample = 2;
		sbytes_per_frame = format->nChannels * 2;
	}
	else
	{
		size = length;
		src = stream_get_tail(s);
		sbytes_per_sample = format->wBitsPerSample / 8;
		sbytes_per_frame = format->nChannels * sbytes_per_sample;
	}

	if (format->nSamplesPerSec == audin->context.dst_format.nSamplesPerSec && format->nChannels == audin->context.dst_format.nChannels)
	{
		frames = size / sbytes_per_frame;
	}
	else
	{
		audin->dsp_context->resample(audin->dsp_context, src, sbytes_per_sample,
			format->nChannels, format->nSamplesPerSec, size / sbytes_per_frame,
			audin->context.dst_format.nChannels, audin->context.dst_format.nSamplesPerSec);
		frames = audin->dsp_context->resampled_frames;
		src = audin->dsp_context->resampled_buffer;
	}

	IFCALL(audin->context.ReceiveSamples, &audin->context, src, frames);

	return TRUE;
}

static void* audin_server_thread_func(void* arg)
{
	void* fd;
	STREAM* s;
	void* buffer;
	BYTE MessageId;
	BOOL ready = FALSE;
	UINT32 bytes_returned = 0;
	audin_server* audin = (audin_server*) arg;
	freerdp_thread* thread = audin->audin_channel_thread;

	if (WTSVirtualChannelQuery(audin->audin_channel, WTSVirtualFileHandle, &buffer, &bytes_returned) == TRUE)
	{
		fd = *((void**) buffer);
		WTSFreeMemory(buffer);

		thread->signals[thread->num_signals++] = CreateWaitObjectEvent(NULL, TRUE, FALSE, fd);
	}

	/* Wait for the client to confirm that the Audio Input dynamic channel is ready */
	while (1)
	{
		if (freerdp_thread_wait(thread) < 0)
			break;

		if (freerdp_thread_is_stopped(thread))
			break;

		if (WTSVirtualChannelQuery(audin->audin_channel, WTSVirtualChannelReady, &buffer, &bytes_returned) == FALSE)
			break;

		ready = *((BOOL*) buffer);

		WTSFreeMemory(buffer);

		if (ready)
			break;
	}

	s = stream_new(4096);

	if (ready)
	{
		audin_server_send_version(audin, s);
	}

	while (ready)
	{
		if (freerdp_thread_wait(thread) < 0)
			break;
		
		if (freerdp_thread_is_stopped(thread))
			break;

		stream_set_pos(s, 0);

		if (WTSVirtualChannelRead(audin->audin_channel, 0, stream_get_head(s),
			stream_get_size(s), &bytes_returned) == FALSE)
		{
			if (bytes_returned == 0)
				break;
			
			stream_check_size(s, (int) bytes_returned);

			if (WTSVirtualChannelRead(audin->audin_channel, 0, stream_get_head(s),
				stream_get_size(s), &bytes_returned) == FALSE)
				break;
		}
		if (bytes_returned < 1)
			continue;

		stream_read_BYTE(s, MessageId);
		bytes_returned--;
		switch (MessageId)
		{
			case MSG_SNDIN_VERSION:
				if (audin_server_recv_version(audin, s, bytes_returned))
					audin_server_send_formats(audin, s);
				break;

			case MSG_SNDIN_FORMATS:
				if (audin_server_recv_formats(audin, s, bytes_returned))
					audin_server_send_open(audin, s);
				break;

			case MSG_SNDIN_OPEN_REPLY:
				audin_server_recv_open_reply(audin, s, bytes_returned);
				break;

			case MSG_SNDIN_DATA_INCOMING:
				break;

			case MSG_SNDIN_DATA:
				audin_server_recv_data(audin, s, bytes_returned);
				break;

			case MSG_SNDIN_FORMATCHANGE:
				break;

			default:
				printf("audin_server_thread_func: unknown MessageId %d\n", MessageId);
				break;
		}
	}

	stream_free(s);
	WTSVirtualChannelClose(audin->audin_channel);
	audin->audin_channel = NULL;
	freerdp_thread_quit(thread);

	return NULL;
}

static BOOL audin_server_open(audin_server_context* context)
{
	audin_server* audin = (audin_server*) context;

	if (audin->audin_channel_thread == NULL)
	{
		audin->audin_channel = WTSVirtualChannelOpenEx(context->vcm, "AUDIO_INPUT", WTS_CHANNEL_OPTION_DYNAMIC);

		if (audin->audin_channel == NULL)
			return FALSE;

		audin->audin_channel_thread = freerdp_thread_new();
		freerdp_thread_start(audin->audin_channel_thread, audin_server_thread_func, audin);

		return TRUE;
	}

	return FALSE;
}

static BOOL audin_server_close(audin_server_context* context)
{
	audin_server* audin = (audin_server*) context;

	if (audin->audin_channel_thread)
	{
		freerdp_thread_stop(audin->audin_channel_thread);
		freerdp_thread_free(audin->audin_channel_thread);
		audin->audin_channel_thread = NULL;
	}
	if (audin->audin_channel)
	{
		WTSVirtualChannelClose(audin->audin_channel);
		audin->audin_channel = NULL;
	}
	audin->context.selected_client_format = -1;

	return TRUE;
}

audin_server_context* audin_server_context_new(WTSVirtualChannelManager* vcm)
{
	audin_server* audin;

	audin = (audin_server*) malloc(sizeof(audin_server));
	ZeroMemory(audin, sizeof(audin_server));

	audin->context.vcm = vcm;
	audin->context.selected_client_format = -1;
	audin->context.frames_per_packet = 4096;
	audin->context.SelectFormat = audin_server_select_format;
	audin->context.Open = audin_server_open;
	audin->context.Close = audin_server_close;

	audin->dsp_context = freerdp_dsp_context_new();

	return (audin_server_context*) audin;
}

void audin_server_context_free(audin_server_context* context)
{
	audin_server* audin = (audin_server*) context;

	audin_server_close(context);

	if (audin->dsp_context)
		freerdp_dsp_context_free(audin->dsp_context);

	if (audin->context.client_formats)
		free(audin->context.client_formats);

	free(audin);
}
