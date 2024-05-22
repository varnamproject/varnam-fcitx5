#ifndef _FCITX5_VARNAM_STATE_H
#define _FCITX5_VARNAM_STATE_H

#include "varnam_candidate.h"

#include <codecvt>
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
  unsigned int cursor;
  unsigned int bufferPos;
  unsigned int utfCharPos;

  InputContext *ic_;
  VarnamEngine *engine_;
  Text preedit_;
  std::u32string u32_preedit;
  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf8_converter;

  std::vector<char> buffer_;
  varray *result_;

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

  // Commit Preedit to text
  void commitPreedit(const FcitxKeySym &key = FcitxKey_None);

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
#endif