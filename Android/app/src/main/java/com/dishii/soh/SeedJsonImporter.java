package com.dishii.soh;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONTokener;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.StandardCharsets;
import java.util.Iterator;

/** Bounded import and structural checks for the native SoH spoiler parser. */
final class SeedJsonImporter {
    static final int MAX_BYTES = 16 * 1024 * 1024;

    static File importSeed(InputStream input, File directory) throws IOException, JSONException {
        if (input == null) throw new IOException("The selected document could not be opened.");
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        byte[] buffer = new byte[32768];
        int count;
        while ((count = input.read(buffer)) != -1) {
            if (count > MAX_BYTES - bytes.size()) throw new IOException("Seed JSON exceeds 16 MiB.");
            bytes.write(buffer, 0, count);
        }
        byte[] data = bytes.toByteArray();
        String json;
        try {
            json = StandardCharsets.UTF_8.newDecoder().onMalformedInput(CodingErrorAction.REPORT)
                    .onUnmappableCharacter(CodingErrorAction.REPORT).decode(ByteBuffer.wrap(data)).toString();
        } catch (CharacterCodingException e) {
            throw new IOException("Seed JSON must be UTF-8.", e);
        }
        // Android's JSON reader tolerates a BOM; strip it before native parsing too.
        if (json.startsWith("\uFEFF")) json = json.substring(1);
        validate(json);
        if (!directory.isDirectory() && !directory.mkdirs()) throw new IOException("Cannot create the Randomizer folder.");
        File output = File.createTempFile("imported-seed-", ".json", directory);
        boolean complete = false;
        try {
            try (FileOutputStream stream = new FileOutputStream(output)) {
                // Re-serialize: org.json accepts some nonstandard JSON which native nlohmann does not.
                stream.write(new JSONObject(json).toString().getBytes(StandardCharsets.UTF_8));
                stream.getFD().sync();
            }
            complete = true;
        } finally {
            if (!complete) output.delete();
        }
        return output;
    }

    static void validate(String text) throws JSONException {
        JSONTokener tokener = new JSONTokener(text);
        Object root = tokener.nextValue();
        require(root instanceof JSONObject && tokener.nextClean() == 0);
        JSONObject json = (JSONObject) root;
        require(json.get("version") instanceof String && !json.getString("version").trim().isEmpty());
        require(json.get("seed") instanceof String);
        integer(json.get("finalSeed"), 0, 0xffffffffL);
        JSONArray hash = json.getJSONArray("file_hash");
        require(hash.length() == 5);
        for (int i = 0; i < hash.length(); i++) integer(hash.get(i), 0, 99);
        JSONObject settings = json.getJSONObject("settings");
        require(settings.length() > 0);
        for (Iterator<String> keys = settings.keys(); keys.hasNext();) require(settings.get(keys.next()) instanceof String);
        JSONObject locations = json.getJSONObject("locations");
        require(locations.length() > 0);
        for (Iterator<String> keys = locations.keys(); keys.hasNext();) {
            Object item = locations.get(keys.next());
            if (item instanceof JSONObject) {
                JSONObject detail = (JSONObject) item;
                require(detail.get("item") instanceof String);
                for (String key : new String[]{"model", "trickName"}) {
                    if (detail.has(key)) require(detail.get(key) instanceof String);
                }
                if (detail.has("price")) integer(detail.get("price"), 0, 65535);
            } else require(item instanceof String);
        }
        for (String key : new String[]{"excludedLocations", "enabledTricks", "masterQuestDungeons", "requiredTrials"}) {
            if (!json.isNull(key)) {
                JSONArray values = json.getJSONArray(key);
                for (int i = 0; i < values.length(); i++) require(values.get(i) instanceof String);
            }
        }
        for (String key : new String[]{"Gossip Stone Hints", "Static Hints"}) {
            if (!json.isNull(key)) {
                JSONObject hints = json.getJSONObject(key);
                for (Iterator<String> keys = hints.keys(); keys.hasNext();) require(hints.get(keys.next()) instanceof JSONObject);
            }
        }
        if (!json.isNull("entrances")) {
            JSONArray entrances = json.getJSONArray("entrances");
            for (int i = 0; i < entrances.length(); i++) {
                JSONObject entrance = entrances.getJSONObject(i);
                for (String key : new String[]{"type", "index", "destination", "override", "overrideDestination"}) {
                    integer(entrance.get(key), -32768, 65535);
                }
            }
        }
    }

    private static void integer(Object value, long min, long max) throws JSONException {
        require(value instanceof Integer || value instanceof Long);
        long number = ((Number) value).longValue();
        require(number >= min && number <= max);
    }

    private static void require(boolean valid) throws JSONException {
        if (!valid) throw new JSONException("Not a compatible SoH seed JSON.");
    }
}
