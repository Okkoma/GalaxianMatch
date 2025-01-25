#pragma once

#include <Urho3D/Scene/Node.h>
#include <Urho3D/UI/UIElement.h>

namespace Urho3D
{
    class Node;
}

using namespace Urho3D;

#include "MemoryObjects.h"
#include "GameRand.h"

class MatchesManager;

#define DEFAULT_MINIMALMATCHES 3
#define DEFAULT_MAXDIMENSION 12
#define DEFAULT_MINDIMENSION 4
#define DEFAULT_BONUSCHANCE  1
#define DEFAULT_BONUSMINIMALSUCCESS 3
#define DEFAULT_BONUSMINIMALMATCHES 4
#define BOSS_BONUSMINIMALMATCHES 3
#define DEFAULT_BONUSGAINMOVE 4
#define DEFAULT_ROCKSCHANCE 5

//#ifdef DEBUG
//#define DEBUGSESSION
//#endif

#ifndef DEBUGSESSION
static const float SCALETIME = 0.7f;
static const float MOVETIME = 0.20f;
static const float PAUSETIME = 0.20f;
static const float MAXMOVETIME = 0.7f;
static const float EXPLOSIONDELAY = 0.20f;
static const float ALPHATIME = 0.5f;
static const float ROTATETIME = 0.4f;
static const float SUCCESSDELAY = 0.7f;
static const float OBJECTIVESUCCESSDELAY = 1.5f;
static const float TOUCHDELAY = 0.5f;
static const float ITEMDELAY = 1.f;
static const float HINTSWITCH = 4.f;
static const float HINTTIME = 3.f;
static const float HINTANGLE = 5.f;
static const unsigned UPDATERAMPSDELAY = 1700U;  // 1.7 seconds
#else
static const float SCALETIME = 0.7f;
static const float MOVETIME = 1.5f;
static const float PAUSETIME = 1.0f;
static const float MAXMOVETIME = 1.5f;
static const float EXPLOSIONDELAY = 0.5f;
static const float ALPHATIME = 1.f;
static const float ROTATETIME = 1.f;
static const float SUCCESSDELAY = 1.f;
static const float OBJECTIVESUCCESSDELAY = 2.f;
static const float TOUCHDELAY = 0.5f;
static const float ITEMDELAY = 2.f;
static const float HINTSWITCH = 4.f;
static const float HINTTIME = 3.f;
static const float HINTANGLE = 5.f;
static const unsigned UPDATERAMPSDELAY = 2400U;
#endif

const float halfTileSize    = 34.f * PIXEL_SIZE;
const Vector3 OBJECTSCALE   = Vector3(0.45f, 0.45f, 1.f);
const Vector3 ITEMSCALE     = Vector3(0.45f, 0.45f, 1.f);

const float HINTSCALEFACTOR = 1.2f;
const float BONUSFACTOR     = 1.1f;
const float ITEMFACTOR      = 4.f;
const float OBJECTIVESCALEFACTOR = 3.f;
const float OBJECTSELECTED  = 1.2f;

static const int BACKGROUND         = 0;
static const int BACKBOSSLAYER      = 25;
static const int BACKGRIDLAYER      = 50;
static const int GRIDLAYER          = 100;
static const int OBJECTIVELAYER     = 150;
static const int OBJECTLAYER        = 200;
static const int MOVELAYER          = 300;
static const int SELECTLAYER        = 400;
static const int BOSSLAYER          = 500;
static const int EFFECTLAYER        = 600;
static const int ITEMLAYER          = 800;
static const int FRONTLAYER         = 1000;
static const int ABILITYLAYER       = 1100;
static const int TUTORIALLAYER      = 1200;

enum GridGroundType
{
    GT_VOID   = 0,
    GT_GRND,
    GT_GRND_FIXED,
    GT_GRND_2LE,
    GT_GRND_2RI,
    GT_GRND_2TO,
    GT_GRND_2BO,
};

enum GroundDirection
{
    GD_None = 0,
    GD_TOLEFT,
    GD_TORIGHT,
    GD_TOTOP,
    GD_TOBOTTOM,
};

