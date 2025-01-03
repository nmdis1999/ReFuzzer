CXX = g++
CXXFLAGS = -std=c++17 -Wall -Werror
LDFLAGS = -lcurl

QUERY_GEN_DIR = query_generator
MODEL2_DIR = model2

NUM_PROGRAMS = 10000

QUERY_GEN = $(QUERY_GEN_DIR)/query_generator
RECOMPILE = $(MODEL2_DIR)/recompile

QUERY_GEN_SOURCES = $(QUERY_GEN_DIR)/query_generator.cpp
QUERY_GEN_HEADERS = $(QUERY_GEN_DIR)/*.hpp
RECOMPILE_SOURCES = $(MODEL2_DIR)/recompile.cpp

JSON_FLAGS = $(shell pkg-config --cflags --libs nlohmann_json)

.PHONY: all clean run

all: $(QUERY_GEN) $(RECOMPILE)

$(QUERY_GEN): $(QUERY_GEN_SOURCES) $(QUERY_GEN_HEADERS)
	$(CXX) $(CXXFLAGS) -I. $< $(LDFLAGS) $(JSON_FLAGS) -o $@
$(RECOMPILE): $(RECOMPILE_SOURCES)
	$(CXX) $(CXXFLAGS) -I$(MODEL2_DIR) $< $(LDFLAGS) $(JSON_FLAGS) -o $@


clean:
	rm -f $(QUERY_GEN) $(RECOMPILE)
