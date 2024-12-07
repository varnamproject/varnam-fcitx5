#include "varnam_candidate.h"
#include "varnam_config.h"
#include "varnam_state.h"

#include <fcitx/candidatelist.h>

namespace fcitx {

VarnamCandidateWord::VarnamCandidateWord(VarnamEngine *engine, const char *text,
                                         int index)
    : CandidateWord(Text(std::move(text))), m_engine(engine), m_index(index) {}

void VarnamCandidateWord::select(InputContext *inputContext) const {
  auto state = inputContext->propertyFor(m_engine->factory());
  state->selectCandidate(m_index);
}

VarnamCandidateList::VarnamCandidateList(VarnamEngine *engine, InputContext *ic)
    : m_engine(engine), m_ic(ic) {
  const VarnamEngineConfig *config =
      static_cast<const VarnamEngineConfig *>(m_engine->getConfig());
  CandidateLayoutHint layout;
  if (!config) {
    VARNAM_WARN() << "Invalid configuration";
    layout = CandidateLayoutHint::Vertical;
  } else {
    layout = config->candidateLayout.value();
  }
  setPageable(this);
  setLayoutHint(layout);
}

void VarnamCandidateList::prev() {
  CommonCandidateList::prev();
  if (currentPage() >= 0) {
    setPage(currentPage());
    setCursorPositionAfterPaging(CursorPositionAfterPaging::ResetToFirst);
  }
  m_ic->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void VarnamCandidateList::next() {
  CommonCandidateList::next();
  if (currentPage() < totalPages()) {
    setPage(currentPage());
    setGlobalCursorIndex(pageSize() * currentPage() - (pageSize() - 1));
  }
  m_ic->updateUserInterface(UserInterfaceComponent::InputPanel);
}

bool VarnamCandidateList::usedNextBefore() const { return true; }

void VarnamCandidateList::prevCandidate() {
  CommonCandidateList::prevCandidate();
  auto state = m_ic->propertyFor(m_engine->factory());
  int index = globalCursorIndex();
  if (index >= pageSize() && (currentPage() > 0)) {
    setPage(currentPage());
  }
  setGlobalCursorIndex(index);
  state->selectCandidate(cursorIndex());
  m_ic->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void VarnamCandidateList::nextCandidate() {
  CommonCandidateList::nextCandidate();
  auto state = m_ic->propertyFor(m_engine->factory());
  int index = globalCursorIndex();
  if (index >= pageSize() && (currentPage() < totalPages())) {
    setPage(currentPage());
  }
  setGlobalCursorIndex(index);
  state->selectCandidate(cursorIndex());
  m_ic->updateUserInterface(UserInterfaceComponent::InputPanel);
}

} // namespace fcitx