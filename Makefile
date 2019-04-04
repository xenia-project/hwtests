OUT_DIR	 =build/bin
OBJ_DIR  =build/obj
SRC_DIR  =src
OUT_NAME =xenon.elf
INT_NAME =xenon.elf32

DEVKITXENON ?=/usr/local/xenon-barebones

OBJCOPY :=$(DEVKITXENON)/bin/xenon-objcopy
STRIP   :=$(DEVKITXENON)/bin/xenon-strip

SRC =$(wildcard $(SRC_DIR)/*.cc)
OBJ =$(addprefix $(OBJ_DIR)/, $(notdir $(SRC:.cc=.o)))

INCLUDES	=-I$(DEVKITXENON)/usr/include

MACHDEP     =-DXENON -m32 -maltivec -fno-pic -mpowerpc64 -mhard-float

CXX         =$(DEVKITXENON)/bin/xenon-g++
CXX_FLAGS  ?=$(MACHDEP)
CXX_FLAGS  +=-std=c++0x -g -O0 -static
CXX_FLAGS  +=-Wno-multichar $(INCLUDES)
CXX_FLAGS  :=$(CXX_FLAGS)

LIB_DIRS	=-L$(DEVKITXENON)/usr/lib -L$(DEVKITXENON)/xenon/lib/32
LIBS		=-lc -lm -g

LIBS       :=$(LIB_DIRS) $(LIBS) $(PKG_LIBS)
LDSCRIPT   :=app.lds
LD_FLAGS   :=$(MACHDEP) -Wl,--gc-sections -Wl,-Map,$(notdir $@).map $(LIBS) -T $(LDSCRIPT)

OUT=$(OUT_DIR)/$(OUT_NAME)
INT=$(OUT_DIR)/$(INT_NAME)

all: setup $(OUT)

setup:
	@mkdir -p $(OUT_DIR)
	@mkdir -p $(OBJ_DIR)

$(OUT): $(INT)
	@$(OBJCOPY) -O elf32-powerpc --adjust-vma 0x80000000 $< $@
	@$(STRIP) $@
	@echo "> Stripped -> $(OUT)"

$(INT): $(OBJ)
	@echo "> Linking..."
	@$(CXX) -o $@ $(OBJ) $(LD_FLAGS) $(CXX_FLAGS)
	@echo "> Built -> $(OUT)32"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cc
	@echo "> $<"
	@$(CXX) -o $@ $< -c $(CXX_FLAGS)

clean:
	@rm -f $(OUT) $(INT) $(OBJ)

