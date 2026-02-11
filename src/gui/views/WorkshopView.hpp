#pragma once
#include <gtk/gtk.h>
#include <adwaita.h>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "../../core/steam/SteamService.hpp"
#include "../../core/steam/DownloadQueue.hpp"
#include "../widgets/WallpaperGrid.hpp"
#include "../widgets/WorkshopCard.hpp"

namespace bwp::gui {

class WorkshopView {
public:
  WorkshopView();
  ~WorkshopView();

  GtkWidget *getWidget() const { return m_box; }
  void refreshInstalled();

private:
  GtkWidget *m_box;
  GtkWidget *m_stack;
  GtkWidget *m_progressBar;

  // Pages
  GtkWidget* m_installedPage;
  std::unique_ptr<WallpaperGrid> m_installedGrid;
  
  GtkWidget* m_browsePage = nullptr;
  GtkWidget* m_browseScrolled = nullptr;
  GtkWidget* m_browseGrid = nullptr;  // FlowBox
  GtkWidget* m_searchEntry = nullptr; // GtkSearchEntry
  GtkWidget* m_searchButton = nullptr; // Magnifying glass button
  GtkWidget* m_loginButton;
  GtkWidget* m_profilePopover = nullptr;
  GtkWidget* m_steamcmdBanner;
  
  // Pagination
  GtkWidget* m_paginationBox = nullptr;
  GtkWidget* m_pageLabel = nullptr;
  GtkWidget* m_prevButton = nullptr;
  GtkWidget* m_nextButton = nullptr;
  int m_currentPage = 1;
  int m_totalPages = 1;
  std::string m_lastQuery;
  std::string m_lastSort = "textsearch";
  
  // Placeholder for logged-out state
  GtkWidget* m_placeholderBox = nullptr;
  
  bool m_isSearching = false;
  bool m_autoLoginAttempted = false;

  // Track workshop cards for download state updates
  std::unordered_map<std::string, WorkshopCard*> m_workshopCards; // workshopId → card
  
  // Track downloaded workshop IDs for checkmark indicators
  std::unordered_set<std::string> m_downloadedIds;

  void setupUi();
  void setupInstalledPage();
  void setupBrowsePage();
  
  // Helpers
  void updateBrowseGrid(const std::vector<bwp::steam::WorkshopItem>& items);
  void refreshLoginState();
  void refreshDownloadedIds();
  bool isWorkshopItemDownloaded(const std::string& workshopId) const;
  void performSearch(const std::string& query, int page = 1, const std::string& sort = "textsearch");
  void loadPopularWallpapers();
  void updatePagination();
  void showProfileMenu();
  void triggerSearch();
  
  // Right-click context menu
  void showCardContextMenu(WorkshopCard* card, double x, double y);
  
  // Dialogs
  void showSteamLoginDialog(const std::string& prefillUser = "", const std::string& errorMsg = "");
  void showSteam2FADialog(const std::string& errorMsg = ""); 
  void showSteamcmdInstallDialog();
  void showLogoutDialog();
  void showApiKeyDialog();

  // Download progress toast tracking
  AdwToast* m_downloadToast = nullptr;
  GtkWidget* m_downloadProgressLabel = nullptr;
  std::string m_downloadingTitle;

  void setupDownloadCallbacks();
  void showDownloadProgress(const std::string& title, float percent);
  void showDownloadComplete(const std::string& title, bool success);
  void dismissDownloadToast();
};

}  
