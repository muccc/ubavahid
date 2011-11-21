# This is free software, licensed under s

include $(TOPDIR)/rules.mk

PKG_REV:=bd9081031dda7a61cbb6d18894e34f7f9847b61f
PKG_NAME:=ubavahid
PKG_VERSION:=0.1.0
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

#PKG_SOURCE_PROTO:=git
#PKG_SOURCE_SUBDIR:=$(PKG_NAME)
#PKG_SOURCE_VERSION:=$(PKG_REV)
#PKG_SOURCE:=$(PKG_NAME)-$(PKG_REV).tar.gz
#PKG_SOURCE_URL:=https://github.com/schneider42/ubd.git

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=http://web.xtort.eu/~schneider/
#PKG_FIXUP:=libtool



#PKG_FIXUP:=autoreconf
#PKG_REMOVE_FILES=aclocal.m4

include $(INCLUDE_DIR)/package.mk
TARGET_CFLAGS += -L$(STAGING_DIR)/usr/lib/libiconv-full/lib -lz
define Package/ubavahid
 SECTION:=base
 CATEGORY:=Network
 DEFAULT:=n
 TITLE:=Use ubavhid to bridge uberbus nodes into the MDNS system.
 URL:=https://wiki.muc.ccc.de/uberbus:start
 MAINTAINER:=Franz Nord <fnord@127.0.0.1>
 DEPENDS:= +libiconv-full +glib2 +libavahi +zlib +libjson
endef

define Package/ubavhid/description
 Receives the multicast packets announcing new nodes in the network and
 creates entries in the local MDNS system.
endef

#define Package/ubavahid/conffiles
#endef

#define Build/Configure
# $(call Build/Configure/Default)
#endef

define Build/Compile
 $(MAKE) -C $(PKG_BUILD_DIR) DESTDIR="$(PKG_INSTALL_DIR)" install
endef

define Package/ubavahid/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/ubavahid $(1)/usr/sbin/ubavahid
endef

$(eval $(call BuildPackage,ubavahid))
