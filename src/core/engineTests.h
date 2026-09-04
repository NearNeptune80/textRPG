#pragma once

#include <string>

namespace EngineTests
{
    /**
     * Executes the complete autonomous engine regression test suite.
     * Returns true if all tests pass, false if any test fails.
     */
    bool runAllTests();

    bool testStateNavigation();
    bool testCharacterCreation();
    bool testSaveLoadRoundtrip();
    bool testClothingDisplacement();
    bool testSettingsAndThemes();
    bool testSubmenuButtonFunctionality();
    bool testContentOptionsAllCategories();
    bool testPlayerStatsAndItemUsage();
    bool testDecouplingAndCaching();
    bool testQuestJournalSystem();
}
