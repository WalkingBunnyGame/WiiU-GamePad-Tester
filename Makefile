ifeq ($(strip $(DEVKITPRO)),)
$(error "DEVKITPRO is not set. Please export DEVKITPRO=/opt/devkitpro")
endif

ifeq ($(strip $(DEVKITPPC)),)
$(error "DEVKITPPC is not set. Please export DEVKITPPC=$(DEVKITPRO)/devkitPPC")
endif

PROJECT_ROOT := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

APP_NAME      := WiiU GamePad Tester
APP_SHORTNAME := GamePad Tester
APP_AUTHOR    := Walking Bunny x Codex
APP_ICON      := $(PROJECT_ROOT)assets/app-icon.png

include $(DEVKITPRO)/wut/share/wut_rules

TARGET      := WiiUDrcTest
BUILD       := build
SOURCES     := source
DATA        := data
INCLUDES    :=

ARCH        := $(MACHDEP)
CFLAGS      := -g -Wall -Wextra -O2 -ffunction-sections $(ARCH)
CFLAGS      += $(INCLUDE)
CXXFLAGS    := $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS     := -g $(ARCH)
LD          := $(CC)
LDFLAGS      = -g $(ARCH) $(RPXSPECS) -Wl,-Map,$(notdir $*.map)
LIBS        := -lm -lwut
LIBDIRS     := $(PORTLIBS) $(WUT_ROOT)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT := $(CURDIR)/$(TARGET)
export VPATH  := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                 $(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

export OFILES_BIN := $(addsuffix .o,$(BINFILES))
export OFILES_SRC := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES     := $(OFILES_BIN) $(OFILES_SRC)
export HFILES     := $(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: all clean $(BUILD)

all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).rpx $(TARGET).wuhb

else

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).wuhb: $(OUTPUT).rpx
$(OUTPUT).elf: $(OFILES)
$(OFILES_SRC): $(HFILES)

%_bin.h %.bin.o: %.bin
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

endif
