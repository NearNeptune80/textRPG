#include "state/sexState.h"

#include <algorithm>
#include <format>
#include <random>

#include "core/characterDescription.h"
#include "core/game.h"
#include "core/textParser.h"
#include "entities/entity.h"
#include "state/explorationState.h"

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

void sexState::handleInput(game* gameContext, const SDL_Event& event) {}

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

    // 1. Kiss
    SexAction kiss;
    kiss.id = "action_kiss";
    kiss.name = "Kiss Lips";
    kiss.description = "Press your lips firmly against partner's in a passionate kiss.";
    kiss.type = SexActionType::KISS;
    kiss.arousalGainSelf = 12.0f;
    kiss.arousalGainPartner = 15.0f;
    kiss.dominanceShift = 3.0f;
    kiss.actorSlot = bodySlot::MOUTH;
    kiss.targetSlot = bodySlot::MOUTH;
    kiss.validStances = { SexStance::MISSIONARY, SexStance::STANDING, SexStance::LAP_SITTING };
    db.push_back(kiss);

    // 2. Caress Breasts
    SexAction caressBreasts;
    caressBreasts.id = "action_caress_breasts";
    caressBreasts.name = "Caress Breasts";
    caressBreasts.description = "Gently knead and stimulate partner's breasts and nipples.";
    caressBreasts.type = SexActionType::CARESS_BREASTS;
    caressBreasts.arousalGainSelf = 15.0f;
    caressBreasts.arousalGainPartner = 20.0f;
    caressBreasts.dominanceShift = 5.0f;
    caressBreasts.actorSlot = bodySlot::HANDS;
    caressBreasts.targetSlot = bodySlot::BREASTS;
    caressBreasts.validStances = { SexStance::MISSIONARY, SexStance::FROM_BEHIND, SexStance::STANDING, SexStance::LAP_SITTING };
    db.push_back(caressBreasts);

    // 3. Give Oral Sex
    SexAction giveOral;
    giveOral.id = "action_give_oral";
    giveOral.name = "Perform Oral Sex";
    giveOral.description = "Use tongue and mouth to intimately pleasure partner's genitals.";
    giveOral.type = SexActionType::ORAL_GIVE;
    giveOral.arousalGainSelf = 15.0f;
    giveOral.arousalGainPartner = 25.0f;
    giveOral.dominanceShift = 2.0f;
    giveOral.actorSlot = bodySlot::MOUTH;
    giveOral.targetSlot = bodySlot::GROIN;
    giveOral.validStances = { SexStance::KNEELING, SexStance::MISSIONARY, SexStance::LAP_SITTING };
    giveOral.requiresExposedGenitals = true;
    db.push_back(giveOral);

    // 4. Receive Oral Sex
    SexAction receiveOral;
    receiveOral.id = "action_receive_oral";
    receiveOral.name = "Receive Oral Sex";
    receiveOral.description = "Guide partner's mouth down to pleasure your genitals.";
    receiveOral.type = SexActionType::ORAL_RECEIVE;
    receiveOral.arousalGainSelf = 25.0f;
    receiveOral.arousalGainPartner = 15.0f;
    receiveOral.dominanceShift = 10.0f;
    receiveOral.actorSlot = bodySlot::GROIN;
    receiveOral.targetSlot = bodySlot::MOUTH;
    receiveOral.validStances = { SexStance::KNEELING, SexStance::MISSIONARY, SexStance::STANDING, SexStance::LAP_SITTING };
    receiveOral.requiresExposedGenitals = true;
    db.push_back(receiveOral);

    // 5. Vaginal Sex
    SexAction vaginalSex;
    vaginalSex.id = "action_vaginal_sex";
    vaginalSex.name = "Vaginal Penetration";
    vaginalSex.description = "Drive deep into partner's lush vaginal passage.";
    vaginalSex.type = SexActionType::VAGINAL_PENETRATION;
    vaginalSex.arousalGainSelf = 25.0f;
    vaginalSex.arousalGainPartner = 25.0f;
    vaginalSex.dominanceShift = 12.0f;
    vaginalSex.actorSlot = bodySlot::GROIN;
    vaginalSex.targetSlot = bodySlot::GROIN;
    vaginalSex.validStances = { SexStance::MISSIONARY, SexStance::FROM_BEHIND, SexStance::STANDING, SexStance::LAP_SITTING };
    vaginalSex.requiresPenetration = true;
    vaginalSex.requiresExposedGenitals = true;
    db.push_back(vaginalSex);

    // 6. Anal Sex
    SexAction analSex;
    analSex.id = "action_anal_sex";
    analSex.name = "Anal Penetration";
    analSex.description = "Press past tight sphincter rings for intense anal intercourse.";
    analSex.type = SexActionType::ANAL_PENETRATION;
    analSex.arousalGainSelf = 25.0f;
    analSex.arousalGainPartner = 25.0f;
    analSex.dominanceShift = 15.0f;
    analSex.actorSlot = bodySlot::GROIN;
    analSex.targetSlot = bodySlot::ASS;
    analSex.requiresPenetration = true;
    analSex.requiresExposedGenitals = true;
    analSex.validStances = { SexStance::MISSIONARY, SexStance::FROM_BEHIND, SexStance::KNEELING, SexStance::STANDING };
    db.push_back(analSex);

    // 7. Mammary Sex
    SexAction mammarySex;
    mammarySex.id = "action_mammary_sex";
    mammarySex.name = "Mammary Sex (Titfuck)";
    mammarySex.description = "Thrust between partner's soft breasts.";
    mammarySex.type = SexActionType::MAMMARY_SEX;
    mammarySex.arousalGainSelf = 20.0f;
    mammarySex.arousalGainPartner = 18.0f;
    mammarySex.dominanceShift = 8.0f;
    mammarySex.actorSlot = bodySlot::GROIN;
    mammarySex.targetSlot = bodySlot::BREASTS;
    mammarySex.validStances = { SexStance::MISSIONARY, SexStance::LAP_SITTING, SexStance::KNEELING };
    db.push_back(mammarySex);

    // 8. Spank
    SexAction spank;
    spank.id = "action_spank";
    spank.name = "Spank Buttocks";
    spank.description = "Deliver a sharp slap to partner's exposed rear.";
    spank.type = SexActionType::SPANK;
    spank.arousalGainSelf = 10.0f;
    spank.arousalGainPartner = 15.0f;
    spank.dominanceShift = 15.0f;
    spank.actorSlot = bodySlot::HANDS;
    spank.targetSlot = bodySlot::ASS;
    spank.validStances = { SexStance::FROM_BEHIND, SexStance::MISSIONARY, SexStance::STANDING, SexStance::LAP_SITTING };
    db.push_back(spank);

    return db;
}

