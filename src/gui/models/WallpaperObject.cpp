#include "WallpaperObject.hpp"
#include "../../core/utils/StringUtils.hpp"
#include <filesystem>
G_DEFINE_TYPE(BwpWallpaperObject, bwp_wallpaper_object, G_TYPE_OBJECT)
static void bwp_wallpaper_object_finalize(GObject *object) {
  BwpWallpaperObject *self = BWP_WALLPAPER_OBJECT(object);
  self->info.~WallpaperInfo();
  G_OBJECT_CLASS(bwp_wallpaper_object_parent_class)->finalize(object);
}
static void bwp_wallpaper_object_init(BwpWallpaperObject *self) {
  new (&self->info) bwp::wallpaper::WallpaperInfo();
}
static void bwp_wallpaper_object_class_init(BwpWallpaperObjectClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  gobject_class->finalize = bwp_wallpaper_object_finalize;
}
BwpWallpaperObject *
bwp_wallpaper_object_new(const bwp::wallpaper::WallpaperInfo &info) {
  BwpWallpaperObject *self =
      (BwpWallpaperObject *)g_object_new(BWP_TYPE_WALLPAPER_OBJECT, nullptr);
  self->info = info;
  return self;
}
const bwp::wallpaper::WallpaperInfo *
bwp_wallpaper_object_get_info(BwpWallpaperObject *self) {
  g_return_val_if_fail(BWP_IS_WALLPAPER_OBJECT(self), nullptr);
  return &self->info;
}
void bwp_wallpaper_object_update_info(BwpWallpaperObject *self,
                                      const bwp::wallpaper::WallpaperInfo &info) {
  g_return_if_fail(BWP_IS_WALLPAPER_OBJECT(self));
  self->info = info;
}
gboolean bwp_wallpaper_object_match(gpointer item, gpointer user_data) {
  auto *obj = BWP_WALLPAPER_OBJECT(item);
  const char *query = (const char *)user_data;
  if (!query || strlen(query) == 0)
    return TRUE;
  std::string title = std::filesystem::path(obj->info.path).stem().string();
  std::string q = bwp::utils::StringUtils::toLower(query);
  if (bwp::utils::StringUtils::toLower(title).find(q) != std::string::npos) {
    return TRUE;
  }
  return FALSE;
}
