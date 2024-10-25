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

#define FLOW_NAMESPACE flow
#define FLOW_NAMESPACE_START                                                                                           \
    /** The core namespace. */                                                                                         \
    namespace FLOW_NAMESPACE                                                                                           \
    {
#define FLOW_SUBNAMESPACE_START(n)                                                                                     \
    namespace FLOW_NAMESPACE                                                                                           \
    {                                                                                                                  \
    namespace n                                                                                                        \
    {
#define FLOW_NAMESPACE_END }
#define FLOW_SUBNAMESPACE_END                                                                                          \
    }                                                                                                                  \
    }
