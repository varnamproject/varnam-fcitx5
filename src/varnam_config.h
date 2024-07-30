#ifndef _FCITX5_VARNAM_CONFIG_H_
#define _FCITX5_VARNAM_CONFIG_H_

#include <fcitx-config/configuration.h>
#include <fcitx-config/enum.h>
#include <fcitx-utils/i18n.h>
#include <fcitx/candidatelist.h>

namespace fcitx {

enum class ChooseModifier { NoModifier, Alt, Control, Super };

FCITX_CONFIG_ENUM_NAME_WITH_I18N(ChooseModifier, N_("None"), N_("Alt"),
                                 N_("Control"), N_("Super"));

FCITX_CONFIG_ENUM_NAME_WITH_I18N(CandidateLayoutHint, N_("Not Set"),
                                 N_("Vertical"), N_("Horizontal"));

FCITX_CONFIGURATION(
    VarnamEngineConfig,

    // Candidate List Layout
    OptionWithAnnotation<CandidateLayoutHint, CandidateLayoutHintI18NAnnotation>
        candidateLayout{this, "CandidateLayout", _("Candidate List Layout"),
                        fcitx::CandidateLayoutHint::Vertical};

    // Page Size
    Option<int, IntConstrain> pageSize{this, "PageSize", _("Page size"), 5,
                                       IntConstrain(3, 10)};

    // Enable Learning Words on commit
    Option<bool> shouldLearnWords{this, "Learn Words", _("Learn New Words"),
                                  true};

    // Enable Indic Numbers
    Option<bool> enableIndicNumbers{this, "EnableIndicNumbers", _("Enable Indic Numbers"),
                                    false};


    // Enable Indic Numbers
    Option<bool> enablePunctuation{this, "enablePunctuation", _("Enable punctuation"),
                                    false};

    // Strictly Follow Schema
    Option<bool> strictlyFollowScheme{
        this, "Strictly Follow Scheme",
        _("Strictly Follow Scheme For Dictionary Results"), false};
   
    // Dictionary Suggestions Limit
    Option<int, IntConstrain> dictionarySuggestionsLimit{
        this, "Dictionary Suggestions Limit", _("Dictionary Suggestions Limit"),
        4, IntConstrain(0, 10)};

    // Pattern Dictionary Suggestions Limit
    Option<int, IntConstrain> patternDictionarySuggestionsLimit{
        this, "Pattern Dictionary Suggestions Limit",
        _("Pattern Dictionary Suggestions Limit"), 3, IntConstrain(0, 10)};

    // Tokenizer Suggestions Limit
    Option<int, IntConstrain> tokenizerSuggestionsLimit{
        this, "Tokenizer Suggestions Limit", _("Tokenizer Suggestions Limit"),
        10, IntConstrain(0, 10)};

    // Previous Candidate Shortcut
    KeyListOption prevCandidate{
        this,
        "PrevCandidate",
        _("Previous Candidate"),
        {Key("Up")},
        KeyListConstrain(KeyConstrainFlag::AllowModifierLess)};

    // Next Candidate Shortcut
    KeyListOption nextCandidate{
        this,
        "NextCandidate",
        _("Next Candidate"),
        {Key("Down")},
        KeyListConstrain(KeyConstrainFlag::AllowModifierLess)};

    // Previous Page
    KeyListOption prevPage{
        this,
        "PrevPage",
        _("Previous Page"),
        {Key("Alt+Up")},
        KeyListConstrain(KeyConstrainFlag::AllowModifierOnly)};

    // Next Page
    KeyListOption nextPage{
        this,
        "NextPage",
        _("Next Page"),
        {Key("Alt+Down")},
        KeyListConstrain(KeyConstrainFlag::AllowModifierOnly)};);

} // namespace fcitx
#endif