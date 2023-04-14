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

#ifndef LOGGING_H_GUARD
#define LOGGING_H_GUARD

#pragma warning(push, 0)
#include <stdio.h>
#pragma warning(pop)

#define UBENCH_PREFIX "[ubench-agent]"

#define LEVEL_PRINTF(level, fmt, ...) fprintf(stderr, UBENCH_PREFIX ": " level "" fmt "\n", ##__VA_ARGS__)

#ifdef UBENCH_DEBUG
#define DEBUG_PRINTF(fmt, ...) LEVEL_PRINTF("", fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINTF(fmt, ...) (void) 0
#endif


#define WARN_PRINTF(fmt, ...) LEVEL_PRINTF("WARNING: ", fmt, ##__VA_ARGS__)
#define ERROR_PRINTF(fmt, ...) LEVEL_PRINTF("ERROR: ", fmt, ##__VA_ARGS__)
#define FATAL_PRINTF(fmt, ...) LEVEL_PRINTF("FATAL: ", fmt, ##__VA_ARGS__)

#endif
