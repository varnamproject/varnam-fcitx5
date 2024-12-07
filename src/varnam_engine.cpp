#include "varnam_engine.h"
#include "varnam_state.h"
#include "varnam_utils.h"

#include <fcitx-config/iniparser.h>
#include <fcitx/inputpanel.h>
#include <libgovarnam/c-shared.h>

extern "C" {
#include <libgovarnam/libgovarnam.h>
}

namespace fcitx {

VarnamEngine::VarnamEngine(Instance *instance)
    : m_instance(instance), m_factory([this](InputContext &ic) {
        return new VarnamState(this, ic);
      }) {
  instance->inputContextManager().registerProperty("varnamState", &m_factory);
}

VarnamEngine::~VarnamEngine() {
  m_factory.unregister();
  if (m_varnam_handle > 0) {
    int rv = varnam_close(m_varnam_handle);
    if (rv != VARNAM_SUCCESS) {
      VARNAM_WARN() << "Failed to close Varnam instance";
    }
  }
}

void VarnamEngine::activate(const InputMethodEntry &entry,
                            InputContextEvent &contextEvent) {
  FCITX_UNUSED(contextEvent);
  reloadConfig();
#ifdef DEBUG_MODE
  VARNAM_INFO() << "activate scheme:" << entry.uniqueName();
#endif
  char *schemeName = const_cast<char *>(entry.uniqueName().c_str());
  int rv = varnam_init_from_id(schemeName, &m_varnam_handle);
  if (rv != VARNAM_SUCCESS) {
    VARNAM_WARN() << "Failed to initialize Varnam";
    throw std::runtime_error("failed to initialize varnam");
  }

  varnam_config(m_varnam_handle, VARNAM_CONFIG_SET_DICTIONARY_MATCH_EXACT,
                m_config.strictlyFollowScheme.value());
  varnam_config(m_varnam_handle, VARNAM_CONFIG_SET_DICTIONARY_SUGGESTIONS_LIMIT,
                m_config.dictionarySuggestionsLimit.value());
  varnam_config(m_varnam_handle,
                VARNAM_CONFIG_SET_PATTERN_DICTIONARY_SUGGESTIONS_LIMIT,
                m_config.patternDictionarySuggestionsLimit.value());
  varnam_config(m_varnam_handle, VARNAM_CONFIG_SET_TOKENIZER_SUGGESTIONS_LIMIT,
                m_config.tokenizerSuggestionsLimit.value());
  varnam_config(m_varnam_handle, VARNAM_CONFIG_USE_INDIC_DIGITS,
                m_config.enableIndicNumbers.value());
}

void VarnamEngine::deactivate(const InputMethodEntry &entry,
                              InputContextEvent &event) {
#ifdef DEBUG_MODE
  VARNAM_INFO() << "deactivate scheme:" << entry.uniqueName();
#endif
  if (event.type() == EventType::InputContextSwitchInputMethod) {
    auto ic = event.inputContext();
    auto state = ic->propertyFor(&m_factory);
    state->commitText();
    state->updateUI();
  }
  reset(entry, event);
  if (m_varnam_handle > 0) {
    varnam_close(m_varnam_handle);
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
  auto state = ic->propertyFor(&m_factory);
  state->processKeyEvent(keyEvent);
}

void VarnamEngine::reset(const InputMethodEntry &entry,
                         InputContextEvent &event) {
  FCITX_UNUSED(entry);
  auto ic = event.inputContext();
  auto state = ic->propertyFor(&m_factory);
  state->reset();
  state->updateUI();
}

void VarnamEngine::setConfig(const RawConfig &config) {
  m_config.load(config);
  safeSaveAsIni(m_config, "conf/varnam.conf");
}

void VarnamEngine::reloadConfig() { readAsIni(m_config, "conf/varnam.conf"); }

} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::VarnamEngineFactory);