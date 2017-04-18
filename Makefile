OUT_DIR	 =build/bin
OBJ_DIR  =build/obj
SRC_DIR  =src
OUT_NAME =test.elf

SRC =$(wildcard $(SRC_DIR)/*.cc)
OBJ =$(addprefix $(OBJ_DIR)/, $(notdir $(SRC:.cc=.o)))

CXX         =powerpc-linux-gnu-g++-4.8
CXX_FLAGS  ?=
CXX_FLAGS  +=-std=c++0x -g -O0 -mpowerpc64 -static -mno-vrsave -nostartfiles
CXX_FLAGS  +=-Wno-multichar

LIB_DIRS	=
LIBS		=

CXX_FLAGS  :=$(CXX_FLAGS) -Wl,-Tlink.txt
LIBS       :=$(LIB_DIRS) $(LIBS) $(PKG_LIBS)

OUT=$(OUT_DIR)/$(OUT_NAME)

all: setup $(OUT)

setup:
	@mkdir -p $(OUT_DIR)
	@mkdir -p $(OBJ_DIR)

$(OUT): $(OBJ)
	@echo "> Linking..."
	@$(CXX) -o $@ $(OBJ) $(LIBS) $(CXX_FLAGS)
	@echo "> Built -> $(OUT)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cc
	@echo "> $<"
	@$(CXX) -o $@ $< -c $(CXX_FLAGS)

clean:
	@rm -f $(OUT) $(OBJ)

