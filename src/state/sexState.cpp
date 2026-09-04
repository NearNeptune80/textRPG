#include "state/sexState.h"

#include <algorithm>
#include <format>

#include "common/randomEngine.h"
#include "core/characterDescription.h"
#include "core/game.h"
#include "core/textParser.h"
#include "entities/entity.h"
#include "state/explorationState.h"

#include <fstream>
#include <nlohmann/json.hpp>

sexState::sexState()
    : m_partner(nullptr), m_partnerRaw(nullptr), m_stance(SexStance::SOLO_BED), m_playerDominance(0.0f)
{
    m_narrativeLog = "You make yourself comfortable and prepare to indulge in private pleasure.";
}

sexState::sexState(std::shared_ptr<entity> partner, SexStance initialStance, float initialDominance)
    : m_partner(partner), m_partnerRaw(partner.get()), m_stance(initialStance), m_playerDominance(initialDominance)
{
    std::string partnerName = m_partnerRaw ? m_partnerRaw->name : "Partner";
    m_narrativeLog = std::format("Erotic encounter initiated with {} in {} stance.", partnerName, sexStanceToString(m_stance));
}

sexState::sexState(entity* partnerPtr, SexStance initialStance, float initialDominance)
    : m_partner(nullptr), m_partnerRaw(partnerPtr), m_stance(initialStance), m_playerDominance(initialDominance)
{
    std::string partnerName = m_partnerRaw ? m_partnerRaw->name : "Partner";
    m_narrativeLog = std::format("Erotic encounter initiated with {} in {} stance.", partnerName, sexStanceToString(m_stance));
}

void sexState::initialise(game* gameContext) {}

