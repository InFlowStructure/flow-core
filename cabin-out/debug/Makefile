CXX := c++
CXXFLAGS := -std=c++20 -fdiagnostics-color -g -DDEBUG -O0
INCLUDES := -I../../include                                                    \
  -isystem/Users/trigaux/.cache/cabin/git/src/thread_pool-cabin-pkg/include    \
  -isystem/Users/trigaux/.cache/cabin/git/src/json-cabin-pkg/include
DEFINES := -DCABIN_FLOW_CORE_PKG_NAME='"flow-core"'                            \
  -DCABIN_FLOW_CORE_PKG_VERSION='"0.1.0"'                                      \
  -DCABIN_FLOW_CORE_PKG_VERSION_MAJOR='"0"'                                    \
  -DCABIN_FLOW_CORE_PKG_VERSION_MINOR='"1"'                                    \
  -DCABIN_FLOW_CORE_PKG_VERSION_PATCH='"0"'                                    \
  -DCABIN_FLOW_CORE_PKG_VERSION_PRE='""'                                       \
  -DCABIN_FLOW_CORE_COMMIT_HASH='"460adfac699d63c2533441f8e2dc810762e85d23"'   \
  -DCABIN_FLOW_CORE_COMMIT_SHORT_HASH='"460adfac"'                             \
  -DCABIN_FLOW_CORE_COMMIT_DATE='"2025-01-10"'                                 \
  -DCABIN_FLOW_CORE_PROFILE='"debug"'
LIBS := 
SRCS :=                                                                        \
  /Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/src/Graph.cpp \
  /Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/src/Node.cpp \
  /Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/src/TypeConversion.cpp \
  /Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/src/lib.cpp \
  /Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/src/Connections.cpp \
  /Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/src/UUID.cpp \
  /Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/src/Env.cpp \
  /Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/src/Connection.cpp \
  /Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/src/Port.cpp \
  /Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/src/NodeFactory.cpp
CABIN_TIDY ?= clang-tidy
TIDY_TARGETS := $(patsubst %,tidy_%,$(SRCS))

.PHONY: $(TIDY_TARGETS) tidy all

all: libflow-core.a

tidy: $(TIDY_TARGETS)

/Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/cabin-out/debug/libflow-core.a: \
  /Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/cabin-out/debug/flow-core.d/lib.o
	$(Q)ar rcs $@ $^

$(TIDY_TARGETS): tidy_%: %
	$(Q)$(CABIN_TIDY) $(CABIN_TIDY_FLAGS) $< -- $(CXXFLAGS) $(DEFINES) -DCABIN_TEST $(INCLUDES)

/Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/cabin-out/debug/flow-core.d/Graph.o: \
  /Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/src/Graph.cpp \
  ../../include/flow/core/Graph.hpp ../../include/flow/core/UUID.hpp           \
  ../../include/flow/core/TypeConversion.hpp                                   \
  ../../include/flow/core/Connection.hpp ../../include/flow/core/Core.hpp      \
  ../../include/flow/core/Connections.hpp                                      \
  ../../include/flow/core/IndexableName.hpp ../../include/flow/core/Env.hpp    \
  ../../include/flow/core/Port.hpp ../../include/flow/core/NodeData.hpp        \
  ../../include/flow/core/Concepts.hpp ../../include/flow/core/NodeFactory.hpp \
  ../../include/flow/core/TypeName.hpp ../../include/flow/core/Event.hpp       \
  ../../include/flow/core/Node.hpp
	@mkdir -p $(@D)
	$(Q)$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

/Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/cabin-out/debug/flow-core.d/Node.o: \
  /Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/src/Node.cpp \
  ../../include/flow/core/Core.hpp ../../include/flow/core/Concepts.hpp        \
  ../../include/flow/core/Event.hpp ../../include/flow/core/Node.hpp           \
  ../../include/flow/core/Env.hpp ../../include/flow/core/IndexableName.hpp    \
  ../../include/flow/core/NodeData.hpp ../../include/flow/core/Port.hpp        \
  ../../include/flow/core/TypeName.hpp ../../include/flow/core/UUID.hpp        \
  ../../include/flow/core/Connection.hpp ../../include/flow/core/Graph.hpp     \
  ../../include/flow/core/Connections.hpp
	@mkdir -p $(@D)
	$(Q)$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

/Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/cabin-out/debug/flow-core.d/NodeFactory.o: \
  /Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/src/NodeFactory.cpp \
  ../../include/flow/core/Core.hpp ../../include/flow/core/Node.hpp            \
  ../../include/flow/core/IndexableName.hpp ../../include/flow/core/Env.hpp    \
  ../../include/flow/core/Concepts.hpp ../../include/flow/core/NodeFactory.hpp \
  ../../include/flow/core/Event.hpp ../../include/flow/core/NodeData.hpp       \
  ../../include/flow/core/Port.hpp ../../include/flow/core/TypeName.hpp        \
  ../../include/flow/core/UUID.hpp ../../include/flow/core/TypeConversion.hpp  \
  ../../include/flow/core/Connection.hpp ../../include/flow/core/Graph.hpp     \
  ../../include/flow/core/Connections.hpp
	@mkdir -p $(@D)
	$(Q)$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

/Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/cabin-out/debug/flow-core.d/TypeConversion.o: \
  /Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/src/TypeConversion.cpp \
  ../../include/flow/core/TypeConversion.hpp ../../include/flow/core/Core.hpp  \
  ../../include/flow/core/NodeData.hpp ../../include/flow/core/Concepts.hpp    \
  ../../include/flow/core/TypeName.hpp
	@mkdir -p $(@D)
	$(Q)$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

/Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/cabin-out/debug/flow-core.d/Port.o: \
  /Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/src/Port.cpp \
  ../../include/flow/core/Core.hpp ../../include/flow/core/IndexableName.hpp   \
  ../../include/flow/core/NodeData.hpp ../../include/flow/core/Port.hpp        \
  ../../include/flow/core/Concepts.hpp ../../include/flow/core/TypeName.hpp
	@mkdir -p $(@D)
	$(Q)$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

/Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/cabin-out/debug/flow-core.d/Env.o: \
  /Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/src/Env.cpp \
  ../../include/flow/core/Env.hpp ../../include/flow/core/IndexableName.hpp    \
  ../../include/flow/core/Connection.hpp ../../include/flow/core/Core.hpp      \
  ../../include/flow/core/Connections.hpp ../../include/flow/core/Port.hpp     \
  ../../include/flow/core/NodeData.hpp ../../include/flow/core/Concepts.hpp    \
  ../../include/flow/core/NodeFactory.hpp ../../include/flow/core/TypeName.hpp \
  ../../include/flow/core/UUID.hpp ../../include/flow/core/TypeConversion.hpp  \
  ../../include/flow/core/Graph.hpp ../../include/flow/core/Event.hpp          \
  ../../include/flow/core/Node.hpp
	@mkdir -p $(@D)
	$(Q)$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

/Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/cabin-out/debug/flow-core.d/UUID.o: \
  /Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/src/UUID.cpp \
  ../../include/flow/core/UUID.hpp ../../include/flow/core/Core.hpp
	@mkdir -p $(@D)
	$(Q)$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

/Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/cabin-out/debug/flow-core.d/Connection.o: \
  /Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/src/Connection.cpp \
  ../../include/flow/core/Connection.hpp ../../include/flow/core/Core.hpp      \
  ../../include/flow/core/IndexableName.hpp                                    \
  ../../include/flow/core/NodeData.hpp ../../include/flow/core/Port.hpp        \
  ../../include/flow/core/Concepts.hpp ../../include/flow/core/TypeName.hpp    \
  ../../include/flow/core/UUID.hpp
	@mkdir -p $(@D)
	$(Q)$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

/Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/cabin-out/debug/flow-core.d/Connections.o: \
  /Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/src/Connections.cpp \
  ../../include/flow/core/Connection.hpp ../../include/flow/core/Core.hpp      \
  ../../include/flow/core/Connections.hpp ../../include/flow/core/Port.hpp     \
  ../../include/flow/core/NodeData.hpp                                         \
  ../../include/flow/core/IndexableName.hpp                                    \
  ../../include/flow/core/Concepts.hpp ../../include/flow/core/TypeName.hpp    \
  ../../include/flow/core/UUID.hpp
	@mkdir -p $(@D)
	$(Q)$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

/Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/cabin-out/debug/flow-core.d/lib.o: \
  /Users/trigaux/dev/flow-designer/third_party/flow-ui/third_party/flow-core/src/lib.cpp
	@mkdir -p $(@D)
	$(Q)$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