enum WallOrientation
{
    WO_NOWALL    = 0,
    WO_WALLNORTH = 1 << 0, // 1
    WO_WALLLEFT  = 1 << 1, // 2
    WO_WALLRIGHT = 1 << 2, // 4
    WO_WALLSOUTH = 1 << 3, // 8
    WO_WALLNL    = WO_WALLNORTH | WO_WALLLEFT, // 3
    WO_WALLNR    = WO_WALLNORTH | WO_WALLRIGHT, // 5
    WO_WALLNS    = WO_WALLNORTH | WO_WALLSOUTH, // 9
    WO_WALLSL    = WO_WALLSOUTH | WO_WALLLEFT, // 10
    WO_WALLSR    = WO_WALLSOUTH | WO_WALLRIGHT, // 12
    WO_WALLLR    = WO_WALLLEFT | WO_WALLRIGHT, // 6
    WO_WALLNLR   = WO_WALLNORTH | WO_WALLLEFT | WO_WALLRIGHT, // 7
    WO_WALLNSL   = WO_WALLNORTH | WO_WALLSOUTH | WO_WALLLEFT, // 11
    WO_WALLNSR   = WO_WALLNORTH | WO_WALLSOUTH | WO_WALLRIGHT, // 13
    WO_WALLSLR   = WO_WALLSOUTH | WO_WALLLEFT | WO_WALLRIGHT, // 14
    WO_WALLALL   = WO_WALLNORTH | WO_WALLLEFT | WO_WALLRIGHT | WO_WALLSOUTH // 15
};

enum GridLayout
{
    L_Square = 0,
    L_Plus,
    L_HRect,
    L_VRect,
    L_LTop,
    L_LBottom,
    L_TTop,
    L_TBottom,
    L_UTop,
    L_UBottom,
    L_MAXSTANDARDLAYOUT,
    L_Boss01,
    L_Boss02,
    L_Boss03,
    L_Boss04,
    L_Boss05,
    L_Custom
};

struct GridTile
{
    GridTile(unsigned char gr=0, unsigned char wt=0, unsigned char wo=0) : ground_(gr), walltype_(wt), wallorientation_(wo) { }
    GridTile(const GridTile& gt) : ground_(gt.ground_), walltype_(gt.walltype_), wallorientation_(gt.wallorientation_) { }
    GridTile& operator =(const GridTile& rhs)
    {
        ground_ = rhs.ground_;
        walltype_ = rhs.walltype_;
        wallorientation_ = rhs.wallorientation_;
        return *this;
    }

    unsigned char ground_;
    unsigned char walltype_;
    unsigned char wallorientation_;
    unsigned char empty_;

    static const GridTile EMPTY;
    static int WALLCHANCE;
};

struct WallInfo
{
    WallInfo(unsigned char x=0, unsigned char y=0, unsigned char wo=0) : x_(x), y_(y), wallorientation_(wo) { }
    unsigned char x_;
    unsigned char y_;
    unsigned char wallorientation_;
};

enum ColorType
{
    NOTYPE = 0,
    BLUE,
    RED,
    GREEN,
    PURPLE,
    BLACK,
    YELLOW,
    ITEMS,
    MOVES,
    STARS,
    COINS,
    ROCKS,
};

#define NUMCOLORTYPES 8

enum EffectType
{
	NOEFFECT = 0,
	XEXPLOSION = 1,
	YEXPLOSION = 2,
	XYEXPLOSION = 3,
	SQREXPLOSION = 4,
	SQREXPLOSION2 = 5,
	SQREXPLOSION3 = 6,
	WALLBREAKER = 7,
	ELECEXPLOSION = 8,    // 1000
	ELECEXPLOSION2 = 9,   // 1001
	ELECEXPLOSION3 = 10,  // 1010
	ELECEXPLOSION4 = 11,  // 1011
	ROCKEXPLOSION = 12,   // 1100
	ROCKEXPLOSION2 = 13,  // 1101
	ROCKEXPLOSION3 = 14,  // 1101
	ROCKEXPLOSION4 = 15   // 1111
};

