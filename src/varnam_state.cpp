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
    : m_ic(&ic), m_engine(engine) {
  m_result = nullptr;
  m_cursor = std::numeric_limits<unsigned int>::max();
  m_candidateSelected = 0;
  m_lastTypedCharIsDigit = false;
}

VarnamState::~VarnamState() {
  if (m_result) {
    varray_free(m_result, nullptr);
  }
}

std::string VarnamState::bufferToString() {
  std::string str;
  std::stringstream strstream;
  for (auto ch : m_buffer) {
    strstream << ch;
  }
  str.assign(strstream.str());
  return str;
}

void VarnamState::updatePreeditCursor() {
  if (m_cursor > m_preedit.textLength()) {
    m_cursor = m_preedit.textLength();
  }
  m_preedit.setCursor(m_cursor);
  if (m_ic->capabilityFlags().test(CapabilityFlag::Preedit)) {
    m_ic->inputPanel().setClientPreedit(m_preedit);
  } else {
    m_ic->inputPanel().setPreedit(m_preedit);
  }
  m_ic->updatePreedit();
  m_ic->updateUserInterface(UserInterfaceComponent::InputPanel);
}

bool VarnamState::getVarnamResult() {
  std::string preedit = bufferToString();
#ifdef DEBUG_MODE
  VARNAM_INFO() << "transliterate preedit:" << preedit;
#endif
  int rv = VARNAM_SUCCESS;
  rv = varnam_transliterate(m_engine->getVarnamHandle(), 1,
                            (char *)preedit.c_str(), &m_result);
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
  if (!m_buffer.empty() && key.isDigit() && !m_lastTypedCharIsDigit) {
    auto idx = key.keyListIndex(selectionKeys);
    selectCandidate(idx);
    commitText(key.sym());
    updateUI();
    keyEvent.filterAndAccept();
    return;
  }

  if (key.states().test(KeyState::Ctrl)) {
    if (m_buffer.empty()) {
      keyEvent.filter();
      return;
    }
    if (key.sym() == FcitxKey_Delete) {
      auto candidates = m_ic->inputPanel().candidateList();
      std::string wordToUnlearn(
          candidates->candidate(m_candidateSelected)
              .text()
              .toStringForCommit()); // TODO try unique_ptr<char[]>
      if (wordToUnlearn.empty()) {
        keyEvent.filter();
        return;
      }
#ifdef DEBUG_MODE
      VARNAM_INFO() << "unlearn word:" << wordToUnlearn;
#endif
      std::thread unlearnThread(varnam_unlearn_word,
                                m_engine->getVarnamHandle(),
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
  if (key.checkKeyList(m_engine->getConfig()->nextCandidate.value())) {
    if (m_buffer.empty()) {
      keyEvent.filter();
      return;
    }
    updateLookupTable(NEXT_CANDIDATE);
    keyEvent.filterAndAccept();
    return;
  }
  if (key.checkKeyList(m_engine->getConfig()->prevCandidate.value())) {
    if (m_buffer.empty()) {
      keyEvent.filter();
      return;
    }
    updateLookupTable(PREV_CANDIDATE);
    keyEvent.filterAndAccept();
    return;
  }
  if (key.checkKeyList(m_engine->getConfig()->nextPage.value())) {
    if (m_buffer.empty()) {
      keyEvent.filter();
      return;
    }
    updateLookupTable(NEXT_PAGE);
    keyEvent.filterAndAccept();
    return;
  }
  if (key.checkKeyList(m_engine->getConfig()->prevPage.value())) {
    if (m_buffer.empty()) {
      keyEvent.filter();
      return;
    }
    updateLookupTable(PREV_PAGE);
    keyEvent.filterAndAccept();
    return;
  }

  auto iterator = m_buffer.begin();
  iterator += m_cursor;

  switch (key.sym()) {
  case FcitxKey_Escape:
  case FcitxKey_space:
  case FcitxKey_Tab:
  case FcitxKey_Return:
    if (m_buffer.empty()) {
      keyEvent.filter();
      return;
    }
    commitText(key.sym());
    updateUI();
    keyEvent.filterAndAccept();
    return;
  case FcitxKey_Left:
    if (m_preedit.empty()) {
      keyEvent.filter();
      return;
    }
    if (m_cursor > 0) {
      --m_cursor;
    }
    updatePreeditCursor();
    keyEvent.filterAndAccept();
    return;
  case FcitxKey_Right:
    if (m_preedit.empty()) {
      keyEvent.filter();
      return;
    }
    if (m_cursor < (m_buffer.size())) {
      ++m_cursor;
    }
    updatePreeditCursor();
    keyEvent.filterAndAccept();
    return;
  case FcitxKey_Up:
  case FcitxKey_Down:
    keyEvent.filter();
    return;
  case FcitxKey_BackSpace:
    if (m_buffer.empty()) {
      keyEvent.filter();
      return;
    }
    if (m_cursor > 0) {
      m_buffer.erase(--iterator);
      --m_cursor;
    }
    getVarnamResult();
    updateUI();
    keyEvent.filterAndAccept();
    return;
  case FcitxKey_Delete:
    if (m_buffer.empty()) {
      keyEvent.filter();
      return;
    }
    if (m_cursor < m_buffer.size()) {
      m_buffer.erase(iterator);
    }
    getVarnamResult();
    updateUI();
    keyEvent.filterAndAccept();
    return;
  case FcitxKey_Home:
    if (m_buffer.empty()) {
      keyEvent.filter();
      return;
    }
    m_cursor = 0;
    updatePreeditCursor();
    keyEvent.filterAndAccept();
    return;
  case FcitxKey_End:
    if (m_buffer.empty()) {
      keyEvent.filter();
      return;
    }
    m_cursor = m_preedit.textLength();
    updatePreeditCursor();
    keyEvent.filterAndAccept();
    return;
  default:
    break;
  }

  if (key.isDigit()) {
    m_lastTypedCharIsDigit = true;
  } else {
    m_lastTypedCharIsDigit = false;
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
  VARNAM_INFO() << "cursor at:" << m_cursor;
#endif
  if (m_cursor >= m_buffer.size()) {
    m_buffer.push_back(input);
    m_cursor = m_buffer.size();
  } else {
    m_buffer.insert(iterator, input);
    ++m_cursor;
  }

  if (m_result) {
    varray_clear(m_result);
  } else {
    m_result = varray_init();
  }

  getVarnamResult();
  updateUI();
  keyEvent.filterAndAccept();
}

void VarnamState::setLookupTable() {
  if (!m_result) {
    return;
  }
  auto candidates = std::make_unique<VarnamCandidateList>(m_engine, m_ic);
  candidates->setSelectionKey(selectionKeys);
  candidates->setCursorPositionAfterPaging(
      CursorPositionAfterPaging::ResetToFirst);
  candidates->setPageSize(m_engine->getConfig()->pageSize.value());

  int count = varray_length(m_result);
  char preeditAppended = 0;
  for (int i = 0; i < count; i++) {
    if ((candidates->pageSize() == 10) &&
        ((i + (preeditAppended ? (1 + preeditAppended) : 1)) % 10 == 0)) {
      // TODO ;}
      candidates->append<VarnamCandidateWord>(m_engine,
                                              m_preedit.toString().c_str(), i);
      ++preeditAppended;
    }
    vword *word = static_cast<vword *>(varray_get(m_result, i));
    candidates->append<VarnamCandidateWord>(m_engine, word->text,
                                            preeditAppended ? i + 1 : i);
  }
  if (!preeditAppended) {
    candidates->append<VarnamCandidateWord>(
        m_engine, m_preedit.toString().c_str(), ++count);
  }
  if (count) {
    candidates->setGlobalCursorIndex(0);
    m_ic->inputPanel().setCandidateList(std::move(candidates));
  }
}

void VarnamState::updateLookupTable(const PageAction &action) {
  auto candidates = m_ic->inputPanel().candidateList();
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
  auto candidateList_ = m_ic->inputPanel().candidateList();
#ifdef DEBUG_MODE
  VARNAM_INFO() << "select candidate at :" << index;
#endif
  if (!candidateList_ || candidateList_->size() <= index) {
    return;
  }
  if (index == 9) {
    m_candidateSelected = 0;
  } else {
    if (candidateList_->size() <= index) {
      return;
    }
    m_candidateSelected = index;
  }
}

void VarnamState::commitText(const FcitxKeySym &key) {
  auto candidates = m_ic->inputPanel().candidateList();
  std::string stringToCommit;
  std::string wordToLearn;
  bool isWordBreakKey = isWordBreak(key);
  bool enableIndicPunctuation =
      m_engine->getConfig()->enablePunctuation.value();

  if (key == FcitxKey_Escape || key == FcitxKey_0 || !candidates ||
      candidates->size() <= 1 || !m_result || varray_is_empty(m_result)) {
    stringToCommit.assign(m_preedit.toStringForCommit());
    m_candidateSelected = 0;
  } else if ((candidates->cursorIndex() <= 0) && !m_candidateSelected) {
    vword *first_result = static_cast<vword *>(varray_get(m_result, 0));
    if (first_result) {
      stringToCommit.assign(first_result->text);
      m_candidateSelected = 1;
    }
  } else {
    stringToCommit.assign(
        candidates->candidate(m_candidateSelected).text().toStringForCommit());
  }
  wordToLearn = stringToCommit;

  if (isWordBreakKey) {
    if (enableIndicPunctuation && m_candidateSelected) {
      m_buffer.clear();
      m_buffer.push_back(getWordBreakChar(key)[0]);
      if (getVarnamResult() && m_result && varray_length(m_result) > 0) {
        vword *m_resultword = static_cast<vword *>(varray_get(m_result, 0));
        if (m_resultword) {
          stringToCommit =
              stringutils::concat(stringToCommit, m_resultword->text);
        }
      }
    } else {
      stringToCommit =
          stringutils::concat(stringToCommit, getWordBreakChar(key));
    }
  }

#ifdef DEBUG_MODE
  VARNAM_INFO() << "string to commit:" << stringToCommit;
#endif
  m_ic->commitString(stringToCommit);

  if (stringToCommit.empty() || !m_candidateSelected ||
      m_lastTypedCharIsDigit ||
      m_ic->capabilityFlags().test(CapabilityFlag::PasswordOrSensitive) ||
      !m_engine->getConfig()->shouldLearnWords.value()) {
    reset();
    return;
  }

#ifdef DEBUG_MODE
  VARNAM_INFO() << "Word To Learn:" << wordToLearn;
#endif

  std::thread learnThread(varnam_learn_word, m_engine->getVarnamHandle(),
                          std::move(wordToLearn), 0);
  learnThread.detach();

  reset();
}

void VarnamState::updateUI() {
  m_ic->inputPanel().reset();
  if (m_buffer.empty()) {
    m_ic->updatePreedit();
    m_ic->updateUserInterface(UserInterfaceComponent::InputPanel);
    return;
  }
  if (m_result == nullptr) {
    return;
  }
  if (varray_length(m_result) == 0x00) {
    return;
  }

  m_preedit.clear();
  m_preedit.append(bufferToString(), TextFormatFlag::HighLight);
  if (m_cursor > m_preedit.textLength()) {
    m_cursor = m_preedit.textLength();
  }
  m_preedit.setCursor(m_cursor);

  if (m_ic->capabilityFlags().test(CapabilityFlag::Preedit)) {
    m_ic->inputPanel().setClientPreedit(m_preedit);
  } else {
    m_ic->inputPanel().setPreedit(m_preedit);
  }
  m_ic->updatePreedit();
  setLookupTable();
  m_ic->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void VarnamState::reset() {
  m_cursor = std::numeric_limits<unsigned int>::max();
  m_candidateSelected = 0;
  m_lastTypedCharIsDigit = false;
  m_buffer.clear();
  m_preedit.clear();
  if (m_result) {
    varray_clear(m_result);
  }
}

} // namespace fcitx