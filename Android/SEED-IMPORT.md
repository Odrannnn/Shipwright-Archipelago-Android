# Importing a shared randomizer seed on Android

1. Save the received full Ship of Harkinian seed `.json` to your phone, for example in Downloads.
2. Open SoH and return to File Select without loading a save.
3. Open **Randomizer → General → Import Seed JSON** and choose the file in Android's document picker.
4. The app saves its own copy in `SOH/Randomizer` and requests that the randomizer load it. Check the seed hash at File Select, then create a new randomizer save.

Use compatible SoH versions on both devices and compare the five seed-hash icons. The imported seed supplies randomizer settings and placements for the new save; it does not replace an existing save or your graphics/controller preferences. The seed-generation menu's editable settings are separate from the loaded seed's runtime settings.

This importer accepts full seed/spoiler JSON files, not settings-only presets, ROMs, or Archipelago configuration files. Cancelling or rejecting a document leaves the selected seed unchanged. Imports use unique filenames so they do not overwrite another seed.

## Regression checks

`app/src/test/java/com/dishii/soh/SeedJsonImporterTest.java` is a standalone JVM test. Compile it together with `SeedJsonImporter.java` against an executable `org.json` implementation (tested with Maven `org.json:json:20240303`), then run `com.dishii.soh.SeedJsonImporterTest`. Android's SDK `android.jar` contains stubs and cannot execute these tests.
