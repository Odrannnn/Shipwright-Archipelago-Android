# Permanent Android release signing

All release APKs use the same private keystore. Release builds fail when signing
configuration is missing; debug builds retain normal Android debug signing.

Keep encrypted backups of the keystore and its passwords outside this repository.
Do not generate a replacement key for routine releases: Android updates require the
same signing identity. Losing this key prevents compatible updates.

Configure these GitHub Actions repository secrets before a release:

| Secret | Value |
| --- | --- |
| `ANDROID_KEYSTORE_BASE64` | Base64 encoding of the permanent keystore |
| `ANDROID_KEYSTORE_PASSWORD` | Keystore password |
| `ANDROID_KEY_ALIAS` | Signing entry alias |
| `ANDROID_KEY_PASSWORD` | Signing entry password |

Workflows decode the key into the runner temporary directory and delete that copy
on completion. The keystore must never be committed or uploaded as an artifact.

For local builds set `ANDROID_KEYSTORE_PATH` to the absolute keystore path and set
the three password/alias environment variables above, then run `./gradlew assembleRelease`
from `Android`. Supply passwords through your local secret manager or a private
environment file, rather than command-line arguments or shell history.

Older releases used temporary debug keys. This permanent identity cannot update
those installations unless they already share its certificate. Back up saves before
the one-time uninstall/reinstall needed to move to the permanent signing identity.

The pinned release certificate SHA-256 is:

```text
8032fd2a79885883916a0de46e3fb8e64b21db502b96c482ee0b55fba85a0464
```

Both release workflows verify the APK signature and this fingerprint before publishing.
