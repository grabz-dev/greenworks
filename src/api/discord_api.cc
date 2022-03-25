#include "nan.h"
#include "v8.h"

#include "greenworks_async_workers.h"
#include "cpp/discord.h"
#include "steam_api_registry.h"
#include "discord_api.h"

namespace greenworks {
namespace api {
namespace {

struct DiscordState {
    std::unique_ptr<discord::Core> core;
};
DiscordState discord_state{};
discord::Activity activity{};
uint64_t discord_app_id;
uint32_t steam_app_id;
bool discord_not_installed = false;

NAN_METHOD(InitDiscordAPI) {
  Nan::HandleScope scope;

  if(info.Length() < 2 || !info[0]->IsString() || !info[1]->IsUint32()) {
    THROW_BAD_ARGS("Bad arguments");
  }

  discord_app_id = utils::strToUint64(*(Nan::Utf8String(info[0])));
  steam_app_id = Nan::To<uint32>(info[1]).FromJust();
  std::string msg = "";
  discord::Result result = _InitDiscord(&msg);

  v8::Local<v8::Object> obj = Nan::New<v8::Object>();
  Nan::Set(obj, Nan::New("result").ToLocalChecked(), Nan::New<v8::Number>((int)result));
  Nan::Set(obj, Nan::New("message").ToLocalChecked(), Nan::New<v8::String>(msg).ToLocalChecked());
  
  info.GetReturnValue().Set(obj);
}

NAN_METHOD(SetDiscordActivity) { 
  Nan::HandleScope scope;

  if(discord_not_installed) {
      info.GetReturnValue().Set(Nan::New<v8::Boolean>(false));
      return;
  }
  
  if(info.Length() < 1 || !info[0]->IsObject()) {
    THROW_BAD_ARGS("Bad arguments");
  }

  v8::Local<v8::Object> root = info[0].As<v8::Object>();
  v8::Local<v8::Value> details = Nan::Get(root, Nan::New("details").ToLocalChecked()).ToLocalChecked();
  v8::Local<v8::Value> state = Nan::Get(root, Nan::New("state").ToLocalChecked()).ToLocalChecked();
  v8::Local<v8::Value> smallImage = Nan::Get(root, Nan::New("smallImage").ToLocalChecked()).ToLocalChecked();
  v8::Local<v8::Value> smallText = Nan::Get(root, Nan::New("smallText").ToLocalChecked()).ToLocalChecked();
  v8::Local<v8::Value> largeImage = Nan::Get(root, Nan::New("largeImage").ToLocalChecked()).ToLocalChecked();
  v8::Local<v8::Value> largeText = Nan::Get(root, Nan::New("largeText").ToLocalChecked()).ToLocalChecked();
  v8::Local<v8::Value> timestampStart = Nan::Get(root, Nan::New("timestampStart").ToLocalChecked()).ToLocalChecked();
  v8::Local<v8::Value> timestampEnd = Nan::Get(root, Nan::New("timestampEnd").ToLocalChecked()).ToLocalChecked();

  activity.SetType(discord::ActivityType::Playing);

  if(details->IsString()) {
    activity.SetDetails(*(Nan::Utf8String(details)));
  }
  if(state->IsString()) {
    activity.SetState(*(Nan::Utf8String(state)));
  }
  if(smallImage->IsString()) {
    activity.GetAssets().SetSmallImage(*(Nan::Utf8String(smallImage)));
  }
  if(smallText->IsString()) {
    activity.GetAssets().SetSmallText(*(Nan::Utf8String(smallText)));
  }
  if(largeImage->IsString()) {
    activity.GetAssets().SetLargeImage(*(Nan::Utf8String(largeImage)));
  }
  if(largeText->IsString()) {
    activity.GetAssets().SetLargeText(*(Nan::Utf8String(largeText)));
  }
  if(timestampStart->IsString()) {
    activity.GetTimestamps().SetStart(utils::strToUint64(*(Nan::Utf8String(timestampStart))));
  }
  if(timestampEnd->IsString()) {
    activity.GetTimestamps().SetEnd(utils::strToUint64(*(Nan::Utf8String(timestampEnd))));

  }

  if(discord_state.core) {
      discord_state.core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {});
      info.GetReturnValue().Set(Nan::New<v8::Boolean>(true));
  }
  else {
      info.GetReturnValue().Set(Nan::Null());
  }
}

NAN_METHOD(RunDiscordCallbacks) { 
  Nan::HandleScope scope;

  if(discord_state.core) {
    discord::Result result = discord_state.core->RunCallbacks();

    if(result == discord::Result::NotInstalled && !discord_not_installed) {
      discord_not_installed = true;
    }
    else if(result == discord::Result::NotRunning) {
      discord_not_installed = false;
      
      std::string msg = "";
      _InitDiscord(&msg);
    }

    info.GetReturnValue().Set(Nan::New<v8::Number>(int(result)));
  }
  else {
    std::string msg = "";
    _InitDiscord(&msg);

    info.GetReturnValue().Set(Nan::Null());
  }
}

discord::Result _InitDiscord(std::string* msg) {
  discord::Core* core{};
  auto result = discord::Core::Create(discord_app_id, DiscordCreateFlags_NoRequireDiscord, &core);
  discord_state.core.reset(core);
  if(!discord_state.core) {
    *msg = "Failed to instantiate discord core!";
    return discord::Result::InternalError;
  }

  if(result == discord::Result::Ok) {
    discord_state.core->ActivityManager().RegisterSteam(steam_app_id);
  }
  else {
    *msg = "Discord core exited with error: " + std::to_string((int)result);
    return result;
  }

  *msg = "Discord initialized successfully.";
  return result;
}

void RegisterAPIs(v8::Local<v8::Object> target) {
  SET_FUNCTION("initDiscordAPI", InitDiscordAPI);
  SET_FUNCTION("setDiscordActivity", SetDiscordActivity);
  SET_FUNCTION("runDiscordCallbacks", RunDiscordCallbacks);
}

SteamAPIRegistry::Add X(RegisterAPIs);


}  // namespace
}  // namespace api
}  // namespace greenworks
