/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * T.124 Generic Conference Control (GCC) Unit Tests
 *
 * Copyright 2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gcc.h"

#include <freerdp/freerdp.h>
#include <winpr/print.h>
#include <freerdp/utils/stream.h>

#include "test_gcc.h"

int init_gcc_suite(void)
{
	return 0;
}

int clean_gcc_suite(void)
{
	return 0;
}

int add_gcc_suite(void)
{
	add_test_suite(gcc);

	add_test_function(gcc_write_conference_create_request);
	add_test_function(gcc_write_client_core_data);
	add_test_function(gcc_write_client_security_data);
	add_test_function(gcc_write_client_cluster_data);
	add_test_function(gcc_write_client_network_data);

	return 0;
}

BYTE gcc_user_data[284] =
	"\x01\xc0\xd8\x00\x04\x00\x08\x00\x00\x05\x00\x04\x01\xCA\x03\xAA"
	"\x09\x04\x00\x00\xCE\x0E\x00\x00\x45\x00\x4c\x00\x54\x00\x4f\x00"
	"\x4e\x00\x53\x00\x2d\x00\x44\x00\x45\x00\x56\x00\x32\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00"
	"\x0c\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x01\xCA\x01\x00\x00\x00\x00\x00\x18\x00\x07\x00"
	"\x01\x00\x36\x00\x39\x00\x37\x00\x31\x00\x32\x00\x2d\x00\x37\x00"
	"\x38\x00\x33\x00\x2d\x00\x30\x00\x33\x00\x35\x00\x37\x00\x39\x00"
	"\x37\x00\x34\x00\x2d\x00\x34\x00\x32\x00\x37\x00\x31\x00\x34\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x04\xC0\x0C\x00\x0D\x00\x00\x00"
	"\x00\x00\x00\x00\x02\xC0\x0C\x00\x1B\x00\x00\x00\x00\x00\x00\x00"
	"\x03\xC0\x2C\x00\x03\x00\x00\x00\x72\x64\x70\x64\x72\x00\x00\x00"
	"\x00\x00\x80\x80\x63\x6c\x69\x70\x72\x64\x72\x00\x00\x00\xA0\xC0"
	"\x72\x64\x70\x73\x6e\x64\x00\x00\x00\x00\x00\xc0";

BYTE gcc_conference_create_request_expected[307] =
	"\x00\x05\x00\x14\x7C\x00\x01\x81\x2A\x00\x08\x00\x10\x00\x01\xC0"
	"\x00\x44\x75\x63\x61\x81\x1c\x01\xc0\xd8\x00\x04\x00\x08\x00\x00"
	"\x05\x00\x04\x01\xCA\x03\xAA\x09\x04\x00\x00\xCE\x0E\x00\x00\x45"
	"\x00\x4c\x00\x54\x00\x4f\x00\x4e\x00\x53\x00\x2d\x00\x44\x00\x45"
	"\x00\x56\x00\x32\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04"
	"\x00\x00\x00\x00\x00\x00\x00\x0c\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\xCA\x01\x00\x00"
	"\x00\x00\x00\x18\x00\x07\x00\x01\x00\x36\x00\x39\x00\x37\x00\x31"
	"\x00\x32\x00\x2d\x00\x37\x00\x38\x00\x33\x00\x2d\x00\x30\x00\x33"
	"\x00\x35\x00\x37\x00\x39\x00\x37\x00\x34\x00\x2d\x00\x34\x00\x32"
	"\x00\x37\x00\x31\x00\x34\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04"
	"\xC0\x0C\x00\x0D\x00\x00\x00\x00\x00\x00\x00\x02\xC0\x0C\x00\x1B"
	"\x00\x00\x00\x00\x00\x00\x00\x03\xC0\x2C\x00\x03\x00\x00\x00\x72"
	"\x64\x70\x64\x72\x00\x00\x00\x00\x00\x80\x80\x63\x6c\x69\x70\x72"
	"\x64\x72\x00\x00\x00\xA0\xC0\x72\x64\x70\x73\x6e\x64\x00\x00\x00"
	"\x00\x00\xc0";

void test_gcc_write_conference_create_request(void)
{
	STREAM* s;
	STREAM user_data;

	user_data.data = gcc_user_data;
	user_data.size = sizeof(gcc_user_data);
	user_data.p = user_data.data + user_data.size;

	s = stream_new(sizeof(gcc_conference_create_request_expected));

	gcc_write_conference_create_request(s, &user_data);
	ASSERT_STREAM(s, (BYTE*) gcc_conference_create_request_expected, sizeof(gcc_conference_create_request_expected));
}