const IntVector2 DIRECTIONALEXPLOSION(XEXPLOSION, XYEXPLOSION);
const IntVector2 SQUAREEXPLOSION(SQREXPLOSION, SQREXPLOSION3);
const IntVector2 ELECBOMB(ELECEXPLOSION, ELECEXPLOSION4);
const IntVector2 ROCKBOMB(ROCKEXPLOSION, ROCKEXPLOSION3);
const IntVector2 SPREADBOMB(ELECEXPLOSION, ROCKEXPLOSION4);
const IntVector2 WALLBOMB(WALLBREAKER, WALLBREAKER);

enum MatchRules
{
    HORIZONTALMATCH = 0,
    VERTICALMATCH,
    SQUAREMATCH,
    LMATCH,
    BOMBMATCH,
    SPREADBOMBMATCH,
	ROCKBOMBMATCH,
	WALLBOMBMATCH,
    NUMMATCHRULES
};

union MatchProperty
{
    struct
    {
        unsigned property_;
    };
    struct
    {
        unsigned char ctype_;    // color type
        signed char otype_;      // game objective index OR game object index in category (for POWER or ITEM)
        unsigned char effect_;   // game effect (for POWER)
        unsigned char qty_;      // item qty
    };
};

class Match
{
public :
    Match() : property_(0) { }

    Match(unsigned char x, unsigned char y, unsigned property) : property_(property), x_(x), y_(y)  { }
    Match(const Match& rhs) : property_(rhs.property_), x_(rhs.x_), y_(rhs.y_) { }

    Match& operator =(const Match& rhs)
    {
        property_ = rhs.property_;
        x_ = rhs.x_;
        y_ = rhs.y_;

        return *this;
    }
    bool operator ==(const Match& rhs) const { return x_ == rhs.x_ && y_ == rhs.y_; }
    bool operator !=(const Match& rhs) const { return x_ != rhs.x_ || y_ != rhs.y_; }

    union
    {
        struct
        {
            unsigned property_;
        };
        struct
        {
            unsigned char ctype_;    // color type
            signed char otype_;      // game objective index OR game object index in category (for POWER or ITEM)
            unsigned char effect_;   // game effect
            unsigned char qty_;      // item qty
        };
    };

    unsigned char x_;
    unsigned char y_;
    unsigned char extra_[2];

    String ToString() const;

    /// Helpers
    static void AddDistinctEntry(Match* entry, Vector<Match*>& output);
    static void AddDistinctEntries(const Vector<Match*>& entries, Vector<Match*>& output);
    static void DumpMatches(Vector<Match*>& matches);

    static const Match EMPTY;

    static int MINIMALMATCHES;
    static int MAXDIMENSION;
    static int MINDIMENSION;
    static int BONUSCHANCE;
    static int BONUSMINIMALSUCCESS;
    static int BONUSMINIMALMATCHES;
    static int BONUSGAINMOVE;
    static int ROCKCHANCE;
};

struct TileEntrance
{
    IntVector2 position_;
    IntVector2 direction_;
};

class MatchGrid
{
public :
    MatchGrid();
    ~MatchGrid() { }

    /// taille des bricks
    static void SetGridUnit(float size) { gridunit_ = size; }
    static void SetAuthorizedTypes(const StringHash& category, const Vector<StringHash>& types);

    void SetId(int id) { gridid_ = id; }
    void ClearGrid();
    void ClearObjects(bool clearmatches=true);
    void ClearAll();
    void SetPhysicsEnable(bool enable);

    /// forme de la grille : en "L" .. en carré ..
    /// ajout des blockers
    void SetLayout(int dimension, GridLayout layout, HorizontalAlignment halign, VerticalAlignment valign, bool randomwalls);
    void SetGridRect(const Rect& rect) { gridRect_ = rect; }
    void SetGridColor(const Color& color) { gridColor_ = color; }
    void Create(Vector<Match*>& newmatches);

