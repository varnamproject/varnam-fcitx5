#ifndef _FCITX5_VARNAM_ENGINE_H_
#define _FCITX5_VARNAM_ENGINE_H_

#include "varnam_utils.h"
#include "varnam_config.h"

#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>

namespace fcitx {

class VarnamState;

class VarnamEngine : public InputMethodEngineV3 {

private:
  int m_varnam_handle;
  Instance *m_instance;
  VarnamEngineConfig m_config;
  KeyState m_selectionKeyModifer;
  FactoryFor<VarnamState> m_factory;

public:
  VarnamEngine(Instance *instance);

  ~VarnamEngine();

  void activate(const InputMethodEntry &, InputContextEvent &) override;

  void deactivate(const InputMethodEntry &entry,
                  InputContextEvent &event) override;

  void keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) override;

  void reset(const InputMethodEntry &, InputContextEvent &) override;

  std::vector<InputMethodEntry> listInputMethods() override;

  auto factory() { return &m_factory; }

  void setConfig(const RawConfig &) override;

  void reloadConfig() override;

  const VarnamEngineConfig *getConfig() const override { return &m_config; }

  const KeyState &getSelectionModifer() const { return m_selectionKeyModifer; }

  int getVarnamHandle() const { return m_varnam_handle; }
};

class VarnamEngineFactory : public AddonFactory {
  AddonInstance *create(AddonManager *manager) override {
    return new VarnamEngine(manager->instance());
  }
};

} // namespace fcitx

#endif // _FCITX5_VARNAM_ENGINE_H_