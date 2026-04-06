#include <M5Cardputer.h>
#include <SD.h>
#include <SPI.h>

#define SD_CS 12
#define MAX_RESULTS 8
#define PAGE_CHARS 180   // 每页字符数，避免超出屏幕

String inputWord = "";
String resultText = "Loading...";
String candidates[MAX_RESULTS];
int candidateCount = 0;
int candidateIndex = 0;
bool showingCandidates = false;

// 分页
int currentPage = 0;
int totalPages = 1;

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

void resetPaging() {
    currentPage = 0;
    totalPages = 1;
}

void updatePagingForText(const String& text) {
    int len = text.length();
    totalPages = (len + PAGE_CHARS - 1) / PAGE_CHARS;
    if (totalPages < 1) totalPages = 1;

    if (currentPage < 0) currentPage = 0;
    if (currentPage >= totalPages) currentPage = totalPages - 1;
}

String getPageText(const String& text) {
    updatePagingForText(text);

    int start = currentPage * PAGE_CHARS;
    int end = start + PAGE_CHARS;
    if (end > text.length()) end = text.length();

    String out = text.substring(start, end);

    // 尽量别在单词中间截断
    if (end < text.length()) {
        int lastSpace = out.lastIndexOf(' ');
        if (lastSpace > PAGE_CHARS / 2) {
            out = out.substring(0, lastSpace);
        }
    }

    return out;
}

void clearCandidates() {
    for (int i = 0; i < MAX_RESULTS; i++) candidates[i] = "";
    candidateCount = 0;
    candidateIndex = 0;
    showingCandidates = false;
    resetPaging();
}

void addCandidate(const String& word, const String& meaning) {
    if (candidateCount >= MAX_RESULTS) return;

    String shown = word + " = " + meaning;
    if (shown.length() > 400) shown = shown.substring(0, 400) + "...";

    candidates[candidateCount++] = shown;
}

String lookupWord(const String& rawTarget) {
    String target = normalizeWord(rawTarget);
    if (target.length() == 0) return "Empty input";

    clearCandidates();

    uint32_t start = 0, end = 0;
    String bucket = makeBucketKey(target);

    if (!findBucket(bucket, start, end)) {
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
            resetPaging();
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
        resetPaging();
        return candidates[0];
    }

    if (firstPrefixMeaning != "") {
        resetPaging();
        return firstPrefixMeaning;
    }

    if (firstContainsMeaning != "") {
        resetPaging();
        return firstContainsMeaning;
    }

    resetPaging();
    return "Not found";
}

void drawSplash() {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextColor(WHITE, BLACK);
    M5Cardputer.Display.setTextDatum(middle_center);

    // 主标题：放大但不超屏
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.drawString("Patrick's", 120, 48);
    M5Cardputer.Display.drawString("Craft", 120, 76);

    // 副标题小一号
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.drawString("Fast Dictionary", 120, 108);

    delay(1500);

    M5Cardputer.Display.setTextDatum(top_left);
}

void drawUI() {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextSize(1);

    // 标题
    M5Cardputer.Display.println("Fast Dictionary");
    M5Cardputer.Display.println("----------------");

    // 输入
    M5Cardputer.Display.print("> ");
    M5Cardputer.Display.println(inputWord);
    M5Cardputer.Display.println();

    // 内容
    if (showingCandidates && candidateCount > 0) {
        String pageText = getPageText(candidates[candidateIndex]);

        M5Cardputer.Display.print("Match ");
        M5Cardputer.Display.print(candidateIndex + 1);
        M5Cardputer.Display.print("/");
        M5Cardputer.Display.println(candidateCount);

        M5Cardputer.Display.print("Page ");
        M5Cardputer.Display.print(currentPage + 1);
        M5Cardputer.Display.print("/");
        M5Cardputer.Display.println(totalPages);

        M5Cardputer.Display.println(pageText);
    } else {
        String pageText = getPageText(resultText);

        M5Cardputer.Display.println("Result:");
        M5Cardputer.Display.print("Page ");
        M5Cardputer.Display.print(currentPage + 1);
        M5Cardputer.Display.print("/");
        M5Cardputer.Display.println(totalPages);

        M5Cardputer.Display.println(pageText);
    }

    // 底部提示，控制在屏幕内
    M5Cardputer.Display.println("----------------");
    M5Cardputer.Display.println("Enter: next match");
    M5Cardputer.Display.println("Fn+n/p: page  Fn+q: clear");
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.fillScreen(BLACK);

    drawSplash();

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
    resetPaging();
    drawUI();
}

void loop() {
    M5Cardputer.update();

    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        for (auto c : status.word) {
            if (c >= 32 && c <= 126) {
                inputWord += c;
            }
        }

        if (status.del && inputWord.length() > 0) {
            inputWord.remove(inputWord.length() - 1);
            showingCandidates = false;
            resultText = "Editing...";
            resetPaging();
        }

        if (status.enter) {
            if (showingCandidates && candidateCount > 1) {
                candidateIndex = (candidateIndex + 1) % candidateCount;
                resultText = candidates[candidateIndex];
                resetPaging();
            } else {
                resultText = lookupWord(inputWord);
                resetPaging();
            }
        }

        if (status.fn && status.word.size() > 0) {
            char fnKey = status.word[0];

            if (fnKey == 'q' || fnKey == 'Q') {
                inputWord = "";
                resultText = "Cleared";
                clearCandidates();
                resetPaging();
            }

            if (fnKey == 'n' || fnKey == 'N') {
                if (currentPage < totalPages - 1) {
                    currentPage++;
                }
            }

            if (fnKey == 'p' || fnKey == 'P') {
                if (currentPage > 0) {
                    currentPage--;
                }
            }
        }

        drawUI();
    }

    delay(15);
}