    void Load(VectorBuffer& buffer);
    void Save(VectorBuffer& buffer);

    int CollapseColumn(int column, Vector<Match*>& collapsematches);
    int AddColumn(int column, Vector<Match*>& newmatches);
    Node* GetPreviewMatch(Match& match, GameRand& random);
    Node* AddPreviewObject(Match& match, GameRand& random);
    Node* AddObject(Match& match, GameRand& random);
    void AddObject(StringHash got, const Match& match, WeakPtr<Node>& object, bool previewobject=false);
    Node* AddPowerBonus(Match* m, int bonusindex=-1);
    Node* AddItemBonus(Match& m, const StringHash& category, int bonusindex=-1);

    void AddSuccessEffect(const Match& match);
    void AddObjectiveEffect(const Match& match);
    void AddTouchEffect(const Vector2& position, float scale=1.f, float delay=TOUCHDELAY, const Color& color=Color::BLUE);
    void AddEffectInNode(Node* node, int effect, float scale, float duration, const Color& color);
    void AddPowerEffects(const Match& match);
    void AddDirectionalTileRamp(unsigned char type, const IntVector2& start, const IntVector2& end, bool entrance, bool exit);

    void SetDrawOrder(const Match& match, Node* node, int baselayer, bool setcolortype=false);
    bool SetSelectAnimation(const Match& match, int orderInLayer);
    bool SetMoveAnimation(const Match& match, Node* node, const Vector3& newposition, float time=MOVETIME);
    bool SetScaleAnimation(Node* node, float factor, float scaletime=SCALETIME, WrapMode loop=WM_LOOP);
    bool SetAlphaAnimation(Node* node);
    bool SetAlphaAnimationNew(Node* node);
    bool SetExplosionAnimation(Node* explosion);
    bool SetEffectAnimation(Node* node);
    bool SetHintAnimation(const Match& match, bool enable);
    void SetHittedWalls(Vector<WallInfo>& hittedwalls);

    void SetMatch(Match& match, unsigned property);
    void SetMatch(Match& match, unsigned char ctype, char otype, unsigned char effect);

    void ResetPosition(const Match& match);
    void ResetScale(const Match& match);
    void Remove(Match& match);
    void Remove(const Match& match);
    void RemoveObject(Node* node);
    void PermuteMatchType(const Match& m1, const Match& m2);
    void UndoPermute();
    void ConfirmPermute();

    void UpdateDirectionalTiles();

    /// Getters
    IntVector2 GetSize() const { return IntVector2(width_, height_); }
    bool ContainsItems() const;
    Vector<const Match*> GetAllItems() const;
    Vector<Match*> GetAllPowers();

    bool IsInside(const Vector2& position) const;
    bool AreMatched(const Match& m1, const Match& m2) const;

    bool IsPower(const Match& match, const IntVector2& effect, int mask=M_MAX_INT) const;
    bool IsPower(const Match& match) const;
    bool IsPowerActivable(Match& match);
    bool IsItem(const Match& match) const;

    bool IsSelectableObject(const Match& match) const;
    bool IsDirectional(unsigned char feature) const;
    bool IsDirectionalTile(const Match& match) const;
    bool HasTileEntrances() const;
    bool HasTileExits() const;

    Vector<Match*> GetPermuttedMatches();

    void GetActivableBonuses(Vector<Match*>& matches, Vector<Match*>& activablebonuses);
    bool GetMatches(Match* entry, Vector<Match*>& destroymatches, Vector<Match*>& successmatches, Vector<Match*>& activablebonuses, Vector<Match*>& brokenrocks, Vector<WallInfo>& hittedwalls);
    void GetHints(unsigned imatch, Vector<Vector<Match*> >& hintstable);
    Match* GetMatch(Node* node);
    Node* GetObject(const Match& m) const;
    Node* GetObject(int x, int y) const;
    Node* GetGridNode() const { return gridNode_; }

