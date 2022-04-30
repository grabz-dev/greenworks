// Copyright (c) 2014 Greenheart Games Pty. Ltd. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "greenworks_async_workers.h"

#include <sstream>
#include <iomanip>
#include "nan.h"
#include "steam/steam_api.h"
#include "v8.h"

#include "greenworks_unzip.h"
#include "greenworks_zip.h"


namespace {

struct FilesContentContainer {
  std::vector<char*> files_content;
  ~FilesContentContainer() {
    for (size_t i = 0; i < files_content.size(); ++i) {
      delete[] files_content[i];
    }
  }
};

};  // namespace

namespace greenworks {

FileContentSaveWorker::FileContentSaveWorker(Nan::Callback* success_callback,
    Nan::Callback* error_callback, std::string file_name, std::string content):
        SteamAsyncWorker(success_callback, error_callback),
        file_name_(file_name),
        content_(content) {
}

void FileContentSaveWorker::Execute() {
  if (!SteamRemoteStorage()->FileWrite(
      file_name_.c_str(), content_.c_str(), content_.size()))
    SetErrorMessage("Error on writing to file.");
}

FilesSaveWorker::FilesSaveWorker(Nan::Callback* success_callback,
    Nan::Callback* error_callback, const std::vector<std::string>& files_path):
        SteamAsyncWorker(success_callback, error_callback),
        files_path_(files_path) {
}

void FilesSaveWorker::Execute() {
  FilesContentContainer container;
  std::vector<int> files_content_length;
  for (size_t i = 0; i < files_path_.size(); ++i) {
    char* content = nullptr;
    int length = 0;
    if (!utils::ReadFile(files_path_[i].c_str(), &content, &length))
      break;
    container.files_content.push_back(content);
    files_content_length.push_back(length);
  }
  if (container.files_content.size() != files_path_.size()) {
    SetErrorMessage("Error on reading files.");
    return;
  }
  for (size_t i = 0; i < files_path_.size(); ++i) {
    std::string file_name = utils::GetFileNameFromPath(files_path_[i]);
    if (!SteamRemoteStorage()->FileWrite(file_name.c_str(),
        container.files_content[i], files_content_length[i])) {
      SetErrorMessage("Error on writing file on Steam Cloud.");
      return;
    }
  }
}

FileDeleteWorker::FileDeleteWorker(Nan::Callback* success_callback,
    Nan::Callback* error_callback, std::string file_name):
        SteamAsyncWorker(success_callback, error_callback),
        file_name_(file_name) {
}

void FileDeleteWorker::Execute() {
  ISteamRemoteStorage* steam_remote_storage = SteamRemoteStorage();

  if (!steam_remote_storage->FileExists(file_name_.c_str())) {
    SetErrorMessage("File doesn't exist.");
    return;
  }

  if (!steam_remote_storage->FileDelete(file_name_.c_str())) {
    SetErrorMessage("Error on deleting file.");
    return;
  }
}

FileReadWorker::FileReadWorker(Nan::Callback* success_callback,
    Nan::Callback* error_callback, std::string file_name):
        SteamAsyncWorker(success_callback, error_callback),
        file_name_(file_name) {
}

void FileReadWorker::Execute() {
  ISteamRemoteStorage* steam_remote_storage = SteamRemoteStorage();

  if (!steam_remote_storage->FileExists(file_name_.c_str())) {
    SetErrorMessage("File doesn't exist.");
    return;
  }

  int32 file_size = steam_remote_storage->GetFileSize(file_name_.c_str());

  auto* content = new char[file_size+1];
  int32 end_pos = steam_remote_storage->FileRead(
      file_name_.c_str(), content, file_size);
  content[end_pos] = '\0';

  if (end_pos == 0 && file_size > 0) {
    SetErrorMessage("Error on reading file.");
  } else {
    content_ = std::string(content);
  }

  delete[] content;
}

void FileReadWorker::HandleOKCallback() {
  Nan::HandleScope scope;

  v8::Local<v8::Value> argv[] = { Nan::New(content_).ToLocalChecked() };
  Nan::AsyncResource resource("greenworks:FileReadWorker.HandleOKCallback");
  callback->Call(1, argv, &resource);
}

CloudQuotaGetWorker::CloudQuotaGetWorker(Nan::Callback* success_callback,
      Nan::Callback* error_callback):SteamAsyncWorker(success_callback,
          error_callback), total_bytes_(-1), available_bytes_(-1) {
}

void CloudQuotaGetWorker::Execute() {
  ISteamRemoteStorage* steam_remote_storage = SteamRemoteStorage();

  if (!steam_remote_storage->GetQuota(&total_bytes_, &available_bytes_)) {
    SetErrorMessage("Error on getting cloud quota.");
    return;
  }
}

void CloudQuotaGetWorker::HandleOKCallback() {
  Nan::HandleScope scope;
  v8::Local<v8::Value> argv[] = {
      Nan::New(utils::uint64ToString(total_bytes_)).ToLocalChecked(),
      Nan::New(utils::uint64ToString(available_bytes_)).ToLocalChecked()};
  Nan::AsyncResource resource("greenworks:CloudQuotaGetWorker.HandleOKCallback");
  callback->Call(2, argv, &resource);
}

ActivateAchievementWorker::ActivateAchievementWorker(
    Nan::Callback* success_callback, Nan::Callback* error_callback,
    const std::string& achievement):
        SteamAsyncWorker(success_callback,
        error_callback), achievement_(achievement) {
}

void ActivateAchievementWorker::Execute() {
  ISteamUserStats* steam_user_stats = SteamUserStats();

  if (!steam_user_stats->SetAchievement(achievement_.c_str())) {
    SetErrorMessage("Achievement name is not valid.");
    return;
  }
  if (!steam_user_stats->StoreStats()) {
    SetErrorMessage("Error on storing user achievement");
    return;
  }
}

GetAchievementWorker::GetAchievementWorker(
    Nan::Callback* success_callback,
    Nan::Callback* error_callback,
    const std::string& achievement):
        SteamAsyncWorker(success_callback, error_callback),
        achievement_(achievement),
        is_achieved_(false) {
}

void GetAchievementWorker::Execute() {
  ISteamUserStats* steam_user_stats = SteamUserStats();
  bool success = steam_user_stats->GetAchievement(achievement_.c_str(),
                                                  &is_achieved_);
  if (!success) {
    SetErrorMessage("Achivement name is not valid.");
  }
}

void GetAchievementWorker::HandleOKCallback() {
  Nan::HandleScope scope;
  v8::Local<v8::Value> argv[] = { Nan::New(is_achieved_) };
  Nan::AsyncResource resource("greenworks:GetAchievementWorker.HandleOKCallback");
  callback->Call(1, argv, &resource);
}

ClearAchievementWorker::ClearAchievementWorker(
    Nan::Callback* success_callback,
    Nan::Callback* error_callback,
    const std::string& achievement):
        SteamAsyncWorker(success_callback, error_callback),
        achievement_(achievement),
        success_(false) {
}

void ClearAchievementWorker::Execute() {
  ISteamUserStats* steam_user_stats = SteamUserStats();
  success_ = steam_user_stats->ClearAchievement(achievement_.c_str());
  if (!success_) {
    SetErrorMessage("Achievement name is not valid.");
    return;
  }
  if (!steam_user_stats->StoreStats()) {
    SetErrorMessage("Fails on uploading user stats to server.");
    return;
  }
}

GetNumberOfPlayersWorker::GetNumberOfPlayersWorker(
    Nan::Callback* success_callback, Nan::Callback* error_callback)
       :SteamCallbackAsyncWorker(success_callback, error_callback),
        num_of_players_(-1) {
}

void GetNumberOfPlayersWorker::Execute() {
  SteamAPICall_t steam_api_call = SteamUserStats()->GetNumberOfCurrentPlayers();
  call_result_.Set(steam_api_call, this,
      &GetNumberOfPlayersWorker::OnGetNumberOfPlayersCompleted);

  WaitForCompleted();
}

void GetNumberOfPlayersWorker::OnGetNumberOfPlayersCompleted(
    NumberOfCurrentPlayers_t* result, bool io_failure) {
  if (io_failure) {
    SetErrorMessage("Error on getting number of players: Steam API IO Failure");
  } else if (result->m_bSuccess) {
    num_of_players_ = result->m_cPlayers;
  } else {
    SetErrorMessage("Error on getting number of players.");
  }
  is_completed_ = true;
}

void GetNumberOfPlayersWorker::HandleOKCallback() {
  Nan::HandleScope scope;

  v8::Local<v8::Value> argv[] = { Nan::New(num_of_players_) };
  Nan::AsyncResource resource("greenworks:GetNumberOfPlayersWorker.HandleOKCallback");
  callback->Call(1, argv, &resource);
}

CreateArchiveWorker::CreateArchiveWorker(Nan::Callback* success_callback,
    Nan::Callback* error_callback, const std::string& zip_file_path,
    const std::string& source_dir, const std::string& password,
    int compress_level)
        :SteamAsyncWorker(success_callback, error_callback),
         zip_file_path_(zip_file_path),
         source_dir_(source_dir),
         password_(password),
         compress_level_(compress_level) {
}

void CreateArchiveWorker::Execute() {
  int result = zip(zip_file_path_.c_str(),
                   source_dir_.c_str(),
                   compress_level_,
                   password_.empty()?nullptr:password_.c_str());
  if (result)
    SetErrorMessage("Error on creating zip file.");
}

ExtractArchiveWorker::ExtractArchiveWorker(Nan::Callback* success_callback,
    Nan::Callback* error_callback, const std::string& zip_file_path,
    const std::string& extract_path, const std::string& password)
        : SteamAsyncWorker(success_callback, error_callback),
          zip_file_path_(zip_file_path),
          extract_path_(extract_path),
          password_(password) {
}

void ExtractArchiveWorker::Execute() {
  int result = unzip(zip_file_path_.c_str(), extract_path_.c_str(),
      password_.empty()?nullptr:password_.c_str());
  if (result)
    SetErrorMessage("Error on extracting zip file.");
}

GetAuthSessionTicketWorker::GetAuthSessionTicketWorker(
  Nan::Callback* success_callback,
  Nan::Callback* error_callback )
    : SteamCallbackAsyncWorker(success_callback, error_callback),
      result(this, &GetAuthSessionTicketWorker::OnGetAuthSessionCompleted),
      handle_(0), ticket_buf_size_(0) {
}

void GetAuthSessionTicketWorker::Execute() {
  handle_ = SteamUser()->GetAuthSessionTicket(ticket_buf_,
                                              sizeof(ticket_buf_),
                                              &ticket_buf_size_);
  WaitForCompleted();
}

void GetAuthSessionTicketWorker::OnGetAuthSessionCompleted(
    GetAuthSessionTicketResponse_t *inCallback) {
  if (inCallback->m_eResult != k_EResultOK)
    SetErrorMessage("Error on getting auth session ticket.");
  is_completed_ = true;
}

void GetAuthSessionTicketWorker::HandleOKCallback() {
  Nan::HandleScope scope;
  v8::Local<v8::Object> ticket = Nan::New<v8::Object>();
  Nan::Set(
      ticket, Nan::New("ticket").ToLocalChecked(),
      Nan::CopyBuffer(reinterpret_cast<char *>(ticket_buf_), ticket_buf_size_)
          .ToLocalChecked());
  Nan::Set(ticket, Nan::New("handle").ToLocalChecked(), Nan::New(handle_));
  v8::Local<v8::Value> argv[] = { ticket };
  Nan::AsyncResource resource("greenworks:GetAuthSessionTicketWorker.HandleOKCallback");
  callback->Call(1, argv, &resource);
}

RequestEncryptedAppTicketWorker::RequestEncryptedAppTicketWorker(
  std::string user_data,
  Nan::Callback* success_callback,
  Nan::Callback* error_callback)
    : SteamCallbackAsyncWorker(success_callback, error_callback),
      user_data_(user_data), ticket_buf_size_(0) {
}

void RequestEncryptedAppTicketWorker::Execute() {
  SteamAPICall_t steam_api_call = SteamUser()->RequestEncryptedAppTicket(
      static_cast<void*>(const_cast<char*>(user_data_.c_str())),
      user_data_.length());
  call_result_.Set(steam_api_call, this,
      &RequestEncryptedAppTicketWorker::OnRequestEncryptedAppTicketCompleted);
  WaitForCompleted();
}

void RequestEncryptedAppTicketWorker::OnRequestEncryptedAppTicketCompleted(
    EncryptedAppTicketResponse_t *inCallback, bool io_failure) {
  if (!io_failure && inCallback->m_eResult == k_EResultOK) {
    SteamUser()->GetEncryptedAppTicket(ticket_buf_, sizeof(ticket_buf_),
        &ticket_buf_size_);
  } else {
    SetErrorMessage("Error on getting encrypted app ticket.");
  }
  is_completed_ = true;
}

void RequestEncryptedAppTicketWorker::HandleOKCallback() {
  Nan::HandleScope scope;
  v8::Local<v8::Value> argv[] = {
      Nan::CopyBuffer(reinterpret_cast<char *>(ticket_buf_), ticket_buf_size_)
          .ToLocalChecked() };
  Nan::AsyncResource resource("greenworks:RequestEncryptedAppTicketWorker.HandleOKCallback");
  callback->Call(1, argv, &resource);
}

ConsumeItemWorker::ConsumeItemWorker(
    Nan::Callback* success_callback,
    Nan::Callback* error_callback,
    SteamItemInstanceID_t itemConsume,
    uint32 unQuantity):
        SteamCallbackAsyncWorker(success_callback, error_callback),
        m_SteamInventoryResult( this, &ConsumeItemWorker::OnSteamInventoryResult ),
        inv_result_(-1),
        itemConsume_(itemConsume),
        unQuantity_(unQuantity)
{
}

void ConsumeItemWorker::Execute() {
  bool success = SteamInventory()->ConsumeItem(&inv_result_, itemConsume_, unQuantity_);

  if (!success) {
    SetErrorMessage("Error - Called from SteamGameServer.");
  }

  WaitForCompleted();
}

void ConsumeItemWorker::HandleOKCallback() {
  Nan::HandleScope scope;

  uint32_t n = item_details_.size();
  v8::Local<v8::Array> arr = Nan::New<v8::Array>(n);
  for (uint32_t i = 0; i < n; i++) {
    SteamItemDetails_t item_details = item_details_[i];
    v8::Local<v8::Object> obj = Nan::New<v8::Object>();
    Nan::Set(obj, Nan::New("m_itemId").ToLocalChecked(), Nan::New(utils::uint64ToString(item_details.m_itemId)).ToLocalChecked());
    Nan::Set(obj, Nan::New("m_iDefinition").ToLocalChecked(), Nan::New<v8::Number>(item_details.m_iDefinition));
    Nan::Set(obj, Nan::New("m_unQuantity").ToLocalChecked(), Nan::New<v8::Number>(item_details.m_unQuantity));
    Nan::Set(obj, Nan::New("m_unFlags").ToLocalChecked(), Nan::New<v8::Number>(item_details.m_unFlags));

    Nan::Set(arr, Nan::New(std::to_string(i)).ToLocalChecked(), obj);
  }

  v8::Local<v8::Value> argv[] = { arr };
  Nan::AsyncResource resource("greenworks:ConsumeItemWorker.HandleOKCallback");
  callback->Call(1, argv, &resource);
}

void ConsumeItemWorker::OnSteamInventoryResult( SteamInventoryResultReady_t *callback ) {
    if (callback->m_handle != inv_result_) return;

    if (callback->m_result != k_EResultOK) {
        SetErrorMessage(("ConsumeItem error code: " + std::to_string(callback->m_result) + ". https://partner.steamgames.com/doc/api/steam_api#EResult").c_str());
    }
    else {
        bool bGotResult = false;
		uint32 count = 0;
		if ( SteamInventory()->GetResultItems( callback->m_handle, NULL, &count ) )
		{
			item_details_.resize( count );
			bGotResult = SteamInventory()->GetResultItems( callback->m_handle, item_details_.data(), &count );
		}

        if ( !bGotResult ) {
            SetErrorMessage("ConsumeItem failed. This indicates that the user has no items, or an internal error.");
        }
    }

    // We're not hanging on the the result after processing it.
	SteamInventory()->DestroyResult( callback->m_handle );

    is_completed_ = true;
}

StartPurchaseWorker::StartPurchaseWorker(
    Nan::Callback* success_callback,
    Nan::Callback* error_callback,
    SteamItemDef_t* pArrayItemDefs,
    uint32_t* punArrayQuantity,
    uint32_t unArrayLength):
        SteamAsyncWorker(success_callback, error_callback),
        pArrayItemDefs_(pArrayItemDefs),
        punArrayQuantity_(punArrayQuantity),
        unArrayLength_(unArrayLength)
{
}

void StartPurchaseWorker::Execute() {
  api_call_ = SteamInventory()->StartPurchase(pArrayItemDefs_, punArrayQuantity_, unArrayLength_);

  if(api_call_ == k_uAPICallInvalid) SetErrorMessage("StartPurchase API call invalid.");
}

ExchangeItemsWorker::ExchangeItemsWorker(
    Nan::Callback* success_callback,
    Nan::Callback* error_callback,
    SteamItemDef_t* pArrayItemDefsGenerate,
    uint32_t* punArrayQuantityGenerate,
    uint32_t unArrayLengthGenerate,
    SteamItemInstanceID_t* pArrayItemInstancesDestroy,
    uint32_t* punArrayQuantityDestroy,
    uint32_t unArrayLengthDestroy):
        SteamCallbackAsyncWorker(success_callback, error_callback),
        m_SteamInventoryResult( this, &ExchangeItemsWorker::OnSteamInventoryResult ),
        pArrayItemDefsGenerate_(pArrayItemDefsGenerate),
        punArrayQuantityGenerate_(punArrayQuantityGenerate),
        unArrayLengthGenerate_(unArrayLengthGenerate),
        pArrayItemInstancesDestroy_(pArrayItemInstancesDestroy),
        punArrayQuantityDestroy_(punArrayQuantityDestroy),
        unArrayLengthDestroy_(unArrayLengthDestroy),
        inv_result_(-1)
{
}

void ExchangeItemsWorker::Execute() {
  bool success = SteamInventory()->ExchangeItems(&inv_result_, pArrayItemDefsGenerate_, punArrayQuantityGenerate_, unArrayLengthGenerate_, pArrayItemInstancesDestroy_, punArrayQuantityDestroy_, unArrayLengthDestroy_);

  if(!success) {
    SetErrorMessage("ExchangeItems failed. Items to generate must be exactly 1.");
  }

  WaitForCompleted();
}

void ExchangeItemsWorker::OnSteamInventoryResult( SteamInventoryResultReady_t *callback ) {
    if (callback->m_handle != inv_result_) return;

    if (callback->m_result != k_EResultOK) {
        SetErrorMessage(("ExchangeItems error code: " + std::to_string(callback->m_result) + ". https://partner.steamgames.com/doc/api/steam_api#EResult").c_str());
    }
    else {
        bool bGotResult = false;
		uint32 count = 0;
		if ( SteamInventory()->GetResultItems( callback->m_handle, NULL, &count ) )
		{
			item_details_.resize( count );
			bGotResult = SteamInventory()->GetResultItems( callback->m_handle, item_details_.data(), &count );
		}

        if ( !bGotResult ) {
            SetErrorMessage("ExchangeItems failed. This indicates that the user has no items, or an internal error.");
        }
    }

    // We're not hanging on the the result after processing it.
	SteamInventory()->DestroyResult( callback->m_handle );

    is_completed_ = true;
}

void ExchangeItemsWorker::HandleOKCallback() {
  Nan::HandleScope scope;

  uint32_t n = item_details_.size();
  v8::Local<v8::Array> arr = Nan::New<v8::Array>(n);
  for (uint32_t i = 0; i < n; i++) {
    SteamItemDetails_t item_details = item_details_[i];
    v8::Local<v8::Object> obj = Nan::New<v8::Object>();
    Nan::Set(obj, Nan::New("m_itemId").ToLocalChecked(), Nan::New(utils::uint64ToString(item_details.m_itemId)).ToLocalChecked());
    Nan::Set(obj, Nan::New("m_iDefinition").ToLocalChecked(), Nan::New<v8::Number>(item_details.m_iDefinition));
    Nan::Set(obj, Nan::New("m_unQuantity").ToLocalChecked(), Nan::New<v8::Number>(item_details.m_unQuantity));
    Nan::Set(obj, Nan::New("m_unFlags").ToLocalChecked(), Nan::New<v8::Number>(item_details.m_unFlags));

    Nan::Set(arr, Nan::New(std::to_string(i)).ToLocalChecked(), obj);
  }

  v8::Local<v8::Value> argv[] = { arr };
  Nan::AsyncResource resource("greenworks:ConsumeItemWorker.HandleOKCallback");
  callback->Call(1, argv, &resource);
}


GetAllItemsWorker::GetAllItemsWorker(
    Nan::Callback* success_callback,
    Nan::Callback* error_callback):
        SteamCallbackAsyncWorker(success_callback, error_callback),
        m_SteamInventoryResult( this, &GetAllItemsWorker::OnSteamInventoryResult ),
	    m_SteamInventoryFullUpdate( this, &GetAllItemsWorker::OnSteamInventoryFullUpdate ),
        inv_result_(-1),
        full_update_(false)
{
}

//We need this to execute SteamInventory()->GetAllItems.
void GetAllItemsWorker::Execute() {
  bool success = SteamInventory()->GetAllItems(&inv_result_);

  if (!success) {
    SetErrorMessage("Error - Called from SteamGameServer.");
  }

  WaitForCompleted();
}

//We need this to pass an argument to our success callback.
void GetAllItemsWorker::HandleOKCallback() {
  Nan::HandleScope scope;

  uint32_t n = item_details_.size();
  v8::Local<v8::Array> arr = Nan::New<v8::Array>(n);
  for (uint32_t i = 0; i < n; i++) {
    SteamItemDetails_t item_details = item_details_[i];
    v8::Local<v8::Object> obj = Nan::New<v8::Object>();
    Nan::Set(obj, Nan::New("m_itemId").ToLocalChecked(), Nan::New(utils::uint64ToString(item_details.m_itemId)).ToLocalChecked());
    Nan::Set(obj, Nan::New("m_iDefinition").ToLocalChecked(), Nan::New<v8::Number>(item_details.m_iDefinition));
    Nan::Set(obj, Nan::New("m_unQuantity").ToLocalChecked(), Nan::New<v8::Number>(item_details.m_unQuantity));
    Nan::Set(obj, Nan::New("m_unFlags").ToLocalChecked(), Nan::New<v8::Number>(item_details.m_unFlags));

    Nan::Set(arr, Nan::New(std::to_string(i)).ToLocalChecked(), obj);
  }

  v8::Local<v8::Value> argv[] = { arr, Nan::New<v8::Boolean>(full_update_) };
  Nan::AsyncResource resource("greenworks:GetAllItemsWorker.HandleOKCallback");
  callback->Call(2, argv, &resource);
}

void GetAllItemsWorker::OnSteamInventoryFullUpdate( SteamInventoryFullUpdate_t *callback ) {
    full_update_ = true;
}

void GetAllItemsWorker::OnSteamInventoryResult( SteamInventoryResultReady_t *callback ) {
    if (callback->m_handle != inv_result_) return;
    
    if (callback->m_result != k_EResultOK) {
        SetErrorMessage(("GetAllItems error code: " + std::to_string(callback->m_result) + ". https://partner.steamgames.com/doc/api/steam_api#EResult").c_str());
    }
    else {
        bool bGotResult = false;
		uint32 count = 0;
		if ( SteamInventory()->GetResultItems( callback->m_handle, NULL, &count ) )
		{
			item_details_.resize( count );
			bGotResult = SteamInventory()->GetResultItems( callback->m_handle, item_details_.data(), &count );
		}

        if ( !bGotResult ) {
            SetErrorMessage("GetAllItems failed. This indicates that the user has no items, or an internal error.");
        }
    }

    // We're not hanging on the the result after processing it.
	SteamInventory()->DestroyResult( callback->m_handle );

    is_completed_ = true;
}

}  // namespace greenworks
