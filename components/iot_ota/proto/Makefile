# Define the list of .proto files
PROTO_FILES := $(wildcard *.proto)

# Define the output directory for generated C files
OUT_DIR := ../proto-c/

# Define the names of the generated .c and .h files
PROTO_C_FILES := $(PROTO_FILES:%.proto=$(OUT_DIR)%.pb-c.c)
PROTO_H_FILES := $(PROTO_FILES:%.proto=$(OUT_DIR)%.pb-c.h)

# The "all" target depends on the generated C files
all: create_out_dir $(PROTO_C_FILES) clean_cmake

# rule to create the output dir for the protobuf c files
create_out_dir:
	mkdir -p $(OUT_DIR)

# Rule to generate .c and .h files from .proto files
$(OUT_DIR)%.pb-c.c $(OUT_DIR)%.pb-c.h: %.proto
	protoc-c --c_out=$(OUT_DIR) -I . $<

# Clean rule to remove CMake-generated files
clean_cmake:
	rm -rf CMakeFiles/ CMakeCache.txt

# Clean rule to remove generated files and CMake-generated files
clean:
	rm -f $(PROTO_C_FILES) $(PROTO_H_FILES)

# Phony targets
.PHONY: all clean