#include "varnam_state.h"
#include "varnam_candidate.h"
#include "varnam_engine.h"
#include "varnam_utils.h"

#include <fcitx-utils/capabilityflags.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/keysymgen.h>
#include <fcitx-utils/stringutils.h>
#include <fcitx-utils/utf8.h>
#include <fcitx/inputpanel.h>

#include <iterator>
#include <limits>
#include <sstream>
#include <string>
#include <thread>

namespace fcitx {

VarnamState::VarnamState(VarnamEngine *engine, InputContext &ic)
    : ic_(&ic), engine_(engine) {
  result_ = nullptr;
  cursor = std::numeric_limits<unsigned int>::max();
  candidateSelected = 0;
  lastTypedCharIsDigit = false;
}

VarnamState::~VarnamState() {
  if (result_) {
    varray_free(result_, nullptr);
  }
}

std::string VarnamState::bufferToString() {
  std::string str;
  std::stringstream strstream;
  for (auto ch : buffer_) {
    strstream << ch;
  }
  str.assign(strstream.str());
  return str;
}

void VarnamState::updatePreeditCursor() {
  if (cursor > preedit_.textLength()) {
    cursor = preedit_.textLength();
  }
  preedit_.setCursor(cursor);
  if (ic_->capabilityFlags().test(CapabilityFlag::Preedit)) {
    ic_->inputPanel().setClientPreedit(preedit_);
  } else {
    ic_->inputPanel().setPreedit(preedit_);
  }
  ic_->updatePreedit();
  ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
}

bool VarnamState::getVarnamResult() {
  std::string preedit = bufferToString();
#ifdef DEBUG_MODE
  VARNAM_INFO() << "transliterate preedit:" << preedit;
#endif
  int rv = VARNAM_SUCCESS;
  rv = varnam_transliterate(engine_->getVarnamHandle(), 1,
                            (char *)preedit.c_str(), &result_);
  if (rv != VARNAM_SUCCESS) {
    VARNAM_WARN() << "varnam transliterate failed! err:" << rv;
    return false;
  }
  return true;
}

void VarnamState::processKeyEvent(KeyEvent &keyEvent) {
#ifdef DEBUG_MODE
  VARNAM_INFO() << "rcvd key:"
                << keyEvent.key().toString(KeyStringFormat::Localized);
#endif
  auto key = keyEvent.key();

  // filter modififer keys
  if (key.checkKeyList(keyListToFilter)) {
    keyEvent.filter();
    return;
  }

  // handle candidate selection through index key
  if (!buffer_.empty() && key.isDigit() && !lastTypedCharIsDigit) {
    auto idx = key.keyListIndex(selectionKeys);
    selectCandidate(idx);
    commitText(key.sym());
    updateUI();
    keyEvent.filterAndAccept();
    return;
  }

  if (key.states().test(KeyState::Ctrl)) {
    if (buffer_.empty()) {
      keyEvent.filter();
      return;
    }
    if (key.sym() == FcitxKey_Delete) {
      auto candidates = ic_->inputPanel().candidateList();
      std::string wordToUnlearn(
          candidates->candidate(candidateSelected)
              .text()
              .toStringForCommit()); // TODO try unique_ptr<char[]>
      if (wordToUnlearn.empty()) {
        keyEvent.filter();
        return;
      }
#ifdef DEBUG_MODE
      VARNAM_INFO() << "unlearn word:" << wordToUnlearn;
#endif
      std::thread unlearnThread(varnam_unlearn_word, engine_->getVarnamHandle(),
                                std::move(wordToUnlearn));
      unlearnThread.detach();
      reset();
      updateUI();
      keyEvent.filterAndAccept();
      return;
    }
    keyEvent.filter();
    return;
  }
  if (key.checkKeyList(engine_->getConfig()->nextCandidate.value())) {
    if (buffer_.empty()) {
      keyEvent.filter();
      return;
    }
    updateLookupTable(NEXT_CANDIDATE);
    keyEvent.filterAndAccept();
    return;
  }
  if (key.checkKeyList(engine_->getConfig()->prevCandidate.value())) {
    if (buffer_.empty()) {
      keyEvent.filter();
      return;
    }
    updateLookupTable(PREV_CANDIDATE);
    keyEvent.filterAndAccept();
    return;
  }
  if (key.checkKeyList(engine_->getConfig()->nextPage.value())) {
    if (buffer_.empty()) {
      keyEvent.filter();
      return;
    }
    updateLookupTable(NEXT_PAGE);
    keyEvent.filterAndAccept();
    return;
  }
  if (key.checkKeyList(engine_->getConfig()->prevPage.value())) {
    if (buffer_.empty()) {
      keyEvent.filter();
      return;
    }
    updateLookupTable(PREV_PAGE);
    keyEvent.filterAndAccept();
    return;
  }

  auto iterator = buffer_.begin();
  iterator += cursor;

  switch (key.sym()) {
  case FcitxKey_Escape:
  case FcitxKey_space:
  case FcitxKey_Tab:
  case FcitxKey_Return:
    if (buffer_.empty()) {
      keyEvent.filter();
      return;
    }
    commitText(key.sym());
    updateUI();
    keyEvent.filterAndAccept();
    return;
  case FcitxKey_Left:
    if (preedit_.empty()) {
      keyEvent.filter();
      return;
    }
    if (cursor > 0) {
      --cursor;
    }
    updatePreeditCursor();
    keyEvent.filterAndAccept();
    return;
  case FcitxKey_Right:
    if (preedit_.empty()) {
      keyEvent.filter();
      return;
    }
    if (cursor < (buffer_.size())) {
      ++cursor;
    }
    updatePreeditCursor();
    keyEvent.filterAndAccept();
    return;
  case FcitxKey_Up:
  case FcitxKey_Down:
    keyEvent.filter();
    return;
  case FcitxKey_BackSpace:
    if (buffer_.empty()) {
      keyEvent.filter();
      return;
    }
    if (cursor > 0) {
      buffer_.erase(--iterator);
      --cursor;
    }
    getVarnamResult();
    updateUI();
    keyEvent.filterAndAccept();
    return;
  case FcitxKey_Delete:
    if (buffer_.empty()) {
      keyEvent.filter();
      return;
    }
    if (cursor < buffer_.size()) {
      buffer_.erase(iterator);
    }
    getVarnamResult();
    updateUI();
    keyEvent.filterAndAccept();
    return;
  case FcitxKey_Home:
    if (buffer_.empty()) {
      keyEvent.filter();
      return;
    }
    cursor = 0;
    updatePreeditCursor();
    keyEvent.filterAndAccept();
    return;
  case FcitxKey_End:
    if (buffer_.empty()) {
      keyEvent.filter();
      return;
    }
    cursor = preedit_.textLength();
    updatePreeditCursor();
    keyEvent.filterAndAccept();
    return;
  default:
    break;
  }

  if (key.isDigit()) {
    lastTypedCharIsDigit = true;
  } else {
    lastTypedCharIsDigit = false;
  }

  if (isWordBreak(keyEvent.key().sym())) {
    commitText(keyEvent.key().sym());
    updateUI();
    keyEvent.filterAndAccept();
    return;
  }

  if (key.sym() > 0x80) {
    keyEvent.filter();
    return;
  }

  unsigned char input =
      *(keyEvent.key().toString(KeyStringFormat::Localized).c_str());
#ifdef DEBUG_MODE
  VARNAM_INFO() << "cursor at:" << cursor;
#endif
  if (cursor >= buffer_.size()) {
    buffer_.push_back(input);
    cursor = buffer_.size();
  } else {
    buffer_.insert(iterator, input);
    ++cursor;
  }

  if (result_) {
    varray_clear(result_);
  } else {
    result_ = varray_init();
  }

  getVarnamResult();
  updateUI();
  keyEvent.filterAndAccept();
}

void VarnamState::setLookupTable() {
  if (!result_) {
    return;
  }
  auto candidates = std::make_unique<VarnamCandidateList>(engine_, ic_);
  candidates->setSelectionKey(selectionKeys);
  candidates->setCursorPositionAfterPaging(
      CursorPositionAfterPaging::ResetToFirst);
  candidates->setPageSize(engine_->getConfig()->pageSize.value());

  int count = varray_length(result_);
  char preeditAppended = 0;
  for (int i = 0; i < count; i++) {
    if ((candidates->pageSize() == 10) &&
        ((i + (preeditAppended ? (1 + preeditAppended) : 1)) % 10 == 0)) {
      // TODO ;}
      candidates->append<VarnamCandidateWord>(engine_,
                                              preedit_.toString().c_str(), i);
      ++preeditAppended;
    }
    vword *word = static_cast<vword *>(varray_get(result_, i));
    candidates->append<VarnamCandidateWord>(engine_, word->text,
                                            preeditAppended ? i + 1 : i);
  }
  if (!preeditAppended) {
    candidates->append<VarnamCandidateWord>(
        engine_, preedit_.toString().c_str(), ++count);
  }
  if (count) {
    candidates->setGlobalCursorIndex(0);
    ic_->inputPanel().setCandidateList(std::move(candidates));
  }
}

void VarnamState::updateLookupTable(const PageAction &action) {
  auto candidates = ic_->inputPanel().candidateList();
  if (candidates == nullptr) {
    return;
  }
  if (candidates->empty()) {
    return;
  }
  switch (action) {
  case PREV_PAGE:
    candidates->toPageable()->prev();
    break;
  case NEXT_PAGE:
    candidates->toPageable()->next();
    break;
  case PREV_CANDIDATE:
    candidates->toCursorMovable()->prevCandidate();
    break;
  case NEXT_CANDIDATE:
    candidates->toCursorMovable()->nextCandidate();
    break;
  }
}

void VarnamState::selectCandidate(int index) {
  auto candidateList_ = ic_->inputPanel().candidateList();
#ifdef DEBUG_MODE
  VARNAM_INFO() << "select candidate at :" << index;
#endif
  if (!candidateList_ || candidateList_->size() <= index) {
    return;
  }
  if (index == 9) {
    candidateSelected = 0;
  } else {
    if (candidateList_->size() <= index) {
      return;
    }
    candidateSelected = index;
  }
}

void VarnamState::commitText(const FcitxKeySym &key) {
    auto candidates = ic_->inputPanel().candidateList();
    std::string stringToCommit;
    std::string wordToLearn;
    bool isWordBreakKey = isWordBreak(key);
    bool enablePunctuation = engine_->getConfig()->enablePunctuation.value();

    if (isWordBreakKey) {
        if (candidates && candidates->cursorIndex() >= 0) {
            stringToCommit = candidates->candidate(candidates->cursorIndex()).text().toStringForCommit();
            wordToLearn = stringToCommit;
            candidateSelected = 1;
        } else {
            stringToCommit = preedit_.toStringForCommit();
        }

        if (enablePunctuation) { // If punctuation processing is enabled
            buffer_.clear(); // Clear buffer
            buffer_.push_back(getWordBreakChar(key)[0]); // Add word break char to buffer

            // Concatenate result word if available
            if (getVarnamResult() && result_ && varray_length(result_) > 0) {
                vword *result_word = static_cast<vword *>(varray_get(result_, 0));
                if (result_word) {
                    stringToCommit = stringutils::concat(stringToCommit, result_word->text);
                }
            }
        } else {
            stringToCommit += getWordBreakChar(key);
        }
    } else if (key == FcitxKey_Escape || key == FcitxKey_0 ||
               !candidates || candidates->size() <= 1 || !result_ || varray_is_empty(result_)) {
        // Handle escape or special keys
        stringToCommit.assign(preedit_.toStringForCommit());
        candidateSelected = 0;
    } else if ((candidates->cursorIndex() <= 0) && !candidateSelected) {
        // Handle the case where no candidate is selected
        vword *first_result = static_cast<vword *>(varray_get(result_, 0));
        if (first_result) {
            stringToCommit.assign(first_result->text);
            wordToLearn = stringToCommit;
            candidateSelected = 1;
        }
    } else {
        // Handle regular candidate selection
        stringToCommit.assign(candidates->candidate(candidateSelected).text().toStringForCommit());
        wordToLearn = stringToCommit;
    }

    ic_->commitString(stringToCommit);

    // Early exit if learning should be skipped
    if (stringToCommit.empty() || lastTypedCharIsDigit ||
        ic_->capabilityFlags().test(CapabilityFlag::PasswordOrSensitive) ||
        !engine_->getConfig()->shouldLearnWords.value() || !candidateSelected) {
        reset();
        return;
    }

#ifdef DEBUG_MODE
    VARNAM_INFO() << "string to commit:" << stringToCommit;
#endif

#ifdef DEBUG_MODE
    VARNAM_INFO() << "Word To Learn:" << wordToLearn; // After adjustments
#endif

    // Start the learning thread with the word to learn (without word break or digit)
    std::thread learnThread(varnam_learn_word, engine_->getVarnamHandle(),
                            std::move(wordToLearn), 0);
    learnThread.detach();

    reset();
}


void VarnamState::updateUI() {
  ic_->inputPanel().reset();
  if (buffer_.empty()) {
    ic_->updatePreedit();
    ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
    return;
  }
  if (result_ == nullptr) {
    return;
  }
  if (varray_length(result_) == 0x00) {
    return;
  }

  preedit_.clear();
  preedit_.append(bufferToString(), TextFormatFlag::HighLight);
  if (cursor > preedit_.textLength()) {
    cursor = preedit_.textLength();
  }
  preedit_.setCursor(cursor);

  if (ic_->capabilityFlags().test(CapabilityFlag::Preedit)) {
    ic_->inputPanel().setClientPreedit(preedit_);
  } else {
    ic_->inputPanel().setPreedit(preedit_);
  }
  ic_->updatePreedit();
  setLookupTable();
  ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void VarnamState::reset() {
  cursor = std::numeric_limits<unsigned int>::max();
  candidateSelected = 0;
  lastTypedCharIsDigit = false;
  buffer_.clear();
  preedit_.clear();
  if (result_) {
    varray_clear(result_);
  }
}

} // namespace fcitx