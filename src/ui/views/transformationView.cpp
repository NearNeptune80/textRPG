#include "ui/views/transformationView.h"

#include <algorithm>
#include <format>
#include <string>
#include <vector>
#include <cmath>

#include "core/game.h"
#include "core/characterDescription.h"
#include "entities/entity.h"
#include "state/transformationState.h"
#include "ui/theme.h"
#include "ui/uiWidget.h"

#include "ui/editorCardWidgets.h"
#include "state/editorConfig.h"

namespace TransformationView
{
    using namespace EditorCardWidgets;

    static const auto& tfColors = EditorCardWidgets::getTransformationColors();
    static const auto& racialTypes = EditorConfig::Catalogs::racialTypes();
    static const auto& minorRacesWithNone = EditorConfig::Catalogs::minorRacesWithNone();
    static const auto& bodySizes = EditorConfig::Catalogs::bodySizes();
    static const auto& muscleTiers = EditorConfig::Catalogs::muscleTiers();
    static const auto& footTypes = EditorConfig::Catalogs::footTypes();
    static const auto& legConfigs = EditorConfig::Catalogs::legConfigs();
    static const auto& genitalPlacements = EditorConfig::Catalogs::genitalPlacements();
    static const auto& bodyHairOptions = EditorConfig::Catalogs::bodyHairOptions();
    static const auto& eyeShapes = EditorConfig::Catalogs::eyeShapes();
    static const auto& allHairStyles = EditorConfig::Catalogs::allHairStyles();
    static const auto& hornTypes = EditorConfig::Catalogs::hornTypes();
    static const auto& antennaTypes = EditorConfig::Catalogs::antennaTypes();
    static const auto& lipSizes = EditorConfig::Catalogs::lipSizes();
    static const auto& wetnessLevels = EditorConfig::Catalogs::wetnessLevels();
    static const auto& orificeModifiers = EditorConfig::Catalogs::orificeModifiers();
    static const auto& penetrationModifiers = EditorConfig::Catalogs::penetrationModifiers();
    static const auto& cupSizes = EditorConfig::Catalogs::cupSizes();
    static const auto& breastShapes = EditorConfig::Catalogs::breastShapes();
    static const auto& nippleShapes = EditorConfig::Catalogs::nippleShapes();
    static const auto& areolaeShapes = EditorConfig::Catalogs::areolaeShapes();
    static const auto& girthLevels = EditorConfig::Catalogs::girthLevels();
    static const auto& wingSizes = EditorConfig::Catalogs::wingSizes();
    static const auto& size5 = EditorConfig::Catalogs::size5();
    static const auto& fluidFlavours = EditorConfig::Catalogs::fluidFlavours();
    static const auto& fluidModifiers = EditorConfig::Catalogs::fluidModifiers();

    float render(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
    {
        transformationState* state = dynamic_cast<transformationState*>(gameContext->getActiveState());
        entity* player = gameContext->getPlayer();
        if (!state || !player) return 0.0f;

        float startY = curY;

        // Centered Content Column (proportional width instead of overly wide stretching)
        float contentMaxW = std::min(rect.w - (48.0f * uiScale), 940.0f * uiScale);
        float padX = rect.x + (rect.w - contentMaxW) / 2.0f;
        float availableW = contentMaxW;

        // 1. Full-Width Header Banner
        float headerH = 28.0f * uiScale;
        SDL_FRect headerRect = { rect.x, curY, rect.w, headerH };
        UIWidget::drawHeader(renderer, headerRect, std::format("FULL TRANSFORMATION & BODY MODIFICATION [{}]", transformationTabToString(state->currentTab)), Theme::colors.bgHeader, Theme::colors.textGold, uiScale);
        curY += headerH + (12.0f * uiScale);

        // 2. Overview Status Card
        SDL_FRect statusCardRect = { padX, curY, availableW, 42.0f * uiScale };
        UIWidget::drawPanel(renderer, statusCardRect, Theme::colors.bgDark, Theme::colors.borderButton);

        std::string genderStr = genderArchetypeToString(player->anatomy.getGenderArchetype());
        std::string racialTitle = player->anatomy.getRacialTitle();
        UIWidget::drawText(renderer, std::format("Subject: {} | Form: {} ({}) | Height: {:.0f}cm", player->name, racialTitle, genderStr, player->anatomy.heightMeters * 100.0f), padX + (12.0f * uiScale), curY + (6.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);

        std::string line2 = "Features: ";
        if (const bodyPart* b = player->anatomy.getPart(bodySlot::BREASTS)) line2 += std::format("Breasts: {} ", bodyPart::getCupSizeName(b->cupSize));
        if (player->anatomy.hasPenis()) line2 += "• Penis ";
        if (player->anatomy.hasVagina()) line2 += "• Vagina ";
        if (player->anatomy.hasPart(bodySlot::HORNS)) line2 += "• Horns ";
        if (player->anatomy.hasPart(bodySlot::WINGS)) line2 += "• Wings ";
        if (player->anatomy.hasPart(bodySlot::TAIL)) line2 += "• Tail ";
        UIWidget::drawText(renderer, line2, padX + (12.0f * uiScale), curY + (22.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.78f);
        curY += statusCardRect.h + (12.0f * uiScale);

        if (!state->statusMessage.empty())
        {
            UIWidget::drawText(renderer, state->statusMessage, padX, curY, Theme::colors.lust, uiScale * 0.85f);
            curY += (18.0f * uiScale);
        }

        // 3. Tab Specific Renderers with encapsulated Widget Cards
        switch (state->currentTab)
        {
            case TransformationTab::CORE:
            {
                int age = player->stats.getBaseStat("appeared_age") > 0 ? static_cast<int>(player->stats.getBaseStat("appeared_age")) : 20;
                curY += drawStepperCard<int>(renderer, gameContext, rect, padX, curY, availableW, "Age Appearance", "Adjust your character's apparent biological age.", std::format("{} years", age), [&](int delta) {
                    int newAge = std::clamp(age + delta, 18, 50);
                    player->stats.setBaseStat("appeared_age", static_cast<float>(newAge));
                }, uiScale, 1, 5, 10);

                curY += drawStepperCard<float>(renderer, gameContext, rect, padX, curY, availableW, "Height (cm)", "Total standing height from head to feet.", std::format("{:.0f} cm", player->anatomy.heightMeters * 100.0f), [&](float deltaCm) {
                    player->anatomy.heightMeters = std::clamp(player->anatomy.heightMeters + (deltaCm * 0.01f), 1.22f, 3.66f);
                }, uiScale, 1.0f, 5.0f, 20.0f);

                std::string curSize = player->anatomy.bodySize.empty() ? "Average" : player->anatomy.bodySize;
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Body Size Proportion", "Adjust overall body frame and fat distribution.", bodySizes, curSize, [&](const std::string& s) {
                    player->anatomy.bodySize = s;
                }, uiScale, 5);

                std::string curMuscle = player->anatomy.muscleTone.empty() ? "Toned" : player->anatomy.muscleTone;
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Muscle Proportions & Tone", "Adjust muscularity, definition, and physique firmness.", muscleTiers, curMuscle, [&](const std::string& m) {
                    player->anatomy.muscleTone = m;
                }, uiScale, 5);

                bodyPart* face = player->anatomy.getPart(bodySlot::HEAD);
                std::string curFace = face ? face->race : "Human";
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Racial Morphs & Body Structure", "Configure racial base types across individual sockets.", racialTypes, curFace, [&](const std::string& r) {
                    if (!player->anatomy.hasPart(bodySlot::HEAD)) {
                        bodyPart p; p.id = "head"; p.name = "Head"; p.race = r; player->anatomy.setPart(bodySlot::HEAD, p);
                    } else player->anatomy.getPart(bodySlot::HEAD)->race = r;
                }, uiScale, 6);

