#include "varnam_engine.h"
#include "varnam_state.h"
#include "varnam_utils.h"

#include <fcitx-config/iniparser.h>
#include <fcitx/inputpanel.h>

extern "C" {
#include <libgovarnam/libgovarnam.h>
}

namespace fcitx {

VarnamEngine::VarnamEngine(Instance *instance)
    : instance_(instance),
      factory_([this](InputContext &ic) { return new VarnamState(this, ic); }) {
  reloadConfig();
  instance->inputContextManager().registerProperty("varnamState", &factory_);
}

VarnamEngine::~VarnamEngine() {
  factory_.unregister();
  if (varnam_handle > 0) {
    int rv = varnam_close(varnam_handle);
    if (rv != VARNAM_SUCCESS) {
      VARNAM_WARN() << "Failed to close Varnam instance";
    }
  }
}

void VarnamEngine::activate(const InputMethodEntry &entry,
                            InputContextEvent &contextEvent) {
#ifdef DEBUG_MODE
  VARNAM_INFO() << "activate scheme:" << entry.uniqueName();
#endif
  char *schemeName = const_cast<char *>(entry.uniqueName().c_str());
  int rv = varnam_init_from_id(schemeName, &varnam_handle);
  if (rv != VARNAM_SUCCESS) {
    VARNAM_WARN() << "Failed to initialize Varnam";
    throw std::runtime_error("failed to initialize varnam");
  }
  
  varnam_config(varnam_handle, VARNAM_CONFIG_SET_DICTIONARY_MATCH_EXACT,
                config_.strictlyFollowScheme.value());
  varnam_config(varnam_handle, VARNAM_CONFIG_SET_DICTIONARY_SUGGESTIONS_LIMIT,
                config_.dictionarySuggestionsLimit.value());
  varnam_config(varnam_handle,
                VARNAM_CONFIG_SET_PATTERN_DICTIONARY_SUGGESTIONS_LIMIT,
                config_.patternDictionarySuggestionsLimit.value());
  varnam_config(varnam_handle, VARNAM_CONFIG_SET_TOKENIZER_SUGGESTIONS_LIMIT,
                config_.tokenizerSuggestionsLimit.value());
}

void VarnamEngine::deactivate(const InputMethodEntry &entry,
                              InputContextEvent &event) {
#ifdef DEBUG_MODE
  VARNAM_INFO() << "deactivate scheme:" << entry.uniqueName();
#endif
  if (event.type() == EventType::InputContextSwitchInputMethod) {
    auto ic = event.inputContext();
    auto state = ic->propertyFor(&factory_);
    state->commitPreedit();
    state->updateUI();
  }
  reset(entry, event);
  if (varnam_handle > 0) {
    varnam_close(varnam_handle);
  }
}

std::vector<InputMethodEntry> VarnamEngine::listInputMethods() {
  std::vector<InputMethodEntry> entries;
  varray *varnam_schemes = nullptr;
  varnam_schemes = varnam_get_all_scheme_details();
  if (varnam_schemes == nullptr) {
    return entries;
  }
#ifdef DEBUG_MODE
  VARNAM_INFO() << "available schemes:";
#endif
  for (int i = 0; i < varray_length(varnam_schemes); i++) {
    SchemeDetails *scheme =
        static_cast<SchemeDetails *>(varray_get(varnam_schemes, i));
    if (scheme == nullptr) {
      continue;
    }
    // skip inscript
    if (strstr(scheme->Identifier, INSCRIPT)) {
      continue;
    }
    std::string iconName = stringutils::concat("varnam-", scheme->LangCode);
    std::string displayName =
        stringutils::concat("Varnam-", scheme->DisplayName);
#ifdef DEBUG_MODE
    VARNAM_INFO() << scheme->LangCode << ":" << displayName;
#endif
    InputMethodEntry entry(scheme->Identifier, displayName, scheme->LangCode,
                           "varnamfcitx");
    entry.setConfigurable(true).setIcon(iconName);
    entries.emplace_back(std::move(entry));
  }
  return entries;
}

void VarnamEngine::keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) {
  FCITX_UNUSED(entry);
  // ignore key release events
  if (keyEvent.isRelease()) {
    return;
  }
  auto ic = keyEvent.inputContext();
  auto state = ic->propertyFor(&factory_);
  state->processKeyEvent(keyEvent);
}

void VarnamEngine::reset(const InputMethodEntry &entry,
                         InputContextEvent &event) {
  FCITX_UNUSED(entry);
  auto ic = event.inputContext();
  auto state = event.inputContext()->propertyFor(&factory_);
  state->reset();
  ic->inputPanel().reset();
  ic->updatePreedit();
  ic->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void VarnamEngine::setConfig(const RawConfig &config) {
  config_.load(config);
  safeSaveAsIni(config_, "conf/varnam.conf");
}

void VarnamEngine::reloadConfig() { readAsIni(config_, "conf/varnam.conf"); }

} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::VarnamEngineFactory);