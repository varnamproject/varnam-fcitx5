#include "varnam_utils.h"

extern "C" {
#include <libgovarnam/libgovarnam.h>
}

namespace fcitx {

const ::fcitx ::LogCategory &VARNAM() {
  static const ::fcitx ::LogCategory category("varnam");
  return category;
}

bool isWordBreak(FcitxKeySym keyVal, bool inscriptMode) {
  switch (keyVal) {
  case FcitxKey_space:
  case FcitxKey_comma:
  case FcitxKey_period:
  case FcitxKey_question:
  case FcitxKey_exclam:
  case FcitxKey_parenleft:
  case FcitxKey_parenright:
    return true;
  default:
    break;
  }
  if (!inscriptMode) {
    switch (keyVal) {
    case FcitxKey_semicolon:
    case FcitxKey_apostrophe:
    case FcitxKey_quotedbl:
      return true;
    default:
      break;
    }
  }
  return false;
}

const std::string getWordBreakChar(FcitxKeySym keyVal, bool inscriptMode) {
  switch (keyVal) {
  case FcitxKey_space:
    return " ";
  case FcitxKey_comma:
    return ",";
  case FcitxKey_period:
    return ".";
  case FcitxKey_question:
    return "?";
  case FcitxKey_exclam:
    return "!";
  case FcitxKey_parenleft:
    return "(";
  case FcitxKey_parenright:
    return ")";
  default:
    break;
  }

  if (!inscriptMode) {
    switch (keyVal) {
    case FcitxKey_semicolon:
      return ";";
    case FcitxKey_apostrophe:
      return "'";
    case FcitxKey_quotedbl:
      return "\"";
    default:
      break;
    }
  }
  return "";
}

int getNumOfUTFCharUnits(char32_t code_point) {
  if (code_point <= 0x7F) {
    return 1;
  } else if (code_point <= 0x7FF) {
    return 2;
  } else if (code_point <= 0xFFFF) {
    return 3;
  } else if (code_point <= 0x10FFFF) {
    return 4;
  } else {
    // Invalid Unicode code point
    return 0;
  }
}

void varnam_learn_word(int varnam_handle_id, const std::string &word_,
                       int weight) {
  char *word = const_cast<char *>(word_.c_str());
  int rv = varnam_learn(varnam_handle_id, word, weight);
  if (rv != VARNAM_SUCCESS) {
    VARNAM_WARN() << "Failed to learn word:" << word;
  }
}

void varnam_unlearn_word(int varnam_handle_id, const std::string &word_) {
  char *word = const_cast<char *>(word_.c_str());
  int rv = varnam_unlearn(varnam_handle_id, word);
  if (rv != VARNAM_SUCCESS) {
    VARNAM_WARN() << "Failed to unlearn word:" << word;
  }
}

} // namespace fcitx