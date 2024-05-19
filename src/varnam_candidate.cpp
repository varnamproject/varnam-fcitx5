#include "varnam_candidate.h"
#include "varnam_utils.h"
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
  if (!config) {
    VARNAM_WARN() << "Invalid configuration";
    throw std::runtime_error("invalid config");
  }
  setPageable(this);
  setLayoutHint(config->candidateLayout.value());
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
  state->selectCandidate(cursorIndex());
  setGlobalCursorIndex(index);
}

void VarnamCandidateList::nextCandidate() {
  CommonCandidateList::nextCandidate();
  auto state = ic_->propertyFor(engine_->factory());
  int index = globalCursorIndex();
  if (index >= pageSize() && (currentPage() < totalPages())) {
    setPage(currentPage());
  }
  state->selectCandidate(cursorIndex());
  setGlobalCursorIndex(index);
}

} // namespace fcitx