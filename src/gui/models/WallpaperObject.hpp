#pragma once
#include "../../core/wallpaper/WallpaperInfo.hpp"
#include <gtk/gtk.h>
#include <string>
#define BWP_TYPE_WALLPAPER_OBJECT (bwp_wallpaper_object_get_type())
G_DECLARE_FINAL_TYPE(BwpWallpaperObject, bwp_wallpaper_object, BWP,
                     WALLPAPER_OBJECT, GObject)
struct _BwpWallpaperObject {
  GObject parent_instance;
  bwp::wallpaper::WallpaperInfo info;
};
BwpWallpaperObject *
bwp_wallpaper_object_new(const bwp::wallpaper::WallpaperInfo &info);
const bwp::wallpaper::WallpaperInfo *
bwp_wallpaper_object_get_info(BwpWallpaperObject *self);
void bwp_wallpaper_object_update_info(BwpWallpaperObject *self,
                                      const bwp::wallpaper::WallpaperInfo &info);
gboolean bwp_wallpaper_object_match(gpointer item, gpointer user_data);
