CXX = g++
CXXFLAGS = -std=c++17 -Wall -Werror
LDFLAGS = -lcurl  # Add curl library linking

QUERY_GEN_DIR = query_generator
MODEL2_DIR = model2

NUM_PROGRAMS = 10000

QUERY_GEN = $(QUERY_GEN_DIR)/query_generator
RECOMPILE = $(MODEL2_DIR)/recompile

QUERY_GEN_SOURCES = $(QUERY_GEN_DIR)/query_generator.cpp
QUERY_GEN_HEADERS = $(QUERY_GEN_DIR)/*.hpp
RECOMPILE_SOURCES = $(MODEL2_DIR)/recompile.cpp

.PHONY: all clean run

all: $(QUERY_GEN) run_generator $(RECOMPILE)

$(QUERY_GEN): $(QUERY_GEN_SOURCES) $(QUERY_GEN_HEADERS)
	$(CXX) $(CXXFLAGS) -I$(QUERY_GEN_DIR) $(QUERY_GEN_SOURCES) $(LDFLAGS) -o $(QUERY_GEN)

run_generator: $(QUERY_GEN)
	cd $(QUERY_GEN_DIR) && python3 generatePrograms.py $(NUM_PROGRAMS)

$(RECOMPILE): $(RECOMPILE_SOURCES)
	$(CXX) $(CXXFLAGS) -I$(MODEL2_DIR) $(RECOMPILE_SOURCES) $(LDFLAGS) -o $(RECOMPILE)

clean:
	rm -f $(QUERY_GEN) $(RECOMPILE)
