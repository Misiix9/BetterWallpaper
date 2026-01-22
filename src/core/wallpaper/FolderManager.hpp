#pragma once
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace bwp::wallpaper {

struct Folder {
  std::string id;
  std::string name;
  std::vector<std::string> wallpaperIds; // References to library wallpapers
  std::string icon = "folder-symbolic";
};

class FolderManager {
public:
  static FolderManager &getInstance();

  // Folder management
  std::vector<Folder> getFolders() const;
  std::optional<Folder> getFolder(const std::string &id) const;

  std::string createFolder(const std::string &name);
  void deleteFolder(const std::string &id);
  void renameFolder(const std::string &id, const std::string &newName);

  // Content management
  void addToFolder(const std::string &folderId, const std::string &wallpaperId);
  void removeFromFolder(const std::string &folderId,
                        const std::string &wallpaperId);

  void load();
  void save();

private:
  FolderManager();
  ~FolderManager();

  std::unordered_map<std::string, Folder> m_folders;
  mutable std::mutex m_mutex;
};

} // namespace bwp::wallpaper
