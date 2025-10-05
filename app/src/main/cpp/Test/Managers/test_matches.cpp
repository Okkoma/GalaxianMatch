#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

#include "Matches.h"

TEST_CASE("Match basic properties")
{
    Match m(1, 2, 0);
    REQUIRE(m.x_ == 1);
    REQUIRE(m.y_ == 2);
    REQUIRE(m.ctype_ == 0);
}

TEST_CASE("MatchGrid minimal rules")
{
    MatchGrid grid;
    grid.SetLayout(6, L_Square, HA_LEFT, VA_TOP, false);
    auto size = grid.GetSize();

    REQUIRE(size.x_ == 6);
    REQUIRE(size.y_ == 6);
}

TEST_CASE("Matching rules - AreMatched")
{
    MatchGrid grid;
    grid.SetLayout(5, L_Square, HA_LEFT, VA_TOP, false);

    Match& a = grid.GetMatch(1, 1);
    Match& b = grid.GetMatch(1, 2);
    a.Set(BLUE, -1, NOEFFECT);
    b.Set(BLUE, -1, NOEFFECT);

    REQUIRE(a.x_ == b.x_);
    REQUIRE(b.y_ == a.y_+1);
    REQUIRE(grid.AreMatched(a, b) == true);
}

TEST_CASE("Match permutation - Swap two matches")
{
    MatchGrid grid;
    grid.SetLayout(5, L_Square, HA_LEFT, VA_TOP, false);

    Match& match1 = grid.GetMatch(1, 1);
    Match& match2 = grid.GetMatch(2, 1);
    
    // Configuration initiale
    match1.Set(BLUE, -1, NOEFFECT);
    match2.Set(RED, -1, NOEFFECT);
    
    // Vérification de l'état initial
    REQUIRE(match1.ctype_ == BLUE);
    REQUIRE(match2.ctype_ == RED);
    
    // Permutation des deux matches
    grid.PermuteMatchType(match1, match2);
    
    // Vérification que les types ont été échangés
    REQUIRE(match1.ctype_ == RED);
    REQUIRE(match2.ctype_ == BLUE);
    
    // Test de l'annulation de la permutation
    grid.UndoPermute();
    REQUIRE(match1.ctype_ == BLUE);
    REQUIRE(match2.ctype_ == RED);
    
    // Test de la confirmation de la permutation
    grid.PermuteMatchType(match1, match2);
    grid.ConfirmPermute();
    REQUIRE(match1.ctype_ == RED);
    REQUIRE(match2.ctype_ == BLUE);
}

TEST_CASE("GetMatches - Horizontal three in a row")
{
    MatchGrid grid;
    grid.SetLayout(6, L_Square, HA_LEFT, VA_TOP, false);

    Vector<Match*> newmatches;
    grid.Create(newmatches);            // active les règles de matching
    grid.ClearObjects(false);           // purge les couleurs pour un état déterministe

    // Ligne horizontale de 3 éléments
    grid.GetMatch(1, 2).Set(RED, -1, NOEFFECT);
    grid.GetMatch(2, 2).Set(RED, -1, NOEFFECT);
    grid.GetMatch(3, 2).Set(RED, -1, NOEFFECT);

    Match* entry = &grid.GetMatch(2, 2);
    Vector<Match*> destroymatches, successmatches, activablebonuses, brokenrocks;
    Vector<WallInfo> hittedwalls;

    REQUIRE(grid.GetMatches(entry, destroymatches, successmatches, activablebonuses, brokenrocks, hittedwalls) == true);
    REQUIRE(successmatches.Size() >= 3);
    REQUIRE(destroymatches.Size() >= 3);
}

