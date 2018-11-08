/*
    Copyright (C) 2018  Zachary Lund

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <napi.h>
#include <cstdio>
#include "fontinfo/fontinfo.h"
#include "fontinfo/endian.h"

#ifdef _WIN32
#include <windows.h>
#endif

using namespace Napi;

namespace {

#ifdef _WIN32

std::string utf16_to_utf8(char *buffer, size_t buffer_length)
{
	DWORD nCodePoints = buffer_length / 2;

	if (buffer_length % 2 != 0) {
		return {};
	}

	DWORD size = WideCharToMultiByte(
		CP_UTF8, 0,
		(WCHAR*)buffer, nCodePoints,
		NULL, 0,
		NULL, NULL
	);

	std::string result(size, char());

	size = WideCharToMultiByte(
		CP_UTF8, 0,
		(WCHAR*)buffer, nCodePoints,
		&result[0], result.size(),
		NULL, NULL
	);

	return result;
}

#else
	#error Not using Windows? Well too bad I guess. I haven't implemented POSIX yet.
#endif

String StringFromFontString(Env env, font_info_string *name)
{
	char *buffer = new char[name->length];
	memcpy(buffer, name->buffer, name->length);

	/* Flip from BE to host order */
	char16_t *iter = (char16_t*)&buffer[0];
	char16_t *end_iter = (char16_t*)&buffer[name->length];

	for (; iter != end_iter; ++iter) {
		iter[0] = be16toh(iter[0]);
	}

	std::string converted(utf16_to_utf8(buffer, name->length));

	String result = String::New(env, converted);

	delete[] buffer;

	return result;
}

static Value getFontInfo(const CallbackInfo& info)
{
	Env env = info.Env();
	std::string filepath(info[0].As<String>());

	font_info *f_info = font_info_create(filepath.c_str());

	if (!f_info) {
		throw Error::New(env, "node-fontinfo: Failed to fetch font info\n");
	}

	/* We now have UTF16 and the size of the buffer, let V8 deal with the rest */
	Object result = Object::New(env);

	result.Set(
		"family_name",
		StringFromFontString(env, &f_info->family_name)
	);

	result.Set(
		"subfamily_name",
		StringFromFontString(env, &f_info->subfamily_name)
	);

	result.Set("italic", f_info->italic);
	result.Set("bold", f_info->bold);

	font_info_destroy(f_info);

	return result;
}

}

Object Init(Env env, Object exports)
{
	exports.Set(
		"getFontInfo",
		Napi::Function::New(env, getFontInfo)
	);

	return exports;
}

NODE_API_MODULE(node_fontinfo, Init)