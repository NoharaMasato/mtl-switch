CC := /usr/bin/g++

OBJ_DIR := obj
DEP_DIR := deps

SRCS := $(wildcard *.cpp)
HEADERS := $(wildcard *.h)
OBJS := $(SRCS:%.cpp=$(OBJ_DIR)/%.o)
DEPS := $(SRCS:%.cpp=$(DEP_DIR)/%.d)

BASE_PATH := ~/lib/acl
CPPFLAGS = -c -std=c++17 -I $(BASE_PATH)/lib_acl_cpp/include -W -Wall
LDFLAGS = -L $(BASE_PATH)/lib_acl_cpp/lib -l_acl_cpp \
				-L $(BASE_PATH)/lib_protocol/lib -l_protocol \
				-L $(BASE_PATH)/lib_acl/lib -l_acl \
				-lpthread

EXE_FILE_NAME=mtl-switch

.DEFAULT_GOAL := dev

.PHONY: dev
dev: CPPFLAGS += -O2
dev: $(EXE_FILE_NAME)

.PHONY: debug
debug: CPPFLAGS += -pg
debug: $(EXE_FILE_NAME)

$(EXE_FILE_NAME): $(OBJS)
	$(CC) $(OBJS) -o $(EXE_FILE_NAME) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJ_DIR)/*.o $(DEP_DIR)/*.d $(EXE_FILE_NAME)

$(OBJ_DIR)/%.o: %.cpp
	$(CC) $(CPPFLAGS) $< -o $@

$(DEP_DIR)/%.d: %.cpp $(HEADERS)
	${CC} -MM $< | sed -e 's/^$*/$(OBJ_DIR)\/$*/g' > $(DEP_DIR)/$*.d

-include $(DEP_DIR)/${DEPS}
