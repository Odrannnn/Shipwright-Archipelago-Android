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

## TLS certificates

Archipelago passes curl's Mozilla-derived CA bundle to `apclientpp` instead of
relying on a platform-specific system certificate lookup. Android packages the
bundle as `assets/networking/cacert.pem` and copies it to `SOH/networking` before
the native client starts. Android TLS is provided by a pinned BoringSSL build;
other platforms continue to use OpenSSL.

The CA bundle is refreshed when the application is built. Update the pinned TLS
dependency and rebuild periodically when maintaining a release branch.
