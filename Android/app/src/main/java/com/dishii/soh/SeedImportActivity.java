package com.dishii.soh;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.widget.Toast;
import android.util.Log;

import org.json.JSONException;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;

/** Standard-launch host: singleInstance SDLActivity cannot receive document-picker results. */
public final class SeedImportActivity extends Activity {
    private static final int PICK_SEED = 21;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        if (state != null) return;
        Intent picker = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        picker.addCategory(Intent.CATEGORY_OPENABLE);
        // Providers often label downloaded JSON as text/plain or application/octet-stream.
        picker.setType("*/*");
        try {
            startActivityForResult(picker, PICK_SEED);
        } catch (ActivityNotFoundException e) {
            Toast.makeText(this, "No document picker is available.", Toast.LENGTH_LONG).show();
            finish();
        }
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request != PICK_SEED) return;
        Uri uri = data == null ? null : data.getData();
        if (result != RESULT_OK || uri == null) {
            finish();
            return;
        }
        Toast.makeText(this, "Importing seed JSON…", Toast.LENGTH_SHORT).show();
        new Thread(() -> {
            try (InputStream input = getContentResolver().openInputStream(uri)) {
                File directory = new File(Environment.getExternalStorageDirectory(), "SOH/Randomizer");
                File saved = SeedJsonImporter.importSeed(input, directory);
                runOnUiThread(() -> {
                    Intent intent = new Intent(this, MainActivity.class);
                    intent.setAction(MainActivity.ACTION_IMPORT_SEED);
                    intent.putExtra(MainActivity.EXTRA_SEED_PATH, saved.getAbsolutePath());
                    startActivity(intent);
                    finish();
                });
            } catch (IOException | JSONException | SecurityException e) {
                Log.w("SoH", "Seed JSON import failed", e);
                runOnUiThread(() -> {
                    Toast.makeText(this, e instanceof JSONException
                            ? "Not a compatible SoH seed JSON. No seed imported."
                            : "Could not import seed JSON: " + e.getMessage(), Toast.LENGTH_LONG).show();
                    finish();
                });
            }
        }, "SeedJsonImport").start();
    }
}