void sexState::onEnter(game* gameContext)
{
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void sexState::onExit(game* gameContext)
{
    entity* player = gameContext ? gameContext->getPlayer() : nullptr;
    entity* partner = getPartner();

    if (player)
    {
        player->inventory.resetAllDisplacements();
    }
    if (partner)
    {
        partner->inventory.resetAllDisplacements();
    }

    appendNarrative("Disengaging from erotic encounter. All clothing displacements automatically restored.");
}

void sexState::update(game* gameContext, float deltaTime) {}

void sexState::appendNarrative(const std::string& text)
{
    if (!m_narrativeLog.empty())
    {
        m_narrativeLog += "\n\n";
    }
    m_narrativeLog += text;
}

std::vector<SexAction> sexState::buildMasterActionDatabase() const
{
    std::vector<SexAction> db;

    std::ifstream file("data/scenes/sex_actions.json");
    if (file.is_open())
    {
        try
        {
            nlohmann::json j;
            file >> j;
            if (j.contains("actions") && j["actions"].is_array())
            {
                for (const auto& aJson : j["actions"])
                {
                    SexAction act;
                    act.id = aJson.value("id", "");
                    act.name = aJson.value("name", "");
                    act.description = aJson.value("description", "");
                    act.category = aJson.value("category", "General");
                    act.isSolo = aJson.value("isSolo", false);
                    act.narrative = aJson.value("narrative", "");

                    if (aJson.contains("values"))
                    {
                        const auto& v = aJson["values"];
                        act.arousalGainSelf = v.value("arousalSelf", 15.0f);
                        act.arousalGainPartner = v.value("arousalPartner", 15.0f);
                        act.dominanceShift = v.value("dominanceShift", 0.0f);
                        act.fluidProducedMl = v.value("fluidProducedMl", 1.0f);
                    }

                    if (aJson.contains("requirements"))
                    {
                        const auto& r = aJson["requirements"];
                        act.requiresPenis = r.value("hasPenis", false);
                        act.requiresVagina = r.value("hasVagina", false);
                        act.requiresBreasts = r.value("hasBreasts", false);
                        act.minArousal = r.value("minArousal", 0.0f);
                    }

                    if (aJson.contains("validStances") && aJson["validStances"].is_array())
                    {
                        for (const auto& st : aJson["validStances"])
                        {
                            act.validStances.push_back(stringToSexStance(st.get<std::string>()));
                        }
                    }

                    db.push_back(act);
                }
                return db;
            }
        }
        catch (...) {}
    }

    // Fallback defaults if file missing
    SexAction stroke;
    stroke.id = "solo_stroke";
    stroke.name = "Stroke";
    stroke.isSolo = true;
    stroke.arousalGainSelf = 18.0f;
    stroke.validStances = { SexStance::SOLO_BED, SexStance::SOLO_CHAIR, SexStance::SOLO_STANDING };
    db.push_back(stroke);

    return db;
}

bool sexState::isActionValidForStance(const SexAction& action) const
{
    return std::find(action.validStances.begin(), action.validStances.end(), m_stance) != action.validStances.end();
}

std::vector<SexAction> sexState::getAvailableActions() const
{
    auto allActions = buildMasterActionDatabase();
    std::vector<SexAction> valid;

    if (isSolo())
    {
        for (const auto& act : allActions)
        {
            if (!act.isSolo) continue;
            if (!isActionValidForStance(act)) continue;
            if (m_playerArousal < act.minArousal) continue;

            valid.push_back(act);
        }
        return valid;
    }

    if (!isPlayerDominant())
    {
        SexAction plead;
        plead.id = "sub_plead";
        plead.name = "[Plead / Suggest Action]";
        plead.type = SexActionType::PLEAD_SUGGEST;
        valid.push_back(plead);

        SexAction endure;
        endure.id = "sub_endure";
        endure.name = "[Endure Passively]";
        endure.type = SexActionType::ENDURE;
        valid.push_back(endure);

        SexAction beg;
        beg.id = "sub_beg";
        beg.name = "[Beg for Climax]";
        beg.type = SexActionType::BEG_CLIMAX;
        valid.push_back(beg);

        SexAction struggle;
        struggle.id = "sub_struggle";
        struggle.name = "[Struggle for Control]";
        struggle.type = SexActionType::STRUGGLE;
        valid.push_back(struggle);

        return valid;
    }

    entity* partner = getPartner();
    for (const auto& act : allActions)
    {
        if (act.isSolo) continue;
        if (!isActionValidForStance(act)) continue;

        if (act.requiresPenis && (!partner || !partner->anatomy.hasPenis())) continue;
        if (act.requiresVagina && (!partner || !partner->anatomy.hasVagina())) continue;
        if (act.requiresBreasts && (!partner || !partner->anatomy.hasBreasts())) continue;

        valid.push_back(act);
    }

    return valid;
}

void sexState::applyClothingDisplacementsForAction(const SexAction& action)
{
    entity* partner = getPartner();

    if (action.targetSlot == bodySlot::GROIN || action.targetSlot == bodySlot::ASS || action.actorSlot == bodySlot::GROIN)
    {
        if (partner)
        {
            partner->inventory.setDisplacement(equipSlot::LEGS_OUTER, DisplacementMode::PULL_DOWN);
            partner->inventory.setDisplacement(equipSlot::GROIN_OVER, DisplacementMode::PULL_ASIDE);
        }
    }

    if (action.targetSlot == bodySlot::BREASTS || action.actorSlot == bodySlot::BREASTS)
    {
        if (partner)
        {
            partner->inventory.setDisplacement(equipSlot::TORSO_UNDER, DisplacementMode::UNBUTTON);
            partner->inventory.setDisplacement(equipSlot::CHEST_WEAR, DisplacementMode::PULL_ASIDE);
        }
    }
}

void sexState::changeStance(SexStance newStance)
{
    m_stance = newStance;
    if (isSolo())
    {
        appendNarrative(std::format("You adjust your position to {}.", sexStanceToString(m_stance)));
    }
    else
    {
        entity* partner = getPartner();
        std::string pName = partner ? partner->name : "your partner";
        appendNarrative(std::format("You maneuver {} into the {} position.", pName, sexStanceToString(m_stance)));
    }
}

bool sexState::executeAction(game* gameContext, const SexAction& action)
{
    if (!gameContext) return false;

    entity* player = gameContext->getPlayer();
    if (!player) return false;

    if (isSolo())
    {
        applyClothingDisplacementsForAction(action);
        m_playerArousal = std::clamp(m_playerArousal + action.arousalGainSelf, 0.0f, 100.0f);
        player->stats.modifyBaseStat("lust", action.arousalGainSelf);

        if (!action.narrative.empty())
        {
            appendNarrative(action.narrative);
        }
        else
        {
            appendNarrative(std::format("You perform {} in the {} position.", action.name, sexStanceToString(m_stance)));
        }

        if (m_playerArousal >= 100.0f || action.id == "solo_climax")
        {
            m_playerClimaxes++;
            m_playerArousal = 0.0f;
            player->stats.setBaseStat("lust", 0.0f);

            bodyPart* groin = player->anatomy.getPart(bodySlot::GROIN);
            if (groin && groin->currentFluidMl > 0.0f)
            {
                float amount = std::min(groin->currentFluidMl, 10.0f);
                groin->currentFluidMl -= amount;
            }

            appendNarrative("Waves of intense pleasure shudder through your entire body as you reach a glorious climax, collapsing back in sweet exhaustion.");
        }

        gameContext->refreshActionGrid();
        return true;
    }

    entity* partner = getPartner();
    if (!partner) return false;

    applyClothingDisplacementsForAction(action);

    m_playerArousal = std::clamp(m_playerArousal + action.arousalGainSelf, 0.0f, 100.0f);
    m_partnerArousal = std::clamp(m_partnerArousal + action.arousalGainPartner, 0.0f, 100.0f);
    m_playerDominance = std::clamp(m_playerDominance + action.dominanceShift, -100.0f, 100.0f);

    // Orifice Stretch Check
    if (action.requiresPenetration)
    {
        bodyPart* playerPenis = player->anatomy.getPart(bodySlot::GROIN);
        float penisDiam = playerPenis ? playerPenis->diameter : 3.5f;

        OrificeData* orifice = partner->anatomy.getOrifice(action.targetSlot);
        if (orifice)
        {
            if (penisDiam > orifice->currentStretch)
            {
                partner->anatomy.stretchOrifice(action.targetSlot, penisDiam);
                appendNarrative(std::format("Your {:.1f}cm thick organ presses into {}'s tight passage, stretching the orifice to accommodate your girth.",
                                            penisDiam, partner->name));
            }
        }
    }

    if (!action.narrative.empty())
    {
        appendNarrative(action.narrative);
    }
    else
    {
        std::string actionText = std::format("Using the {} position, you perform {} on {}.",
                                             sexStanceToString(m_stance), action.name, partner->name);
        appendNarrative(actionText);
    }

    // Check Orgasms
    if (m_playerArousal >= 100.0f)
    {
        processOrgasm(gameContext, player, partner, action.targetSlot);
    }
    if (m_partnerArousal >= 100.0f)
    {
        processOrgasm(gameContext, partner, player, action.actorSlot);
    }

    gameContext->refreshActionGrid();
    return true;
}

void sexState::processOrgasm(game* gameContext, entity* orgasmingEntity, entity* receivingEntity, bodySlot receivingSlot)
{
    if (!orgasmingEntity || !receivingEntity) return;

    std::string oName = orgasmingEntity->name;
    std::string rName = receivingEntity->name;

    appendNarrative(std::format("A intense shudder violently wracks {}'s body as they surrender to a powerful orgasm!", oName));

    bodyPart* groin = orgasmingEntity->anatomy.getPart(bodySlot::GROIN);
    if (groin && groin->currentFluidMl > 0.0f)
    {
        float cumAmount = std::min(groin->currentFluidMl, 15.0f);
        groin->currentFluidMl -= cumAmount;

        if (receivingEntity->anatomy.hasOrifice(receivingSlot))
        {
            receivingEntity->anatomy.transferFluidToOrifice(receivingSlot, "cum", cumAmount);
            appendNarrative(std::format("{} ejaculates {:.0f}ml of warm cum directly into {}'s orifice!", oName, cumAmount, rName));

            // Gestation Check
            if (receivingSlot == bodySlot::GROIN && receivingEntity->anatomy.hasVagina() && gameContext->settings.content.pregnancyEnabled)
            {
                bool success = receivingEntity->gestation.impregnate(
                    orgasmingEntity->id, orgasmingEntity->name,
                    orgasmingEntity->anatomy.getDominantRace(),
                    receivingEntity->anatomy.getDominantRace()
                );
                if (success)
                {
                    appendNarrative(std::format("{}'s womb receives the potent seed, conceiving a new offspring!", rName));
                }
            }
        }
        else
        {
            appendNarrative(std::format("{} ejaculates {:.0f}ml of warm cum across {}'s body surface!", oName, cumAmount, rName));
        }
    }

    if (orgasmingEntity == gameContext->getPlayer())
    {
        m_playerArousal = 15.0f;
        m_playerClimaxes++;
    }
    else
    {
        m_partnerArousal = 15.0f;
        m_partnerClimaxes++;
    }
}

void sexState::handlePleadSuggest(game* gameContext)
{
    entity* partner = getPartner();
    std::string pName = partner ? partner->name : "Partner";
    appendNarrative(std::format("You plead softly with {}, asking for gentle treatment.", pName));
    processPartnerTurn(gameContext);
}

void sexState::handleEndure(game* gameContext)
{
    entity* partner = getPartner();
    std::string pName = partner ? partner->name : "Partner";
    m_playerArousal = std::clamp(m_playerArousal + 15.0f, 0.0f, 100.0f);
    m_playerDominance = std::clamp(m_playerDominance - 5.0f, -100.0f, 100.0f);

    appendNarrative(std::format("You endure {}'s touch passively, submitting to their pace.", pName));
    processPartnerTurn(gameContext);
}

void sexState::handleBegClimax(game* gameContext)
{
    if (m_playerArousal >= 70.0f)
    {
        appendNarrative("You beg desperately for climax. Your partner smiles and intensifies their pace!");
        m_playerArousal = 100.0f;
        processOrgasm(gameContext, gameContext->getPlayer(), getPartner(), bodySlot::GROIN);
    }
    else
    {
        appendNarrative("You beg for climax, but your partner denies your request, teasing you further!");
    }
    processPartnerTurn(gameContext);
}

void sexState::handleStruggle(game* gameContext)
{
    entity* player = gameContext->getPlayer();
    entity* partner = getPartner();

    float pPhys = player ? player->getStat("physique") : 10.0f;
    float partnerPhys = partner ? partner->getStat("physique") : 10.0f;

    if (pPhys + dice::rollFloat(0.0f, 10.0f) >= partnerPhys)
    {
        m_playerDominance += 35.0f;
        appendNarrative(std::format("You forcefully shove {} back, breaking free from their hold and seizing control!", partner ? partner->name : "them"));
    }
    else
    {
        m_playerDominance -= 10.0f;
        appendNarrative("Your attempt to struggle fails as partner easily holds you down!");
        processPartnerTurn(gameContext);
    }
    gameContext->refreshActionGrid();
}

void sexState::processPartnerTurn(game* gameContext)
{
    entity* partner = getPartner();
    if (!partner) return;

    m_partnerArousal = std::clamp(m_partnerArousal + 15.0f, 0.0f, 100.0f);
    m_playerArousal = std::clamp(m_playerArousal + 20.0f, 0.0f, 100.0f);

    appendNarrative(std::format("{} takes advantage of their dominant position to intimately caress you.", partner->name));

    if (m_partnerArousal >= 100.0f)
    {
        processOrgasm(gameContext, partner, gameContext->getPlayer(), bodySlot::GROIN);
    }
    if (m_playerArousal >= 100.0f)
    {
        processOrgasm(gameContext, gameContext->getPlayer(), partner, bodySlot::GROIN);
    }

    gameContext->refreshActionGrid();
}

void sexState::handleCommand(game* gameContext, const UICommand& cmd)
{
    if (!gameContext) return;

    if (cmd.type == CommandType::EXECUTE_SEX_ACTION)
    {
        auto actions = getAvailableActions();
        int idx = cmd.intPayload1;
        if (idx >= 0 && static_cast<size_t>(idx) < actions.size())
        {
            SexAction act = actions[idx];
            if (act.type == SexActionType::PLEAD_SUGGEST) handlePleadSuggest(gameContext);
            else if (act.type == SexActionType::ENDURE) handleEndure(gameContext);
            else if (act.type == SexActionType::BEG_CLIMAX) handleBegClimax(gameContext);
            else if (act.type == SexActionType::STRUGGLE) handleStruggle(gameContext);
            else executeAction(gameContext, act);
        }
    }
    else if (cmd.type == CommandType::CHANGE_SEX_STANCE)
    {
        SexStance newStance = static_cast<SexStance>(cmd.intPayload1);
        changeStance(newStance);
        gameContext->refreshActionGrid();
    }
    else if (cmd.type == CommandType::END_SEX_SCENE || cmd.type == CommandType::CLOSE_MENU)
    {
        if (!gameContext->sceneStack.empty())
        {
            gameContext->popScene();
        }
        else
        {
            gameContext->changeState(std::make_unique<explorationState>());
        }
    }
}