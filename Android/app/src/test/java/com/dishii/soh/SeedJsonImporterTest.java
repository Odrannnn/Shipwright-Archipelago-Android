package com.dishii.soh;

import org.json.JSONException;
import org.json.JSONObject;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.Arrays;

/** Standalone JVM regression tests; run with an executable org.json implementation. */
public final class SeedJsonImporterTest {
    private static final String VALID = "{\"version\":\"9.2.1-p7\",\"seed\":\"sample\",\"finalSeed\":4294967295,"
            + "\"file_hash\":[0,1,50,98,99],\"settings\":{\"Logic Rules\":\"Glitchless\"},"
            + "\"locations\":{\"Kokiri Sword Chest\":\"Kokiri Sword\"},\"entrances\":[],"
            + "\"enabledTricks\":[],\"requiredTrials\":[],\"masterQuestDungeons\":[],\"excludedLocations\":[]}";
    private static int checks;

    public static void main(String[] args) throws Exception {
        SeedJsonImporter.validate(VALID);
        checks++;
        rejects("{}");
        rejects("{\"settings\":{\"Logic Rules\":\"Glitchless\"}}");
        rejects(VALID + " {}");
        rejects(VALID.replace("[0,1,50,98,99]", "[0,1,2,3,100]"));
        rejects(VALID.replace("[0,1,50,98,99]", "[-1,1,2,3,4]"));
        rejects(VALID.replace("[0,1,50,98,99]", "[0,1,2,3]"));
        rejects(VALID.replace("[0,1,50,98,99]", "[0,1,2,3,4,5]"));
        rejects(VALID.replace("[0,1,50,98,99]", "[0,1,2,3,4.5]"));
        rejects(VALID.replace("\"version\":\"9.2.1-p7\",", ""));
        rejects(VALID.replace("9.2.1-p7", ""));
        rejects(VALID.replace("4294967295", "4294967296"));
        File directory = Files.createTempDirectory("soh-seed-import-test-").toFile();
        try {
            File existing = new File(directory, "existing.json");
            byte[] original = "existing seed must survive".getBytes(StandardCharsets.UTF_8);
            Files.write(existing.toPath(), original);
            File first = SeedJsonImporter.importSeed(input(VALID), directory);
            File second = SeedJsonImporter.importSeed(input("\uFEFF" + VALID), directory);
            check(!first.equals(second), "Imports must use unique filenames");
            check(new JSONObject(Files.readString(first.toPath())).getString("seed").equals("sample"), "Saved JSON contents");
            check(first.getName().endsWith(".json"), "Native drop JSON extension");
            int before = directory.list().length;
            try {
                SeedJsonImporter.importSeed(input("{}"), directory);
                throw new AssertionError("Invalid file accepted");
            } catch (JSONException expected) { checks++; }
            try {
                SeedJsonImporter.importSeed(new InputStream() {
                    int remaining = SeedJsonImporter.MAX_BYTES + 1;
                    public int read() { return remaining-- > 0 ? ' ' : -1; }
                    public int read(byte[] b, int offset, int length) {
                        if (remaining == 0) return -1;
                        int count = Math.min(remaining, length);
                        Arrays.fill(b, offset, offset + count, (byte) ' ');
                        remaining -= count;
                        return count;
                    }
                }, directory);
                throw new AssertionError("Oversized file accepted");
            } catch (IOException expected) { check(expected.getMessage().contains("16 MiB"), "Size bound"); }
            try {
                SeedJsonImporter.importSeed(new ByteArrayInputStream(new byte[]{(byte) 0xff}), directory);
                throw new AssertionError("Invalid UTF-8 accepted");
            } catch (IOException expected) { checks++; }
            check(directory.list().length == before, "Failed imports leave no partial files");
            check(Arrays.equals(original, Files.readAllBytes(existing.toPath())), "Existing file preserved");
        } finally {
            for (File file : directory.listFiles()) Files.delete(file.toPath());
            Files.delete(directory.toPath());
        }
        System.out.println("SeedJsonImporter: " + checks + " checks passed");
    }

    private static ByteArrayInputStream input(String value) {
        return new ByteArrayInputStream(value.getBytes(StandardCharsets.UTF_8));
    }
    private static void rejects(String value) throws Exception {
        try { SeedJsonImporter.validate(value); } catch (JSONException expected) { checks++; return; }
        throw new AssertionError("Invalid seed accepted: " + value);
    }
    private static void check(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
        checks++;
    }
}