TEST_CASE("GetMatches - Vertical three in a column")
{
    MatchGrid grid;
    grid.SetLayout(6, L_Square, HA_LEFT, VA_TOP, false);

    Vector<Match*> newmatches;
    grid.Create(newmatches);
    grid.ClearObjects(false);

    // Colonne verticale de 3 éléments
    grid.GetMatch(4, 1).Set(BLUE, -1, NOEFFECT);
    grid.GetMatch(4, 2).Set(BLUE, -1, NOEFFECT);
    grid.GetMatch(4, 3).Set(BLUE, -1, NOEFFECT);

    Match* entry = &grid.GetMatch(4, 2);
    Vector<Match*> destroymatches, successmatches, activablebonuses, brokenrocks;
    Vector<WallInfo> hittedwalls;

    REQUIRE(grid.GetMatches(entry, destroymatches, successmatches, activablebonuses, brokenrocks, hittedwalls) == true);
    REQUIRE(successmatches.Size() >= 3);
    REQUIRE(destroymatches.Size() >= 3);
}

TEST_CASE("GetMatches - Square 2x2")
{
    MatchGrid grid;
    grid.SetLayout(6, L_Square, HA_LEFT, VA_TOP, false);

    Vector<Match*> newmatches;
    grid.Create(newmatches);
    grid.ClearObjects(false);

    // Carré 2x2
    grid.GetMatch(2, 2).Set(GREEN, -1, NOEFFECT);
    grid.GetMatch(3, 2).Set(GREEN, -1, NOEFFECT);
    grid.GetMatch(2, 3).Set(GREEN, -1, NOEFFECT);
    grid.GetMatch(3, 3).Set(GREEN, -1, NOEFFECT);

    Match* entry = &grid.GetMatch(2, 2);
    Vector<Match*> destroymatches, successmatches, activablebonuses, brokenrocks;
    Vector<WallInfo> hittedwalls;

    REQUIRE(grid.GetMatches(entry, destroymatches, successmatches, activablebonuses, brokenrocks, hittedwalls) == true);
    REQUIRE(successmatches.Size() >= 4);
    REQUIRE(destroymatches.Size() >= 4);
}

TEST_CASE("GetMatches - L shape (3 + 2 avec angle commun)")
{
    MatchGrid grid;
    grid.SetLayout(7, L_Square, HA_LEFT, VA_TOP, false);

    Vector<Match*> newmatches;
    grid.Create(newmatches);
    grid.ClearObjects(false);

    // Forme en L avec angle commun en (2,2)
    grid.GetMatch(2, 2).Set(PURPLE, -1, NOEFFECT);
    grid.GetMatch(3, 2).Set(PURPLE, -1, NOEFFECT);
    grid.GetMatch(4, 2).Set(PURPLE, -1, NOEFFECT);
    grid.GetMatch(2, 3).Set(PURPLE, -1, NOEFFECT);

    Match* entry = &grid.GetMatch(2, 2);
    Vector<Match*> destroymatches, successmatches, activablebonuses, brokenrocks;
    Vector<WallInfo> hittedwalls;

    REQUIRE(grid.GetMatches(entry, destroymatches, successmatches, activablebonuses, brokenrocks, hittedwalls) == true);
    REQUIRE(successmatches.Size() >= 4);
    REQUIRE(destroymatches.Size() >= 4);
}


TEST_CASE("GetHints - L shape")
{
    MatchGrid grid;
    grid.SetLayout(7, L_Square, HA_LEFT, VA_TOP, false);
    grid.SetDefaultActivedRules(false);
    grid.SetActivedRule(LMATCH);

    Vector<Match*> newmatches;
    grid.Create(newmatches);
    grid.ClearObjects(false);

    // L horizontal bas-droite
    // X Y Z .
    // . . v W
    grid.GetMatch(2, 2).Set(PURPLE, -1, NOEFFECT); // X
    grid.GetMatch(3, 2).Set(PURPLE, -1, NOEFFECT); // Y
    grid.GetMatch(4, 2).Set(PURPLE, -1, NOEFFECT); // Z
    grid.GetMatch(4, 3).Set(RED, -1, NOEFFECT); // v
    grid.GetMatch(5, 3).Set(PURPLE, -1, NOEFFECT); // W

    Vector<Vector<Match*> > hintstable;
    grid.GetHints(grid.GetIndex(2,2), hintstable);

    REQUIRE(hintstable.Size() == 1);
}