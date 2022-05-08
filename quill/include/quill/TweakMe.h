/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

/**
 * Edit this file to squeeze more performance, or to customise supported
 * features that are enabled/disabled in compile time
 *
 * All the values below can either be passed as compiler flags or get uncommented here
 *
 * HINT:
 * It is better passing those flags using CMake or during compilation instead of uncommenting them
 * here as this gives more flexibility invoking different builds by passing different flags rather
 * than editing this file
 *
 * a) When invoking cmake : cmake . -DCMAKE_CXX_FLAGS="-DQUILL_ACTIVE_LOG_LEVEL=QUILL_LOG_LEVEL_INFO"
 */

/**************************************************************************************************/
/* Anything after this point requires the whole library to be recompiled with the desired option. */
/**************************************************************************************************/

/**
 * Disables all exceptions and replaces them with std::abort()
 */
// #define QUILL_NO_EXCEPTIONS

