CPU_DIR := ../../CPU
INCLUDE_FLAGS := -I$(CPU_DIR)/src -I$(CPU_DIR)//src/include -I$(CPU_DIR)//src/tower_sensing
CXXFLAGS := -O3 -std=c++17 -pthread $(INCLUDE_FLAGS)


all: \
	test_aggregation/test_aggregation \
	test_decryption_aggregation/compress_and_save

test_aggregation/test_aggregation: test_aggregation/test_aggregation.cpp
	g++ $(CXXFLAGS) $< -o $@

test_decryption_aggregation/compress_and_save: test_decryption_aggregation/compress_and_save.cpp
	g++ $(CXXFLAGS) $< -o $@

clean:
	rm -f test_aggregation/test_aggregation
	rm -f test_decryption_aggregation/compress_and_save
