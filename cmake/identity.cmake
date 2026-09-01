set(PROJECT_NAME darkf)
set(PROJECT_VERSION 0.1)

set(SERVER_NAME darkfded)
set(CLIENT_NAME darkf)

# Must match BASEGAME in code/qcommon/q_shared.h (STANDALONE branch)
set(BASEGAME darkf)

set(CGAME_MODULE cgame)
set(GAME_MODULE qagame)
set(UI_MODULE ui)

set(WINDOWS_ICON_PATH ${CMAKE_SOURCE_DIR}/misc/windows/quake3.ico)

set(MACOS_ICON_PATH ${CMAKE_SOURCE_DIR}/misc/macos/quake3_flat.icns)
set(MACOS_BUNDLE_ID org.ioquake.${CLIENT_NAME})

set(COPYRIGHT "DarkF. Built on ioquake3 and id Tech 3, Copyright © 1999-2005 id Software, Inc. Released under the GNU GPL v2.")

set(CONTACT_EMAIL "cjlynch38@gmail.com")
set(PROTOCOL_HANDLER_SCHEME darkf)
