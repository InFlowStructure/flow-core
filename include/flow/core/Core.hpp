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

#if defined(__x86_64__) || defined(_M_X64) || defined(__amd64__)
#define FLOW_X86_64
#elif defined(__i386__) || defined(_M_IX86)
#define FLOW_X86
#elif defined(__arm__) || defined(__aarch64__) || defined(_M_ARM64)
#define FLOW_ARM
#else
#error "Architecture unsupported"
#endif

#ifdef FLOW_WINDOWS
#define FLOW_CORE_CALL __stdcall
#else
#define FLOW_CORE_CALL
#endif

// clang-format off
#define FLOW_NAMESPACE_BEGIN namespace flow {
#define FLOW_SUBNAMESPACE_BEGIN(nested) namespace flow { namespace nested {
#define FLOW_NAMESPACE_END }
#define FLOW_SUBNAMESPACE_END } }
//clang-format on
