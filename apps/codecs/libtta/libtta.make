#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# libtta
TTALIB := $(CODECDIR)/libtta.a
TTALIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libtta/SOURCES)
TTALIB_OBJ := $(call c2obj, $(TTALIB_SRC))
OTHER_SRC += $(TTALIB_SRC)

$(TTALIB): $(TTALIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

TTAFLAGS = $(filter-out -O%,$(CODECFLAGS))
TTAFLAGS += -O3 -funroll-loops

$(CODECDIR)/libtta/%.o: $(ROOTDIR)/apps/codecs/libtta/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(TTAFLAGS) -c $< -o $@

$(CODECDIR)/libtta/%.o: $(ROOTDIR)/apps/codecs/libtta/%.S
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(TTAFLAGS) -c $< -o $@