    void GetAllHorizontalMatches(const Match& entry, Vector<Match*>& matches, Vector<Match*>& brokenrocks, Vector<WallInfo>* hittedwalls=0);
    void GetAllVerticalMatches(const Match& entry, Vector<Match*>& matches, Vector<Match*>& brokenrocks, Vector<WallInfo>* hittedwalls=0);
	void GetAllWallsAround(const Match& entry, int range, Vector<WallInfo>& hittedwalls);

    Vector3 CalculateMatchPosition(int x, int y) const;

private:
    friend class MatchesManager;
    friend class MatchGridInfo;

    void CleanSavedMatches();

    void RandomizeWalls();

    void InitializeTypes();
    void InitializeTiles();
    void InitializeWalls();
    void InitializeObjects(Vector<Match*>& newmatches);

    void SetObjects(const PODVector<StringHash>& gots, const PODVector<StringHash>& previewgots);

    Vector3 GetEntrance(const Match& match) const;

	bool HaveSameTypeAndNoWalls(unsigned char ctype, const Match& m1, const Match& m2) const;
    bool HaveSameMatchType(const Match& m1, const Match& m2) const;
    bool HaveSameMatchType(const Match& m1, const Match& m2, const Match& m3, const Match& m4) const;
    bool HasGround(const Match& m) const;
    bool HasWalls(const Match& m, int direction) const;
    bool HasNoWalls(const Match& m, int direction) const;
    bool HaveNoAdjacentWalls(const Match& m1, const Match& m2) const;

    bool GetBonusesInMatches(const Vector<Match*>& matches, Vector<Match*>& bonuses, const IntVector2& effect, int mask=M_MAX_INT);
    Match* GetFirstBonusInMatches(const Vector<Match*>& matches, const IntVector2& effect, int mask=M_MAX_INT);

    void CheckMatches_Horizontal(const Match& entry, Vector<Match*>& matches);
    void CheckMatches_Vertical(const Match& entry, Vector<Match*>& matches);
    void CheckMatches_Horizontal_Bonus(const Match& entry, Vector<Match*>& matches);
    void CheckMatches_Vertical_Bonus(const Match& entry, Vector<Match*>& matches);
    void CheckMatches_Squares(const Match& entry, Vector<Match*>& matches);
    void CheckMatches_L(const Match& entry, Vector<Match*>& matches);
    void CheckMatches_NeighborHood(const Match& entry, Vector<Match*>& matches);

    void CheckHints_Explosion(Match& X, Vector<Vector<Match*> >& hints);
    void CheckHints_Horizontal(Match& X, Vector<Vector<Match*> >& hints);
    void CheckHints_Vertical(Match& X, Vector<Vector<Match*> >& hints);
    void CheckHints_Square(Match& X, Vector<Vector<Match*> >& hints);

    /// Grid Infos
    int gridid_;
    unsigned char dimension_, width_, height_;
    unsigned size_;
    GridLayout layout_;
    Rect gridRect_;
    Color gridColor_;
    unsigned char previewLines_;

    /// Grounds and Walls
    Matrix2D<GridTile> grid_;
    Vector<TileEntrance> tileentrances_;
    Vector<IntVector2> tileexits_;
    Vector<Vector<IntVector2> > ramptiles_;
    Vector<unsigned char> fixedcolumns_;
    WeakPtr<Node> gridNode_;
    Matrix2D<WeakPtr<Node> > tiles_;
    Matrix2D<WeakPtr<Node> > walls_;

    /// Objects
    WeakPtr<Node> objectsNode_;
    Matrix2D<Match> matches_;
    Matrix2D<WeakPtr<Node> > objects_;
    Matrix2D<Match> previewmatches_;
    Matrix2D<WeakPtr<Node> > previewobjects_;

    /// Temporary Saved Matches
    Match savedM1_, savedM2_;
    bool permutation_;

    /// Matches Options
    bool freeDirectionExplosion_;

    /// Authorized Objects
    static HashMap<StringHash, Vector<StringHash> > authorizedTypes_;
    static Vector<int> authorizedColors_;

    static float gridunit_;
    static int optionSameType_;
    static int optionCheckMatches_;
};




