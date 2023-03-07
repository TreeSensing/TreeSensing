SRC_DIR := src
BUILD_DIR := ./build
INCLUDE_FLAGS := -I$(SRC_DIR) -I$(SRC_DIR)/include -I$(SRC_DIR)/tower_sensing
CXXFLAGS := -O3 -std=c++17 -shared -fPIC $(INCLUDE_FLAGS) $(USER_DEFINES)

ifeq ($(findstring -DSIMD_COMPRESS, $(USER_DEFINES)), -DSIMD_COMPRESS)
    CXXFLAGS += -rdynamic -march=native
endif


all: \
	$(BUILD_DIR)/CMSketch.so \
	$(BUILD_DIR)/CountSketch.so \
	$(BUILD_DIR)/CUSketch.so \
	$(BUILD_DIR)/CMMSketch.so \
	$(BUILD_DIR)/CMLSketch.so \
	$(BUILD_DIR)/CSMSketch.so \
	$(BUILD_DIR)/ClusterReduceWrapper.so \

# The Count sketch uses int32_t counters
$(BUILD_DIR)/CountSketch.so: $(SRC_DIR)/sketch_api.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $< -DSKETCH=CountSketch -DCOUNTER_TYPE=int32_t -o $@

# Other sketches use uint32_t counters
$(BUILD_DIR)/%.so: $(SRC_DIR)/sketch_api.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $< -DSKETCH=$* -o $@

$(BUILD_DIR)/ClusterReduceWrapper.so: $(SRC_DIR)/prior_art/cluster_reduce/wrapper.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@


# Below are extra tests
$(BUILD_DIR)/test_sensing_flex: extra_tests/test_sensing_flex.cpp
	$(CXX) $(INCLUDE_FLAGS) $< -o $@

$(BUILD_DIR)/distributed_linear_faster: extra_tests/encrypt_linear_faster.cpp
	$(CXX) $(INCLUDE_FLAGS) $< -o $@

$(BUILD_DIR)/encrypt_linear_faster: extra_tests/encrypt_linear_faster.cpp
	$(CXX) $(INCLUDE_FLAGS) $< -o $@

$(BUILD_DIR)/test_tower_shape: extra_tests/test_tower_shape.cpp
	$(CXX) $(INCLUDE_FLAGS) $< -o $@

$(BUILD_DIR)/test_tower_error: extra_tests/test_tower_error.cpp
	$(CXX) $(INCLUDE_FLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)
