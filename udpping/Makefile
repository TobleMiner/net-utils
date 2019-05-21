ifndef TOPDIR

all: udpping

src/udpping: $(wildcard src/*.c)
	$(MAKE) -C src

udpping: src/udpping
	cp src/udpping udpping

clean:
	$(MAKE) -C src clean
	rm -f udpping

else
include $(TOPDIR)/rules.mk

PKG_NAME:=udpping
PKG_VERSION:=1
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(TOPDIR)/../package/gluon.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/udpping
	SECTION:=net
	CATEGORY:=Network
	TITLE:=Simple UDP-based network testing utility
endef

#define Package/udpping/install
#	$(call Gluon/Build/Install,$(1))
#	$(INSTALL_DIR) $(1)/usr/bin
#	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/udpping $(1)/usr/bin/install
#endef

$(eval $(call BuildPackage,udpping))
endif
