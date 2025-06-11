// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#if defined(_WIN32)
#define FLOW_WINDOWS
#elif defined(__APPLE__)
#define FLOW_APPLE
#elif defined(__linux__)
#define FLOW_LINUX
#else
#error "Platform unsupported"
#endif

#ifdef FLOW_WINDOWS
#define FLOW_CORE_CALL __stdcall
#else
#define FLOW_CORE_CALL
#endif

// clang-format off
#define FLOW_NAMESPACE_BEGIN namespace flow {
#define FLOW_SUBNAMESPACE_START(nested) namespace flow { namespace nested {
#define FLOW_NAMESPACE_END }
#define FLOW_SUBNAMESPACE_END } }
//clang-format on
