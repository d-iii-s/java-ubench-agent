/*
 * Copyright 2014 Charles University in Prague
 * Copyright 2014 Vojtech Horky
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
#ifndef STRUTIL_H_GUARD
#define STRUTIL_H_GUARD

#pragma warning(push, 0)
#include <stdlib.h>
#include <string.h>

#ifndef _MSC_VER
#include <strings.h>
#endif
#pragma warning(pop)

static inline char*
ubench_str_dup(const char* str) {
	char* result = malloc(sizeof(char) * strlen(str) + 1);
	if (result == NULL) {
		return NULL;
	}

	/*
	 * We know that the buffer is big enough and thus we can use strcpy()
	 * without worrying that we would run past the buffer.
	 */
#pragma warning(suppress : 4996)
	strcpy(result, str);

	return result;
}

static inline int
ubench_str_is_icase_equal(const char* a, const char* b) {
#ifdef _MSC_VER
	return _stricmp(a, b) == 0;
#else
	return strcasecmp(a, b) == 0;
#endif
}

static inline int
ubench_str_starts_with_icase(const char* str, const char* prefix) {
#ifdef _MSC_VER
	return _strnicmp(str, prefix, strlen(prefix)) == 0;
#else
	return strncasecmp(str, prefix, strlen(prefix)) == 0;
#endif
}

#endif
