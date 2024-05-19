#ifndef _FCITX_VARNAM_UTILS_H_
#define _FCITX_VARNAM_UTILS_H_

#include <fcitx-utils/key.h>
#include <fcitx-utils/log.h>
#include <string>

namespace fcitx {

typedef struct varnam_word_t {
  const char *text;
  int confidence;
} vword;

enum PageAction { PREV_PAGE, NEXT_PAGE, PREV_CANDIDATE, NEXT_CANDIDATE };

// LOGGERS
const ::fcitx ::LogCategory &VARNAM();
#define VARNAM_INFO() FCITX_LOGC(VARNAM, Info)
#define VARNAM_WARN() FCITX_LOGC(VARNAM, Warn)

const static char INSCRIPT[] = "inscript";

static KeyList selectionKeys = {
    Key{FcitxKey_1}, Key{FcitxKey_2}, Key{FcitxKey_3}, Key{FcitxKey_4},
    Key{FcitxKey_5}, Key{FcitxKey_6}, Key{FcitxKey_7}, Key{FcitxKey_8},
    Key{FcitxKey_9}, Key{FcitxKey_0},
};

// check if the input is a word break character
bool isWordBreak(FcitxKeySym keyVal, bool inscriptMode = false);

// get the word break character
const std::string getWordBreakChar(FcitxKeySym keyVal,
                                   bool inscriptMode = false);

// get the number of unicode character units in a code point
int getNumOfUTFCharUnits(char32_t code_point);

// varnam learn function, to run on a separate thread
void varnam_learn_word(int varnam_handle_id, std::string word_, int weight);

// varnam unlearn function, to run on a separate thread
void varnam_unlearn_word(int varnam_handle_id, std::string word_);

// check the current key and states against a key list and return the index
template <typename Container>
int keyListIndexWithState(const fcitx::Key key, const Container &c,
                          const fcitx::KeyStates states) {
  size_t idx = 0;
  for (const fcitx::Key &toCheck : c) {
    if (key.check(toCheck.sym(), states)) {
      break;
    }
    idx++;
  }
  if (idx == c.size()) {
    return -1;
  }
  return idx;
}

} // namespace fcitx

#endif