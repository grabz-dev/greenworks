'use strict'

const path = require('path')

// Allow setting a different steamworks_sdk path via an environment variable.
if (process.env.DISCORD_GAME_SDK_PATH) {
  console.log(process.env.DISCORD_GAME_SDK_PATH)
} else {
  // Otherwise, use the default path (deps/steamworks_sdk)
  console.log(path.join(__dirname, '..', 'deps', 'discord_game_sdk'))
}
