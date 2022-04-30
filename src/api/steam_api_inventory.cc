#include "nan.h"
#include "v8.h"

#include "greenworks_async_workers.h"
#include "steam/steam_api.h"
#include "steam_api_registry.h"

namespace greenworks {
namespace api {
namespace {

NAN_METHOD(ConsumeItem) {
  Nan::HandleScope scope;

  if (info.Length() < 3 || !info[0]->IsString() || !info[1]->IsUint32() || !info[2]->IsFunction()) {
    THROW_BAD_ARGS("Bad arguments");
  }

  Nan::Callback* success_callback = new Nan::Callback(info[2].As<v8::Function>());
  Nan::Callback* error_callback = nullptr;

  if (info.Length() > 3 && info[3]->IsFunction()) 
    error_callback = new Nan::Callback(info[3].As<v8::Function>());
  
  std::string itemConsumeStr = (*(Nan::Utf8String(info[0])));
  uint64_t itemConsume = utils::strToUint64(itemConsumeStr);
  uint32_t unQuantity = Nan::To<uint32>(info[1]).FromJust();

  Nan::AsyncQueueWorker(new greenworks::ConsumeItemWorker(success_callback, error_callback, itemConsume, unQuantity));
  info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(ExchangeItems) {
  Nan::HandleScope scope;

  if (info.Length() < 3 || !info[0]->IsArray() || !info[1]->IsArray() || !info[2]->IsFunction()) {
    THROW_BAD_ARGS("Bad arguments");
  }

  v8::Local<v8::Array> itemsGenerate = info[0].As<v8::Array>();
  v8::Local<v8::Array> itemsDestroy = info[1].As<v8::Array>();

  uint32_t unArrayLengthGenerate = itemsGenerate->Length();
  uint32_t unArrayLengthDestroy = itemsDestroy->Length();

  SteamItemDef_t* pArrayItemDefsGenerate = new SteamItemDef_t[unArrayLengthGenerate];
  SteamItemInstanceID_t* pArrayItemInstancesDestroy = new SteamItemInstanceID_t[unArrayLengthDestroy];

  uint32_t* punArrayQuantityGenerate = new uint32_t[unArrayLengthGenerate];
  uint32_t* punArrayQuantityDestroy = new uint32_t[unArrayLengthDestroy];

  for (uint32_t i = 0; i < unArrayLengthGenerate; ++i) {
    if (!Nan::Get(itemsGenerate, i).ToLocalChecked()->IsObject())
      THROW_BAD_ARGS("Bad arguments");
      
    v8::Local<v8::Object> item = v8::Local<v8::Object>::Cast(Nan::Get(itemsGenerate, i).ToLocalChecked());
    v8::Local<v8::Number> item_def = v8::Local<v8::Number>::Cast(Nan::Get(item, Nan::New("item_def").ToLocalChecked()).ToLocalChecked());
    v8::Local<v8::Number> quantity = v8::Local<v8::Number>::Cast(Nan::Get(item, Nan::New("quantity").ToLocalChecked()).ToLocalChecked());

    if (!item_def->IsInt32() || !quantity->IsUint32()) {
      THROW_BAD_ARGS("Each object in the array must have the 'item_def' and 'quantity' properties.");
    }

    pArrayItemDefsGenerate[i] = Nan::To<SteamItemDef_t>(item_def).FromJust();
    punArrayQuantityGenerate[i] = Nan::To<uint32>(quantity).FromJust();
  }

  for (uint32_t i = 0; i < unArrayLengthDestroy; ++i) {
    if (!Nan::Get(itemsDestroy, i).ToLocalChecked()->IsObject())
      THROW_BAD_ARGS("Bad arguments");
      
    v8::Local<v8::Object> item = v8::Local<v8::Object>::Cast(Nan::Get(itemsDestroy, i).ToLocalChecked());
    v8::Local<v8::String> item_ins = v8::Local<v8::String>::Cast(Nan::Get(item, Nan::New("item_ins").ToLocalChecked()).ToLocalChecked());
    v8::Local<v8::Number> quantity = v8::Local<v8::Number>::Cast(Nan::Get(item, Nan::New("quantity").ToLocalChecked()).ToLocalChecked());

    if (!item_ins->IsString() || !quantity->IsUint32()) {
      THROW_BAD_ARGS("Each object in the array must have the 'item_ins' and 'quantity' properties.");
    }

    pArrayItemInstancesDestroy[i] = utils::strToUint64(*(Nan::Utf8String(item_ins)));
    punArrayQuantityDestroy[i] = Nan::To<uint32>(quantity).FromJust();
  }

  Nan::Callback* success_callback = new Nan::Callback(info[2].As<v8::Function>());
  Nan::Callback* error_callback = nullptr;

  if (info.Length() > 3 && info[3]->IsFunction())
    error_callback = new Nan::Callback(info[3].As<v8::Function>());

  Nan::AsyncQueueWorker(new greenworks::ExchangeItemsWorker(success_callback, error_callback, pArrayItemDefsGenerate, punArrayQuantityGenerate, unArrayLengthGenerate, pArrayItemInstancesDestroy, punArrayQuantityDestroy, unArrayLengthDestroy));
  info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(StartPurchase) {
  Nan::HandleScope scope;

  if (info.Length() < 2 || !info[0]->IsArray() || !info[1]->IsFunction()) {
    THROW_BAD_ARGS("Bad arguments");
  }

  v8::Local<v8::Array> items = info[0].As<v8::Array>();

  uint32_t unArrayLength = items->Length();
  SteamItemDef_t* pArrayItemDefs = new SteamItemDef_t[unArrayLength];
  uint32_t* punArrayQuantity = new uint32_t[unArrayLength];

  for (uint32_t i = 0; i < unArrayLength; ++i) {
    if (!Nan::Get(items, i).ToLocalChecked()->IsObject())
      THROW_BAD_ARGS("Bad arguments");
      
    v8::Local<v8::Object> item = v8::Local<v8::Object>::Cast(Nan::Get(items, i).ToLocalChecked());
    v8::Local<v8::Number> item_def = v8::Local<v8::Number>::Cast(Nan::Get(item, Nan::New("item_def").ToLocalChecked()).ToLocalChecked());
    v8::Local<v8::Number> quantity = v8::Local<v8::Number>::Cast(Nan::Get(item, Nan::New("quantity").ToLocalChecked()).ToLocalChecked());

    if (!item_def->IsInt32() || !quantity->IsUint32()) {
      THROW_BAD_ARGS("Each object in the array must have the 'item_def' and 'quantity' properties.");
    }

    pArrayItemDefs[i] = Nan::To<SteamItemDef_t>(item_def).FromJust();
    punArrayQuantity[i] = Nan::To<uint32>(quantity).FromJust();
  }

  Nan::Callback* success_callback = new Nan::Callback(info[1].As<v8::Function>());
  Nan::Callback* error_callback = nullptr;

  if (info.Length() > 2 && info[2]->IsFunction())
    error_callback = new Nan::Callback(info[2].As<v8::Function>());

  Nan::AsyncQueueWorker(new greenworks::StartPurchaseWorker(success_callback, error_callback, pArrayItemDefs, punArrayQuantity, unArrayLength));
  info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(GetAllItems) {
  Nan::HandleScope scope;

  if (info.Length() < 1 || !info[0]->IsFunction()) {
    THROW_BAD_ARGS("Bad arguments");
  }
  Nan::Callback* success_callback =
      new Nan::Callback(info[0].As<v8::Function>());
  Nan::Callback* error_callback = nullptr;

  if (info.Length() > 1 && info[1]->IsFunction())
    error_callback = new Nan::Callback(info[1].As<v8::Function>());

  Nan::AsyncQueueWorker(new greenworks::GetAllItemsWorker(
      success_callback, error_callback));
  info.GetReturnValue().Set(Nan::Undefined());
}

void RegisterAPIs(v8::Local<v8::Object> target) {
  SET_FUNCTION("consumeItem", ConsumeItem)
  SET_FUNCTION("startPurchase", StartPurchase);
  SET_FUNCTION("exchangeItems", ExchangeItems);
  SET_FUNCTION("getAllItems", GetAllItems);
}

SteamAPIRegistry::Add X(RegisterAPIs);


}  // namespace
}  // namespace api
}  // namespace greenworks