                bodyPart* torso = player->anatomy.getPart(bodySlot::TORSO);
                std::string torsoColor = torso ? torso->primaryColor : "Pale";
                curY += drawColorSwatchCard(renderer, gameContext, rect, padX, curY, availableW, "Skin Tone & Body Pigment", "Color palette for skin and body covering.", tfColors, torsoColor, [&](const std::string& col) {
                    if (!player->anatomy.hasPart(bodySlot::TORSO)) {
                        bodyPart tp; tp.id = "torso"; tp.name = "Torso"; tp.race = "Human"; tp.primaryColor = col;
                        player->anatomy.setPart(bodySlot::TORSO, tp);
                    } else {
                        player->anatomy.getPart(bodySlot::TORSO)->primaryColor = col;
                    }
                }, uiScale);

                bodyPart* feet = player->anatomy.getPart(bodySlot::FEET);
                std::string curFoot = (feet && !feet->style.empty()) ? feet->style : "Plantigrade";
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Foot Anatomy", "Foot configuration and digit stance.", footTypes, curFoot, [&](const std::string& f) {
                    if (!player->anatomy.hasPart(bodySlot::FEET)) {
                        bodyPart fp; fp.id = "feet"; fp.name = "Feet"; fp.race = "Human"; fp.style = f; player->anatomy.setPart(bodySlot::FEET, fp);
                    } else player->anatomy.getPart(bodySlot::FEET)->style = f;
                }, uiScale, 4);

                bodyPart* legs = player->anatomy.getPart(bodySlot::LEGS);
                std::string curLeg = (legs && !legs->style.empty()) ? legs->style : "Bipedal";
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Lower Body Setup", "Leg configuration and lower body morphology.", legConfigs, curLeg, [&](const std::string& l) {
                    if (!player->anatomy.hasPart(bodySlot::LEGS)) {
                        bodyPart lp; lp.id = "legs"; lp.name = "Legs"; lp.race = "Human"; lp.style = l; player->anatomy.setPart(bodySlot::LEGS, lp);
                    } else player->anatomy.getPart(bodySlot::LEGS)->style = l;
                }, uiScale, 6);

                bodyPart* groin = player->anatomy.getPart(bodySlot::GROIN);
                std::string curPlace = (groin && !groin->style.empty()) ? groin->style : "Normal";
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Genital Placement", "Anatomical position of genital sockets.", genitalPlacements, curPlace, [&](const std::string& gp) {
                    if (auto* g = player->anatomy.getPart(bodySlot::GROIN)) g->style = gp;
                }, uiScale, 3);
                break;
            }

            case TransformationTab::EYES:
            {
                bodyPart* eyes = player->anatomy.getPart(bodySlot::EYES);
                std::string eyeRace = eyes ? eyes->race : "Human";
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Ocular Morphology", "Customize eye socket race and demonic properties.", racialTypes, eyeRace, [&](const std::string& r) {
                    if (!player->anatomy.hasPart(bodySlot::EYES)) {
                        bodyPart ep; ep.id = "eyes_" + r; ep.name = "Eyes"; ep.race = r; ep.primaryColor = "Azure Blue"; player->anatomy.setPart(bodySlot::EYES, ep);
                    } else player->anatomy.getPart(bodySlot::EYES)->race = r;
                }, uiScale, 6);

                std::string curEyeShape = (eyes && !eyes->style.empty()) ? eyes->style : "Round";
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Iris & Pupil Shapes", "Change iris and pupil structural geometry.", eyeShapes, curEyeShape, [&](const std::string& es) {
                    if (!player->anatomy.hasPart(bodySlot::EYES)) {
                        bodyPart ep; ep.id = "eyes"; ep.name = "Eyes"; ep.race = "Human"; ep.style = es; player->anatomy.setPart(bodySlot::EYES, ep);
                    } else player->anatomy.getPart(bodySlot::EYES)->style = es;
                }, uiScale, 5);

                std::string eyeColor = eyes ? eyes->primaryColor : "Azure Blue";
                curY += drawColorSwatchCard(renderer, gameContext, rect, padX, curY, availableW, "Iris Color Swatches", "Color of the eye irises.", tfColors, eyeColor, [&](const std::string& col) {
                    if (!player->anatomy.hasPart(bodySlot::EYES)) {
                        bodyPart ep; ep.id = "eyes"; ep.name = "Eyes"; ep.race = "Human"; ep.primaryColor = col; player->anatomy.setPart(bodySlot::EYES, ep);
                    } else player->anatomy.getPart(bodySlot::EYES)->primaryColor = col;
                }, uiScale);

                std::string scleraColor = (eyes && !eyes->secondaryColor.empty()) ? eyes->secondaryColor : "Pure White";
                curY += drawColorSwatchCard(renderer, gameContext, rect, padX, curY, availableW, "Sclera Color Swatches", "Color of the surrounding eye whites.", tfColors, scleraColor, [&](const std::string& col) {
                    if (!player->anatomy.hasPart(bodySlot::EYES)) {
                        bodyPart ep; ep.id = "eyes"; ep.name = "Eyes"; ep.race = "Human"; ep.secondaryColor = col; player->anatomy.setPart(bodySlot::EYES, ep);
                    } else player->anatomy.getPart(bodySlot::EYES)->secondaryColor = col;
                }, uiScale);
                break;
            }

            case TransformationTab::HAIR:
            {
                bodyPart* hair = player->anatomy.getPart(bodySlot::HAIR);

                curY += drawStepperCard<float>(renderer, gameContext, rect, padX, curY, availableW, "Hair Length (cm)", "Head hair length measured in centimeters.", std::format("{:.0f} cm", hair ? hair->length : 15.0f), [&](float delta) {
                    if (!hair) { bodyPart hp; hp.id = "hair"; hp.name = "Hair"; hp.length = 15.0f; player->anatomy.setPart(bodySlot::HAIR, hp); }
                    if (auto* hp = player->anatomy.getPart(bodySlot::HAIR)) hp->length = std::clamp(hp->length + delta, 0.0f, 150.0f);
                }, uiScale, 1.0f, 5.0f, 25.0f);

                std::string curStyle = hair ? hair->style : "Short";
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Hairstyle", "Styling, cuts, braids, and grooming.", allHairStyles, curStyle, [&](const std::string& s) {
                    if (!player->anatomy.hasPart(bodySlot::HAIR)) {
                        bodyPart hp; hp.id = "hair"; hp.name = "Hair"; hp.style = s; player->anatomy.setPart(bodySlot::HAIR, hp);
                    } else player->anatomy.getPart(bodySlot::HAIR)->style = s;
                }, uiScale, 6);

                std::string hairColor = hair ? hair->primaryColor : "Chestnut Brown";
                curY += drawColorSwatchCard(renderer, gameContext, rect, padX, curY, availableW, "Hair Color Swatches", "Select head hair pigment.", tfColors, hairColor, [&](const std::string& col) {
                    if (!player->anatomy.hasPart(bodySlot::HAIR)) {
                        bodyPart hp; hp.id = "hair"; hp.name = "Hair"; hp.primaryColor = col; player->anatomy.setPart(bodySlot::HAIR, hp);
                    } else player->anatomy.getPart(bodySlot::HAIR)->primaryColor = col;
                }, uiScale);

                bodyPart* head = player->anatomy.getPart(bodySlot::HEAD);
                std::string curFacialHair = (head && !head->style.empty()) ? head->style : "None";
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Facial Hair & Grooming", "Beard and facial grooming.", bodyHairOptions, curFacialHair, [&](const std::string& fh) {
                    if (!player->anatomy.hasPart(bodySlot::HEAD)) {
                        bodyPart hp; hp.id = "head"; hp.name = "Head"; hp.style = fh; player->anatomy.setPart(bodySlot::HEAD, hp);
                    } else player->anatomy.getPart(bodySlot::HEAD)->style = fh;
                }, uiScale, 4);
                break;
            }

            case TransformationTab::HEAD_FACE:
            {
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Ears & Facial Structure", "Change ear morphology and race.", racialTypes, player->anatomy.hasPart(bodySlot::EARS) ? player->anatomy.getPart(bodySlot::EARS)->race : "Human", [&](const std::string& r) {
                    bodyPart ear; ear.id = "ears_" + r; ear.name = "Ears"; ear.race = r;
                    player->anatomy.setPart(bodySlot::EARS, ear);
                }, uiScale, 6);

                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Horns & Crests", "Grow, shape, or remove horns.", hornTypes, player->anatomy.hasPart(bodySlot::HORNS) ? player->anatomy.getPart(bodySlot::HORNS)->race : "None", [&](const std::string& r) {
                    if (r == "None") player->anatomy.removePart(bodySlot::HORNS);
                    else {
                        bodyPart h; h.id = "horns_" + r; h.name = "Horns"; h.race = r; h.count = 2; h.length = 20.0f;
                        player->anatomy.setPart(bodySlot::HORNS, h);
                    }
                }, uiScale, 5);

                if (bodyPart* horns = player->anatomy.getPart(bodySlot::HORNS))
                {
                    curY += drawStepperCard<float>(renderer, gameContext, rect, padX, curY, availableW, "Horn Length (cm)", "Length of horns projecting from skull.", std::format("{:.0f} cm", horns->length), [&](float delta) {
                        horns->length = std::clamp(horns->length + delta, 2.0f, 80.0f);
                    }, uiScale, 1.0f, 5.0f, 20.0f);

                    curY += drawStepperCard<int>(renderer, gameContext, rect, padX, curY, availableW, "Horn Pairs", "Number of horn rows on head.", std::format("{} pairs", horns->count / 2), [&](int deltaPairs) {
                        horns->count = std::clamp(horns->count + (deltaPairs * 2), 2, 8);
                    }, uiScale, 1, 2, 0);
                }

                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Antennae", "Grow, shape, or remove antennae.", antennaTypes, player->anatomy.hasPart(bodySlot::ANTENNAE) ? player->anatomy.getPart(bodySlot::ANTENNAE)->race : "None", [&](const std::string& a) {
                    if (a == "None") player->anatomy.removePart(bodySlot::ANTENNAE);
                    else {
                        bodyPart ap; ap.id = "antennae_" + a; ap.name = "Antennae"; ap.race = a; ap.count = 2;
                        player->anatomy.setPart(bodySlot::ANTENNAE, ap);
                    }
                }, uiScale, 5);

                bodyPart* mouth = player->anatomy.getPart(bodySlot::MOUTH);
                if (!mouth) { bodyPart mp; mp.id = "mouth"; mp.name = "Mouth"; mp.orifice.exists = true; mp.orifice.wetnessLevel = 2; player->anatomy.setPart(bodySlot::MOUTH, mp); mouth = player->anatomy.getPart(bodySlot::MOUTH); }

                std::string curLip = mouth->style.empty() ? "Average" : mouth->style;
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Lip Dimensions", "Size and plumpness of lips.", lipSizes, curLip, [&](const std::string& l) {
                    mouth->style = l;
                }, uiScale, 5);

                bool puffy = mouth->hasTag("puffy_lips");
                curY += drawToggleCard(renderer, gameContext, rect, padX, curY, availableW, "Puffy Lips", "Extra softness and swelling.", puffy, [&](bool p) {
                    if (p) mouth->tags.push_back("puffy_lips");
                    else std::erase(mouth->tags, "puffy_lips");
                }, uiScale, "Puffy", "Natural");

                curY += drawTogglePillCard(renderer, gameContext, rect, padX, curY, availableW, "Throat Orifice Modifiers", "Special internal qualities of your throat.", orificeModifiers, mouth->tags, [&](const std::string& mod, bool act) {
                    if (act) mouth->tags.push_back(mod);
                    else std::erase(mouth->tags, mod);
                }, uiScale, 4);

                int curWetIdx = std::clamp(mouth->orifice.wetnessLevel, 0, static_cast<int>(wetnessLevels.size()) - 1);
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Throat Wetness", "Saliva and moisture levels.", wetnessLevels, wetnessLevels[curWetIdx], [&](const std::string& w) {
                    for (size_t i = 0; i < wetnessLevels.size(); ++i) if (wetnessLevels[i] == w) mouth->orifice.wetnessLevel = static_cast<int>(i);
                }, uiScale, 4);
                break;
            }

            case TransformationTab::ASS_HIPS:
            {
                bodyPart* ass = player->anatomy.getPart(bodySlot::ASS);
                if (!ass) { bodyPart ap; ap.id = "ass"; ap.name = "Ass"; ap.orifice.exists = true; player->anatomy.setPart(bodySlot::ASS, ap); ass = player->anatomy.getPart(bodySlot::ASS); }
                bodyPart* hips = player->anatomy.getPart(bodySlot::HIPS);
                if (!hips) { bodyPart hp; hp.id = "hips"; hp.name = "Hips"; player->anatomy.setPart(bodySlot::HIPS, hp); hips = player->anatomy.getPart(bodySlot::HIPS); }

                std::string curAss = ass->style.empty() ? "Average" : ass->style;
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Ass Dimensions", "Rear cheek volume and projection.", size5, curAss, [&](const std::string& a) {
                    ass->style = a;
                }, uiScale, 5);

                std::string curHips = hips->style.empty() ? "Average" : hips->style;
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Hip Breadth", "Pelvic width and hourglass curvature.", size5, curHips, [&](const std::string& h) {
                    hips->style = h;
                }, uiScale, 5);

                curY += drawStepperCard<float>(renderer, gameContext, rect, padX, curY, availableW, "Anus Depth (cm)", "Internal rectal depth before obstruction.", std::format("{:.0f} cm", ass->orifice.depthCm), [&](float delta) {
                    ass->orifice.depthCm = std::clamp(ass->orifice.depthCm + delta, 5.0f, 40.0f);
                }, uiScale, 1.0f, 3.0f, 10.0f);

                curY += drawStepperCard<float>(renderer, gameContext, rect, padX, curY, availableW, "Anal Elasticity", "Stretch accommodation and recovery.", std::format("{:.0f}%", ass->orifice.elasticity), [&](float delta) {
                    ass->orifice.elasticity = std::clamp(ass->orifice.elasticity + delta, 0.0f, 100.0f);
                }, uiScale, 1.0f, 5.0f, 25.0f);

                bool bleached = ass->hasTag("bleached");
                curY += drawToggleCard(renderer, gameContext, rect, padX, curY, availableW, "Bleached Anus", "Cosmetic treatment for anal sphincter.", bleached, [&](bool state) {
                    if (state) ass->tags.push_back("bleached");
                    else std::erase(ass->tags, "bleached");
                }, uiScale);

                curY += drawTogglePillCard(renderer, gameContext, rect, padX, curY, availableW, "Anal Modifiers", "Special internal qualities of your rectum.", orificeModifiers, ass->tags, [&](const std::string& mod, bool act) {
                    if (act) ass->tags.push_back(mod);
                    else std::erase(ass->tags, mod);
                }, uiScale, 4);

                int curAnalWetIdx = std::clamp(ass->orifice.wetnessLevel, 0, static_cast<int>(wetnessLevels.size()) - 1);
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Anal Wetness", "Natural lubrication level.", wetnessLevels, wetnessLevels[curAnalWetIdx], [&](const std::string& w) {
                    for (size_t i = 0; i < wetnessLevels.size(); ++i) if (wetnessLevels[i] == w) ass->orifice.wetnessLevel = static_cast<int>(i);
                }, uiScale, 4);
                break;
            }

            case TransformationTab::BREASTS:
            {
                bodyPart* breasts = player->anatomy.getPart(bodySlot::BREASTS);
                if (!breasts) { bodyPart bp; bp.id = "breasts"; bp.name = "Breasts"; player->anatomy.setPart(bodySlot::BREASTS, bp); breasts = player->anatomy.getPart(bodySlot::BREASTS); }
                bodyPart* nipples = player->anatomy.getPart(bodySlot::NIPPLES);
                if (!nipples) { bodyPart np; np.id = "nipples"; np.name = "Nipples"; player->anatomy.setPart(bodySlot::NIPPLES, np); nipples = player->anatomy.getPart(bodySlot::NIPPLES); }

                int curCupIdx = std::clamp(breasts->cupSize, 0, static_cast<int>(cupSizes.size()) - 1);
                curY += drawStepperCard<int>(renderer, gameContext, rect, padX, curY, availableW, "Cup Size", "Chest breast volume and mass rating.", std::format("{}-cup", cupSizes[curCupIdx]), [&](int delta) {
                    breasts->cupSize = std::clamp(breasts->cupSize + delta, 0, static_cast<int>(cupSizes.size()) - 1);
                }, uiScale, 1, 3, 6);

                std::string curBreastShape = breasts->style.empty() ? "Round" : breasts->style;
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Breast Shape", "Projection and fullness contour.", breastShapes, curBreastShape, [&](const std::string& s) {
                    breasts->style = s;
                }, uiScale, 6);

                curY += drawToggleCard(renderer, gameContext, rect, padX, curY, availableW, "Lactation Active", "Enable or disable continuous milk production.", breasts->isLactating, [&](bool l) {
                    breasts->isLactating = l;
                    if (l && breasts->maxFluidMl <= 0.0f) { breasts->maxFluidMl = 1000.0f; breasts->currentFluidMl = 250.0f; }
                }, uiScale);

                if (breasts->isLactating)
                {
                    curY += drawStepperCard<float>(renderer, gameContext, rect, padX, curY, availableW, "Milk Storage (ml)", "Maximum mammary fluid capacity.", std::format("{:.0f} ml", breasts->maxFluidMl), [&](float delta) {
                        breasts->maxFluidMl = std::clamp(breasts->maxFluidMl + delta, 100.0f, 100000.0f);
                        breasts->currentFluidMl = std::min(breasts->currentFluidMl, breasts->maxFluidMl);
                    }, uiScale, 25.0f, 250.0f, 1000.0f);

                    std::string curMilkFlav = breasts->secondaryColor.empty() ? "Milk" : breasts->secondaryColor;
                    curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Milk Flavours", "Taste profile of expressed milk.", fluidFlavours, curMilkFlav, [&](const std::string& f) {
                        breasts->secondaryColor = f;
                    }, uiScale, 6);

                    curY += drawTogglePillCard(renderer, gameContext, rect, padX, curY, availableW, "Milk Modifiers", "Psychoactive and physical milk traits.", fluidModifiers, breasts->tags, [&](const std::string& m, bool act) {
                        if (act) breasts->tags.push_back(m);
                        else std::erase(breasts->tags, m);
                    }, uiScale, 5);
                }

                std::string curNippleShape = nipples->style.empty() ? "Normal" : nipples->style;
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Nipple Shape", "Morphology of nipples.", nippleShapes, curNippleShape, [&](const std::string& ns) {
                    nipples->style = ns;
                }, uiScale, 4);

                std::string curAreolaShape = nipples->secondaryColor.empty() ? "Round" : nipples->secondaryColor;
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Areolae Shape", "Contour of surrounding areolae.", areolaeShapes, curAreolaShape, [&](const std::string& as) {
                    nipples->secondaryColor = as;
                }, uiScale, 3);
                break;
            }

            case TransformationTab::VAGINA:
            {
                bool hasVag = player->anatomy.hasVagina();
                curY += drawToggleCard(renderer, gameContext, rect, padX, curY, availableW, "Vagina Active", "Grow, modify, or remove female genitalia.", hasVag, [&](bool v) {
                    bodyPart* g = player->anatomy.getPart(bodySlot::GROIN);
                    if (v) {
                        if (!g) {
                            bodyPart gp; gp.id = "groin"; gp.name = "Vagina"; gp.orifice.exists = true; gp.orifice.depthCm = 16.0f;
                            gp.tags.push_back("has_vagina");
                            player->anatomy.setPart(bodySlot::GROIN, gp);
                        } else {
                            g->orifice.exists = true;
                            if (g->orifice.depthCm <= 0.0f) g->orifice.depthCm = 16.0f;
                            if (std::find(g->tags.begin(), g->tags.end(), "has_vagina") == g->tags.end()) g->tags.push_back("has_vagina");
                            if (player->anatomy.hasPenis()) g->name = "Hermaphrodite Genitalia";
                            else g->name = "Vagina";
                        }
                    } else {
                        if (g) {
                            g->orifice.exists = false;
                            std::erase(g->tags, "has_vagina");
                            std::erase(g->tags, "vagina");
                            if (!player->anatomy.hasPenis()) {
                                player->anatomy.removePart(bodySlot::GROIN);
                            } else {
                                g->name = "Penis";
                            }
                        }
                    }
                }, uiScale);

                if (player->anatomy.hasVagina())
                {
                    bodyPart* g = player->anatomy.getPart(bodySlot::GROIN);
                    curY += drawStepperCard<float>(renderer, gameContext, rect, padX, curY, availableW, "Vaginal Depth", "Internal vaginal canal length.", std::format("{:.0f} cm", g ? g->orifice.depthCm : 16.0f), [&](float delta) {
                        if (g) g->orifice.depthCm = std::clamp(g->orifice.depthCm + delta, 5.0f, 35.0f);
                    }, uiScale, 1.0f, 3.0f, 10.0f);

                    std::string curClitSize = (g && !g->secondaryColor.empty()) ? g->secondaryColor : "Average";
                    curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Clitoris Size", "Clitoral length and prominence.", size5, curClitSize, [&](const std::string& cs) {
                        if (g) g->secondaryColor = cs;
                    }, uiScale, 5);

                    std::string curClitGirth = (g && !g->style.empty()) ? g->style : "Average";
                    curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Clitoris Girth", "Clitoral thickness and circumference.", girthLevels, curClitGirth, [&](const std::string& cg) {
                        if (g) g->style = cg;
                    }, uiScale, 4);

                    std::vector<std::string> curTags = g ? g->tags : std::vector<std::string>{};
                    curY += drawTogglePillCard(renderer, gameContext, rect, padX, curY, availableW, "Clitoral Penetration Modifiers", "Special traits allowing clitoral penetration.", penetrationModifiers, curTags, [&](const std::string& mod, bool act) {
                        if (g) {
                            if (act) g->tags.push_back(mod);
                            else std::erase(g->tags, mod);
                        }
                    }, uiScale, 4);

                    bool squirter = g && g->hasTag("squirter");
                    curY += drawToggleCard(renderer, gameContext, rect, padX, curY, availableW, "Squirter", "Copious ejaculation during orgasm.", squirter, [&](bool sq) {
                        if (g) {
                            if (sq) g->tags.push_back("squirter");
                            else std::erase(g->tags, "squirter");
                        }
                    }, uiScale, "Squirter", "Normal");

                    bool hymen = !g || g->hasTag("virgin_hymen");
                    curY += drawToggleCard(renderer, gameContext, rect, padX, curY, availableW, "Hymen Intact", "Virginity membrane status.", hymen, [&](bool hy) {
                        if (g) {
                            if (hy) g->tags.push_back("virgin_hymen");
                            else std::erase(g->tags, "virgin_hymen");
                        }
                    }, uiScale, "Intact", "Broken");

                    bool eggLayer = g && g->hasTag("egg_layer");
                    curY += drawToggleCard(renderer, gameContext, rect, padX, curY, availableW, "Egg Layer", "Reproductive mode for oviparous species.", eggLayer, [&](bool el) {
                        if (g) {
                            if (el) g->tags.push_back("egg_layer");
                            else std::erase(g->tags, "egg_layer");
                        }
                    }, uiScale, "Egg-layer", "Live young");

                    std::string curGirlFlav = "Girlcum";
                    if (g) {
                        for (const auto& f : fluidFlavours) if (g->hasTag("flav_" + f)) { curGirlFlav = f; break; }
                    }
                    curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Girlcum Flavours", "Taste of vaginal arousal fluid.", fluidFlavours, curGirlFlav, [&](const std::string& f) {
                        if (g) {
                            for (const auto& fl : fluidFlavours) std::erase(g->tags, "flav_" + fl);
                            g->tags.push_back("flav_" + f);
                        }
                    }, uiScale, 6);

                    curY += drawTogglePillCard(renderer, gameContext, rect, padX, curY, availableW, "Girlcum Modifiers", "Qualities and effects of vaginal fluid.", fluidModifiers, curTags, [&](const std::string& mod, bool act) {
                        if (g) {
                            if (act) g->tags.push_back(mod);
                            else std::erase(g->tags, mod);
                        }
                    }, uiScale, 5);
                }
                break;
            }

            case TransformationTab::PENIS:
            {
                bool hasPen = player->anatomy.hasPenis();
                curY += drawToggleCard(renderer, gameContext, rect, padX, curY, availableW, "Penis Active", "Grow, modify, or remove male phallus.", hasPen, [&](bool p) {
                    bodyPart* g = player->anatomy.getPart(bodySlot::GROIN);
                    if (p) {
                        if (!g) {
                            bodyPart gp; gp.id = "groin"; gp.name = "Penis"; gp.length = 16.0f; gp.diameter = 3.8f; gp.maxFluidMl = 30.0f; gp.currentFluidMl = 15.0f;
                            gp.tags.push_back("has_penis");
                            player->anatomy.setPart(bodySlot::GROIN, gp);
                        } else {
                            if (g->length <= 0.0f) g->length = 16.0f;
                            if (g->diameter <= 0.0f) g->diameter = 3.8f;
                            if (g->maxFluidMl <= 0.0f) g->maxFluidMl = 30.0f;
                            if (std::find(g->tags.begin(), g->tags.end(), "has_penis") == g->tags.end()) g->tags.push_back("has_penis");
                            if (player->anatomy.hasVagina()) g->name = "Hermaphrodite Genitalia";
                            else g->name = "Penis";
                        }
                    } else {
                        if (g) {
                            g->length = 0.0f;
                            std::erase(g->tags, "has_penis");
                            std::erase(g->tags, "penis");
                            if (!player->anatomy.hasVagina()) {
                                player->anatomy.removePart(bodySlot::GROIN);
                            } else {
                                g->name = "Vagina";
                            }
                        }
                    }
                }, uiScale);

                if (player->anatomy.hasPenis())
                {
                    bodyPart* g = player->anatomy.getPart(bodySlot::GROIN);
                    curY += drawStepperCard<float>(renderer, gameContext, rect, padX, curY, availableW, "Penis Length (cm)", "Erect phallic length from base to tip.", std::format("{:.1f} cm", g ? g->length : 16.0f), [&](float delta) {
                        if (g) g->length = std::clamp(g->length + delta, 3.0f, 100.0f);
                    }, uiScale, 0.5f, 2.5f, 10.0f);

                    curY += drawStepperCard<float>(renderer, gameContext, rect, padX, curY, availableW, "Penis Girth (cm)", "Shaft diameter across the center.", std::format("{:.1f} cm", g ? g->diameter : 3.8f), [&](float delta) {
                        if (g) g->diameter = std::clamp(g->diameter + delta, 1.5f, 15.0f);
                    }, uiScale, 0.1f, 0.5f, 2.0f);

                    std::vector<std::string> curTags = g ? g->tags : std::vector<std::string>{};
                    curY += drawTogglePillCard(renderer, gameContext, rect, padX, curY, availableW, "Penis Modifiers", "Knots, barbs, flares, sheaths, and special traits.", penetrationModifiers, curTags, [&](const std::string& mod, bool act) {
                        if (g) {
                            if (act) g->tags.push_back(mod);
                            else std::erase(g->tags, mod);
                        }
                    }, uiScale, 4);

                    bool internalBalls = g && g->hasTag("internal_testicles");
                    curY += drawToggleCard(renderer, gameContext, rect, padX, curY, availableW, "Internal Testicles", "Testicles retracted inside lower abdomen.", internalBalls, [&](bool ib) {
                        if (g) {
                            if (ib) g->tags.push_back("internal_testicles");
                            else std::erase(g->tags, "internal_testicles");
                        }
                    }, uiScale, "Internal", "External");

                    curY += drawStepperCard<float>(renderer, gameContext, rect, padX, curY, availableW, "Cum Storage (ml)", "Total volume of stored seminal fluid.", std::format("{:.0f} ml", g ? g->maxFluidMl : 30.0f), [&](float delta) {
                        if (g) {
                            g->maxFluidMl = std::clamp(g->maxFluidMl + delta, 5.0f, 50000.0f);
                            g->currentFluidMl = std::min(g->currentFluidMl, g->maxFluidMl);
                        }
                    }, uiScale, 5.0f, 50.0f, 500.0f);

                    std::string curCumFlav = "Cum";
                    if (g) {
                        for (const auto& f : fluidFlavours) if (g->hasTag("cum_flav_" + f)) { curCumFlav = f; break; }
                    }
                    curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Cum Flavours", "Taste profile of male seed.", fluidFlavours, curCumFlav, [&](const std::string& f) {
                        if (g) {
                            for (const auto& fl : fluidFlavours) std::erase(g->tags, "cum_flav_" + fl);
                            g->tags.push_back("cum_flav_" + f);
                        }
                    }, uiScale, 6);

                    curY += drawTogglePillCard(renderer, gameContext, rect, padX, curY, availableW, "Cum Modifiers", "Psychoactive and physical cum traits.", fluidModifiers, curTags, [&](const std::string& mod, bool act) {
                        if (g) {
                            if (act) g->tags.push_back(mod);
                            else std::erase(g->tags, mod);
                        }
                    }, uiScale, 5);
                }
                break;
            }

            case TransformationTab::CROTCH_BOOBS:
            {
                static const std::vector<std::string> crotchTypes = { "None", "Bovine Udders", "Caprine Udders", "Feline Crotch-Boobs", "Canine Teats" };
                bodyPart* crotchPart = player->anatomy.getPart(bodySlot::HIPS);
                std::string curCrotchType = (crotchPart && !crotchPart->style.empty()) ? crotchPart->style : "None";
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Crotch-Boobs & Udders", "Secondary udder / crotch mammary glands.", crotchTypes, curCrotchType, [&](const std::string& ct) {
                    if (ct == "None") {
                        player->anatomy.removePart(bodySlot::HIPS);
                    } else {
                        if (!crotchPart) {
                            bodyPart hp; hp.id = "crotch_udders"; hp.name = ct; hp.style = ct; hp.cupSize = 1; player->anatomy.setPart(bodySlot::HIPS, hp);
                        } else {
                            crotchPart->name = ct;
                            crotchPart->style = ct;
                        }
                    }
                }, uiScale, 5);

                crotchPart = player->anatomy.getPart(bodySlot::HIPS);
                int crotchCup = crotchPart ? crotchPart->cupSize : 0;
                int clampedCup = std::clamp(crotchCup, 0, static_cast<int>(cupSizes.size()) - 1);
                curY += drawStepperCard<int>(renderer, gameContext, rect, padX, curY, availableW, "Crotch Mammary Cup Size", "Volume of secondary lower mammary glands.", std::format("{}-cup", cupSizes[clampedCup]), [&](int delta) {
                    if (!player->anatomy.hasPart(bodySlot::HIPS)) {
                        bodyPart hp; hp.id = "crotch_udders"; hp.name = "Crotch Udders"; hp.style = "Bovine Udders"; hp.cupSize = 1;
                        player->anatomy.setPart(bodySlot::HIPS, hp);
                    }
                    if (auto* hp = player->anatomy.getPart(bodySlot::HIPS)) {
                        hp->cupSize = std::clamp(hp->cupSize + delta, 0, static_cast<int>(cupSizes.size()) - 1);
                    }
                }, uiScale, 1, 3, 6);
                break;
            }

            case TransformationTab::APPENDAGES:
            {
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Wings & Flight Organs", "Grow, size, or modify back wings.", minorRacesWithNone, player->anatomy.hasPart(bodySlot::WINGS) ? player->anatomy.getPart(bodySlot::WINGS)->race : "None", [&](const std::string& r) {
                    if (r == "None") player->anatomy.removePart(bodySlot::WINGS);
                    else {
                        bodyPart w; w.id = "wings_" + r; w.name = "Wings"; w.race = r; w.count = 2; w.style = "Average";
                        player->anatomy.setPart(bodySlot::WINGS, w);
                    }
                }, uiScale, 6);

                bodyPart* wings = player->anatomy.getPart(bodySlot::WINGS);
                std::string curWingSize = (wings && !wings->style.empty()) ? wings->style : "Average";
                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Wing Wingspan Size", "Size rating of wings.", wingSizes, curWingSize, [&](const std::string& ws) {
                    if (wings) wings->style = ws;
                }, uiScale, 4);

                curY += drawPillCard(renderer, gameContext, rect, padX, curY, availableW, "Tails & Appendages", "Grow or modify tail racial type.", minorRacesWithNone, player->anatomy.hasPart(bodySlot::TAIL) ? player->anatomy.getPart(bodySlot::TAIL)->race : "None", [&](const std::string& r) {
                    if (r == "None") player->anatomy.removePart(bodySlot::TAIL);
                    else {
                        bodyPart t; t.id = "tail_" + r; t.name = "Tail"; t.race = r; t.count = 1; t.length = 75.0f;
                        player->anatomy.setPart(bodySlot::TAIL, t);
                    }
                }, uiScale, 6);

                if (bodyPart* tail = player->anatomy.getPart(bodySlot::TAIL))
                {
                    curY += drawStepperCard<int>(renderer, gameContext, rect, padX, curY, availableW, "Tail Count", "Number of rear tails.", std::format("{}", tail->count), [&](int delta) {
                        tail->count = std::clamp(tail->count + delta, 1, 9);
                    }, uiScale, 1, 2, 4);

                    curY += drawStepperCard<float>(renderer, gameContext, rect, padX, curY, availableW, "Tail Length (cm)", "Length of tails extending behind character.", std::format("{:.0f} cm", tail->length), [&](float delta) {
                        tail->length = std::clamp(tail->length + delta, 10.0f, 250.0f);
                    }, uiScale, 1.0f, 10.0f, 50.0f);

                    curY += drawTogglePillCard(renderer, gameContext, rect, padX, curY, availableW, "Spinneret Orifice Modifiers", "Silk production and gland traits.", orificeModifiers, tail->tags, [&](const std::string& mod, bool act) {
                        if (act) tail->tags.push_back(mod);
                        else std::erase(tail->tags, mod);
                    }, uiScale, 4);
                }
                break;
            }

            case TransformationTab::INSPECT_PRESETS:
            {
                // Live Prose Card
                std::string fullDesc = characterDescription::generateFullDescription(player);
                SDL_FRect descCard = { padX, curY, availableW, 140.0f * uiScale };
                UIWidget::drawPanel(renderer, descCard, Theme::colors.bgSlot, Theme::colors.borderNormal);
                UIWidget::drawText(renderer, "Live Anatomical Prose Description", padX + (12.0f * uiScale), curY + (8.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);
                float proseH = UIWidget::drawTextWrapped(renderer, fullDesc, padX + (12.0f * uiScale), curY + (28.0f * uiScale), availableW - (24.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.82f);
                descCard.h = std::max(60.0f * uiScale, proseH + (40.0f * uiScale));
                curY += descCard.h + (14.0f * uiScale);

                // Presets Card
                SDL_FRect presetsCard = { padX, curY, availableW, 60.0f * uiScale };
                UIWidget::drawPanel(renderer, presetsCard, Theme::colors.bgSlot, Theme::colors.borderNormal);
                UIWidget::drawText(renderer, "Transformation Presets", padX + (12.0f * uiScale), curY + (8.0f * uiScale), Theme::colors.textGold, uiScale * 0.88f);
                UIWidget::drawText(renderer, "Save, restore, or reset full-body baseline configurations.", padX + (12.0f * uiScale), curY + (26.0f * uiScale), Theme::colors.textSecondary, uiScale * 0.74f);

                // Save current preset button
                float btnY = curY + (22.0f * uiScale);
                float btnW = 140.0f * uiScale;
                SDL_FRect saveBtnRect = { padX + availableW - (btnW * 2.0f) - (20.0f * uiScale), btnY, btnW, 26.0f * uiScale };
                auto mousePos = gameContext->input.getMousePosition();
                bool clicked = gameContext->input.isLeftMouseJustClicked();
                bool inPanel = (mousePos.y >= rect.y && mousePos.y <= rect.y + rect.h);

                bool sHov = inPanel && (btnY + 26.0f * uiScale >= rect.y && btnY <= rect.y + rect.h) &&
                             (mousePos.x >= saveBtnRect.x && mousePos.x <= saveBtnRect.x + saveBtnRect.w &&
                              mousePos.y >= saveBtnRect.y && mousePos.y <= saveBtnRect.y + saveBtnRect.h);

                UIWidget::drawButton(renderer, saveBtnRect, "Save New Preset", sHov, true, false, uiScale * 0.8f);
                if (sHov && clicked)
                {
                    state->savePreset(gameContext, "Preset_" + player->anatomy.getRacialTitle());
                    gameContext->input.consumeMouseClick();
                }

                SDL_FRect rBtnRect = { padX + availableW - btnW - (10.0f * uiScale), btnY, btnW, 26.0f * uiScale };
                bool rHov = inPanel && (btnY + 26.0f * uiScale >= rect.y && btnY <= rect.y + rect.h) &&
                             (mousePos.x >= rBtnRect.x && mousePos.x <= rBtnRect.x + rBtnRect.w &&
                              mousePos.y >= rBtnRect.y && mousePos.y <= rBtnRect.y + rBtnRect.h);
                UIWidget::drawButton(renderer, rBtnRect, "Reset Human", rHov, true, false, uiScale * 0.8f);
                if (rHov && clicked)
                {
                    state->resetToHuman(gameContext);
                    gameContext->input.consumeMouseClick();
                }
                curY += presetsCard.h + (14.0f * uiScale);

                // List saved presets (cached to prevent 60fps disk reads)
                const auto& presets = state->getPresetNames();
                if (!presets.empty())
                {
                    for (const auto& pName : presets)
                    {
                        SDL_FRect pRect = { padX, curY, availableW - (100.0f * uiScale), 26.0f * uiScale };
                        UIWidget::drawPanel(renderer, pRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
                        UIWidget::drawText(renderer, pName, pRect.x + (10.0f * uiScale), curY + (5.0f * uiScale), Theme::colors.textGold, uiScale * 0.82f);

                        // Load button
                        SDL_FRect loadRect = { padX + availableW - (95.0f * uiScale), curY, 45.0f * uiScale, 26.0f * uiScale };
                        bool lHov = inPanel && (curY + 26.0f * uiScale >= rect.y && curY <= rect.y + rect.h) &&
                                     (mousePos.x >= loadRect.x && mousePos.x <= loadRect.x + loadRect.w &&
                                      mousePos.y >= loadRect.y && mousePos.y <= loadRect.y + loadRect.h);
                        UIWidget::drawButton(renderer, loadRect, "Load", lHov, true, false, uiScale * 0.74f);
                        if (lHov && clicked)
                        {
                            state->loadPreset(gameContext, pName);
                            gameContext->input.consumeMouseClick();
                        }

                        // Del button
                        SDL_FRect delRect = { padX + availableW - (45.0f * uiScale), curY, 40.0f * uiScale, 26.0f * uiScale };
                        bool dHov = inPanel && (curY + 26.0f * uiScale >= rect.y && curY <= rect.y + rect.h) &&
                                     (mousePos.x >= delRect.x && mousePos.x <= delRect.x + delRect.w &&
                                      mousePos.y >= delRect.y && mousePos.y <= delRect.y + delRect.h);
                        UIWidget::drawButton(renderer, delRect, "Del", dHov, true, false, uiScale * 0.74f);
                        if (dHov && clicked)
                        {
                            state->deletePreset(pName);
                            gameContext->input.consumeMouseClick();
                        }
                        curY += (32.0f * uiScale);
                    }
                }
                break;
            }
        }

        return curY - startY;
    }
}
