#pragma once
#include <string>
#include <functional>
#include <vector>
#include <gtk/gtk.h>
#include "../wallpaper/WallpaperLibrary.hpp"
#include "../utils/Logger.hpp"

namespace bwp::core::services {

class AutoTagService {
public:
    static AutoTagService& getInstance() {
        static AutoTagService instance;
        return instance;
    }

    void scan(const std::string& wallpaperId) {
        LOG_INFO("AutoTagService: Scanning " + wallpaperId);
        auto& lib = bwp::wallpaper::WallpaperLibrary::getInstance();
        auto infoOpt = lib.getWallpaper(wallpaperId);
        if(!infoOpt) return;

        // Mark as scanning
        auto info = *infoOpt;
        if(info.isScanning) return; // Already scanning

        info.isScanning = true;
        lib.updateWallpaper(info);

        // Simulate delay
        std::string* idPtr = new std::string(wallpaperId);
        g_timeout_add(2000, +[](gpointer data) -> gboolean {
            std::string* id = static_cast<std::string*>(data);
            auto& s = AutoTagService::getInstance();
            s.completeScan(*id);
            delete id;
            return G_SOURCE_REMOVE;
        }, idPtr);
    }

private:
    void completeScan(const std::string& id) {
        LOG_INFO("AutoTagService: Scan Complete for " + id);
        auto& lib = bwp::wallpaper::WallpaperLibrary::getInstance();
        auto infoOpt = lib.getWallpaper(id);
        if(!infoOpt) return;

        auto info = *infoOpt;
        info.isScanning = false;
        info.isAutoTagged = true;
        
        // Add mock tags
        std::vector<std::string> newTags = {"AI-Generated", "Landscape", "Dark"};
        // Merge unique
        for(const auto& t : newTags) {
             bool exists = false;
             for(const auto& et : info.tags) if(et == t) exists = true;
             if(!exists) info.tags.push_back(t);
        }

        lib.updateWallpaper(info);
    }

    AutoTagService() {}
};

} // namespace bwp::core::services
