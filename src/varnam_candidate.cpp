#include "varnam_candidate.h"
#include "varnam_config.h"
#include "varnam_state.h"

#include <fcitx/candidatelist.h>

namespace fcitx {

VarnamCandidateWord::VarnamCandidateWord(VarnamEngine *engine, const char *text,
                                         int index)
    : CandidateWord(Text(std::move(text))), engine_(engine), index_(index) {}

void VarnamCandidateWord::select(InputContext *inputContext) const {
  auto state = inputContext->propertyFor(engine_->factory());
  state->selectCandidate(index_);
}

VarnamCandidateList::VarnamCandidateList(VarnamEngine *engine, InputContext *ic)
    : engine_(engine), ic_(ic) {
  const VarnamEngineConfig *config =
      static_cast<const VarnamEngineConfig *>(engine_->getConfig());
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
  ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void VarnamCandidateList::next() {
  CommonCandidateList::next();
  if (currentPage() < totalPages()) {
    setPage(currentPage());
    setGlobalCursorIndex(pageSize() * currentPage() - (pageSize() - 1));
  }
  ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
}

bool VarnamCandidateList::usedNextBefore() const { return true; }

void VarnamCandidateList::prevCandidate() {
  CommonCandidateList::prevCandidate();
  auto state = ic_->propertyFor(engine_->factory());
  int index = globalCursorIndex();
  if (index >= pageSize() && (currentPage() > 0)) {
    setPage(currentPage());
  }
  setGlobalCursorIndex(index);
  state->selectCandidate(cursorIndex());
  ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void VarnamCandidateList::nextCandidate() {
  CommonCandidateList::nextCandidate();
  auto state = ic_->propertyFor(engine_->factory());
  int index = globalCursorIndex();
  if (index >= pageSize() && (currentPage() < totalPages())) {
    setPage(currentPage());
  }
  setGlobalCursorIndex(index);
  state->selectCandidate(cursorIndex());
  ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
}

} // namespace fcitx