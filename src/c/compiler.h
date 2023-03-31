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

#ifndef COMPILER_H_GUARD
#define COMPILER_H_GUARD

#ifdef _MSC_VER
/* MSVC offers inline only for C++ code. */
#define inline __inline
#endif

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

/*
 * Macro to reduce visibility of non-static functions that
 * are not intended to be exported.
 */
#if __has_attribute(visibility)
#define INTERNAL __attribute__((visibility("hidden")))
#else
#define INTERNAL
#endif

/*
 * Macros to silence compiler warnings related to unused
 * parameters and variables.
 */
#if __has_attribute(unused)
#define UNUSED_PARAMETER(name) name __attribute__((unused))
#else
#define UNUSED_PARAMETER(name) (name)
#endif

#define UNUSED_VARIABLE(name) (void) name

/*
 * Macros to prevent 'condition expression is constant' warning in wrappers
 * around multi-statement macros (the do { ... } while (0) construct).
 */
#ifdef _MSC_VER
#define ONCE \
	__pragma(warning(push)) \
	__pragma(warning(disable : 4127)) \
	while (0) \
	__pragma(warning(pop))
#else
#define ONCE while (0)
#endif

/*
 * Disable selected MSVC warnings.
 */
#ifdef _MSC_VER
// C4100: unreferenced formal parameter.
#pragma warning(disable : 4100)
// C4204: nonstandard extension used: non-constant aggregate initializer.
#pragma warning(disable : 4204)
// C4710: function not inlined ('inline' present).
#pragma warning(disable : 4710)
// C4711: function selected for automatic inline expansion ('inline' missing).
#pragma warning(disable : 4711)
// C4820: padding added after data member.
#pragma warning(disable : 4820)
#endif

#endif
