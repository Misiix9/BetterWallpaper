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

/**
 * @brief Create a new WallpaperObject
 */
BwpWallpaperObject *
bwp_wallpaper_object_new(const bwp::wallpaper::WallpaperInfo &info);

/**
 * @brief Get the WallpaperInfo from the object
 */
const bwp::wallpaper::WallpaperInfo *
bwp_wallpaper_object_get_info(BwpWallpaperObject *self);

/**
 * @brief Update the WallpaperInfo stored in the object
 */
void bwp_wallpaper_object_update_info(BwpWallpaperObject *self,
                                      const bwp::wallpaper::WallpaperInfo &info);

/**
 * @brief Match function for filtering
 */
gboolean bwp_wallpaper_object_match(gpointer item, gpointer user_data);
