#ifndef _FCITX5_VARNAM_CANDIDATE_H
#define _FCITX5_VARNAM_CANDIDATE_H

#include "varnam_engine.h"

#include <fcitx/candidatelist.h>
#include <fcitx/inputcontext.h>

namespace fcitx {

class VarnamCandidateWord : public CandidateWord {
private:
  VarnamEngine *m_engine;
  int m_index;

public:
  VarnamCandidateWord(VarnamEngine *engine, const char *text, int index);

  void select(InputContext *inputContext) const override;
};

class VarnamCandidateList : public CommonCandidateList {
private:
  VarnamEngine *m_engine;
  InputContext *m_ic;

public:
  VarnamCandidateList(VarnamEngine *engine, InputContext *ic);

  void prev() override;

  void next() override;

  void prevCandidate() override;

  void nextCandidate() override;

  bool usedNextBefore() const override;
};

} // namespace fcitx
#endif