bool sexState::isActionValidForStance(const SexAction& action) const
{
    return std::find(action.validStances.begin(), action.validStances.end(), m_stance) != action.validStances.end();
}

std::vector<SexAction> sexState::getAvailableActions() const
{
    if (!isPlayerDominant())
    {
        // Submissive actions
        std::vector<SexAction> subActions;

        SexAction plead;
        plead.id = "sub_plead";
        plead.name = "[Plead / Suggest Action]";
        plead.type = SexActionType::PLEAD_SUGGEST;
        subActions.push_back(plead);

        SexAction endure;
        endure.id = "sub_endure";
        endure.name = "[Endure Passively]";
        endure.type = SexActionType::ENDURE;
        subActions.push_back(endure);

        SexAction beg;
        beg.id = "sub_beg";
        beg.name = "[Beg for Climax]";
        beg.type = SexActionType::BEG_CLIMAX;
        subActions.push_back(beg);

        SexAction struggle;
        struggle.id = "sub_struggle";
        struggle.name = "[Struggle for Control]";
        struggle.type = SexActionType::STRUGGLE;
        subActions.push_back(struggle);

        return subActions;
    }

    auto allActions = buildMasterActionDatabase();
    std::vector<SexAction> valid;

    entity* partner = getPartner();

    for (const auto& act : allActions)
    {
        if (!isActionValidForStance(act)) continue;

        if (act.type == SexActionType::VAGINAL_PENETRATION)
        {
            if (partner && partner->anatomy.hasVagina())
            {
                valid.push_back(act);
            }
        }
        else if (act.type == SexActionType::MAMMARY_SEX)
        {
            if (partner && partner->anatomy.hasBreasts())
            {
                valid.push_back(act);
            }
        }
        else
        {
            valid.push_back(act);
        }
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
    entity* partner = getPartner();
    std::string pName = partner ? partner->name : "your partner";
    appendNarrative(std::format("You maneuver {} into the {} position.", pName, sexStanceToString(m_stance)));
}

bool sexState::executeAction(game* gameContext, const SexAction& action)
{
    if (!gameContext) return false;

    entity* player = gameContext->getPlayer();
    entity* partner = getPartner();
    if (!player || !partner) return false;

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

    std::string actionText = std::format("Using the {} position, you perform {} on {}.",
                                         sexStanceToString(m_stance), action.name, partner->name);
    appendNarrative(actionText);

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

    if (pPhys + (rand() % 10) >= partnerPhys)
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