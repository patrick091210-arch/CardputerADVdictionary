# 📖 Patrick's Craft – Fast Dictionary for Cardputer

A lightweight, fast, offline English dictionary running on **M5Stack Cardputer (ESP32-S3)**.

Designed for **speed, simplicity, and readability**, with a terminal-style UI and efficient SD-card lookup.

---

## ✨ Features

* ⚡ Fast dictionary lookup (indexed search)
* 🔎 Prefix & fuzzy matching suggestions
* 📑 Multiple candidate results
* 📄 Pagination for long definitions
* ⌨️ Full keyboard input support
* 💾 SD card-based large dictionary support
* 🧠 Memory-efficient (optimized for ESP32)
* 🚀 Smooth UI (no flicker)

---

## 🖥️ UI Overview

```
Fast Dictionary
----------------
> apple

Result:
Page 1/3
A fruit that grows on trees...

----------------
Enter: next match
Fn+n/p: page  Fn+q: clear
```

---

## 🎮 Controls

| Key    | Function                |
| ------ | ----------------------- |
| Enter  | Search / Next candidate |
| Del    | Delete last character   |
| Fn + Q | Clear input             |
| Fn + N | Next page               |
| Fn + P | Previous page           |

---

## 📂 Required Files (SD Card)

Place these in the **root of the SD card**:

```
/dict_sorted.txt
/dict_index.txt
```

### Format

#### dict_sorted.txt

```
apple=A fruit
ability=The power to do something
about=Concerning something
```

#### dict_index.txt

```
ab=0:10234
ac=10235:20456
...
```

---

## ⚙️ Build & Upload (PlatformIO)

### 1. Install PlatformIO

VS Code → Install **PlatformIO Extension**

---

### 2. Example `platformio.ini`

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino

lib_deps =
    m5stack/M5Cardputer

monitor_speed = 115200
```

---

### 3. Build

```bash
pio run
```

### 4. Upload

```bash
pio run -t upload
```

---

## 🚀 Performance Design

This project uses:

* 📌 **Bucket indexing (2-letter index)**
* 📌 File `seek()` instead of full scan
* 📌 Lazy loading (only read needed chunk)

➡️ Result: **Huge speed improvement vs full dictionary scan**

---

## 🧠 How It Works

1. Input word → normalize
2. Convert to 2-letter bucket (e.g. `ap`)
3. Lookup byte range from `dict_index.txt`
4. Seek directly into dictionary file
5. Scan only relevant region
6. Return result or suggestions

---

## 🔧 Customization

You can tweak:

```cpp
#define PAGE_CHARS 180
```

* Increase → fewer pages, more text per page
* Decrease → cleaner layout, more pages

---

## 🛠 Future Improvements (Optional)

* ⭐ Favorites system
* 🕘 History tracking
* 🔍 Auto-complete while typing
* 🎨 Advanced UI (Kindle-style layout)
* 🌐 Online dictionary sync

---

# 📜 Changelog / Development Log

## v1.0 (Initial)

* Basic dictionary lookup
* SD card reading
* Simple UI

---

## v1.1

* Added candidate suggestions
* Improved matching logic (prefix + contains)

---

## v1.2

* Added paging system
* Fixed long text overflow
* Improved text wrapping

---

## v1.3

* Added bucket index system
* Massive speed improvement
* Reduced full file scan

---

## v1.4

* Fixed UI overflow issue
* Adjusted font size for screen fit
* Improved layout stability

---

## v1.5 (Current)(main.cpp)

* Added splash screen: **Patrick's Craft**
* Optimized rendering (no flicker)
* Improved paging experience
* Cleaner UI structure

---

## 🚀 Author

**Patrick**
Patrick's Craft

---


