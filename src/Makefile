CC := /usr/bin/g++

OBJ_DIR := obj
DEP_DIR := deps

# :=は即時評価, =は遅延評価
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

EXE_FILE_NAME=FMSwitch

# デフォルトのターゲット指定
.DEFAULT_GOAL := dev

.PHONY: dev
dev: CPPFLAGS += -O2
dev: $(EXE_FILE_NAME)

.PHONY: debug
debug: CPPFLAGS += -pg
debug: $(EXE_FILE_NAME)

# オブジェクトファイルをリンクする
$(EXE_FILE_NAME): $(OBJS)
	$(CC) $(OBJS) -o $(EXE_FILE_NAME) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJ_DIR)/*.o $(DEP_DIR)/*.d $(EXE_FILE_NAME)

# これ以下は変換ルールの指定

# .cppから.oに変換する際のルールを指定(コンパイル)
# $<は依存するファイルの最初のもの
$(OBJ_DIR)/%.o: %.cpp
	$(CC) $(CPPFLAGS) $< -o $@

# 依存関係ファイルの作成(sedにより、一行目の先頭にobj/という文字を追加している)
$(DEP_DIR)/%.d: %.cpp $(HEADERS)
	${CC} -MM $< | sed -e 's/^$*/$(OBJ_DIR)\/$*/g' > $(DEP_DIR)/$*.d

# オブジェクトファイルの依存関係を上書きする
-include $(DEP_DIR)/${DEPS}
