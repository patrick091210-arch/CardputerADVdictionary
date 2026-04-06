
#include <M5Cardputer.h>
#include <SD.h>
#include <SPI.h>

#define SD_CS 12
#define MAX_RESULTS 8

String inputWord = "";
String resultText = "Loading...";
String candidates[MAX_RESULTS];
int candidateCount = 0;
int candidateIndex = 0;
bool showingCandidates = false;

struct BucketIndex {
    String key;
    uint32_t start;
    uint32_t end;
};

BucketIndex buckets[900];
int bucketCount = 0;

String normalizeWord(String s) {
    s.trim();
    s.toLowerCase();
    return s;
}

String makeBucketKey(const String& word) {
    if (word.length() == 0) return "__";
    char a = tolower(word[0]);
    char b = (word.length() > 1) ? tolower(word[1]) : '_';

    if (a < 'a' || a > 'z') a = '_';
    if (!((b >= 'a' && b <= 'z') || b == '_')) b = '_';

    String key = "";
    key += a;
    key += b;
    return key;
}

bool loadIndex() {
    File idx = SD.open("/dict_index.txt");
    if (!idx) return false;

    bucketCount = 0;
    while (idx.available() && bucketCount < 900) {
        String line = idx.readStringUntil('\n');
        line.trim();
        if (line.length() == 0 || line.startsWith("#")) continue;

        int p1 = line.indexOf('=');
        int p2 = line.indexOf(':', p1 + 1);
        if (p1 <= 0 || p2 <= p1) continue;

        buckets[bucketCount].key = line.substring(0, p1);
        buckets[bucketCount].start = (uint32_t) line.substring(p1 + 1, p2).toInt();
        buckets[bucketCount].end = (uint32_t) line.substring(p2 + 1).toInt();
        bucketCount++;
    }
    idx.close();
    return bucketCount > 0;
}

bool findBucket(const String& key, uint32_t &start, uint32_t &end) {
    for (int i = 0; i < bucketCount; i++) {
        if (buckets[i].key == key) {
            start = buckets[i].start;
            end = buckets[i].end;
            return true;
        }
    }
    return false;
}

void clearCandidates() {
    for (int i = 0; i < MAX_RESULTS; i++) candidates[i] = "";
    candidateCount = 0;
    candidateIndex = 0;
    showingCandidates = false;
}

void addCandidate(const String& word, const String& meaning) {
    if (candidateCount >= MAX_RESULTS) return;
    String shown = word + " = " + meaning;
    if (shown.length() > 100) shown = shown.substring(0, 100) + "...";
    candidates[candidateCount++] = shown;
}

String lookupWord(const String& rawTarget) {
    String target = normalizeWord(rawTarget);
    if (target.length() == 0) return "Empty input";

    clearCandidates();

    uint32_t start = 0, end = 0;
    String bucket = makeBucketKey(target);
    if (!findBucket(bucket, start, end)) {
        // fallback to one-letter bucket like a_
        String fallback = "";
        fallback += target[0];
        fallback += '_';
        if (!findBucket(fallback, start, end)) {
            return "Not found";
        }
    }

    File file = SD.open("/dict_sorted.txt");
    if (!file) return "No /dict_sorted.txt";
    file.seek(start);

    String firstPrefixMeaning = "";
    String firstContainsMeaning = "";
    String firstExactMeaning = "";

    while (file.position() < end && file.available()) {
        String line = file.readStringUntil('\n');
        if (line.length() == 0) continue;
        line.trim();

        int sep = line.indexOf('=');
        if (sep <= 0) continue;

        String word = line.substring(0, sep);
        String meaning = line.substring(sep + 1);
        String wordN = normalizeWord(word);

        if (wordN == target) {
            file.close();
            return meaning;
        }

        if (wordN.startsWith(target)) {
            if (firstPrefixMeaning == "") firstPrefixMeaning = meaning;
            addCandidate(word, meaning);
        } else if (wordN.indexOf(target) >= 0) {
            if (firstContainsMeaning == "") firstContainsMeaning = meaning;
            addCandidate(word, meaning);
        }

        if (candidateCount >= MAX_RESULTS && firstPrefixMeaning != "") break;
    }

    file.close();

    if (candidateCount > 0) {
        showingCandidates = true;
        candidateIndex = 0;
        return candidates[0];
    }
    if (firstExactMeaning != "") return firstExactMeaning;
    if (firstPrefixMeaning != "") return firstPrefixMeaning;
    if (firstContainsMeaning != "") return firstContainsMeaning;
    return "Not found";
}

void drawUI() {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextSize(1.5);

    M5Cardputer.Display.println("Fast Dictionary");
    M5Cardputer.Display.println("--------------------------");

    M5Cardputer.Display.print("> ");
    M5Cardputer.Display.println(inputWord);
    M5Cardputer.Display.println();

    if (showingCandidates && candidateCount > 0) {
        M5Cardputer.Display.print("Match ");
        M5Cardputer.Display.print(candidateIndex + 1);
        M5Cardputer.Display.print("/");
        M5Cardputer.Display.println(candidateCount);
        M5Cardputer.Display.println(candidates[candidateIndex]);
        M5Cardputer.Display.println();
        M5Cardputer.Display.println("Enter: next");
    } else {
        M5Cardputer.Display.println("Result:");
        String out = resultText;
        if (out.length() > 220) out = out.substring(0, 220) + "...";
        M5Cardputer.Display.println(out);
    }

    M5Cardputer.Display.println();
    M5Cardputer.Display.println("Del: backspace");
    M5Cardputer.Display.println("Fn+q: clear");
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setTextSize(1.5);
    M5Cardputer.Display.fillScreen(BLACK);

    SPI.begin(40, 39, 14, SD_CS);

    if (!SD.begin(SD_CS)) {
        resultText = "SD Failed";
        drawUI();
        return;
    }

    if (!loadIndex()) {
        resultText = "Missing /dict_index.txt";
        drawUI();
        return;
    }

    resultText = "Ready";
    drawUI();
}

void loop() {
    M5Cardputer.update();

    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        for (auto c : status.word) {
            if (c >= 32 && c <= 126) inputWord += c;
        }

        if (status.del && inputWord.length() > 0) {
            inputWord.remove(inputWord.length() - 1);
            showingCandidates = false;
            resultText = "Editing...";
        }

        if (status.enter) {
            if (showingCandidates && candidateCount > 1) {
                candidateIndex = (candidateIndex + 1) % candidateCount;
                resultText = candidates[candidateIndex];
            } else {
                resultText = lookupWord(inputWord);
            }
        }

        if (status.fn && status.word.size() > 0) {
            char fnKey = status.word[0];
            if (fnKey == 'q' || fnKey == 'Q') {
                inputWord = "";
                resultText = "Cleared";
                clearCandidates();
            }
        }

        drawUI();
    }

    delay(15);
}
