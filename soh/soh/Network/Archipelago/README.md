# Archipelago integration

This client matches the `Soh_1.4.2` Ocarina of Time APWorld. Install
`oot_soh.apworld` from that release in Archipelago on the host computer before
generating a game.

## Playing

1. Generate and host the multiworld with the `Soh_1.4.2` APWorld.
2. Open the SoH enhancements menu, then open **Network > Archipelago >
   Archipelago Settings**.
3. Enter the server address, slot name, and optional room password. For a server
   on another computer on the same network, use that computer's LAN address and
   port; `localhost` refers to the Android device itself.
4. Connect, return to file select, choose the Archipelago option for a new file,
   and select **Connect and Start Archipelago**.

The settings, console, and hints windows are all available from the Archipelago
section of the Network menu.

## Android connection intents

Android can launch the app and immediately connect it to an Archipelago server.
The explicit intent action is `com.dishii.soh.action.CONNECT_ARCHIPELAGO` and it
accepts the string extras `archipelago_address`, `archipelago_slot`, and the
optional `archipelago_password`:

```sh
adb shell am start -n com.dishii.soh/.MainActivity \
  -a com.dishii.soh.action.CONNECT_ARCHIPELAGO \
  --es archipelago_address archipelago.gg:38281 \
  --es archipelago_slot Player1
```

The equivalent deep-link format is:

```text
soh://archipelago/connect?address=archipelago.gg%3A38281&slot=Player1
```

Add `password` to the deep link or `archipelago_password` to the explicit intent
when the room requires one. Prefer the explicit intent extra for passwords,
because URI parameters can be retained in browser history or system logs. New
intents replace any pending request; if the client is already active, it safely
reconnects using the supplied details.

## TLS certificates

Archipelago passes curl's Mozilla-derived CA bundle to `apclientpp` instead of
relying on a platform-specific system certificate lookup. Android packages the
bundle as `assets/networking/cacert.pem` and copies it to `SOH/networking` before
the native client starts. Android TLS is provided by a pinned BoringSSL build;
other platforms continue to use OpenSSL.

The CA bundle is refreshed when the application is built. Update the pinned TLS
dependency and rebuild periodically when maintaining a release branch.
