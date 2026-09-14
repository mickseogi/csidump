include $(TOPDIR)/rules.mk

PKG_NAME:=csidump-v1
PKG_VERSION:=1.0.0
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/csidump-v1
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Versioned MediaTek CSI UDP exporter
  DEPENDS:=+libstdcpp +libpthread +libnl-tiny
endef

define Package/csidump-v1/description
 Sends exactly 64 native-order signed-int16 CSI bins using the documented CSI1 V1 UDP protocol.
endef

CMAKE_OPTIONS += -DBUILD_ROUTER_BINARY=ON -DBUILD_TESTING=OFF

define Build/Prepare
	$(call Build/Prepare/Default)
	$(CP) $(CURDIR)/../CMakeLists.txt $(PKG_BUILD_DIR)/
	$(CP) $(CURDIR)/../include $(PKG_BUILD_DIR)/
	$(CP) $(CURDIR)/../src $(PKG_BUILD_DIR)/
endef

define Package/csidump-v1/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/CSIdumpV1 $(1)/usr/bin/CSIdumpV1
endef

$(eval $(call BuildPackage,csidump-v1))