BYTE gcc_client_core_data_expected[216] =
	"\x01\xc0\xd8\x00\x04\x00\x08\x00\x00\x05\x00\x04\x01\xCA\x03\xAA"
	"\x09\x04\x00\x00\xCE\x0E\x00\x00\x45\x00\x4c\x00\x54\x00\x4f\x00"
	"\x4e\x00\x53\x00\x2d\x00\x44\x00\x45\x00\x56\x00\x32\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00"
	"\x0c\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x01\xCA\x01\x00\x00\x00\x00\x00\x18\x00\x07\x00"
	"\x01\x00\x36\x00\x39\x00\x37\x00\x31\x00\x32\x00\x2d\x00\x37\x00"
	"\x38\x00\x33\x00\x2d\x00\x30\x00\x33\x00\x35\x00\x37\x00\x39\x00"
	"\x37\x00\x34\x00\x2d\x00\x34\x00\x32\x00\x37\x00\x31\x00\x34\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00";

void test_gcc_write_client_core_data(void)
{
	STREAM* s;
	rdpSettings* settings;

	s = stream_new(512);
	settings = freerdp_settings_new(NULL);

	settings->width = 1280;
	settings->height = 1024;
	settings->rdp_version = 5;
	settings->color_depth = 24;
	settings->kbd_layout = 0x409;
	settings->kbd_type = 0x04;
	settings->kbd_fn_keys = 12;
	settings->client_build = 3790;
	strcpy(settings->ClientHostname, "ELTONS-DEV2");
	strcpy(settings->ClientProductId, "69712-783-0357974-42714");

	gcc_write_client_core_data(s, settings);

	ASSERT_STREAM(s, (BYTE*) gcc_client_core_data_expected, sizeof(gcc_client_core_data_expected));
}

BYTE gcc_client_security_data_expected[12] =
		"\x02\xC0\x0C\x00\x1B\x00\x00\x00\x00\x00\x00\x00";

void test_gcc_write_client_security_data(void)
{
	STREAM* s;
	rdpSettings* settings;

	s = stream_new(12);
	settings = freerdp_settings_new(NULL);

	settings->DisableEncryption = 1; /* turn on encryption */
	settings->EncryptionMethods =
			ENCRYPTION_METHOD_40BIT |
			ENCRYPTION_METHOD_56BIT |
			ENCRYPTION_METHOD_128BIT |
			ENCRYPTION_METHOD_FIPS;

	gcc_write_client_security_data(s, settings);

	ASSERT_STREAM(s, (BYTE*) gcc_client_security_data_expected, sizeof(gcc_client_security_data_expected));
}

BYTE gcc_client_cluster_data_expected[12] =
		"\x04\xC0\x0C\x00\x0D\x00\x00\x00\x00\x00\x00\x00";

void test_gcc_write_client_cluster_data(void)
{
	STREAM* s;
	rdpSettings* settings;

	s = stream_new(12);
	settings = freerdp_settings_new(NULL);

	gcc_write_client_cluster_data(s, settings);

	ASSERT_STREAM(s, (BYTE*) gcc_client_cluster_data_expected, sizeof(gcc_client_cluster_data_expected));
}

BYTE gcc_client_network_data_expected[44] =
		"\x03\xC0\x2C\x00\x03\x00\x00\x00\x72\x64\x70\x64\x72\x00\x00\x00"
		"\x00\x00\x80\x80\x63\x6c\x69\x70\x72\x64\x72\x00\x00\x00\xA0\xC0"
		"\x72\x64\x70\x73\x6e\x64\x00\x00\x00\x00\x00\xc0";

void test_gcc_write_client_network_data(void)
{
	STREAM* s;
	rdpSettings* settings;

	s = stream_new(44);
	settings = freerdp_settings_new(NULL);

	settings->ChannelCount = 3;
	memset(settings->ChannelDefArray, 0, sizeof(rdpChannel) * settings->ChannelCount);

	strcpy(settings->ChannelDefArray[0].Name, "rdpdr");
	settings->ChannelDefArray[0].options = 0x80800000;

	strcpy(settings->ChannelDefArray[1].Name, "cliprdr");
	settings->ChannelDefArray[1].options = 0xc0A00000;

	strcpy(settings->ChannelDefArray[2].Name, "rdpsnd");
	settings->ChannelDefArray[2].options = 0xc0000000;

	gcc_write_client_network_data(s, settings);

	ASSERT_STREAM(s, (BYTE*) gcc_client_network_data_expected, sizeof(gcc_client_network_data_expected));
}
