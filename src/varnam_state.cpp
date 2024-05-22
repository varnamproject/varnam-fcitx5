#include "varnam_state.h"
#include "varnam_candidate.h"
#include "varnam_engine.h"
#include "varnam_utils.h"

#include <algorithm>
#include <fcitx-utils/key.h>
#include <fcitx-utils/stringutils.h>
#include <fcitx-utils/utf8.h>
#include <fcitx/inputpanel.h>

#include <iterator>
#include <sstream>
#include <string>
#include <thread>

namespace fcitx {

VarnamState::VarnamState(VarnamEngine *engine, InputContext &ic)
    : engine_(engine), ic_(&ic) {
  result_ = nullptr;
  cursor = 0;
  bufferPos = 0;
  utfCharPos = 0;
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
  if (preedit_.textLength() < cursor) {
    return;
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
  if (!buffer_.empty() && key.isDigit()) {
    if (auto candidateList = ic_->inputPanel().candidateList();
        candidateList && candidateList->size()) {
      auto idx = key.keyListIndex(selectionKeys);
      if (idx >= 0 && idx < candidateList->size()) {
        selectCandidate(idx);
      }
      keyEvent.filterAndAccept();
      return;
    }
  }

  if (key.states().test(KeyState::Ctrl)) {
    if (buffer_.empty()) {
      keyEvent.filter();
      return;
    }
    if (key.sym() == FcitxKey_Delete) {
      if (preedit_.empty()) {
        keyEvent.filter();
        return;
      }
#ifdef DEBUG_MODE
      VARNAM_INFO() << "unlearn word:" << preedit_.toString();
#endif
      std::string wordToUnlearn(
          preedit_.toStringForCommit()); // [TODO] try unique_ptr<char[]>
      std::thread unlearnThread(varnam_unlearn_word, engine_->getVarnamHandle(),
                                std::move(wordToUnlearn));
      unlearnThread.detach();
      keyEvent.filterAndAccept();
      return;
    }
    keyEvent.filter();
    return;
  }
  if (key.checkKeyList(engine_->getConfig()->nextCandidate.value())) {
    updateLookupTable(NEXT_CANDIDATE);
    keyEvent.filterAndAccept();
    return;
  }
  if (key.checkKeyList(engine_->getConfig()->prevCandidate.value())) {
    updateLookupTable(PREV_CANDIDATE);
    keyEvent.filterAndAccept();
    return;
  }
  if (key.checkKeyList(engine_->getConfig()->nextPage.value())) {
    updateLookupTable(NEXT_PAGE);
    keyEvent.filterAndAccept();
    return;
  }
  if (key.checkKeyList(engine_->getConfig()->prevPage.value())) {
    updateLookupTable(PREV_PAGE);
    keyEvent.filterAndAccept();
    return;
  }

  switch (key.sym()) {
  case FcitxKey_space:
  case FcitxKey_Tab:
  case FcitxKey_Return:
    if (buffer_.empty()) {
      keyEvent.filter();
      return;
    }
    commitPreedit(key.sym());
    updateUI();
    keyEvent.filterAndAccept();
    return;
  case FcitxKey_Escape:
    commitPreedit();
    updateUI();
    keyEvent.filterAndAccept();
    return;
  case FcitxKey_Left: // [TODO] move cursor left
    if (preedit_.empty()) {
      keyEvent.filter();
      return;
    }
    if (cursor > 0) {
      unsigned int offset = getNumOfUTFCharUnits(u32_preedit[utfCharPos]);
      if (cursor >= offset) {
        cursor -= offset;
      }
      if (bufferPos > 0) {
        --bufferPos;
      }
      if (utfCharPos > 0) {
        --utfCharPos;
      }
#ifdef DEBUG_MODE
      VARNAM_INFO() << "[left] cursor at:" << cursor
                    << " buffer pos:" << bufferPos
                    << " preedit cursor: " << utfCharPos;
#endif
    }
    updatePreeditCursor();
    keyEvent.filterAndAccept();
    return;
  case FcitxKey_Right: //[TODO] move cursor right
    if (preedit_.empty()) {
      keyEvent.filter();
      return;
    }
    if (bufferPos < (buffer_.size() - 1)) {
      ++bufferPos;
    }
    if (utfCharPos < (u32_preedit.size() - 1)) {
      ++utfCharPos;
    }
    if (cursor < preedit_.textLength()) {

      cursor += getNumOfUTFCharUnits(u32_preedit[utfCharPos]);
#ifdef DEBUG_MODE
      VARNAM_INFO() << "[right] cursor at:" << cursor
                    << " buffer pos:" << bufferPos
                    << " preedit cursor: " << utfCharPos;
#endif
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
    // [TODO] use cursor position to remove elements
    buffer_.pop_back();
    getVarnamResult();
    updateUI();
    keyEvent.filterAndAccept();
    return;
  case FcitxKey_Delete:
    keyEvent.filter();
    return;
  case FcitxKey_Home:
    keyEvent.filter();
    return;
  case FcitxKey_End:
    keyEvent.filter();
    return;
  default:
    break;
  }
  if (result_) {
    varray_clear(result_);
  } else {
    result_ = varray_init();
  }

  if (key.sym() > 0x80) {
    keyEvent.filter();
    return;
  }

  unsigned char input =
      *(keyEvent.key().toString(KeyStringFormat::Localized).c_str());
#ifdef DEBUG_MODE
  VARNAM_INFO() << "cursor at:" << cursor << " buffer pos:" << bufferPos
                << " buf size:" << buffer_.size();
#endif
  auto iterator = buffer_.begin();
  iterator += bufferPos;
  if (bufferPos >= buffer_.size()) {
    buffer_.push_back(input);
    ++bufferPos;
  } else {
    buffer_.insert(iterator, input);
    ++bufferPos;
  }
  getVarnamResult();
  if (isWordBreak(keyEvent.key().sym())) {
    commitPreedit(keyEvent.key().sym());
  }
  updateUI();
  keyEvent.filterAndAccept();
}

void VarnamState::setLookupTable() {
  if (!result_) {
    return;
  }
  auto candidates_p = std::make_unique<VarnamCandidateList>(engine_, ic_);
  candidates_p->setSelectionKey(selectionKeys);
  candidates_p->setCursorPositionAfterPaging(
      CursorPositionAfterPaging::ResetToFirst);
  candidates_p->setPageSize(engine_->getConfig()->pageSize.value());
  int count = varray_length(result_);
  for (int i = 0; i < count; i++) {
    vword *word = static_cast<vword *>(varray_get(result_, i));
    candidates_p->append<VarnamCandidateWord>(engine_, word->text, i);
  }
  std::string userInput = bufferToString();
  candidates_p->append<VarnamCandidateWord>(engine_, userInput.c_str(), count);
  if (count) {
    candidates_p->setGlobalCursorIndex(0);
    ic_->inputPanel().setCandidateList(std::move(candidates_p));
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
  const Text *candidate_ = &candidateList_->candidate(index).text();
  if (candidate_ == nullptr || candidate_->empty()) {
    return;
  }
  preedit_.clear();
  preedit_.append(candidate_->toString(), TextFormatFlag::HighLight);
  preedit_.setCursor(preedit_.textLength());
  if (ic_->capabilityFlags().test(CapabilityFlag::Preedit)) {
    ic_->inputPanel().setClientPreedit(preedit_);
  } else {
    ic_->inputPanel().setPreedit(preedit_);
  }
  ic_->updatePreedit();
  ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void VarnamState::commitPreedit(const FcitxKeySym &key) {
  std::string stringToCommit;
  stringToCommit.assign(preedit_.toStringForCommit());
  if (!stringToCommit.empty() &&
      engine_->getConfig()->shouldLearnWords.value()) {
#ifdef DEBUG_MODE
    VARNAM_INFO() << "learn word:" << preedit_.toString();
#endif
    std::string wordToLearn(
        preedit_.toStringForCommit()); // [TODO] try unique_ptr<char[]>
    std::thread learnThread(varnam_learn_word, engine_->getVarnamHandle(),
                            std::move(wordToLearn), 0);
    learnThread.detach();
  }

  if (isWordBreak(key)) {
    stringToCommit = stringutils::concat(stringToCommit, getWordBreakChar(key));
  }
#ifdef DEBUG_MODE
  VARNAM_INFO() << "string to commit:" << stringToCommit << " "
                << getWordBreakChar(key);
#endif
  ic_->commitString(stringToCommit);
  reset();
}

void VarnamState::updateUI() {
  vword *first_res = nullptr;
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
  first_res = static_cast<vword *>(varray_get(result_, 0));
  if (first_res == nullptr) {
    return;
  }
  std::string preedit_res(first_res->text);
  if (preedit_res.empty()) {
    return;
  }
  u32_preedit.clear();
  u32_preedit.assign(utf8_converter.from_bytes(preedit_res.c_str()));
  cursor = preedit_res.length();
  utfCharPos = u32_preedit.length() - 1;
  preedit_.clear();
  preedit_.append(preedit_res, TextFormatFlag::HighLight);
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
  cursor = 0;
  bufferPos = 0;
  utfCharPos = 0;
  buffer_.clear();
  preedit_.clear();
  if (result_) {
    varray_clear(result_);
  }
}

} // namespace fcitx