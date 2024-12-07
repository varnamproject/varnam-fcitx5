#ifndef _FCITX5_VARNAM_STATE_H
#define _FCITX5_VARNAM_STATE_H

#include "varnam_candidate.h"

#include <fcitx/inputcontext.h>
#include <fcitx/text.h>

extern "C" {
#include <libgovarnam/libgovarnam.h>
}

namespace fcitx {

class VarnamEngine;

class VarnamState : public InputContextProperty {

private:
  // Private Variables
  unsigned int m_cursor;
  char m_candidateSelected;
  bool m_lastTypedCharIsDigit;

  InputContext *m_ic;
  VarnamEngine *m_engine;
  Text m_preedit;

  std::vector<char> m_buffer;
  varray *m_result;

  // Private Methods

  // convert buffer to std::string
  std::string bufferToString();

  // generate Varnam Result
  bool getVarnamResult();

  // update Preedit Cursor Position
  void updatePreeditCursor();

public:
  VarnamState(VarnamEngine *, InputContext &);

  ~VarnamState();

  // Handle KeyEvents
  void processKeyEvent(KeyEvent &);

  // Commit Selected Candidate to text
  void commitText(const FcitxKeySym &key = FcitxKey_None);

  // Generate Candidate List/Lookup tables
  void setLookupTable();

  // Update Lookup table entries
  void updateLookupTable(const PageAction &);

  // Select Candidate from Candidate List/Lookup Table
  void selectCandidate(int);

  // Update Input panel
  void updateUI();

  // Reset context properties
  void reset();
};

} // namespace fcitx
#endif // end of _FCITX5_VARNAM_STATE_H