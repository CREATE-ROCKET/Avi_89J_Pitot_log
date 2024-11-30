#include "SD_fast.h"
#include <FS.h>
#include <SD_MMC.h>
#include "lib.h"
#include "debug.h"

#if !defined(DEBUG) || defined(SD_FAST)

namespace sd_mmc {
  void _listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root) {
      Serial.println("Failed to open directory");
      return;
    }
    if (!root.isDirectory()) {
      Serial.println("Not a directory");
      return;
    }

    File file = root.openNextFile();
    while (file) {
      if (file.isDirectory()) {
        Serial.print("  DIR : ");
        Serial.println(file.name());
        if (levels) {
          _listDir(fs, file.path(), levels - 1);
        }
      } else {
        Serial.print("  FILE: ");
        Serial.print(file.name());
        Serial.print("  SIZE: ");
        Serial.println(file.size());
      }
      file = root.openNextFile();
    }
  }

  int init() {
    if (!SD_MMC.begin()) {
        pr_debug("SD_init failed");
        return 1; // init failed
    }
    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
        pr_debug("No SD_MMC card attached");
        return 2; // sd none
    }
    #ifdef DEBUG
    Serial.print("SD_MMC Card Type: ");
    if (cardType == CARD_MMC) {
        pr_debug("MMC");
    } else if (cardType == CARD_SD) {
        pr_debug("SDSC");
    } else if (cardType == CARD_SDHC) {
        pr_debug("SDHC");
    } else {
        pr_debug("UNKNOWN");
    }

    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    pr_debug("SD_MMC Card Size: %lluMB\n", cardSize);
    _listDir(SD_MMC, "/", 0);
    #endif
    return 0; // sd init success
  }
}
#endif