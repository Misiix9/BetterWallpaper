void WorkshopView::onDownloadClicked(GtkButton *  ,
                                     gpointer user_data) {
  ItemData *data = static_cast<ItemData *>(user_data);
  
  bwp::steam::DownloadQueue::getInstance().addToQueue(data->id);
  bwp::steam::DownloadQueue::getInstance().startQueue();
  
  bwp::core::utils::ToastRequest req;
  req.message = "Added " + data->id + " to download queue";
  req.type = bwp::core::utils::ToastType::Success;
  bwp::core::utils::ToastManager::getInstance().showToast(req);
}
