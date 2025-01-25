#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/Resource.h>
#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Material.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>

#include <Urho3D/Urho2D/Drawable2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/SpriterInstance2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/CollisionShape2D.h>
#include <Urho3D/Urho2D/Renderer2D.h>

#include "TimerRemover.h"
#include "DelayAction.h"

#include "Game.h"
#include "GameAttributes.h"
#include "GameStatics.h"
#include "GameHelpers.h"
#include "GameEvents.h"
#include "GameUI.h"

#include "MAN_Matches.h"
#include "Tutorial.h"

#include "Matches.h"


Color MatchColors[NUMCOLORTYPES] =
{
    Color(1.0f, 1.0f, 1.0f, 0.9f),  // WHITE
    Color(0.0f, 0.0f, 1.0f, 0.9f),  // BLUE
    Color(1.0f, 0.0f, 0.0f, 0.9f),  // RED
    Color(0.0f, 1.0f, 0.0f, 0.9f),  // GREEN
    Color(1.0f, 0.0f, 1.0f, 0.9f),  // PURPLE
    Color(0.0f, 0.0f, 0.0f, 0.9f),  // BLACK
    Color(1.0f, 1.0f, 0.0f, 0.9f),  // YELLOW
	Color(0.5f, 0.3f, 0.3f, 0.9f)	// ROCKS
};

const char* ColorTypeNames[] =
{
    "NOTYPE",
    "BLUE",
    "RED",
    "GREEN",
    "PURPLE",
    "BLACK",
    "YELLOW",
    "ITEMS",
    "MOVES",
    "STARS",
    "COINS",
    "ROCKS",
};

const GridTile GridTile::EMPTY = GridTile();
int GridTile::WALLCHANCE = 0;

bool activedRules_[NUMMATCHRULES];

const Quaternion HINTROTATE_MIN = Quaternion(-30.f);
const Quaternion HINTROTATE_MAX = Quaternion(30.f);

const Match Match::EMPTY = Match(255, 255, 0);

int Match::MINIMALMATCHES       = DEFAULT_MINIMALMATCHES;
int Match::MAXDIMENSION         = DEFAULT_MAXDIMENSION;
int Match::MINDIMENSION         = DEFAULT_MINDIMENSION;
int Match::BONUSCHANCE          = DEFAULT_BONUSCHANCE;
int Match::BONUSMINIMALSUCCESS  = DEFAULT_BONUSMINIMALSUCCESS;
int Match::BONUSMINIMALMATCHES  = DEFAULT_BONUSMINIMALMATCHES;
int Match::BONUSGAINMOVE        = DEFAULT_BONUSGAINMOVE;
int Match::ROCKCHANCE           = DEFAULT_ROCKSCHANCE;


String Match::ToString() const
{
    String s;
    return s.AppendWithFormat("%u %u color=%s(%d) effect=%u", x_, y_, ColorTypeNames[ctype_], ctype_, effect_);
}

/// Helpers

// Push Distinct Match Only (like a Set)
void Match::AddDistinctEntry(Match* entry, Vector<Match*>& output)
{
    if (!output.Contains(entry))
        output.Push(entry);
}

// Push Distinct Match Only (like a Set)
void Match::AddDistinctEntries(const Vector<Match*>& entries, Vector<Match*>& output)
{
    for (Vector<Match*>::ConstIterator it=entries.Begin(); it!=entries.End(); ++it)
        if (!output.Contains(*it))
            output.Push(*it);
}

void Match::DumpMatches(Vector<Match*>& matches)
{
    String s;
    for (Vector<Match*>::ConstIterator it=matches.Begin(); it!=matches.End(); ++it)
        s += String((void*)(*it)) + " - ";

    URHO3D_LOGINFOF("Match() - DumpMatches : Num=%u Matches=%s", matches.Size(), s.CString());
}


int MatchGrid::optionSameType_     = 0;
int MatchGrid::optionCheckMatches_ = 0;
HashMap<StringHash, Vector<StringHash> > MatchGrid::authorizedTypes_;
Vector<int> MatchGrid::authorizedColors_;


MatchGrid::MatchGrid()
{
    previewLines_ = 1;

    MatchGrid::authorizedTypes_.Clear();
    MatchGrid::authorizedColors_.Clear();

    freeDirectionExplosion_ = true;
}

void MatchGrid::ClearGrid()
{
    if (gridNode_)
        gridNode_->Remove();

    gridNode_.Reset();
    tileentrances_.Clear();
    tileexits_.Clear();
    fixedcolumns_.Clear();
    ramptiles_.Clear();
    tiles_.Clear();
    walls_.Clear();

    URHO3D_LOGINFO("MatchGrid() - ClearGrid : Tiles and Walls have been removed !");
}

void MatchGrid::ClearObjects(bool clearmatches)
{
    objects_.Clear();
    previewobjects_.Clear();

    if (objectsNode_)
        objectsNode_->Remove();

    if (clearmatches)
    {
        matches_.Clear();
        previewmatches_.Clear();
    }
    else
    {
        // purge color only
        for (unsigned i=0; i < matches_.Size(); i++)
            matches_[i].ctype_ = 0;
        for (unsigned i=0; i < previewmatches_.Size(); i++)
            previewmatches_[i].ctype_ = 0;
    }

    CleanSavedMatches();

    URHO3D_LOGINFO("MatchGrid() - ClearObjects : Objects and Rocks have been removed !");
}

void MatchGrid::ClearAll()
{
    URHO3D_LOGINFO("MatchGrid() - ClearAll ...");

    GameStatics::switchScaleXY_ = false;

    ClearGrid();
    ClearObjects();

    URHO3D_LOGINFO("MatchGrid() - ClearAll ... OK !");
}



void MatchGrid::SetPhysicsEnable(bool enable)
{
    if (!tiles_.Size())
        return;

    for (unsigned y=0; y < height_; y++)
    for (unsigned x=0; x < width_ ; x++)
    {
        WeakPtr<Node>& tile = tiles_(x,y);
        if (tile)
        {
            tile->GetDerivedComponent<CollisionShape2D>()->SetEnabled(enable);
            tile->GetComponent<RigidBody2D>()->SetEnabled(enable);
        }
    }
}


void MatchGrid::SetLayout(int dimension, GridLayout layout, HorizontalAlignment halign, VerticalAlignment valign, bool randomwalls)
{
    width_ = height_ = 0;

    dimension_ = Clamp(dimension, Match::MINDIMENSION, Match::MAXDIMENSION);

    layout_ = layout;

    int thickness = 3;

    switch (layout_)
    {
    case L_Square:
        width_ = height_ = dimension_;
        grid_.Resize(width_, height_);
        grid_.SetBuffer(GridTile(GT_GRND));
        URHO3D_LOGINFOF("MatchGrid() - SetLayout : Grid in <L_Square> shape : dim=%u", width_);
        break;
    case L_Plus:
        width_ = height_ = dimension_;
        grid_.Resize(width_, height_);
        grid_.SetBufferValue(GT_VOID);
        for (int y = thickness; y < Min(2*thickness, height_); y++)
            for (int x = 0; x < width_; x++)
                grid_(x,y).ground_ = GT_GRND;
        for (int x = thickness; x < Min(2*thickness, width_); x++)
            for (int y = 0; y < height_; y++)
                grid_(x,y).ground_ = GT_GRND;
        URHO3D_LOGINFOF("MatchGrid() - SetLayout : Grid in <L_Plus> shape : dim=%ux%u thickness=%d", width_, height_, thickness);
        break;
    case L_HRect:
        width_ = dimension_;
        height_ = Match::MINDIMENSION+1;
        grid_.Resize(width_, height_);
        grid_.SetBuffer(GridTile(GT_GRND));
        URHO3D_LOGINFOF("MatchGrid() - SetLayout : Grid in <L_HRect> shape : dim=%ux%u", width_, height_);
        break;
    case L_VRect:
        height_ = dimension_;
        width_ = Match::MINDIMENSION+1;
        grid_.Resize(width_, height_);
        grid_.SetBuffer(GridTile(GT_GRND));
        URHO3D_LOGINFOF("MatchGrid() - SetLayout : Grid in <L_VRect> shape : dim=%ux%u", width_, height_);
        break;
    case L_LTop:
        height_ = dimension_;
        width_ = Match::MINDIMENSION+1;
        grid_.Resize(width_, height_);
        grid_.SetBufferValue(GT_VOID);
        for (int y = 0; y < height_; y++)
            for (int x = 0; x < thickness; x++)
                grid_(x,y).ground_ = GT_GRND;
        for (int x = thickness; x < width_; x++)
            for (int y = height_-thickness; y < height_; y++)
                grid_(x,y).ground_ = GT_GRND;
        URHO3D_LOGINFOF("MatchGrid() - SetLayout : Grid in <L_LTop> shape : dim=%ux%u", width_, height_);
        break;
    case L_LBottom:
        width_ = dimension_;
        height_ = Match::MINDIMENSION+1;
        grid_.Resize(width_, height_);
        grid_.SetBufferValue(GT_VOID);
        for (int y = 0; y < height_; y++)
            for (int x = width_-thickness; x < width_; x++)
                grid_(x,y).ground_ = GT_GRND;
        for (int x = 0; x < width_-thickness; x++)
            for (int y = height_-thickness; y < height_; y++)
                grid_(x,y).ground_ = GT_GRND;
        URHO3D_LOGINFOF("MatchGrid() - SetLayout : Grid in <L_LBottom> shape : dim=%ux%u", width_, height_);
        break;
    case L_TTop:
        height_ = dimension_;
        width_ = dimension_;
        grid_.Resize(width_, height_);
        grid_.SetBufferValue(GT_VOID);
        for (int y = 0; y < thickness; y++)
            for (int x = 0; x < width_; x++)
                grid_(x,y).ground_ = GT_GRND;
        for (int x = thickness; x < Min(2*thickness, width_); x++)
            for (int y = thickness; y < height_; y++)
                grid_(x,y).ground_ = GT_GRND;
        URHO3D_LOGINFOF("MatchGrid() - SetLayout : Grid in <L_TTop> shape : dim=%ux%u", width_, height_);
        break;
    case L_TBottom:
        width_ = dimension_;
        height_ = dimension_;
        grid_.Resize(width_, height_);
        grid_.SetBufferValue(GT_VOID);
        for (int y = height_-thickness; y < height_; y++)
            for (int x = 0; x < width_; x++)
                grid_(x,y).ground_ = GT_GRND;
        for (int x = thickness; x < Min(2*thickness, width_); x++)
            for (int y = 0; y < height_-thickness; y++)
                grid_(x,y).ground_ = GT_GRND;
        URHO3D_LOGINFOF("MatchGrid() - SetLayout : Grid in <L_TBottom> shape : dim=%ux%u", width_, height_);
        break;
    case L_UTop:
        height_ = dimension_;
        width_ = dimension_;
        grid_.Resize(width_, height_);
        grid_.SetBufferValue(GT_VOID);
        for (int y = 0; y < height_; y++)
        {
            for (int x = 0; x < thickness; x++)
                grid_(x,y).ground_ = GT_GRND;
            for (int x = width_-thickness; x < width_; x++)
                grid_(x,y).ground_ = GT_GRND;
        }
        for (int y = 0; y < thickness; y++)
        {
            for (int x = thickness; x < width_-thickness; x++)
                grid_(x,y).ground_ = GT_GRND;
        }
        URHO3D_LOGINFOF("MatchGrid() - SetLayout : Grid in <L_UTop> shape : dim=%ux%u", width_, height_);
        break;
    case L_UBottom:
        width_ = dimension_;
        height_ = dimension_;
        grid_.Resize(width_, height_);
        grid_.SetBufferValue(GT_VOID);
        for (int y = 0; y < height_; y++)
        {
            for (int x = 0; x < thickness; x++)
                grid_(x,y).ground_ = GT_GRND;
            for (int x = width_-thickness; x < width_; x++)
                grid_(x,y).ground_ = GT_GRND;
        }
        for (int y = height_-thickness; y < height_; y++)
        {
            for (int x = thickness; x < width_-thickness; x++)
                grid_(x,y).ground_ = GT_GRND;
        }
        URHO3D_LOGINFOF("MatchGrid() - SetLayout : Grid in <L_UBottom> shape : dim=%ux%u", width_, height_);
        break;
    case L_Boss01:
        width_ = 9;
        height_ = 9;
        grid_.Resize(width_, height_);

        URHO3D_LOGINFOF("MatchGrid() - SetLayout : Grid in <L_Boss01> shape : dim=%u", width_);

        AddDirectionalTileRamp(GT_GRND_2TO, IntVector2(4, height_-1), IntVector2(4, 6), false, false);
        AddDirectionalTileRamp(GT_GRND_2RI, IntVector2(1, 8), IntVector2(3, 8), false, false);
        AddDirectionalTileRamp(GT_GRND_2LE, IntVector2(7, 8), IntVector2(5, 8), false, false);
        AddDirectionalTileRamp(GT_GRND_2RI, IntVector2(0, 5), IntVector2(width_-1, 5), true, true);

        fixedcolumns_.Push(1);
        fixedcolumns_.Push(7);
        for (int y = 6; y < height_-1; y++)
        {
            grid_(1, y).ground_ = GT_GRND;       // keep gravity
            grid_(7, y).ground_ = GT_GRND;       // keep gravity
        }
        break;
    case L_Boss02:
        width_ = 8;
        height_ = 10;
        grid_.Resize(width_, height_);

        AddDirectionalTileRamp(GT_GRND_2BO, IntVector2(3, 0), IntVector2(3, 0), true, false);
        AddDirectionalTileRamp(GT_GRND_2LE, IntVector2(3, 1), IntVector2(1, 1), false, false);
        AddDirectionalTileRamp(GT_GRND_2BO, IntVector2(0, 1), IntVector2(0, 7), false, false);
        AddDirectionalTileRamp(GT_GRND_2RI, IntVector2(0, 8), IntVector2(2, 8), false, false);
        AddDirectionalTileRamp(GT_GRND_2BO, IntVector2(3, 8), IntVector2(3, 9), false, true);

        AddDirectionalTileRamp(GT_GRND_2BO, IntVector2(4, 0), IntVector2(4, 0), true, false);
        AddDirectionalTileRamp(GT_GRND_2RI, IntVector2(4, 1), IntVector2(6, 1), false, false);
        AddDirectionalTileRamp(GT_GRND_2BO, IntVector2(7, 1), IntVector2(7, 7), false, false);
        AddDirectionalTileRamp(GT_GRND_2LE, IntVector2(7, 8), IntVector2(5, 8), false, false);
        AddDirectionalTileRamp(GT_GRND_2BO, IntVector2(4, 8), IntVector2(4, 9), false, true);

        grid_(1,2).ground_ = GT_GRND_FIXED; // store tile
        grid_(6,2).ground_ = GT_GRND_FIXED; // store tile
        grid_(1,7).ground_ = GT_GRND_FIXED; // store tile
        grid_(6,7).ground_ = GT_GRND_FIXED; // store tile

        URHO3D_LOGINFOF("MatchGrid() - SetLayout : Grid in <L_Boss02> shape : dim=%u", width_);
        break;
    case L_Boss03:
        width_ = 10;
        height_ = 10;
        grid_.Resize(width_, height_);
        grid_.SetBuffer(GridTile(GT_GRND));

        for (int y = thickness; y < Min(2*thickness+1, height_); y++)
        for (int x = thickness; x < Min(2*thickness+1, width_); x++)
        {
            grid_(x, y) = GridTile::EMPTY;
        }

        URHO3D_LOGINFOF("MatchGrid() - SetLayout : Grid in <L_Boss03> shape : dim=%u thickness=%d", width_, thickness);
        break;
    case L_Boss04:
        width_ = 9;
        height_ = 9;
        grid_.Resize(width_, height_);

        URHO3D_LOGINFOF("MatchGrid() - SetLayout : Grid in <L_Boss04> shape : dim=%u", width_);

        AddDirectionalTileRamp(GT_GRND_2TO, IntVector2(4, height_-1), IntVector2(4, 6), false, false);
        AddDirectionalTileRamp(GT_GRND_2RI, IntVector2(1, 8), IntVector2(3, 8), false, false);
        AddDirectionalTileRamp(GT_GRND_2LE, IntVector2(7, 8), IntVector2(5, 8), false, false);
        AddDirectionalTileRamp(GT_GRND_2RI, IntVector2(0, 5), IntVector2(width_-1, 5), true, true);

        fixedcolumns_.Push(1);
        fixedcolumns_.Push(7);
        for (int y = 6; y < height_-1; y++)
        {
            grid_(1, y).ground_ = GT_GRND;       // keep gravity
            grid_(7, y).ground_ = GT_GRND;       // keep gravity
        }
        break;
    case L_Boss05:
        width_ = 11;
        height_ = 11;
        grid_.Resize(width_, height_);

        URHO3D_LOGINFOF("MatchGrid() - SetLayout : Grid in <L_Boss05> shape : dim=%u", width_);

        grid_.SetBuffer(GridTile(GT_GRND));
        for (int y = 0; y < 4; y++)
        for (int x = 0; x < width_; x++)
        {
            grid_(x, y) = GridTile::EMPTY;
        }

        break;
    default:
        break;
    }

    size_ = width_*height_;

    if (randomwalls && GridTile::WALLCHANCE && layout < L_MAXSTANDARDLAYOUT)
        RandomizeWalls();
}


void MatchGrid::RandomizeWalls()
{
    // Randomize Walls for Testing
    GameRand& random = GameRand::GetRandomizer(OBJRAND);

    for (int y = 0; y < height_; y++)
    {
        for (int x = 0; x < width_; x++)
        {
            GridTile& tile = grid_(x,y);
            if (tile.ground_ && !tile.walltype_)
            {
                int dr = (GridTile::WALLCHANCE - random.Get(100));
                if (dr > 0)
                {
                    dr %= 4;
                    tile.walltype_ = 1;
                    if (dr == 0)
                        tile.wallorientation_ = WO_WALLALL;
                    else if (dr == 1)
                        tile.wallorientation_ = WO_WALLNORTH + random.Get(4);
                    else if (dr == 2)
                        tile.wallorientation_ = WO_WALLNL + random.Get(6);
                    else
                        tile.wallorientation_ = WO_WALLNLR + random.Get(4);

                    URHO3D_LOGINFOF("MatchGrid() - SetLayout : Wall Orientation = %d !", tile.wallorientation_);
                }
            }
        }
    }
}

void MatchGrid::SetAuthorizedTypes(const StringHash& category, const Vector<StringHash>& types)
{
    MatchGrid::authorizedTypes_[category] = types;
}


void MatchGrid::Load(VectorBuffer& buffer)
{
    if (buffer.GetSize() < 6)
    {
        URHO3D_LOGERRORF("MatchGrid() - Load : ... buffer error !");
        return;
    }

    buffer.Seek(0);
    GridLayout layout = (GridLayout)buffer.ReadInt();
    unsigned char width  = buffer.ReadUByte();
    unsigned char height = buffer.ReadUByte();
    unsigned char dimension = buffer.ReadUByte();
    unsigned char previewLines = buffer.ReadUByte();

    if ((dimension != width && dimension != height) || previewLines > 2)
    {
        URHO3D_LOGERRORF("MatchGrid() - Load : ... load infos w=%u h=%u d=%u previewlines=%u ... buffer error !", width, height, dimension, previewLines);
        return;
    }

    width_ = width;
    height_ = height;
    dimension_ = dimension;
    previewLines_ = previewLines;

    URHO3D_LOGINFOF("MatchGrid() - Load : ... load infos w=%u h=%u ... buffer position = %u", width_, height_, buffer.GetPosition());

    CleanSavedMatches();

    grid_.Clear();
    matches_.Clear();
    previewmatches_.Clear();
    tileentrances_.Clear();
    tileexits_.Clear();
    fixedcolumns_.Clear();
    ramptiles_.Clear();
    tiles_.Clear();
    walls_.Clear();
    objects_.Clear();
    previewobjects_.Clear();

    grid_.Resize(width_, height_);
    matches_.Resize(width_, height_);
    previewmatches_.Resize(width_, previewLines_);
    PODVector<StringHash> gots(width_ * height_);
    PODVector<StringHash> previewgots(width_ * previewLines_);

    buffer.Read(grid_.Buffer(), width_ * height_ * sizeof(GridTile));
    URHO3D_LOGINFOF("MatchGrid() - Load : ... load gridtiles ... buffer position = %u", buffer.GetPosition());
    buffer.Read(matches_.Buffer(), width_ * height_ * sizeof(Match));
    URHO3D_LOGINFOF("MatchGrid() - Load : ... load matches ... buffer position = %u", buffer.GetPosition());
    buffer.Read(previewmatches_.Buffer(), width_ * previewLines_ * sizeof(Match));
    URHO3D_LOGINFOF("MatchGrid() - Load : ... load prevmatches ... buffer position = %u", buffer.GetPosition());
    buffer.Read(gots.Buffer(), gots.Size() * sizeof(StringHash));
    URHO3D_LOGINFOF("MatchGrid() - Load : ... load objects ... buffer position = %u", buffer.GetPosition());
    buffer.Read(previewgots.Buffer(), previewgots.Size() * sizeof(StringHash));
    URHO3D_LOGINFOF("MatchGrid() - Load : ... load prevobjects ... buffer position = %u", buffer.GetPosition());

    if (gridNode_)
        gridNode_->RemoveAllChildren();
    if (objectsNode_)
        objectsNode_->RemoveAllChildren();

    InitializeTiles();
    InitializeWalls();
    SetObjects(gots, previewgots);
}

void MatchGrid::Save(VectorBuffer& buffer)
{
    PODVector<StringHash> gots(objects_.Size());
    for (unsigned i=0; i < objects_.Size(); i++)
        gots[i] = objects_[i] ? objects_[i]->GetVar(GOA::GOT).GetStringHash() : StringHash::ZERO;

    PODVector<StringHash> previewgots(previewobjects_.Size());
    for (unsigned i=0; i < previewobjects_.Size(); i++)
        previewgots[i] = previewobjects_[i] ? previewobjects_[i]->GetVar(GOA::GOT).GetStringHash() : StringHash::ZERO;

    buffer.WriteInt(layout_);
    buffer.WriteUByte(width_);
    buffer.WriteUByte(height_);
    buffer.WriteUByte(dimension_);
    buffer.WriteUByte(previewLines_);
    URHO3D_LOGINFOF("MatchGrid() - Save : ... add infos w=%u h=%u ... buffer size = %u", width_, height_, buffer.GetSize());
    buffer.Write(grid_.Buffer(), grid_.Size() * sizeof(GridTile));
    URHO3D_LOGINFOF("MatchGrid() - Save : ... add gridtiles ... buffer size = %u (sizeof(GridTile)=%u)", buffer.GetSize(), sizeof(GridTile));
    buffer.Write(matches_.Buffer(), matches_.Size() * sizeof(Match));
    URHO3D_LOGINFOF("MatchGrid() - Save : ... add matches ... buffer size = %u (sizeof(Match)=%u)", buffer.GetSize(), sizeof(Match));
    buffer.Write(previewmatches_.Buffer(), previewmatches_.Size() * sizeof(Match));
    URHO3D_LOGINFOF("MatchGrid() - Save : ... add prevmatches ... buffer size = %u", buffer.GetSize());
    buffer.Write(gots.Buffer(), gots.Size() * sizeof(StringHash));
    URHO3D_LOGINFOF("MatchGrid() - Save : ... add objects ... buffer size = %u", buffer.GetSize());
    buffer.Write(previewgots.Buffer(), previewgots.Size() * sizeof(StringHash));
    URHO3D_LOGINFOF("MatchGrid() - Save : ... add prevobjects ... buffer size = %u", buffer.GetSize());
}


void MatchGrid::Create(Vector<Match*>& newmatches)
{
    URHO3D_LOGINFO("MatchGrid() - Create : ...");

    for (int i=0; i < NUMMATCHRULES; i++)
        activedRules_[i] = true;

    matches_.Resize(width_, height_);
    for (unsigned y=0; y < height_; y++)
    {
        for (unsigned x=0; x < width_; x++)
        {
            Match& match = matches_(x, y);
            match.x_ = x;
            match.y_ = y;
            match.property_ = 0;
        }
    }

    if (previewLines_)
    {
        previewmatches_.Resize(width_, previewLines_);
        for (unsigned y=0; y < previewLines_; y++)
        {
            for (unsigned x=0; x < width_; x++)
            {
                Match& match = previewmatches_(x, y);
                match.x_ = x;
                match.y_ = y;
                match.property_ = 0;
            }
        }
    }

    InitializeTiles();
    InitializeWalls();
    InitializeTypes();
    InitializeObjects(newmatches);

    URHO3D_LOGINFO("MatchGrid() - Create : ... OK !");
}

void MatchGrid::InitializeTypes()
{
    URHO3D_LOGINFO("MatchGrid() - InitializeTypes : ... ");

    authorizedColors_.Clear();

    Vector<StringHash>& enemiesTypes = authorizedTypes_[COT::ENEMIES];
    Vector<StringHash>::Iterator it = enemiesTypes.Begin();
    while (it < enemiesTypes.End())
    {
        Node* object = GOT::GetObject(*it);
        if (object)
        {
            int colortype = object->GetVar(GOA::MATCHTYPE).GetInt();

            if (colortype >= NUMCOLORTYPES)
            {
                it = enemiesTypes.Erase(it);
            }
            else
            {
                if (!authorizedColors_.Contains(colortype))
                    authorizedColors_.Push(colortype);

                it++;
            }
        }
        else
        {
            it = enemiesTypes.Erase(it);
        }
    }

    URHO3D_LOGINFO("MatchGrid() - InitializeTypes : ... OK !");
}

static const Vector2 WallPositions[] = { Vector2(0.f, halfTileSize), Vector2(-halfTileSize, 0.f), Vector2(halfTileSize, 0.f), Vector2(0.f, -halfTileSize) };
static const float   WallRotations[] = { 0.f, 90.f, 90.f, 0.f };

void MatchGrid::InitializeTiles()
{
    URHO3D_LOGINFOF("MatchGrid() - InitializeTiles : ... ");

    String gridname;
    gridname.AppendWithFormat("Grid%d", gridid_);
    gridNode_ = GameStatics::rootScene_->GetChild("Scene")->GetChild(gridname, LOCAL);
    if (!gridNode_)
        gridNode_ = GameStatics::rootScene_->GetChild("Scene")->CreateChild(gridname, LOCAL);

    // Initialize Ground Tiles

    const StringHash groundcat(COT::GROUND2);

    const Vector<StringHash>& groundtypes = COT::GetObjectsInCategory(groundcat);
    tiles_.Resize(width_, height_);

    unsigned char feature;
    Vector3 position;
    for (unsigned y=0; y < height_; y++)
    {
        for (unsigned x=0; x < width_; x++)
        {
            WeakPtr<Node>& tile = tiles_(x, y);
            feature = grid_(x, y).ground_;

            if (!feature)
            {
                tile.Reset();
                continue;
            }

            StringHash got = groundtypes[--feature];
            tile = GOT::GetObject(got)->Clone(LOCAL, true, 0, 0, gridNode_);
            tile->SetVar(GOA::GOT, got);
            position.x_ = (float(2*int(x)-int(width_)) + 1.f) * halfTileSize;
            position.y_ = (float(int(height_)-2*int(y)) - 1.f) * halfTileSize;
            tile->SetPosition(position);
            tile->SetEnabled(true);
            tile->SetVar(GOA::GRIDCOORD, IntVector3(x, y, gridid_));
            StaticSprite2D* sprite = tile->GetDerivedComponent<StaticSprite2D>();
            if (sprite)
            {
                sprite->SetColor(gridColor_);
                sprite->SetAlpha(1.f);
            }
        }
    }

    // BOSSMODE : add a default tilegrid
    if (GameStatics::GetLevelInfo().mode_ == BOSSMODE)
    {
        const Vector<StringHash>& groundtypes = COT::GetObjectsInCategory(COT::GROUND1);
        Node* backgrid = gridNode_->CreateChild("backgrid", LOCAL);
        Vector3 position;

        for (unsigned y=0; y < height_; y++)
        {
            for (unsigned x=0; x < width_ ; x++)
            {
                Node* node = GOT::GetObject(COT::GROUND1)->Clone(LOCAL, true, 0, 0, backgrid);
                position.x_ = (float(2*int(x)-int(width_)) + 1.f) * halfTileSize;
                position.y_ = (float(int(height_)-2*int(y)) - 1.f) * halfTileSize;
                node->SetVar(GOA::GOT, COT::GROUND1);
                node->SetPosition(position);
                node->SetEnabled(true);
                node->GetComponent<RigidBody2D>()->Remove();
                StaticSprite2D* sprite = node->GetDerivedComponent<StaticSprite2D>();
                if (sprite)
                {
                    sprite->SetLayer(BACKGRIDLAYER);
                    sprite->SetColor(gridColor_);
                    sprite->SetAlpha(0.5f);
                }
            }
        }
    }

    if (previewLines_)
    {
        Node* backgrid = gridNode_->CreateChild("previewgrid", LOCAL);

        for (unsigned y=0; y < previewLines_; y++)
        {
            for (unsigned x=0; x < width_; x++)
            {
                if (!grid_(x, 0).ground_)
                    continue;

                Node* node = GOT::GetObject(COT::GROUND1)->Clone(LOCAL, true, 0, 0, backgrid);
                position.x_ = (float(2*int(x)-int(width_)) + 1.f) * halfTileSize;
                position.y_ = (float(int(height_+previewLines_+1)-2*int(y)) - 1.f) * halfTileSize;
                node->SetVar(GOA::GOT, COT::GROUND1);
                node->SetPosition(position);
                node->SetEnabled(true);
                node->GetComponent<RigidBody2D>()->Remove();
                StaticSprite2D* sprite = node->GetDerivedComponent<StaticSprite2D>();
                if (sprite)
                {
                    sprite->SetLayer(BACKGRIDLAYER);
                    sprite->SetColor(gridColor_);
                    sprite->SetAlpha(0.2f);
                }
            }
        }
    }

    URHO3D_LOGINFO("MatchGrid() - InitializeTiles ... OK !");
}

void MatchGrid::InitializeWalls()
{
    URHO3D_LOGINFOF("MatchGrid() - InitializeWalls : ... ");

    const Vector<StringHash>& walltypes = COT::GetObjectsInCategory(COT::WALLS);
    walls_.Resize(width_, height_);

    unsigned char walltype, wallorientation;
    Vector3 position;
    float angle;

    PODVector<Node*> wallnodes;

    for (unsigned y=0; y < height_; y++)
    for (unsigned x=0; x < width_ ; x++)
    {
        WeakPtr<Node>& wallroot = walls_(x,y);
        wallroot.Reset();

        walltype = grid_(x,y).walltype_;

        if (!walltype)
            continue;

        position.x_ = (float(2*int(x)-int(width_)) + 1.f) * halfTileSize;
        position.y_ = (float(int(height_)-2*int(y)) - 1.f) * halfTileSize;

        wallroot = gridNode_->CreateChild("Wall", LOCAL);
        wallroot->SetPosition(position);

        URHO3D_LOGINFOF("MatchGrid() - InitializeWalls : add walltype=%u at (%d, %d) !", walltype, x, y);

        wallorientation = grid_(x,y).wallorientation_;
        StringHash got = walltypes[--walltype];
        for (int i=0; i < 4; i++)
        {
            if (wallorientation & (1 << i))
            {
                Node* wall = GOT::GetObject(got)->Clone(LOCAL, true, 0, 0, wallroot);
                wall->SetVar(GOA::GOT, got);
                wall->SetName(String(1 << i));
                wall->SetPosition2D(WallPositions[i]);
                wall->SetRotation2D(WallRotations[i]);
                wall->SetEnabled(true);
                wallnodes.Push(wall);
            }
        }
    }

    if (wallnodes.Size() && Game::Get()->GetCompanion() && Game::Get()->GetCompanion()->IsNewMessage("tuto_walls_01"))
        Game::Get()->GetCompanion()->AddMessage(STATE_PLAY, true, "tuto_walls_01", "jona", 0.f, "UI/Companion/animatedcursors.scml", "cursor_arrow_ontop", wallnodes.Front(), Vector2::ZERO);

    URHO3D_LOGINFOF("MatchGrid() - InitializeWalls : ... OK !");
}

void MatchGrid::InitializeObjects(Vector<Match*>& newmatches)
{
    URHO3D_LOGINFO("MatchGrid() - InitializeObjects ... ");

    String objectsname;
    objectsname.AppendWithFormat("Objects%d", gridid_);
    objectsNode_ = GameStatics::rootScene_->GetChild("Scene")->GetChild(objectsname, LOCAL);
    if (!objectsNode_)
        objectsNode_ = GameStatics::rootScene_->GetChild("Scene")->CreateChild(objectsname, LOCAL);

    objects_.Resize(width_, height_);
    previewobjects_.Resize(width_, previewLines_);

    if (GameStatics::GetLevelInfo().mode_ == CLASSICLEVELMODE)
        for (unsigned x=0; x < width_; x++)
            AddColumn(x, newmatches);

    URHO3D_LOGINFO("MatchGrid() - InitializeObjects ... OK !");
}

void MatchGrid::SetObjects(const PODVector<StringHash>& gots, const PODVector<StringHash>& previewgots)
{
    URHO3D_LOGINFO("MatchGrid() - SetObjects ... ");

    String objectsname;
    objectsname.AppendWithFormat("Objects%d", gridid_);
    objectsNode_ = GameStatics::rootScene_->GetChild("Scene")->GetChild(objectsname, LOCAL);
    if (!objectsNode_)
        objectsNode_ = GameStatics::rootScene_->GetChild("Scene")->CreateChild(objectsname, LOCAL);

    objects_.Resize(matches_.Width(), matches_.Height());
    for (unsigned i=0; i < objects_.Size(); i++)
        AddObject(gots[i], matches_[i], objects_[i], false);

    previewobjects_.Resize(previewmatches_.Width(), previewmatches_.Height());
    for (unsigned i=0; i < previewobjects_.Size(); i++)
        AddObject(previewgots[i], previewmatches_[i], previewobjects_[i], true);
}


int MatchGrid::CollapseColumn(int x, Vector<Match*>& collapsematches)
{
    int distance = 0;

    for (int y=height_-1; y >= 0; y--)
    {
        if (grid_(x, y).wallorientation_ & WO_WALLNORTH)
            break;

        if (grid_(x, y).ground_ && !matches_(x, y).ctype_)
        {
            Match& m1 = matches_(x, y);

            for (int y2=y-1; y2 >= 0; y2--)
            {
                if (matches_(x, y2).ctype_)
                {
                    Match& m2 = matches_(x, y2);

                    if (IsDirectionalTile(m2) || (grid_(x, y2).wallorientation_ & WO_WALLNORTH))
                        break;

                    m1.property_ = m2.property_;
                    m2.property_ = 0;

                    WeakPtr<Node>& object = objects_(x, y);
                    object = objects_(x, y2);
                    objects_(x, y2).Reset();

                    SetDrawOrder(m1, object, OBJECTLAYER);
                    SetMoveAnimation(m1, object, CalculateMatchPosition(x, y), MOVETIME*float(y-y2));

                    collapsematches.Push(&m1);

                    if (y-y2 > distance)
                        distance = y-y2;

                    break;
                }
            }
        }
    }

//    URHO3D_LOGINFOF("MatchGrid() - CollapseColumn : column=%d distance=%d !", x, distance);

    return distance;
}

int MatchGrid::AddColumn(int x, Vector<Match*>& newmatches)
{
    int distance = 0;

    GameRand& random = GameRand::GetRandomizer(OBJRAND);

    const bool wallblocking = false;

    for (int y=height_-1; y >= 0; y--)
    {
        if (!grid_(x, y).ground_)
        {
//            wallblocking = false;
//            URHO3D_LOGINFOF("MatchGrid() - AddColumn : addobject at %d,%d => Blocked by Ground !", x, y);
            continue;
        }

        if (wallblocking && grid_(x,y).wallorientation_ & WO_WALLNORTH && grid_(x,y).wallorientation_ & WO_WALLSOUTH)
        {
//            wallblocking = true;
//            URHO3D_LOGINFOF("MatchGrid() - AddColumn : addobject at %d,%d => Blocked by Wall !", x, y);
            continue;
        }

        if (!wallblocking && !matches_(x, y).ctype_)
        {
            Match& match = matches_(x, y);

            Node* node = GetPreviewMatch(match, random);
            if (!node)
                node = AddObject(match, random);

            if (node)
            {
                node->SetPosition(GetEntrance(match));

//                URHO3D_LOGINFOF("MatchGrid() - AddColumn : addobject at %d,%d !", x, y);

                SetMoveAnimation(match, node, CalculateMatchPosition(x, y), MOVETIME*float(y+1));
                newmatches.Push(&match);

                if (distance < y+1)
                    distance = y+1;
            }
            else
                URHO3D_LOGERRORF("MatchGrid() - AddColumn : addobject at %d,%d => No Node !", x, y);
        }
    }

//    URHO3D_LOGINFOF("MatchGrid() - AddColumn : column=%d distance=%d !", x, distance);

    return distance;
}

Node* MatchGrid::GetPreviewMatch(Match& match, GameRand& random)
{
    if (!previewLines_)
        return 0;

    if (!grid_(match.x_, 0).ground_)
    {
        URHO3D_LOGINFOF("MatchGrid() - GetPreviewMatch : no ground at %d 0!", match.x_);
        return 0;
    }

    // fill preview add if need
    for (unsigned y=0; y < previewLines_; y++)
    {
        Match& previewmatch = previewmatches_(match.x_, y);
        if (!previewmatch.ctype_)
            previewobjects_(match.x_, y) = AddPreviewObject(previewmatch, random);
    }

    // transfer preview match in the match
    unsigned char savedy = match.y_;
    match = previewmatches_(match.x_, 0);
    match.y_ = savedy;
    WeakPtr<Node> previewobject = previewobjects_(match.x_, 0);

    // set the object of the match
    WeakPtr<Node>& object = objects_(match.x_, match.y_);
    if (object)
        RemoveObject(object);

    if (previewobject)
    {
        object = previewobject;
        StaticSprite2D* sprite = object->GetDerivedComponent<StaticSprite2D>();
        sprite->SetAlpha(1.f);
        SetDrawOrder(match, object, OBJECTLAYER, match.effect_);
    }

    // remove the previewmatch and move preview
    previewmatches_(match.x_, 0).property_ = 0;
    previewobjects_(match.x_, 0).Reset();

    // TODO : if previewlines > 1 then moves

    // fill preview add if need
    Match& previewmatch = previewmatches_(match.x_, 0);
    if (!previewmatch.ctype_)
        previewobjects_(match.x_, 0) = AddPreviewObject(previewmatch, random);

    URHO3D_LOGINFOF("MatchGrid() - GetPreviewMatch : object at %d,%d!", match.x_, match.y_);
    return object.Get();
}

Node* MatchGrid::AddPreviewObject(Match& match, GameRand& random)
{
    const Vector<StringHash>& bonusesTypes = authorizedTypes_[COT::POWERS];
    const Vector<StringHash>& rocksTypes = authorizedTypes_[COT::ROCKS];
    const Vector<StringHash>& enemiesTypes = authorizedTypes_[COT::ENEMIES];
    const bool addPower = bonusesTypes.Size() ? random.Get(100) < Match::BONUSCHANCE : false;
    const bool addRock = rocksTypes.Size() ? random.Get(100) < Match::ROCKCHANCE : false;

    WeakPtr<Node>& object = previewobjects_(match.x_, match.y_);
    RemoveObject(object);

    if (addPower)
    {
        StringHash bonustype(bonusesTypes[random.Get(bonusesTypes.Size())]);
        object = GOT::GetObject(bonustype)->Clone(LOCAL, true, 0, 0, objectsNode_);
        object->SetVar(GOA::GOT, bonustype);
        // set match type
        match.ctype_  = authorizedColors_[random.Get(authorizedColors_.Size())];
        match.otype_  = COT::IsInCategory(bonustype, COT::POWERS);
        match.effect_ = object->GetVar(GOA::BONUS).GetIntVector2().x_;

        URHO3D_LOGINFOF("MatchGrid() - AddPreviewObject : addPower match=%s !", match.ToString().CString());
    }
    else if (addRock)
    {
        // set object
        StringHash rocktype = rocksTypes[random.Get(rocksTypes.Size())];
        object = GOT::GetObject(rocktype)->Clone(LOCAL, true, 0, 0, objectsNode_);
        object->SetVar(GOA::GOT, rocktype);
        // set match type
        match.ctype_    = ROCKS;
        match.otype_    = -1;
        match.effect_   = NOEFFECT;

        if (Game::Get()->GetCompanion() && Game::Get()->GetCompanion()->IsNewMessage("tuto_rocks_01"))
        {
            Game::Get()->GetCompanion()->AddMessage(STATE_PLAY, true, "tuto_rocks_01", "jona", 0.f, "UI/Companion/animatedcursors.scml", "cursor_arrow_ontop", object.Get(), Vector2::ZERO);
            Game::Get()->GetCompanion()->AddMessage(STATE_PLAY, true, "tuto_rocks_02", "jona", 0.f, "UI/Companion/animatedcursors.scml", "cursor_arrow_ontop", object.Get(), Vector2::ZERO);
        }
    }
    else
    {
        if (!enemiesTypes.Size())
        {
//            URHO3D_LOGERRORF("MatchGrid() - AddPreviewObject : No ennemies Types registered !");
            return 0;
        }

        // set object
        const StringHash& got = enemiesTypes[random.Get(enemiesTypes.Size())];
        object = GOT::GetObject(got)->Clone(LOCAL, true, 0, 0, objectsNode_);
        object->SetVar(GOA::GOT, got);
        // set match type
        match.ctype_    = object->GetVar(GOA::MATCHTYPE).GetInt();
        match.otype_    = MatchesManager::GetObjectiveIndex(got, gridid_);
        match.effect_   = NOEFFECT;

//		URHO3D_LOGINFOF("MatchGrid() - AddPreviewObject : ennemy match=%s !", match.ToString().CString());
    }

    object->SetScale(OBJECTSCALE);
    object->SetEnabled(true);
    object->SetPosition(GetEntrance(match));

    SetMoveAnimation(match, object, CalculateMatchPosition(match.x_, -match.y_-1), MOVETIME*float(match.y_+1));

    if (IsPower(match))
        SetScaleAnimation(object, BONUSFACTOR);

    SetDrawOrder(match, object, OBJECTLAYER, addPower);

    StaticSprite2D* sprite = object->GetDerivedComponent<StaticSprite2D>();
    sprite->SetAlpha(0.4f);

    return object.Get();
}

Node* MatchGrid::AddObject(Match& match, GameRand& random)
{
    const Vector<StringHash>& bonusesTypes = authorizedTypes_[COT::POWERS];
    const Vector<StringHash>& rocksTypes = authorizedTypes_[COT::ROCKS];
    const Vector<StringHash>& enemiesTypes = authorizedTypes_[COT::ENEMIES];

    bool addPower = bonusesTypes.Size() ? random.Get(100) < Match::BONUSCHANCE : false;
    bool addRock = rocksTypes.Size() ? random.Get(100) < Match::ROCKCHANCE : false;

    WeakPtr<Node>& object = objects_(match.x_, match.y_);
    RemoveObject(object);

    if (addPower)
    {
        StringHash bonustype(bonusesTypes[random.Get(bonusesTypes.Size())]);
        object = GOT::GetObject(bonustype)->Clone(LOCAL, true, 0, 0, objectsNode_);
        object->SetVar(GOA::GOT, bonustype);
//        int bonusindex = GameRand::GetRand(OBJRAND, bonusesTypes.Size());
//        object = COT::GetObjectFromCategory(COT::POWERS, bonusindex)->Clone(LOCAL, true, 0, 0, objectsNode_);

        // set match type
        match.ctype_  = authorizedColors_[random.Get(authorizedColors_.Size())];
        match.otype_  = COT::IsInCategory(bonustype, COT::POWERS);
        match.effect_ = object->GetVar(GOA::BONUS).GetIntVector2().x_;

        URHO3D_LOGINFOF("MatchGrid() - AddObject : addPower match=%s !", match.ToString().CString());
    }
    else if (addRock)
    {
        // set object
        StringHash rocktype = rocksTypes[random.Get(rocksTypes.Size())];
        object = GOT::GetObject(rocktype)->Clone(LOCAL, true, 0, 0, objectsNode_);
        object->SetVar(GOA::GOT, rocktype);
        // set match type
        match.ctype_    = ROCKS;
        match.otype_    = -1;
        match.effect_   = NOEFFECT;

        if (Game::Get()->GetCompanion() && Game::Get()->GetCompanion()->IsNewMessage("tuto_rocks_01"))
        {
            Game::Get()->GetCompanion()->AddMessage(STATE_PLAY, true, "tuto_rocks_01", "jona", 0.f, "UI/Companion/animatedcursors.scml", "cursor_arrow_ontop", object.Get(), Vector2::ZERO);
            Game::Get()->GetCompanion()->AddMessage(STATE_PLAY, true, "tuto_rocks_02", "jona", 0.f, "UI/Companion/animatedcursors.scml", "cursor_arrow_ontop", object.Get(), Vector2::ZERO);
        }
    }
    else
    {
        if (!enemiesTypes.Size())
            return 0;

        // set object
        const StringHash& got = enemiesTypes[random.Get(enemiesTypes.Size())];
        object = GOT::GetObject(got)->Clone(LOCAL, true, 0, 0, objectsNode_);
        object->SetVar(GOA::GOT, got);
        // set match type
        match.ctype_    = object->GetVar(GOA::MATCHTYPE).GetInt();
        match.otype_    = MatchesManager::GetObjectiveIndex(got, gridid_);
        match.effect_   = NOEFFECT;

//		URHO3D_LOGINFOF("MatchGrid() - AddObject : ennemy match=%s !", match.ToString().CString());
    }

    object->SetScale(OBJECTSCALE);
    object->SetEnabled(true);

    if (IsPower(match))
        SetScaleAnimation(object, BONUSFACTOR);

    SetDrawOrder(match, object, OBJECTLAYER, addPower);

    return object.Get();
}

void MatchGrid::AddObject(StringHash got, const Match& match, WeakPtr<Node>& object, bool previewobject)
{
    if (object)
        RemoveObject(object);

    if (!got.Value())
        return;

    object = GOT::GetObject(got)->Clone(LOCAL, true, 0, 0, objectsNode_);
    if (object)
    {
        object->SetVar(GOA::GOT, got);
        object->SetScale(OBJECTSCALE);
        object->SetEnabled(true);

        if (IsPower(match))
            SetScaleAnimation(object, BONUSFACTOR);

        SetDrawOrder(match, object, OBJECTLAYER, IsPower(match));

        StaticSprite2D* sprite = object->GetDerivedComponent<StaticSprite2D>();
        if (sprite)
            sprite->SetAlpha(previewobject ? 0.4f : 1.f);

        object->SetPosition(GetEntrance(match));

        if (previewobject)
            SetMoveAnimation(match, object, CalculateMatchPosition(match.x_, -match.y_-1), MOVETIME*float(match.y_+1));
        else
            SetMoveAnimation(match, object, CalculateMatchPosition(match.x_, match.y_), MOVETIME*float(match.y_+1));
    }
}

Node* MatchGrid::AddPowerBonus(Match* m, int bonusindex)
{
    const Vector<StringHash>& bonusesTypes = authorizedTypes_[COT::POWERS];

    if (!bonusesTypes.Size())
    {
        URHO3D_LOGWARNING("MatchGrid() - AddPowerBonus : no bonus types authorized !");
        return 0;
    }

	Node* bonus = 0;
	StringHash bonustype;

    // Randomize bonus type in authorizedTypes
	if (bonusindex == -1)
    {
        bonustype = bonusesTypes[GameRand::GetRand(OBJRAND, bonusesTypes.Size())];
        bonusindex = COT::IsInCategory(bonustype, COT::POWERS);
    }
    // Get the bonus object directly (bypass authorizedTypes)
	else
		bonustype = COT::GetTypeFromCategory(COT::POWERS, bonusindex);

	bonus = GOT::GetObject(bonustype);
	if (!bonus)
	{
		URHO3D_LOGWARNINGF("MatchGrid() - AddPowerBonus : bonusindex=%d no bonus for color=%d in COT::POWERS !", bonusindex, m->ctype_);
		return 0;
	}

    // update match
    Match& match = m == &matches_(m->x_, m->y_) ? *m : matches_(m->x_, m->y_);
	if (&match != m)
	{
		match.x_ = m->x_;
		match.y_ = m->y_;
		match.property_ = m->property_;
	}

    WeakPtr<Node>& object = objects_(match.x_, match.y_);

    // Remove Existing Object
    RemoveObject(object);

    object = bonus->Clone(LOCAL, true, 0, 0, objectsNode_);
    object->SetVar(GOA::GOT, bonustype);
    object->SetPosition(GetEntrance(match));
    object->SetScale(OBJECTSCALE);
    object->SetEnabled(true);

    match.otype_  = bonusindex;
    match.effect_ = object->GetVar(GOA::BONUS).GetIntVector2().x_;

    SetDrawOrder(match, object, OBJECTLAYER, true);
    SetScaleAnimation(object, BONUSFACTOR);
    SetMoveAnimation(match, object, CalculateMatchPosition(match.x_, match.y_));

    URHO3D_LOGINFOF("MatchGrid() - AddPowerBonus : %s(ptr=%u) entry=%s(ptr=%u) powerid=%d !",
                    match.ToString().CString(), &match, m->ToString().CString(), m, bonusindex+1);

    return object;
}

Node* MatchGrid::AddItemBonus(Match& m, const StringHash& category, int bonusindex)
{
    const Vector<StringHash>& bonusesTypes = authorizedTypes_[category];
    if (!bonusesTypes.Size())
    {
        URHO3D_LOGWARNING("MatchGrid() - AddItemBonus : no bonus types authorized !");
        return 0;
    }

    // Randomize bonus type in authorized types
    if (bonusindex == -1)
        bonusindex = GameRand::GetRand(OBJRAND, bonusesTypes.Size());

    StringHash bonustype = bonusesTypes[bonusindex];
    Node* bonus = GOT::GetObject(bonustype);
    if (!bonus)
    {
        URHO3D_LOGWARNINGF("MatchGrid() - AddItemBonus : no bonus for color=%d in COT::ITEMS !", m.ctype_);
        return 0;
    }

    // update match
    Match& match = matches_(m.x_, m.y_);
    match = m;

    WeakPtr<Node>& object = objects_(match.x_, match.y_);

    // Remove Existing Object
    RemoveObject(object);

    object = bonus->Clone(LOCAL, true, 0, 0, objectsNode_);
    object->SetVar(GOA::GOT, bonustype);
    object->SetPosition(GetEntrance(match));
    object->SetScale(ITEMSCALE);
    object->SetEnabled(true);

    match.ctype_  = ITEMS + COT::GetCategoryIndexIn(COT::ITEMS, category);
    match.otype_  = bonusindex;
    match.effect_ = NOEFFECT;
    match.qty_    = object->GetVar(GOA::BONUS).GetInt();

    SetDrawOrder(match, object, ITEMLAYER, false);

    SetMoveAnimation(match, object, CalculateMatchPosition(match.x_, match.y_));
    SetScaleAnimation(object, ITEMFACTOR, ITEMDELAY, WM_ONCE);

    URHO3D_LOGINFOF("MatchGrid() - AddItemBonus : %s !", match.ToString().CString());

    return object;
}

void MatchGrid::AddSuccessEffect(const Match& match)
{
    if (!objects_(match.x_, match.y_))
        return;

    Node* effect = COT::GetObjectFromCategory(COT::EXPLOSIONS, 5);
    if (!effect)
    {
        URHO3D_LOGWARNINGF("MatchGrid() - AddSuccessEffect : no effect for %s !", match.ToString().CString());
        return;
    }

    Node* node = effect->Clone(LOCAL, true, 0, 0, objectsNode_);
    node->SetEnabled(true);
    node->SetPosition(objects_(match.x_, match.y_)->GetPosition());

    {
        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(node->GetContext()));
        SharedPtr<ValueAnimation> scaleAnimation(new ValueAnimation(node->GetContext()));
        scaleAnimation->SetKeyFrame(0.f, OBJECTSCALE);
        scaleAnimation->SetKeyFrame(SUCCESSDELAY, OBJECTSCALE*1.2f);
        objectAnimation->AddAttributeAnimation("Scale", scaleAnimation, WM_ONCE);
        node->SetObjectAnimation(objectAnimation);
    }

    StaticSprite2D* drawable = node->GetDerivedComponent<StaticSprite2D>();
    if (drawable)
    {
        drawable->SetLayer(GRIDLAYER + match.y_);
        drawable->SetOrderInLayer(match.x_);
        drawable->SetColor(MatchColors[match.ctype_]);
        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(node->GetContext()));
        SharedPtr<ValueAnimation> alphaAnimation(new ValueAnimation(drawable->GetContext()));
        alphaAnimation->SetKeyFrame(0.f, drawable->GetAlpha());
        alphaAnimation->SetKeyFrame(SUCCESSDELAY, 0.f);
        objectAnimation->AddAttributeAnimation("Alpha", alphaAnimation, WM_ONCE);
        drawable->SetObjectAnimation(objectAnimation);
    }

//    GameHelpers::SpawnSound(GameStatics::rootScene_, "Sounds/blg_energy_field_11a.wav");
    TimerRemover::Get()->Start(node, SUCCESSDELAY + 0.1f);
}

void MatchGrid::AddObjectiveEffect(const Match& match)
{
    if (!objects_(match.x_, match.y_))
        return;

    Node* effect = COT::GetObjectFromCategory(COT::EXPLOSIONS, 5);
    if (!effect)
    {
        URHO3D_LOGWARNINGF("MatchGrid() - AddObjectiveEffect : no effect for %s !", match.ToString().CString());
        return;
    }

    Node* node = effect->Clone(LOCAL, true, 0, 0, objectsNode_);
    node->SetEnabled(true);
    node->SetPosition(objects_(match.x_, match.y_)->GetPosition());
    SetDrawOrder(match, node, OBJECTIVELAYER, true);

    {
        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(node->GetContext()));
        SharedPtr<ValueAnimation> scaleAnimation(new ValueAnimation(node->GetContext()));
        scaleAnimation->SetKeyFrame(0.f, OBJECTSCALE);
        scaleAnimation->SetKeyFrame(OBJECTIVESUCCESSDELAY, OBJECTSCALE * OBJECTIVESCALEFACTOR);
        objectAnimation->AddAttributeAnimation("Scale", scaleAnimation, WM_ONCE);
        node->SetObjectAnimation(objectAnimation);
    }

    StaticSprite2D* drawable = node->GetDerivedComponent<StaticSprite2D>();
    if (drawable)
    {
        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(node->GetContext()));
        SharedPtr<ValueAnimation> alphaAnimation(new ValueAnimation(drawable->GetContext()));
        alphaAnimation->SetKeyFrame(0.f, drawable->GetAlpha());
        alphaAnimation->SetKeyFrame(OBJECTIVESUCCESSDELAY, 0.f);
        objectAnimation->AddAttributeAnimation("Alpha", alphaAnimation, WM_ONCE);
        drawable->SetObjectAnimation(objectAnimation);
    }

//    URHO3D_LOGINFOF("MatchGrid() - AddObjectiveEffect : %s", match.ToString().CString());

    GameHelpers::SpawnSound(GameStatics::rootScene_, SND_OBJECTIVESUCCESS, 0.5f);
    TimerRemover::Get()->Start(node, OBJECTIVESUCCESSDELAY + 0.1f);
}

void MatchGrid::AddTouchEffect(const Vector2& position, float scale, float delay, const Color& color)
{
    Node* effect = COT::GetObjectFromCategory(COT::EFFECTS, 4);
    if (!effect)
    {
        URHO3D_LOGWARNINGF("MatchGrid() - AddTouchEffect : no effect !");
        return;
    }

    Node* node = effect->Clone(LOCAL, true, 0, 0, objectsNode_);
    node->SetEnabled(true);

    // touch effect is in local gridnode==objectnode, position is worldposition so adjust scale
    node->SetWorldPosition(position);

    StaticSprite2D* drawable = node->GetDerivedComponent<StaticSprite2D>();
    drawable->SetLayer(FRONTLAYER);
    drawable->SetColor(color);

    {
        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(node->GetContext()));
        SharedPtr<ValueAnimation> scaleAnimation(new ValueAnimation(node->GetContext()));
        scaleAnimation->SetKeyFrame(0.f, OBJECTSCALE);
        scaleAnimation->SetKeyFrame(delay, scale * OBJECTSCALE * OBJECTIVESCALEFACTOR);
        objectAnimation->AddAttributeAnimation("Scale", scaleAnimation, WM_ONCE);
        node->SetObjectAnimation(objectAnimation);
    }

    if (drawable)
    {
        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(node->GetContext()));
        SharedPtr<ValueAnimation> alphaAnimation(new ValueAnimation(drawable->GetContext()));
        alphaAnimation->SetKeyFrame(0.f, drawable->GetAlpha());
        alphaAnimation->SetKeyFrame(delay, 0.f);
        objectAnimation->AddAttributeAnimation("Alpha", alphaAnimation, WM_ONCE);
        drawable->SetObjectAnimation(objectAnimation);
    }

    GameHelpers::SpawnSound(GameStatics::rootScene_, SND_OBJECTIVESUCCESS, 0.5f);
    TimerRemover::Get()->Start(node, delay + 0.1f);
}

void MatchGrid::AddEffectInNode(Node* root, int e, float scale, float duration, const Color& color)
{
    Node* effect = COT::GetObjectFromCategory(COT::EFFECTS, e);
    if (!effect)
    {
        URHO3D_LOGWARNINGF("MatchGrid() - AddEffectInNode : no effect !");
        return;
    }

    Node* node = effect->Clone(LOCAL, true, 0, 0, root);
    node->SetEnabled(true);

    StaticSprite2D* drawable = node->GetDerivedComponent<StaticSprite2D>();
    drawable->SetLayer(FRONTLAYER);
    drawable->SetColor(color);

    {
        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(GameStatics::context_));
        SharedPtr<ValueAnimation> scaleAnimation(new ValueAnimation(GameStatics::context_));
        scaleAnimation->SetKeyFrame(0.f, OBJECTSCALE);
        scaleAnimation->SetKeyFrame(duration, scale * OBJECTSCALE * OBJECTIVESCALEFACTOR);
        objectAnimation->AddAttributeAnimation("Scale", scaleAnimation, WM_ONCE);
        node->SetObjectAnimation(objectAnimation);
    }

    if (drawable)
    {
        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(GameStatics::context_));
        SharedPtr<ValueAnimation> alphaAnimation(new ValueAnimation(GameStatics::context_));
        alphaAnimation->SetKeyFrame(0.f, drawable->GetAlpha());
        alphaAnimation->SetKeyFrame(duration, 0.f);
        objectAnimation->AddAttributeAnimation("Alpha", alphaAnimation, WM_ONCE);
        drawable->SetObjectAnimation(objectAnimation);
    }

    GameHelpers::SpawnSound(GameStatics::rootScene_, SND_OBJECTIVESUCCESS, 0.5f);
    TimerRemover::Get()->Start(node, duration + 0.1f);
}

void MatchGrid::AddPowerEffects(const Match& match)
{
    if (match.effect_ == NOEFFECT)
        return;

    Vector3 position = CalculateMatchPosition(match.x_, match.y_);

    if (match.effect_ >= ELECEXPLOSION)
    {
        Node* effect = COT::GetObjectFromCategory(COT::EFFECTS, match.effect_ < ROCKEXPLOSION ? 1 : 0);
        if (!effect)
        {
            URHO3D_LOGWARNINGF("MatchGrid() - AddPowerEffects : no effect for %s !", match.ToString().CString());
            return;
        }
        Node* node = effect->Clone(LOCAL, true, 0, 0, objectsNode_);
        node->SetVar(GOA::FACTION, GO_Player);
        node->SetEnabled(true);
        node->SetPosition(position);
        SetDrawOrder(match, node, EFFECTLAYER, true);
        SetEffectAnimation(node);
        GameHelpers::SpawnSound(GameStatics::rootScene_, SND_EXPLOSION1);

		URHO3D_LOGINFOF("MatchGrid() - AddPowerEffects : %s !", match.ToString().CString());
    }
	else if (match.effect_ >= SQREXPLOSION)
	{
		/// TODO
	}
	else if (match.effect_ <= XYEXPLOSION)
    {
        Node* explosion = COT::GetObjectFromCategory(COT::EXPLOSIONS, 0);
//        Node* explosion = COT::GetObjectFromCategory(COT::EXPLOSIONS, bonuses_[match.ctype_]);

        if (!explosion)
        {
            URHO3D_LOGWARNINGF("MatchGrid() - AddPowerEffects : no effect for %s !", match.ToString().CString());
            return;
        }

        if (match.effect_ & XEXPLOSION)
		{
			Node* node = explosion->Clone(LOCAL, true, 0, 0, objectsNode_);
			node->SetVar(GOA::FACTION, GO_Player);
			node->SetEnabled(true);
			node->SetPosition(position);
			SetDrawOrder(match, node, EFFECTLAYER, true);
			SetExplosionAnimation(node);
			GameHelpers::SpawnSound(GameStatics::rootScene_, SND_EXPLOSION1);
        }
        if (match.effect_ & YEXPLOSION)
        {
			Node* node = explosion->Clone(LOCAL, true, 0, 0, objectsNode_);
			node->SetVar(GOA::FACTION, GO_Player);
			node->SetEnabled(true);
			node->SetPosition(position);
			node->SetRotation2D(90.f);
			SetDrawOrder(match, node, EFFECTLAYER, true);
			SetExplosionAnimation(node);
            GameHelpers::SpawnSound(GameStatics::rootScene_, SND_EXPLOSION2);
        }
    }
}

void MatchGrid::AddDirectionalTileRamp(unsigned char type, const IntVector2& from, const IntVector2& to, bool entrance, bool exit)
{
    if (!IsDirectional(type))
        return;

    // find the ramp direction
    int dir = type - GT_GRND - 1;
    IntVector2 direction;

    if (type == GT_GRND_2LE)
        direction.x_ = -1;
    else if (type == GT_GRND_2RI)
        direction.x_ = 1;
    else if (type == GT_GRND_2TO)
        direction.y_ = -1;
    else if (type == GT_GRND_2BO)
        direction.y_ = 1;

    // set entrance
    if (entrance)
    {
        tileentrances_.Resize(tileentrances_.Size()+1);
        tileentrances_.Back().position_ = from;
        tileentrances_.Back().direction_ = direction;
    }

    // set exit
    if (exit)
        tileexits_.Push(to);

    URHO3D_LOGINFOF("MatchGrid() - AddDirectionalTileRamp from=%s to=%s dir=%s", from.ToString().CString(), to.ToString().CString(), direction.ToString().CString());

    // fill the grid and ramptiles

    IntVector2 position = from;

    ramptiles_.Resize(ramptiles_.Size()+1);
    Vector<IntVector2>& ramptile = ramptiles_.Back();
    // ramptile[0] = direction
    ramptile.Push(direction);

    while (position != to)
    {
        URHO3D_LOGINFOF("MatchGrid() - AddDirectionalTileRamp ... : position=%s", position.ToString().CString());
        grid_(position.x_, position.y_).ground_ = type;
        ramptile.Push(position);
        position += direction;
    }
    // add "to"
    {
        URHO3D_LOGINFOF("MatchGrid() - AddDirectionalTileRamp ... : position=%s", to.ToString().CString());
        grid_(to.x_, to.y_).ground_ = type;
        ramptile.Push(to);
    }
}

void MatchGrid::SetDrawOrder(const Match& match, Node* node, int baselayer, bool setcolortype)
{
    Drawable2D* drawable = node->GetDerivedComponent<Drawable2D>();
    drawable->SetLayer(baselayer + match.y_);
    drawable->SetOrderInLayer(match.x_);

    if (setcolortype)
    {
        const Color& color = MatchColors[Min(match.ctype_, NUMCOLORTYPES-1)];
        static_cast<StaticSprite2D*>(drawable)->SetColor(color);
		//URHO3D_LOGINFOF("MatchGrid() - SetDrawOrder ... : match=%s color=%s", match.ToString().CString(), color.ToString().CString());
    }
}

bool MatchGrid::SetSelectAnimation(const Match& match, int orderInLayer)
{
    if (!objects_.Size())
        return false;

    Node* node = objects_(match.x_, match.y_);
    if (!node)
        return false;

    SetScaleAnimation(node, OBJECTSELECTED);

    Drawable2D* drawable = node->GetDerivedComponent<Drawable2D>();
    drawable->SetLayer(SELECTLAYER);
    drawable->SetOrderInLayer(orderInLayer);

    return true;
}

bool MatchGrid::SetMoveAnimation(const Match& match, Node* node, const Vector3& newposition, float time)
{
    if (!node)
        return false;

    bool addnewobject = !node->GetObjectAnimation();
    SharedPtr<ObjectAnimation> objectAnimation(addnewobject ? new ObjectAnimation(node->GetContext()) : node->GetObjectAnimation());

    SharedPtr<ValueAnimation> moveAnimation(new ValueAnimation(node->GetContext()));
    moveAnimation->SetKeyFrame(0.f, node->GetPosition());
    moveAnimation->SetKeyFrame(Min(time, MAXMOVETIME), newposition);
    objectAnimation->AddAttributeAnimation("Position", moveAnimation, WM_ONCE);

	if (addnewobject)
		node->SetObjectAnimation(objectAnimation);

    if (!IsPower(match) && !IsItem(match))
        node->RemoveAttributeAnimation("Scale");

    return true;
}

bool MatchGrid::SetScaleAnimation(Node* node, float factor, float scaletime, WrapMode loop)
{
    if (!node)
        return false;

    bool addnewobject = !node->GetObjectAnimation();
    SharedPtr<ObjectAnimation> objectAnimation(addnewobject ? new ObjectAnimation(node->GetContext()) : node->GetObjectAnimation());

    SharedPtr<ValueAnimation> scaleAnimation(new ValueAnimation(node->GetContext()));
    scaleAnimation->SetKeyFrame(0.f, OBJECTSCALE);
    scaleAnimation->SetKeyFrame(scaletime*0.6f, OBJECTSCALE*factor);
    scaleAnimation->SetKeyFrame(scaletime, OBJECTSCALE);
    objectAnimation->AddAttributeAnimation("Scale", scaleAnimation, loop);

    if (addnewobject)
        node->SetObjectAnimation(objectAnimation);

    return true;
}

bool MatchGrid::SetAlphaAnimation(Node* node)
{
    if (!node)
        return false;

    StaticSprite2D* drawable = node->GetDerivedComponent<StaticSprite2D>();
    if (!drawable)
        return false;

    bool addnewobject = !drawable->GetObjectAnimation();
    SharedPtr<ObjectAnimation> objectAnimation(addnewobject ? new ObjectAnimation(drawable->GetContext()) : drawable->GetObjectAnimation());

    SharedPtr<ValueAnimation> alphaAnimation(new ValueAnimation(drawable->GetContext()));
    alphaAnimation->SetKeyFrame(0.f, drawable->GetAlpha());
    alphaAnimation->SetKeyFrame(ALPHATIME, 0.f);
    objectAnimation->AddAttributeAnimation("Alpha", alphaAnimation, WM_ONCE);

    if (addnewobject)
        drawable->SetObjectAnimation(objectAnimation);

    return true;
}

bool MatchGrid::SetAlphaAnimationNew(Node* node)
{
    if (!node)
        return false;

    StaticSprite2D* drawable = node->GetDerivedComponent<StaticSprite2D>();
    if (!drawable)
        return false;

    if (drawable->GetObjectAnimation())
        drawable->RemoveObjectAnimation();

    SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(drawable->GetContext()));
    SharedPtr<ValueAnimation> alphaAnimation(new ValueAnimation(drawable->GetContext()));
    alphaAnimation->SetKeyFrame(0.f, drawable->GetAlpha());
    alphaAnimation->SetKeyFrame(ALPHATIME, 0.f);
    objectAnimation->AddAttributeAnimation("Alpha", alphaAnimation, WM_ONCE);
    drawable->SetObjectAnimation(objectAnimation);

    return true;
}

bool MatchGrid::SetExplosionAnimation(Node* node)
{
    if (!node)
        return false;

    const Vector3& scale = node->GetScale();

    SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(node->GetContext()));
    SharedPtr<ValueAnimation> scaleAnimation(new ValueAnimation(node->GetContext()));
    scaleAnimation->SetKeyFrame(0.f, Vector3::ZERO);
    scaleAnimation->SetKeyFrame(EXPLOSIONDELAY, scale*Vector3(5.f, 0.75f, 1.f));
    objectAnimation->AddAttributeAnimation("Scale", scaleAnimation, WM_ONCE);
    node->SetObjectAnimation(objectAnimation);

    node->SetScale(Vector3::ZERO);

    TimerRemover::Get()->Start(node, EXPLOSIONDELAY + 0.1f);

    return true;
}

bool MatchGrid::SetEffectAnimation(Node* node)
{
    if (!node)
        return false;

    StaticSprite2D* drawable = node->GetDerivedComponent<StaticSprite2D>();

    if (!drawable)
        return false;

	float delay = EXPLOSIONDELAY + 0.1f;

	if (drawable->IsInstanceOf<AnimatedSprite2D>())
	{
		AnimatedSprite2D* animated = static_cast<AnimatedSprite2D*>(drawable);
		delay = animated->GetSpriterAnimation()->length_ + 0.1f;
	}

    SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(node->GetContext()));
    SharedPtr<ValueAnimation> alphaAnimation(new ValueAnimation(drawable->GetContext()));
    alphaAnimation->SetKeyFrame(0.f, drawable->GetAlpha());
    alphaAnimation->SetKeyFrame(ALPHATIME, 0.f);
    objectAnimation->AddAttributeAnimation("Alpha", alphaAnimation, WM_ONCE);

    TimerRemover::Get()->Start(node, delay);

    return true;
}

bool MatchGrid::SetHintAnimation(const Match& match, bool enable)
{
    Node* node = objects_(match.x_, match.y_);
    if (!node)
    {
        URHO3D_LOGWARNINGF("MatchGrid() - SetHintAnimation : no node for %s !", match.ToString().CString());
        return false;
    }
    if (enable)
    {
        bool addnewobject = !node->GetObjectAnimation();
        SharedPtr<ObjectAnimation> objectAnimation(addnewobject ? new ObjectAnimation(node->GetContext()) : node->GetObjectAnimation());

        SharedPtr<ValueAnimation> rotateAnimation(new ValueAnimation(node->GetContext()));
        rotateAnimation->SetKeyFrame(0.f, Quaternion::IDENTITY);
        rotateAnimation->SetKeyFrame(ROTATETIME*0.3f, Quaternion(-HINTANGLE));
        rotateAnimation->SetKeyFrame(ROTATETIME*0.5f, Quaternion(HINTANGLE));
        rotateAnimation->SetKeyFrame(ROTATETIME*0.7f, Quaternion::IDENTITY);
        rotateAnimation->SetKeyFrame(ROTATETIME, Quaternion::IDENTITY);
        objectAnimation->AddAttributeAnimation("Rotation", rotateAnimation, WM_LOOP);

        if (addnewobject)
            node->SetObjectAnimation(objectAnimation);

        node->SetScale(OBJECTSCALE*HINTSCALEFACTOR);
    }
    else
    {
        if (!node->GetObjectAnimation())
            return true;

        node->GetObjectAnimation()->RemoveAttributeAnimation("Rotation");
        node->SetRotation(Quaternion::IDENTITY);
        node->SetScale(OBJECTSCALE);
    }

    return true;
}

void MatchGrid::SetHittedWalls(Vector<WallInfo>& hittedwalls)
{
    for (unsigned i=0; i < hittedwalls.Size(); i++)
    {
        WallInfo& wallinfo = hittedwalls[i];
        if ((grid_(wallinfo.x_, wallinfo.y_).wallorientation_ & wallinfo.wallorientation_) == 0)
            continue;

        grid_(wallinfo.x_, wallinfo.y_).wallorientation_ = grid_(wallinfo.x_, wallinfo.y_).wallorientation_ & ~wallinfo.wallorientation_;

        Node* rootnode = walls_(wallinfo.x_, wallinfo.y_);
        if (!rootnode)
            continue;

		for (int j = 0; j < 4; j++)
		{
			if (wallinfo.wallorientation_ & (1 << j))
			{
				Node* node = rootnode->GetChild(String((int)(1 << j)));

				if (!node)
					continue;

				SetExplosionAnimation(node);
			}
		}
    }
}

void MatchGrid::SetMatch(Match& match, unsigned property)
{
    match.property_ = property;
}

void MatchGrid::SetMatch(Match& match, unsigned char ctype, char otype, unsigned char effect)
{
    match.ctype_ = ctype;
    match.otype_ = otype;
    match.effect_ = effect;
}

// reset scale and position
void MatchGrid::ResetPosition(const Match& match)
{
    if (!objects_.Size())
        return;

    WeakPtr<Node>& object = objects_(match.x_, match.y_);
    if (!object)
        return;

    if (IsPower(match))
        SetScaleAnimation(object, BONUSFACTOR);
    else
        object->RemoveAttributeAnimation("Scale");

    object->RemoveAttributeAnimation("Position");
    object->SetPosition(CalculateMatchPosition(match.x_, match.y_));
    object->SetScale(OBJECTSCALE);

    Drawable2D* drawable = object->GetDerivedComponent<Drawable2D>();
    drawable->SetLayer(OBJECTLAYER + match.y_);
    drawable->SetOrderInLayer(match.x_);

    object->SetEnabled(true);
}

void MatchGrid::ResetScale(const Match& match)
{
    WeakPtr<Node>& object = objects_(match.x_, match.y_);

    if (!object)
        return;

    if (IsPower(match))
        SetScaleAnimation(object, BONUSFACTOR);
    else
        object->RemoveAttributeAnimation("Scale");

    object->SetScale(OBJECTSCALE);

    Drawable2D* drawable = object->GetDerivedComponent<Drawable2D>();
    drawable->SetLayer(OBJECTLAYER + match.y_);
    drawable->SetOrderInLayer(match.x_);

    object->SetEnabled(true);
}

void MatchGrid::Remove(Match& match)
{
    if (permutation_ && match.x_ == savedM1_.x_ && match.y_ == savedM1_.y_)
    {
        RemoveObject(objects_(savedM2_.x_, savedM2_.y_));
        matches_(savedM2_.x_, savedM2_.y_).property_ = 0;
        permutation_ = false;
//        URHO3D_LOGERRORF("MatchGrid() - Remove a permutted Match : m1=%s m2(removed)=%s", savedM1_.ToString().CString(), savedM2_.ToString().CString());
    }
    else if (permutation_ && match.x_ == savedM2_.x_ && match.y_ == savedM2_.y_)
    {
        RemoveObject(objects_(savedM1_.x_, savedM1_.y_));
        matches_(savedM1_.x_, savedM1_.y_).property_ = 0;
        permutation_ = false;
//        URHO3D_LOGERRORF("MatchGrid() - Remove a permutted Match : m1(removed)=%s m2=%s", savedM1_.ToString().CString(), savedM2_.ToString().CString());
    }
    else
    {
        RemoveObject(objects_(match.x_, match.y_));
        match.property_ = 0;
    }
}

void MatchGrid::Remove(const Match& match)
{
    RemoveObject(objects_(match.x_, match.y_));
    matches_(match.x_, match.y_).property_ = 0;
}

void MatchGrid::RemoveObject(Node* node)
{
    if (node)
    {
        SetAlphaAnimationNew(node);
        TimerRemover::Get()->Start(node, ALPHATIME + 0.1f);
    }
}

void MatchGrid::CleanSavedMatches()
{
    savedM1_.ctype_ = savedM2_.ctype_ = NOTYPE;
    permutation_ = false;
}

void MatchGrid::PermuteMatchType(const Match& e1, const Match& e2)
{
    permutation_ = true;

    Match& m1 = matches_(e1.x_, e1.y_);
    Match& m2 = matches_(e2.x_, e2.y_);
    savedM1_  = m1;
    savedM2_  = m2;
//    URHO3D_LOGINFOF("MatchGrid() - PermuteMatchType : Before m1type=%d m2type=%d ...", m1.ctype_, m2.ctype_);

    m1.property_ = m2.property_;
    m2.property_ = savedM1_.property_;

//    URHO3D_LOGINFOF("MatchGrid() - PermuteMatchType : After m1type=%d m2type=%d !", m1.ctype_, m2.ctype_);
}

void MatchGrid::UndoPermute()
{
    matches_(savedM1_.x_, savedM1_.y_) = savedM1_;
    matches_(savedM2_.x_, savedM2_.y_) = savedM2_;

    CleanSavedMatches();
}

void MatchGrid::ConfirmPermute()
{
//    URHO3D_LOGINFOF("MatchGrid() - ConfirmPermute : Before o1=%u o2=%u ...", objects_(savedM1_.x_, savedM1_.y_).Get(), objects_(savedM2_.x_, savedM2_.y_).Get());

    if (!permutation_)
    {
//        URHO3D_LOGERRORF("MatchGrid() - ConfirmPermute : m1=%s m2=%s : permutation breaked !", savedM1_.ToString().CString(), savedM2_.ToString().CString());
        return;
    }

    WeakPtr<Node> tmpobject = objects_(savedM1_.x_, savedM1_.y_);
    objects_(savedM1_.x_, savedM1_.y_) = objects_(savedM2_.x_, savedM2_.y_);
    objects_(savedM2_.x_, savedM2_.y_) = tmpobject;

    permutation_ = false;

//    URHO3D_LOGINFOF("MatchGrid() - ConfirmPermute : m1=%s m2=%s !", savedM1_.ToString().CString(), savedM2_.ToString().CString());
}

void MatchGrid::UpdateDirectionalTiles()
{
    // remove items on tileexits
    for (Vector<IntVector2>::ConstIterator it = tileexits_.Begin(); it != tileexits_.End(); ++it)
    {
        // Remove current item
        if (matches_(it->x_, it->y_).ctype_)
            Remove(matches_(it->x_, it->y_));
    }

    // move items on tileramps
    if (ramptiles_.Size())
    {
        // gridupdates prevents double moves when a tile changes of direction
        static Matrix2D<int> gridupdates;
        gridupdates.Resize(width_, height_);
        gridupdates.SetBufferValue(-1);

        for (int i=ramptiles_.Size()-1; i >= 0; --i)
        {
            const Vector<IntVector2>& ramptile = ramptiles_[i];
            IntVector2 direction = ramptile.At(0);

            for (int j=ramptile.Size()-1; j > 0; --j)
            {
                const IntVector2& position = ramptile[j];

                // check in grid
                if (direction.x_ != 0)
                {
                    if (direction.x_ > 0)
                    {
                        if (position.x_+direction.x_ >= width_)
                            continue;
                    }
                    else
                    {
                        if (position.x_+direction.x_ < 0)
                            continue;
                    }
                }
                else
                {
                    if (direction.y_ > 0)
                    {
                        if (position.y_+direction.y_ >= height_)
                            continue;
                    }
                    else
                    {
                        if (position.y_+direction.y_ < 0)
                            continue;
                    }
                }

                int& updatedfrom = gridupdates(position.x_, position.y_);
                int& updatedto = gridupdates(position.x_ + direction.x_, position.y_ + direction.y_);

                // gridupdates is needed to prevent double moves when a tile changes of ramp
                if (updatedfrom != -1 && updatedfrom != i)
                    continue;

                Match& mfrom = matches_(position.x_, position.y_);
                Match& mto = matches_(position.x_ + direction.x_, position.y_ + direction.y_);

    //            URHO3D_LOGINFOF("MatchGrid() - UpdateDirectionalTiles : Move=%s dir=%s", position.ToString().CString(), direction.ToString().CString());
                if (mfrom.ctype_ && !mto.ctype_ && grid_(position.x_ + direction.x_, position.y_ + direction.y_).ground_ > GT_VOID)
                {
                    // Move the match to the direction
                    if (objects_(mfrom.x_, mfrom.y_))
                    {
                        mto.property_ = mfrom.property_;
                        mfrom.property_ = 0;
                        WeakPtr<Node>& object = objects_(mto.x_, mto.y_);
                        object = objects_(mfrom.x_, mfrom.y_);
                        objects_(mfrom.x_, mfrom.y_).Reset();
                        object->SetScale(OBJECTSCALE);
                        SetDrawOrder(mto, object, OBJECTLAYER);
                        SetMoveAnimation(mto, object, CalculateMatchPosition(mto.x_, mto.y_), MOVETIME);

                        updatedfrom = i;
                        updatedto = i;
                    }
                    else
                    {
                        URHO3D_LOGERRORF("MatchGrid() - UpdateDirectionalTiles : Error on Move=%s dir=%s no object => reset match", position.ToString().CString(), direction.ToString().CString());
                        mfrom.property_ = 0;
                    }
                }
            }
        }
    }

    // add matches on entrance tiles if it's empty
    if (tileentrances_.Size())
    {
        GameRand& random = GameRand::GetRandomizer(OBJRAND);

        for (Vector<TileEntrance>::ConstIterator it = tileentrances_.Begin(); it != tileentrances_.End(); ++it)
        {
            const TileEntrance& entrance = *it;

            Match& match = matches_(entrance.position_.x_, entrance.position_.y_);
            if (!match.ctype_)
            {
                Node* node = AddObject(match, random);
                if (node)
                {
                    node->SetPosition(CalculateMatchPosition(match.x_-entrance.direction_.x_, match.y_-entrance.direction_.y_));
                    SetMoveAnimation(match, node, CalculateMatchPosition(match.x_, match.y_), MOVETIME);
                }
            }
        }
    }
}



/// Getters

// return local position in grid node (no scene scaled) for a match who has column x and row y
Vector3 MatchGrid::CalculateMatchPosition(int x, int y) const
{
    Vector3 position;

    position.x_ = (2.f*(float)x-(float)width_+1.f) * halfTileSize;
    position.y_ = ((float)height_-2.f*(float)y-1.f) * halfTileSize;

    return position;
}

// return an top entrance local position in grid node (no scene scaled) for a match
Vector3 MatchGrid::GetEntrance(const Match& match) const
{
    Vector3 position;

    position.x_ = (float(2*int(match.x_)-int(width_)) + 1.f) * halfTileSize;
    position.y_ = float(height_) * halfTileSize;

    // add a variation : prevent all node from a column on the same entrance
    position.y_ -= float(2*match.y_) * PIXEL_SIZE;

    return position;
}

// check if the grid contains items (collectables)
bool MatchGrid::ContainsItems() const
{
    for (int y = 0; y < height_; y++)
    for (int x = 0; x < width_; x++)
    {
        const Match& match = matches_(x, y);
        if (IsItem(match))
            return true;
    }

    return false;
}

Vector<const Match*> MatchGrid::GetAllItems() const
{
    Vector<const Match*> items;

    for (int y = 0; y < height_; y++)
    for (int x = 0; x < width_; x++)
    {
        const Match& match = matches_(x, y);
        if (IsItem(match))
            items.Push(&match);
    }

    return items;
}

Vector<Match*> MatchGrid::GetAllPowers()
{
    Vector<Match*> powers;

    for (int y = 0; y < height_; y++)
    for (int x = 0; x < width_; x++)
    {
        Match& match = matches_(x, y);
        if (IsPower(match))
            powers.Push(&match);
    }

    return powers;
}

// check if a world position is in the grid (gridRect_ is scene scaled)
bool MatchGrid::IsInside(const Vector2& position) const
{
    return gridRect_.IsInside(position) == INSIDE;
}

bool MatchGrid::AreMatched(const Match& m1, const Match& m2) const
{
    if (HaveSameMatchType(m1,m2))
        if (HaveNoAdjacentWalls(m1, m2))
            return true;

    return false;
}

bool MatchGrid::HaveSameTypeAndNoWalls(unsigned char ctype, const Match& m1, const Match& m2) const
{
	if (ctype == m2.ctype_)
		if (HaveNoAdjacentWalls(m1, m2))
			return true;

	return false;
}

bool MatchGrid::HaveSameMatchType(const Match& m1, const Match& m2) const
{
    unsigned char ctype = matches_(m1.x_, m1.y_).ctype_;
    return ctype && ctype < ROCKS && ctype == m2.ctype_;
}

bool MatchGrid::HaveSameMatchType(const Match& m1, const Match& m2, const Match& m3, const Match& m4) const
{
    unsigned char ctype = matches_(m1.x_, m1.y_).ctype_;
    return ctype && ctype < ROCKS && ctype == m2.ctype_ && ctype == m3.ctype_ && ctype == m4.ctype_;
}

bool MatchGrid::HasGround(const Match& m) const
{
    return grid_(m.x_, m.y_).ground_ != 0;
}

bool MatchGrid::HasNoWalls(const Match& m, int direction) const
{
    return (grid_(m.x_, m.y_).wallorientation_ & direction) == 0;
}

bool MatchGrid::HasWalls(const Match& m, int direction) const
{
    return (grid_(m.x_, m.y_).wallorientation_ & direction) == direction;
}

bool MatchGrid::HaveNoAdjacentWalls(const Match& m1, const Match& m2) const
{
    if (m1.x_ == m2.x_)
    {
        if (m1.y_ == m2.y_+1)
        {
            if ((grid_(m1.x_, m1.y_).wallorientation_ & WO_WALLNORTH) != 0 || (grid_(m2.x_, m2.y_).wallorientation_ & WO_WALLSOUTH) != 0)
                return false;
        }
        else if (m1.y_ == m2.y_-1)
        {
            if ((grid_(m1.x_, m1.y_).wallorientation_ & WO_WALLSOUTH) != 0 || (grid_(m2.x_, m2.y_).wallorientation_ & WO_WALLNORTH) != 0)
                return false;
        }
    }
    else if (m1.y_ == m2.y_)
    {
        if (m1.x_ == m2.x_+1)
        {
            if ((grid_(m1.x_, m1.y_).wallorientation_ & WO_WALLLEFT) != 0 || (grid_(m2.x_, m2.y_).wallorientation_ & WO_WALLRIGHT) != 0)
                return false;
        }
        else if (m1.x_ == m2.x_-1)
        {
            if ((grid_(m1.x_, m1.y_).wallorientation_ & WO_WALLRIGHT) != 0 || (grid_(m2.x_, m2.y_).wallorientation_ & WO_WALLLEFT) != 0)
                return false;
        }
    }

    return true;
}

bool MatchGrid::IsPower(const Match& match) const
{
    return (match.effect_ != NOEFFECT);
}

bool MatchGrid::IsPower(const Match& match, const IntVector2& effect, int mask) const
{
    return (match.effect_ >= effect.x_ && match.effect_ <= effect.y_ && (match.effect_ & mask));
}

bool MatchGrid::IsItem(const Match& match) const
{
    return (match.ctype_ >= ITEMS && match.ctype_ < ROCKS);
}

bool MatchGrid::IsSelectableObject(const Match& match) const
{
    return (match.ctype_ && match.ctype_ < ROCKS);
}

bool MatchGrid::IsDirectional(unsigned char feature) const
{
    return feature > GT_GRND;
}

bool MatchGrid::IsDirectionalTile(const Match& match) const
{
    return IsDirectional(grid_(match.x_, match.y_).ground_);
}

bool MatchGrid::HasTileEntrances() const
{
    return tileentrances_.Size();
}

bool MatchGrid::HasTileExits() const
{
    return tileexits_.Size();
}

bool MatchGrid::GetBonusesInMatches(const Vector<Match*>& matches, Vector<Match*>& bonuses, const IntVector2& effect, int mask)
{
    for (Vector<Match*>::ConstIterator it=matches.Begin(); it!=matches.End(); ++it)
    {
		Match* m = *it;
        if (IsPower(*m, effect, mask))
            bonuses.Push(m);
    }

    return bonuses.Size();
}

Match* MatchGrid::GetFirstBonusInMatches(const Vector<Match*>& matches, const IntVector2& effect, int mask)
{
    for (Vector<Match*>::ConstIterator it=matches.Begin(); it!=matches.End(); ++it)
    {
		Match* m = *it;
        if (IsPower(*m, effect, mask))
            return m;
    }

    return 0;
}

Vector<Match*> MatchGrid::GetPermuttedMatches()
{
    Vector<Match*> matches;

    matches.Push(&matches_(savedM1_.x_, savedM1_.y_));
    matches.Push(&matches_(savedM2_.x_, savedM2_.y_));

    return matches;
}

Match* MatchGrid::GetMatch(Node* object)
{
    if (object && object->GetVar(GOA::GRIDCOORD) != Variant::EMPTY)
    {
        const IntVector3& coord = object->GetVar(GOA::GRIDCOORD).GetIntVector3();
        return &matches_(coord.x_, coord.y_);
    }

    return 0;
}

Node* MatchGrid::GetObject(const Match& m) const
{
    return objects_(m.x_, m.y_).Get();
}

Node* MatchGrid::GetObject(int x, int y) const
{
    return width_ * height_ > 0 ? objects_(x, y).Get() : 0;
}


/// MATCHES

bool MatchGrid::GetMatches(Match* match, Vector<Match*>& destroymatches, Vector<Match*>& successmatches, Vector<Match*>& activablebonuses, Vector<Match*>& brokenrocks, Vector<WallInfo>& hittedwalls)
{
    if (!grid_(match->x_, match->y_).ground_)
        return false;

    Match& entry = matches_(match->x_, match->y_);

    if (IsItem(entry))
        return false;

//    URHO3D_LOGINFOF("MatchGrid() - GetMatches : entry=%s ...", entry.ToString().CString());

	// "Spread Bomb" Matches
	if (activedRules_[SPREADBOMBMATCH] && IsPower(entry, SPREADBOMB))
	{
		Vector<Match*> matches;

		CheckMatches_NeighborHood(entry, matches);

		if (entry.effect_ < ROCKEXPLOSION)
		{
			URHO3D_LOGINFOF("MatchGrid() - GetMatches : entry=%s ... SPREADBOMBMATCH ...success=%u", entry.ToString().CString(), matches.Size());
			if (matches.Size() >= Match::MINIMALMATCHES)
			{
			    // add effects on matches
			    for (Vector<Match*>::ConstIterator mt=matches.Begin(); mt != matches.End(); ++mt)
                    (*mt)->effect_ = entry.effect_;

			    // add entries
				Match::AddDistinctEntries(matches, successmatches);
				Match::AddDistinctEntries(successmatches, activablebonuses);
				Match::AddDistinctEntries(matches, destroymatches);
				return true;
			}
		}
		else
		{
			URHO3D_LOGINFOF("MatchGrid() - GetMatches : entry=%s ... SPREADBOMBMATCH ROCKS ...success=%u", entry.ToString().CString(), matches.Size());
			if (matches.Size() > 1)
			{
			    // add effects on matches
			    for (Vector<Match*>::ConstIterator mt=matches.Begin(); mt != matches.End(); ++mt)
                    (*mt)->effect_ = entry.effect_;

				Match::AddDistinctEntries(matches, successmatches);
				Match::AddDistinctEntries(successmatches, activablebonuses);
				Match::AddDistinctEntries(matches, destroymatches);
				return true;
			}
		}
	}

	if (activedRules_[WALLBOMBMATCH] && IsPower(entry, WALLBOMB))
	{
		GetAllWallsAround(entry, 1, hittedwalls);
		if (hittedwalls.Size())
		{
			Match::AddDistinctEntry(&entry, activablebonuses);
			Match::AddDistinctEntry(&entry, destroymatches);
			return true;
		}
	}

    // "Bomb" Matches
    if (activedRules_[BOMBMATCH] && IsPower(entry, DIRECTIONALEXPLOSION))
    {
        Vector<Match*> bonush;
        Vector<Match*> bonusv;

        CheckMatches_Horizontal_Bonus(entry, bonush);
        if (bonush.Size() > 0)
        {
            Match::AddDistinctEntry(&entry, bonush);
            Match::AddDistinctEntries(bonush, successmatches);
            Match::AddDistinctEntries(successmatches, activablebonuses);

            URHO3D_LOGINFOF("MatchGrid() - GetMatches : entry=%s ... BOMBMATCH horiz=%u ...", entry.ToString().CString(), successmatches.Size());

            Vector<Match*> bonus2;
            for (int i=0; i < bonush.Size(); i++)
            {
                if (IsPower(*bonush[i], DIRECTIONALEXPLOSION, YEXPLOSION))
                    GetAllVerticalMatches(*bonush[i], bonus2, brokenrocks, &hittedwalls);
                if (IsPower(*bonush[i], DIRECTIONALEXPLOSION, XEXPLOSION))
                    GetAllHorizontalMatches(*bonush[i], bonus2, brokenrocks, &hittedwalls);
            }
            Match::AddDistinctEntries(bonus2, destroymatches);

            if (entry.effect_ & XEXPLOSION)
            {
                GetAllHorizontalMatches(entry, bonush, brokenrocks, &hittedwalls);
                Match::AddDistinctEntries(bonush, destroymatches);
            }
        }

        CheckMatches_Vertical_Bonus(entry, bonusv);
        if (bonusv.Size() > 0)
        {
            Match::AddDistinctEntry(&entry, bonusv);
            Match::AddDistinctEntries(bonusv, successmatches);
            Match::AddDistinctEntries(successmatches, activablebonuses);

            URHO3D_LOGINFOF("MatchGrid() - GetMatches : entry=%s ... BOMBMATCH verti=%u ...", entry.ToString().CString(), successmatches.Size());

            Vector<Match*> bonus2;
            for (int i=0; i < bonusv.Size(); i++)
            {
                if (IsPower(*bonusv[i], DIRECTIONALEXPLOSION, YEXPLOSION))
                    GetAllVerticalMatches(*bonusv[i], bonus2, brokenrocks, &hittedwalls);
                if (IsPower(*bonusv[i], DIRECTIONALEXPLOSION, XEXPLOSION))
                    GetAllHorizontalMatches(*bonusv[i], bonus2, brokenrocks, &hittedwalls);
            }
            Match::AddDistinctEntries(bonus2, destroymatches);

            if (entry.effect_ & YEXPLOSION)
            {
                GetAllVerticalMatches(entry, bonusv, brokenrocks, &hittedwalls);
                Match::AddDistinctEntries(bonusv, destroymatches);
            }
        }

        if (bonush.Size() > 1 || bonusv.Size() > 1)
            return true;
    }

    // "Square" Matches
    if (activedRules_[SQUAREMATCH])
    {
        Vector<Match*> matchesq;

        matchesq.Push(&entry);
        CheckMatches_Squares(entry, matchesq);

        if (matchesq.Size() > Match::MINIMALMATCHES)
        {
            Match::AddDistinctEntries(matchesq, successmatches);

			// TODO
//            entry.effect_ = SQREXPLOSION;
//            Match::AddDistinctEntry(&entry, activablebonuses);
            Match::AddDistinctEntries(matchesq, destroymatches);

            URHO3D_LOGINFOF("MatchGrid() - GetMatches : entry=%s - SQUAREMATCH NumMatches=%u SUCCESS !", entry.ToString().CString(), matchesq.Size());
        }
    }

    if (!freeDirectionExplosion_)
    {
        bool horizBonusActive = false;

        // Horizontal Matches
        if (activedRules_[HORIZONTALMATCH])
        {
            Vector<Match*> matchesh;

            matchesh.Push(&entry);
            CheckMatches_Horizontal(entry, matchesh);

            if (matchesh.Size() >= Match::MINIMALMATCHES)
            {
                Match::AddDistinctEntries(matchesh, successmatches);

                Vector<Match*> bonuses;
                if (GetBonusesInMatches(matchesh, bonuses, DIRECTIONALEXPLOSION))
                {
                    // Check HorizontalBomb in the matches
                    Match* firstpower = GetFirstBonusInMatches(bonuses, DIRECTIONALEXPLOSION, XEXPLOSION);

                    if (firstpower)
                    {
                        URHO3D_LOGINFOF("MatchGrid() - GetMatches : entry=%s - HORIZONTAL HasPowers=%u !", entry.ToString().CString(), bonuses.Size());

                        matchesh.Clear();

                        GetAllHorizontalMatches(entry, matchesh, brokenrocks, &hittedwalls);

                        horizBonusActive = true;

                        if (bonuses.Size() == 1)
                        {
                            firstpower->effect_ &= ~YEXPLOSION;
                        }
                        else
                        {
                            for (Vector<Match*>::ConstIterator it=bonuses.Begin(); it!=bonuses.End(); ++it)
                            {
                                Match* power = *it;

                                if (power == firstpower)
                                    continue;

                                if (IsPower(*power, DIRECTIONALEXPLOSION, XEXPLOSION))
                                {
                                    GetAllVerticalMatches(*power, matchesh, brokenrocks, &hittedwalls);
                                    power->effect_ &= ~XEXPLOSION;
                                }
                            }
                        }

                        activablebonuses.Push(bonuses);
                    }
                }

                Match::AddDistinctEntries(matchesh, destroymatches);

                URHO3D_LOGINFOF("MatchGrid() - GetMatches : entry=%s - HORIZONTAL NumMatches=%u SUCCESS !", entry.ToString().CString(), matchesh.Size());
            }
        }

        // Vertical Matches
        if (activedRules_[VERTICALMATCH])
        {
            Vector<Match*> matchesv;

            matchesv.Push(&entry);
            CheckMatches_Vertical(entry, matchesv);

            if (matchesv.Size() >= Match::MINIMALMATCHES)
            {
                Match::AddDistinctEntries(matchesv, successmatches);

                // Check VerticalBomb in the matches
                Vector<Match*> bonuses;
                if (GetBonusesInMatches(matchesv, bonuses, DIRECTIONALEXPLOSION))
                {
                    // Check HorizontalBomb in the matches
                    Match* firstpower = GetFirstBonusInMatches(bonuses, DIRECTIONALEXPLOSION, YEXPLOSION);

                    if (firstpower)
                    {
                        URHO3D_LOGINFOF("MatchGrid() - GetMatches : entry=%s - VERTICAL HasPowers=%u !", entry.ToString().CString(), bonuses.Size());

                        matchesv.Clear();

                        GetAllVerticalMatches(entry, matchesv, brokenrocks, &hittedwalls);

                        if (!horizBonusActive && bonuses.Size() == 1)
                            firstpower->effect_ &= ~XEXPLOSION;
                        else
                        {
                            for (Vector<Match*>::ConstIterator it=bonuses.Begin(); it!=bonuses.End(); ++it)
                            {
                                Match* power = *it;

                                if (power == firstpower)
                                    continue;

                                if (IsPower(*power, DIRECTIONALEXPLOSION, YEXPLOSION))
                                {
                                    GetAllHorizontalMatches(*power, matchesv, brokenrocks, &hittedwalls);
                                    power->effect_ &= ~YEXPLOSION;
                                }
                            }
                        }

                        activablebonuses.Push(bonuses);
                    }
                }

    //            if (GetBonusesInMatches(matchesv, bonuses, DIRECTIONALEXPLOSION, YEXPLOSION))
    //            {
    //                matchesv.Clear();
    //
    //                GetAllVerticalMatches(entry, matchesv, brokenrocks, &hittedwalls);
    //
    //                activablebonuses.Push(bonuses);
    //
    //                Match* bonus = *bonuses.Begin();
    //
    //                if (!horizBonusActive)
    //                    bonus->effect_ &= ~XEXPLOSION;
    //
    //                for (Vector<Match*>::ConstIterator it=bonuses.Begin()+1; it!=bonuses.End(); ++it)
    //                {
    //                    GetAllHorizontalMatches(**it, matchesv, brokenrocks, &hittedwalls);
    //                    (*it)->effect_ &= ~YEXPLOSION;
    //                }
    //            }

                Match::AddDistinctEntries(matchesv, destroymatches);

                URHO3D_LOGINFOF("MatchGrid() - GetMatches : entry=%s - VERTICAL NumMatches=%u SUCCESS !", entry.ToString().CString(), matchesv.Size());
            }
        }
    }
    else
    {
        Vector<Match*> matchesh;
        Vector<Match*> matchesv;

        // Horizontal Matches
        if (activedRules_[HORIZONTALMATCH])
        {
            matchesh.Push(&entry);
            CheckMatches_Horizontal(entry, matchesh);

            if (matchesh.Size() >= Match::MINIMALMATCHES)
            {
                unsigned nummatches = matchesh.Size();

                Match::AddDistinctEntries(matchesh, successmatches);
                Match::AddDistinctEntries(matchesh, destroymatches);

                Vector<Match*> bonuses;
                if (GetBonusesInMatches(matchesh, bonuses, DIRECTIONALEXPLOSION))
                {
                    for (Vector<Match*>::ConstIterator it=bonuses.Begin(); it!=bonuses.End(); ++it)
                    {
                        Match* power = *it;
                        if (IsPower(*power, DIRECTIONALEXPLOSION, XEXPLOSION))
                        {
                            GetAllHorizontalMatches(*power, matchesh, brokenrocks, &hittedwalls);
                            Match::AddDistinctEntries(matchesh, destroymatches);
                            nummatches += matchesh.Size();
                        }
                        if (IsPower(*power, DIRECTIONALEXPLOSION, YEXPLOSION))
                        {
                            GetAllVerticalMatches(*power, matchesv, brokenrocks, &hittedwalls);
                            Match::AddDistinctEntries(matchesv, destroymatches);
                            nummatches += matchesv.Size();
                        }
                    }

                    activablebonuses.Push(bonuses);
                }

                URHO3D_LOGINFOF("MatchGrid() - GetMatches : entry=%s - free HORIZONTAL NumMatches=%u SUCCESS !", entry.ToString().CString(), nummatches);
            }
        }

        // Vertical Matches
        if (activedRules_[VERTICALMATCH])
        {
            matchesh.Clear();
            matchesv.Clear();

            matchesv.Push(&entry);
            CheckMatches_Vertical(entry, matchesv);

            if (matchesv.Size() >= Match::MINIMALMATCHES)
            {
                unsigned nummatches = matchesv.Size();

                Match::AddDistinctEntries(matchesv, successmatches);
                Match::AddDistinctEntries(matchesv, destroymatches);

                Vector<Match*> bonuses;
                if (GetBonusesInMatches(matchesv, bonuses, DIRECTIONALEXPLOSION))
                {
                    for (Vector<Match*>::ConstIterator it=bonuses.Begin(); it!=bonuses.End(); ++it)
                    {
                        Match* power = *it;
                        if (IsPower(*power, DIRECTIONALEXPLOSION, XEXPLOSION))
                        {
                            GetAllHorizontalMatches(*power, matchesh, brokenrocks, &hittedwalls);
                            Match::AddDistinctEntries(matchesh, destroymatches);
                            nummatches += matchesh.Size();
                        }
                        if (IsPower(*power, DIRECTIONALEXPLOSION, YEXPLOSION))
                        {
                            GetAllVerticalMatches(*power, matchesv, brokenrocks, &hittedwalls);
                            Match::AddDistinctEntries(matchesv, destroymatches);
                            nummatches += matchesv.Size();
                        }
                    }

                    activablebonuses.Push(bonuses);
                }

                URHO3D_LOGINFOF("MatchGrid() - GetMatches : entry=%s - free VERTICAL NumMatches=%u SUCCESS !", entry.ToString().CString(), nummatches);
            }
        }
    }

    // "L" Matches
    if (activedRules_[LMATCH])
    {
        /// TODO
        Vector<Match*> matchesl;
    }

    return destroymatches.Size() >= Match::MINIMALMATCHES;
}

void MatchGrid::CheckMatches_Horizontal(const Match& entry, Vector<Match*>& matches)
{
    // check left
    if (entry.x_ > 0)
        for (int x=entry.x_-1; x >= 0; x--)
        {
            Match& test = matches_(x, entry.y_);

//            if (!HasGround(test))
//                continue;

            if (!HaveSameMatchType(entry , test))
                break;

            if ((grid_(x, entry.y_).wallorientation_ & WO_WALLRIGHT) != 0 || (grid_(x+1, entry.y_).wallorientation_ & WO_WALLLEFT) != 0)
                break;

            matches.Push(&test);
        }

    // check right
    if (entry.x_ < width_-1)
        for (int x=entry.x_+1; x < width_; x++)
        {
            Match& test = matches_(x, entry.y_);

//            if (!HasGround(test))
//                continue;

            if (!HaveSameMatchType(entry , test))
                break;

            if ((grid_(x, entry.y_).wallorientation_ & WO_WALLLEFT) != 0 || (grid_(x-1, entry.y_).wallorientation_ & WO_WALLRIGHT) != 0)
                break;

            matches.Push(&test);
        }
}

void MatchGrid::CheckMatches_Horizontal_Bonus(const Match& entry, Vector<Match*>& matches)
{
    if (!IsPower(entry))
        return;

    // check left
    if (entry.x_ > 0)
        for (int x=entry.x_-1; x >= 0; x--)
        {
            Match& test = matches_(x, entry.y_);

//            if (!HasGround(test))
//                continue;

            if (!IsPower(test))
                break;

            if (!HaveSameMatchType(entry , test))
                break;

            if ((grid_(x, entry.y_).wallorientation_ & WO_WALLRIGHT) != 0 || (grid_(x+1, entry.y_).wallorientation_ & WO_WALLLEFT) != 0)
                break;

            matches.Push(&test);
        }

    // check right
    if (entry.x_ < width_-1)
        for (int x=entry.x_+1; x < width_; x++)
        {
            Match& test = matches_(x, entry.y_);

//            if (!HasGround(test))
//                continue;

            if (!IsPower(test))
                break;

            if (!HaveSameMatchType(entry , test))
                break;

            if ((grid_(x, entry.y_).wallorientation_ & WO_WALLLEFT) != 0 || (grid_(x-1, entry.y_).wallorientation_ & WO_WALLRIGHT) != 0)
                break;

            matches.Push(&test);
        }
}

void MatchGrid::CheckMatches_Vertical(const Match& entry, Vector<Match*>& matches)
{
    // check top
    if (entry.y_ > 0)
        for (int y=entry.y_-1; y >= 0; y--)
        {
            Match& test = matches_(entry.x_, y);

//            if (!HasGround(test))
//                continue;

            if (!HaveSameMatchType(entry , test))
                break;

            if ((grid_(entry.x_, y).wallorientation_ & WO_WALLSOUTH) != 0 || (grid_(entry.x_, y+1).wallorientation_ & WO_WALLNORTH) != 0)
                break;

            matches.Push(&test);
        }

    // check bottom
    if (entry.y_ < height_-1)
        for (int y=entry.y_+1; y < height_; y++)
        {
            Match& test = matches_(entry.x_, y);

//            if (!HasGround(test))
//                continue;

            if (!HaveSameMatchType(entry , test))
                break;

            if ((grid_(entry.x_, y).wallorientation_ & WO_WALLNORTH) != 0 || (grid_(entry.x_, y-1).wallorientation_ & WO_WALLSOUTH) != 0)
                break;

            matches.Push(&test);
        }
}

void MatchGrid::CheckMatches_Vertical_Bonus(const Match& entry, Vector<Match*>& matches)
{
    if (!IsPower(entry))
        return;

    // check top
    if (entry.y_ > 0)
        for (int y=entry.y_-1; y >= 0; y--)
        {
            Match& test = matches_(entry.x_, y);

//            if (!HasGround(test))
//                continue;

            if (!IsPower(test))
                break;

            if (!HaveSameMatchType(entry , test))
                break;

            if ((grid_(entry.x_, y).wallorientation_ & WO_WALLSOUTH) != 0 || (grid_(entry.x_, y+1).wallorientation_ & WO_WALLNORTH) != 0)
                break;

            matches.Push(&test);
        }

    // check bottom
    if (entry.y_ < height_-1)
        for (int y=entry.y_+1; y < height_; y++)
        {
            Match& test = matches_(entry.x_, y);

//            if (!HasGround(test))
//                continue;

            if (!IsPower(test))
                break;

            if (!HaveSameMatchType(entry , test))
                break;

            if ((grid_(entry.x_, y).wallorientation_ & WO_WALLNORTH) != 0 || (grid_(entry.x_, y-1).wallorientation_ & WO_WALLSOUTH) != 0)
                break;

            matches.Push(&test);
        }
}

void MatchGrid::CheckMatches_Squares(const Match& entry, Vector<Match*>& matches)
{
    // e X
    // X X
    if (entry.y_ < height_-1 && entry.x_ < width_-1 && AreMatched(entry, matches_(entry.x_+1, entry.y_)) && AreMatched(entry, matches_(entry.x_, entry.y_+1))
        && AreMatched(matches_(entry.x_, entry.y_+1), matches_(entry.x_+1, entry.y_+1)) && AreMatched(matches_(entry.x_+1, entry.y_), matches_(entry.x_+1, entry.y_+1)))
    {
        matches.Push(&matches_(entry.x_+1, entry.y_));
        matches.Push(&matches_(entry.x_, entry.y_+1));
        matches.Push(&matches_(entry.x_+1, entry.y_+1));
    }
    // X e
    // X X
    else if (entry.y_ < height_-1 && entry.x_ > 0 && AreMatched(entry, matches_(entry.x_-1, entry.y_)) && AreMatched(entry, matches_(entry.x_, entry.y_+1))
        && AreMatched(matches_(entry.x_-1, entry.y_), matches_(entry.x_-1, entry.y_+1)) && AreMatched(matches_(entry.x_, entry.y_+1), matches_(entry.x_-1, entry.y_+1)))
    {
        matches.Push(&matches_(entry.x_-1, entry.y_));
        matches.Push(&matches_(entry.x_-1, entry.y_+1));
        matches.Push(&matches_(entry.x_, entry.y_+1));
    }
    // X X
    // e X
    else if (entry.y_ > 0 && entry.x_ < width_-1 && AreMatched(entry, matches_(entry.x_+1, entry.y_)) && AreMatched(entry, matches_(entry.x_, entry.y_-1))
        && AreMatched(matches_(entry.x_, entry.y_-1), matches_(entry.x_+1, entry.y_-1)) && AreMatched(matches_(entry.x_+1, entry.y_), matches_(entry.x_+1, entry.y_-1)))
    {
        matches.Push(&matches_(entry.x_+1, entry.y_));
        matches.Push(&matches_(entry.x_, entry.y_-1));
        matches.Push(&matches_(entry.x_+1, entry.y_-1));
    }
    // X X
    // X e
    else if (entry.y_ > 0 && entry.x_ > 0 && AreMatched(entry, matches_(entry.x_-1, entry.y_)) && AreMatched(entry, matches_(entry.x_, entry.y_-1))
        && AreMatched(matches_(entry.x_-1, entry.y_), matches_(entry.x_-1, entry.y_-1)) && AreMatched(matches_(entry.x_, entry.y_-1), matches_(entry.x_-1, entry.y_-1)))
    {
        matches.Push(&matches_(entry.x_-1, entry.y_));
        matches.Push(&matches_(entry.x_-1, entry.y_-1));
        matches.Push(&matches_(entry.x_, entry.y_-1));
    }
}

void MatchGrid::CheckMatches_L(const Match& entry, Vector<Match*>& matches)
{
    /// TODO
}

void MatchGrid::CheckMatches_NeighborHood(const Match& entry, Vector<Match*>& matches)
{
	List<Match* > white;
	List<Match* > black;

	white.Push(&matches_(entry.x_, entry.y_));

	int range = 2;
	unsigned char ctype = entry.ctype_;

	if (entry.effect_ < ROCKEXPLOSION)
	{
		range += entry.effect_ - ELECEXPLOSION;
	}
	else
	{
		range += entry.effect_ - ROCKEXPLOSION;
		ctype = ROCKS;
	}

    int xmin = 0;
    int xmax = width_-1;
    int ymin = 0;
    int ymax = height_-1;

    if (entry.x_ > range)
        xmin = entry.x_ - range;
    if (entry.y_ > range)
        ymin = entry.y_ - range;
    if (entry.x_ + range < width_-1)
        xmax = entry.x_ + range;
    if (entry.y_ + range < height_-1)
        ymax = entry.y_ + range;

    URHO3D_LOGINFOF("MatchGrid() - CheckMatches_NeighborHood : entry=%s ctype=%u ... range=%d (%d %d %d %d)...",
                    entry.ToString().CString(), ctype, range, xmin, ymin, xmax, ymax);

	while (white.Size())
	{
		Match* match = white.Back();
		white.Pop();

		if (black.Contains(match))
			continue;

		if (match->ctype_ == ctype)
		{
			matches.Push(match);
		}
		else if (ctype == ROCKS)
		{
			if (white.Size() > 0)
				continue;

			matches.Push(match);
		}

		int x = match->x_;
		int y = match->y_;

		if (y > ymin && HaveSameTypeAndNoWalls(ctype, entry, matches_(x, y - 1)))
			white.Push(&matches_(x, y - 1));
		if (y < ymax && HaveSameTypeAndNoWalls(ctype, entry, matches_(x, y + 1)))
			white.Push(&matches_(x, y + 1));

		if (x > xmin)
		{
			if (HaveSameTypeAndNoWalls(ctype, entry, matches_(x - 1, y)))
				white.Push(&matches_(x - 1, y));
			if (y > ymin && HaveSameTypeAndNoWalls(ctype, entry, matches_(x - 1, y - 1)))
				white.Push(&matches_(x - 1, y - 1));
			if (y < ymax && HaveSameTypeAndNoWalls(ctype, entry, matches_(x - 1, y + 1)))
				white.Push(&matches_(x - 1, y + 1));
		}
		if (x < xmax)
		{
			if (HaveSameTypeAndNoWalls(ctype, entry, matches_(x + 1, y)))
				white.Push(&matches_(x + 1, y));
			if (y > ymin && HaveSameTypeAndNoWalls(ctype, entry, matches_(x + 1, y - 1)))
				white.Push(&matches_(x + 1, y - 1));
			if (y < ymax && HaveSameTypeAndNoWalls(ctype, entry, matches_(x + 1, y + 1)))
				white.Push(&matches_(x + 1, y + 1));
		}

		black.Push(match);
	}
}


/// HINTS


bool MatchGrid::IsPowerActivable(Match& X)
{
    Vector<Vector<Match*> > hintstable;

    CheckHints_Explosion(X, hintstable);
    CheckHints_Square(X, hintstable);
    CheckHints_Horizontal(X, hintstable);
    CheckHints_Vertical(X, hintstable);

    return hintstable.Size();
}


void MatchGrid::GetHints(unsigned imatch, Vector<Vector<Match*> >& hintstable)
{
    Match& X = matches_[imatch];

    if (IsItem(X))
    {
        Vector<Match*> matches;
        matches.Push(&X);
        hintstable.Push(matches);
    }
    else
    {
        unsigned oldhintsize = hintstable.Size();

        CheckHints_Explosion(X, hintstable);
        CheckHints_Square(X, hintstable);
        CheckHints_Horizontal(X, hintstable);
        CheckHints_Vertical(X, hintstable);
        /// TODO
        // CheckHints_L(X, hintstable);

        // Add Power Tutorial
        if (GameStatics::playerState_->tutorialEnabled_ && IsPower(X) && hintstable.Size() > oldhintsize)
        {
            int powerid = X.otype_+1;
            if (!powerid)
            {
                URHO3D_LOGERRORF("MatchGrid() - GetHints : Check Tutorial => Power Error !");
                return;
            }

            if (GameStatics::playerState_->powers_[powerid-1].shown_ > 0)
                return;

            Node* node = GetObject(X);
            if (!node)
                return;

            URHO3D_LOGINFOF("MatchGrid() - GetHints : send GAME_POWERADDED for Match=%s hintsize=%u !", X.ToString().CString(), hintstable.Size());
            VariantMap& eventdata = GameStatics::context_->GetEventDataMap();
            eventdata[Game_PowerAdded::MATCHPROPERTY] = X.property_;
            eventdata[Game_PowerAdded::NODE] = node;
            node->SendEvent(GAME_POWERADDED, eventdata);
        }
    }
}


void MatchGrid::CheckHints_Explosion(Match& X, Vector<Vector<Match*> >& hints)
{
    if (!IsPower(X))
        return;

    // Schema
    //         W .
    //         v X
    //         Z .
    if (X.x_ > 0)
    {
        Match& v = matches_(X.x_-1, X.y_);

        if (IsSelectableObject(v) && HasNoWalls(X, WO_WALLLEFT) && HasNoWalls(v, WO_WALLRIGHT))
        {
            if (X.y_ > 0 && HasNoWalls(v, WO_WALLNORTH))
            {
                Match& W = matches_(X.x_-1, X.y_-1);
                if (HaveSameMatchType(X, W))
                    if (IsPower(W) && HasNoWalls(W, WO_WALLSOUTH))
                    {
                        Vector<Match*> matches;
                        matches.Push(&X);
                        matches.Push(&W);
                        hints.Push(matches);
                    }
            }
            if (X.y_ < height_-1 && HasNoWalls(v, WO_WALLSOUTH))
            {
                Match& Z = matches_(X.x_-1, X.y_+1);
                if (HaveSameMatchType(X, Z))
                    if (IsPower(Z) && HasNoWalls(Z, WO_WALLNORTH))
                    {
                        Vector<Match*> matches;
                        matches.Push(&X);
                        matches.Push(&Z);
                        hints.Push(matches);
                    }
            }
        }
    }

    // Schema
    //         . W
    //         X v
    //         . Z
    if (X.x_ < width_-1)
    {
        Match& v = matches_(X.x_+1, X.y_);

        if (IsSelectableObject(v) && HasNoWalls(X, WO_WALLRIGHT) && HasNoWalls(v, WO_WALLLEFT))
        {
            if (X.y_ > 0 && HasNoWalls(v, WO_WALLNORTH))
            {
                Match& W = matches_(X.x_+1, X.y_-1);
                if (HaveSameMatchType(X, W))
                    if (IsPower(W) && HasNoWalls(W, WO_WALLSOUTH))
                    {
                        Vector<Match*> matches;
                        matches.Push(&X);
                        matches.Push(&W);
                        hints.Push(matches);
                    }
            }
            if (X.y_ < height_-1 && HasNoWalls(v, WO_WALLSOUTH))
            {
                Match& Z = matches_(X.x_+1, X.y_+1);
                if (HaveSameMatchType(X, Z))
                    if (IsPower(Z) && HasNoWalls(Z, WO_WALLNORTH))
                    {
                        Vector<Match*> matches;
                        matches.Push(&X);
                        matches.Push(&Z);
                        hints.Push(matches);
                    }
            }
        }
    }

    // Schema
    //         W v Z
    //         . X .
    if (X.y_ > 0)
    {
        Match& v = matches_(X.x_, X.y_-1);

        if (IsSelectableObject(v) && HasNoWalls(X, WO_WALLNORTH) && HasNoWalls(v, WO_WALLSOUTH))
        {
            if (X.x_ > 0 && HasNoWalls(v, WO_WALLLEFT))
            {
                Match& W = matches_(X.x_-1, X.y_-1);
                if (HaveSameMatchType(X, W))
                    if (IsPower(W) && HasNoWalls(W, WO_WALLRIGHT))
                    {
                        Vector<Match*> matches;
                        matches.Push(&X);
                        matches.Push(&W);
                        hints.Push(matches);
                    }
            }
            if (X.x_ < width_-1 && HasNoWalls(v, WO_WALLRIGHT))
            {
                Match& Z = matches_(X.x_+1, X.y_-1);
                if (HaveSameMatchType(X, Z))
                    if (IsPower(Z) && HasNoWalls(Z, WO_WALLLEFT))
                    {
                        Vector<Match*> matches;
                        matches.Push(&X);
                        matches.Push(&Z);
                        hints.Push(matches);
                    }
            }
        }
    }

    // Schema
    //         . X .
    //         W v Z
    if (X.y_ < height_-1)
    {
        Match& v = matches_(X.x_, X.y_+1);

        if (IsSelectableObject(v) && HasNoWalls(X, WO_WALLSOUTH) && HasNoWalls(v, WO_WALLNORTH))
        {
            if (X.x_ > 0 && HasNoWalls(v, WO_WALLLEFT))
            {
                Match& W = matches_(X.x_-1, X.y_+1);
                if (HaveSameMatchType(X, W))
                    if (IsPower(W) && HasNoWalls(W, WO_WALLRIGHT))
                    {
                        Vector<Match*> matches;
                        matches.Push(&X);
                        matches.Push(&W);
                        hints.Push(matches);
                    }
            }
            if (X.x_ < width_-1 && HasNoWalls(v, WO_WALLRIGHT))
            {
                Match& Z = matches_(X.x_+1, X.y_+1);
                if (HaveSameMatchType(X, Z))
                    if (IsPower(Z) && HasNoWalls(Z, WO_WALLLEFT))
                    {
                        Vector<Match*> matches;
                        matches.Push(&X);
                        matches.Push(&Z);
                        hints.Push(matches);
                    }
            }
        }
    }
}

void MatchGrid::CheckHints_Horizontal(Match& X, Vector<Vector<Match*> >& hints)
{
    // Schema
    //         W . .
    //         v X Y
    //         Z . .
    if (X.x_ > 0 && X.x_ < width_-1)
    {
        Match& v = matches_(X.x_-1, X.y_);

        if (IsSelectableObject(v))
        {
            Match& Y = matches_(X.x_+1, X.y_);

            if (HaveSameMatchType(X, Y) && HasNoWalls(X, WO_WALLRIGHT | WO_WALLLEFT) && HasNoWalls(Y, WO_WALLLEFT) && HasNoWalls(v, WO_WALLRIGHT))
            {
                if (X.y_ >= 1 && HasNoWalls(v, WO_WALLNORTH))
                {
                    Match& W = matches_(X.x_-1, X.y_-1);
                    if (HaveSameMatchType(X, W) && HasNoWalls(W, WO_WALLSOUTH))
                    {
                        Vector<Match*> matches;
                        matches.Push(&X);
                        matches.Push(&Y);
                        matches.Push(&W);
                        hints.Push(matches);
                    }
                }
                if (X.y_ < height_-1 && HasNoWalls(v, WO_WALLSOUTH))
                {
                    Match& Z = matches_(X.x_-1, X.y_+1);
                    if (HaveSameMatchType(X, Z) && HasNoWalls(Z, WO_WALLNORTH))
                    {
                        Vector<Match*> matches;
                        matches.Push(&X);
                        matches.Push(&Y);
                        matches.Push(&Z);
                        hints.Push(matches);
                    }
                }
            }
        }
    }
    // Schema
    //         . . W
    //         X Y v
    //         . . Z
    if (X.x_ < width_-2)
    {
        Match& v = matches_(X.x_+2, X.y_);

        if (IsSelectableObject(v))
        {
            Match& Y = matches_(X.x_+1, X.y_);

            if (HaveSameMatchType(X, Y) && HasNoWalls(Y, WO_WALLRIGHT | WO_WALLLEFT) && HasNoWalls(X, WO_WALLRIGHT) && HasNoWalls(v, WO_WALLLEFT))
            {
                if (X.y_ >= 1 && HasNoWalls(v, WO_WALLNORTH))
                {
                    Match& W = matches_(X.x_+2, X.y_-1);
                    if (HaveSameMatchType(X, W) && HasNoWalls(W, WO_WALLSOUTH))
                    {
                        Vector<Match*> matches;
                        matches.Push(&X);
                        matches.Push(&Y);
                        matches.Push(&W);
                        hints.Push(matches);
                    }
                }
                if (X.y_ < height_-1 && HasNoWalls(v, WO_WALLSOUTH))
                {
                    Match& Z = matches_(X.x_+2, X.y_+1);
                    if (HaveSameMatchType(X, Z) && HasNoWalls(Z, WO_WALLNORTH))
                    {
                        Vector<Match*> matches;
                        matches.Push(&X);
                        matches.Push(&Y);
                        matches.Push(&Z);
                        hints.Push(matches);
                    }
                }
            }
        }
    }
    // Schema
    //         . . . .
    //         X Y v Z
    //         . . . .
    if (X.x_ < width_-3)
    {
        Match& v = matches_(X.x_+2, X.y_);

        if (IsSelectableObject(v))
        {
            Match& Y = matches_(X.x_+1, X.y_);
            Match& Z = matches_(X.x_+3, X.y_);

            if (HaveSameMatchType(X, Y) && HaveSameMatchType(X, Z) && !HaveSameMatchType(X, v) &&
                HasNoWalls(Z, WO_WALLLEFT) && HasNoWalls(Y, WO_WALLRIGHT | WO_WALLLEFT) && HasNoWalls(v, WO_WALLRIGHT | WO_WALLLEFT) && HasNoWalls(X, WO_WALLRIGHT))
            {
                Vector<Match*> matches;
                matches.Push(&X);
                matches.Push(&Y);
                matches.Push(&Z);
                hints.Push(matches);
            }
        }
    }
    // Schema
    //         . . . .
    //         Z v Y X
    //         . . . .
    if (X.x_ > 2)
    {
        Match& v = matches_(X.x_-2, X.y_);

        if (IsSelectableObject(v))
        {
            Match& Y = matches_(X.x_-1, X.y_);
            Match& Z = matches_(X.x_-3, X.y_);

            if (HaveSameMatchType(X, Y) && HaveSameMatchType(X, Z) && !HaveSameMatchType(X, v) &&
                HasNoWalls(X, WO_WALLLEFT) && HasNoWalls(Y, WO_WALLRIGHT | WO_WALLLEFT) && HasNoWalls(v, WO_WALLRIGHT | WO_WALLLEFT) && HasNoWalls(Z, WO_WALLRIGHT))
            {
                Vector<Match*> matches;
                matches.Push(&X);
                matches.Push(&Y);
                matches.Push(&Z);
                hints.Push(matches);
            }
        }
    }
    // Schema
    //         . W .
    //         X v Y
    //         . Z .
    if (X.x_ < width_-2)
    {
        Match& v = matches_(X.x_+1, X.y_);

        if (IsSelectableObject(v))
        {
            Match& Y = matches_(X.x_+2, X.y_);

            if (HaveSameMatchType(X, Y) && HasNoWalls(Y, WO_WALLLEFT) &&
                HasNoWalls(X, WO_WALLRIGHT) && HasNoWalls(v, WO_WALLRIGHT | WO_WALLLEFT))
            {
                if (X.y_ >= 1 && HasNoWalls(v, WO_WALLNORTH))
                {
                    Match& W = matches_(X.x_+1, X.y_-1);
                    if (HaveSameMatchType(X, W) && HasNoWalls(W, WO_WALLSOUTH))
                    {
                        Vector<Match*> matches;
                        matches.Push(&X);
                        matches.Push(&Y);
                        matches.Push(&W);
                        hints.Push(matches);
                    }
                }
                if (X.y_ < height_-1 && HasNoWalls(v, WO_WALLSOUTH))
                {
                    Match& Z = matches_(X.x_+1, X.y_+1);
                    if (HaveSameMatchType(X, Z) && HasNoWalls(Z, WO_WALLNORTH))
                    {
                        Vector<Match*> matches;
                        matches.Push(&X);
                        matches.Push(&Y);
                        matches.Push(&Z);
                        hints.Push(matches);
                    }
                }
            }
        }
    }
}

void MatchGrid::CheckHints_Vertical(Match& X, Vector<Vector<Match*> >& hints)
{
    // Schema
    //         W v Z
    //         . X .
    //         . Y .
    //         . . .
    if (X.y_ > 0 && X.y_ < height_-1)
    {
        Match& v = matches_(X.x_, X.y_-1);

        if (IsSelectableObject(v))
        {
            Match& Y = matches_(X.x_, X.y_+1);

            if (HaveSameMatchType(X, Y) && HasNoWalls(X, WO_WALLNORTH | WO_WALLSOUTH) && HasNoWalls(Y, WO_WALLNORTH) && HasNoWalls(v, WO_WALLSOUTH))
            {
                if (X.x_ >= 1 && HasNoWalls(v, WO_WALLLEFT))
                {
                    Match& W = matches_(X.x_-1, X.y_-1);
                    if (HaveSameMatchType(X, W) && HasNoWalls(W, WO_WALLRIGHT))
                    {
                        Vector<Match*> matches;
                        matches.Push(&X);
                        matches.Push(&Y);
                        matches.Push(&W);
                        hints.Push(matches);
                    }
                }
                if (X.x_ < width_-1 && HasNoWalls(v, WO_WALLRIGHT))
                {
                    Match& Z = matches_(X.x_+1, X.y_-1);
                    if (HaveSameMatchType(X, Z) && HasNoWalls(Z, WO_WALLLEFT))
                    {
                        Vector<Match*> matches;
                        matches.Push(&X);
                        matches.Push(&Y);
                        matches.Push(&Z);
                        hints.Push(matches);
                    }
                }
            }
        }
    }
    // Schema
    //         . . .
    //         . X .
    //         . Y .
    //         W v Z
    if (X.y_ < height_-2)
    {
        Match& v = matches_(X.x_, X.y_+2);

        if (IsSelectableObject(v))
        {
            Match& Y = matches_(X.x_, X.y_+1);

            if (HaveSameMatchType(X, Y) && HasNoWalls(Y, WO_WALLNORTH | WO_WALLSOUTH) && HasNoWalls(X, WO_WALLSOUTH) && HasNoWalls(v, WO_WALLNORTH))
            {
                if (X.x_ >= 1 && HasNoWalls(v, WO_WALLLEFT))
                {
                    Match& W = matches_(X.x_-1, X.y_+2);
                    if (HaveSameMatchType(X, W) && HasNoWalls(W, WO_WALLRIGHT))
                    {
                        Vector<Match*> matches;
                        matches.Push(&X);
                        matches.Push(&Y);
                        matches.Push(&W);
                        hints.Push(matches);
                    }
                }
                if (X.x_ < width_-1 && HasNoWalls(v, WO_WALLRIGHT))
                {
                    Match& Z = matches_(X.x_+1, X.y_+2);
                    if (HaveSameMatchType(X, Z) && HasNoWalls(Z, WO_WALLLEFT))
                    {
                        Vector<Match*> matches;
                        matches.Push(&X);
                        matches.Push(&Y);
                        matches.Push(&Z);
                        hints.Push(matches);
                    }
                }
            }
        }
    }
    // Schema
    //         . X .
    //         . Y .
    //         . v .
    //         . Z .
    if (X.y_ < height_-3)
    {
        Match& v = matches_(X.x_, X.y_+2);

        if (IsSelectableObject(v))
        {
            Match& Y = matches_(X.x_, X.y_+1);
            Match& Z = matches_(X.x_, X.y_+3);

            if (HaveSameMatchType(X, Y) && HaveSameMatchType(X, Z) && !HaveSameMatchType(X, v) &&
                HasNoWalls(X, WO_WALLSOUTH) && HasNoWalls(Y, WO_WALLNORTH | WO_WALLSOUTH) && HasNoWalls(v, WO_WALLNORTH | WO_WALLSOUTH) && HasNoWalls(Z, WO_WALLNORTH))
            {
                Vector<Match*> matches;
                matches.Push(&X);
                matches.Push(&Y);
                matches.Push(&Z);
                hints.Push(matches);
            }
        }
    }
    // Schema
    //         . Z .
    //         . v .
    //         . Y .
    //         . X .
    if (X.y_ > 2)
    {
        Match& v = matches_(X.x_, X.y_-2);

        if (IsSelectableObject(v))
        {
            Match& Y = matches_(X.x_, X.y_-1);
            Match& Z = matches_(X.x_, X.y_-3);

            if (HaveSameMatchType(X, Y) && HaveSameMatchType(X, Z) && !HaveSameMatchType(X, v) &&
                HasNoWalls(Z, WO_WALLSOUTH) && HasNoWalls(Y, WO_WALLNORTH | WO_WALLSOUTH) && HasNoWalls(v, WO_WALLNORTH | WO_WALLSOUTH) && HasNoWalls(X, WO_WALLNORTH))
            {
                Vector<Match*> matches;
                matches.Push(&X);
                matches.Push(&Y);
                matches.Push(&Z);
                hints.Push(matches);
            }
        }
    }
    // Schema
    //         . X .
    //         W v Z
    //         . Y .
    if (X.y_ < height_-2)
    {
        Match& v = matches_(X.x_, X.y_+1);

        if (IsSelectableObject(v))
        {
            Match& Y = matches_(X.x_, X.y_+2);

            if (HaveSameMatchType(X, Y) && HasNoWalls(Y, WO_WALLNORTH) &&
                HasNoWalls(X, WO_WALLSOUTH) && HasNoWalls(v, WO_WALLNORTH | WO_WALLSOUTH))
            {
                if (X.x_ >= 1 && HasNoWalls(v, WO_WALLLEFT))
                {
                    Match& W = matches_(X.x_-1, X.y_+1);
                    if (HaveSameMatchType(X, W) && HasNoWalls(W, WO_WALLRIGHT))
                    {
                        Vector<Match*> matches;
                        matches.Push(&X);
                        matches.Push(&Y);
                        matches.Push(&W);
                        hints.Push(matches);
                    }
                }
                if (X.x_ < width_-1 && HasNoWalls(v, WO_WALLRIGHT))
                {
                    Match& Z = matches_(X.x_+1, X.y_+1);
                    if (HaveSameMatchType(X, Z) && HasNoWalls(Z, WO_WALLLEFT))
                    {
                        Vector<Match*> matches;
                        matches.Push(&X);
                        matches.Push(&Y);
                        matches.Push(&Z);
                        hints.Push(matches);
                    }
                }
            }
        }
    }
}

void MatchGrid::CheckHints_Square(Match& X, Vector<Vector<Match*> >& hints)
{
    // X v W
    // Y Z .
    if (X.y_ < height_-1 && X.x_ < width_-2)
    {
        Match& v = matches_(X.x_+2, X.y_);
        Match& W = matches_(X.x_+1, X.y_);
        Match& Y = matches_(X.x_, X.y_+1);
        Match& Z = matches_(X.x_+1, X.y_+1);

        if (HaveSameMatchType(X, Y, Z, W) && IsSelectableObject(v) &&
            HasNoWalls(X, WO_WALLSOUTH | WO_WALLRIGHT) && HasNoWalls(Y, WO_WALLNORTH | WO_WALLRIGHT) &&
            HasNoWalls(Z, WO_WALLNORTH | WO_WALLLEFT) && HasNoWalls(W, WO_WALLLEFT) &&
            HasNoWalls(v, WO_WALLSOUTH | WO_WALLLEFT | WO_WALLRIGHT))
        {
            Vector<Match*> matches;
            matches.Push(&X);
            matches.Push(&Y);
            matches.Push(&W);
            matches.Push(&Z);
            hints.Push(matches);
        }
    }
    // Y Z .
    // X v W
    if (X.y_ > 0 && X.x_ < width_-2)
    {
        Match& v = matches_(X.x_+1, X.y_);
        Match& W = matches_(X.x_+2, X.y_);
        Match& Y = matches_(X.x_, X.y_-1);
        Match& Z = matches_(X.x_+1, X.y_-1);

        if (HaveSameMatchType(X, Y, Z, W) && IsSelectableObject(v) &&
            HasNoWalls(X, WO_WALLNORTH | WO_WALLRIGHT) && HasNoWalls(Y, WO_WALLSOUTH | WO_WALLRIGHT) &&
            HasNoWalls(Z, WO_WALLSOUTH | WO_WALLLEFT) && HasNoWalls(W, WO_WALLLEFT) &&
            HasNoWalls(v, WO_WALLNORTH | WO_WALLLEFT | WO_WALLRIGHT))
        {
            Vector<Match*> matches;
            matches.Push(&X);
            matches.Push(&Y);
            matches.Push(&W);
            matches.Push(&Z);
            hints.Push(matches);
        }
    }
    // W v X
    // . Z Y
    if (X.y_ < height_-1 && X.x_ > 1)
    {
        Match& v = matches_(X.x_-1, X.y_);
        Match& W = matches_(X.x_-2, X.y_);
        Match& Y = matches_(X.x_, X.y_+1);
        Match& Z = matches_(X.x_-1, X.y_+1);

        if (HaveSameMatchType(X, Y, Z, W) && IsSelectableObject(v) &&
            HasNoWalls(X, WO_WALLSOUTH | WO_WALLLEFT) && HasNoWalls(Y, WO_WALLNORTH | WO_WALLLEFT) &&
            HasNoWalls(Z, WO_WALLNORTH | WO_WALLRIGHT) && HasNoWalls(W, WO_WALLRIGHT) &&
            HasNoWalls(v, WO_WALLSOUTH | WO_WALLLEFT | WO_WALLRIGHT))
        {
            Vector<Match*> matches;
            matches.Push(&X);
            matches.Push(&Y);
            matches.Push(&W);
            matches.Push(&Z);
            hints.Push(matches);
        }
    }
    // . Z Y
    // W v X
    if (X.y_ > 0 && X.x_ > 1)
    {
        Match& v = matches_(X.x_-1, X.y_);
        Match& W = matches_(X.x_-2, X.y_);
        Match& Y = matches_(X.x_, X.y_-1);
        Match& Z = matches_(X.x_-1, X.y_-1);

        if (HaveSameMatchType(X, Y, Z, W) && IsSelectableObject(v) &&
            HasNoWalls(X, WO_WALLNORTH | WO_WALLLEFT) && HasNoWalls(Y, WO_WALLSOUTH | WO_WALLLEFT) &&
            HasNoWalls(Z, WO_WALLSOUTH | WO_WALLRIGHT) && HasNoWalls(W, WO_WALLRIGHT) &&
            HasNoWalls(v, WO_WALLNORTH | WO_WALLLEFT | WO_WALLRIGHT))
        {
            Vector<Match*> matches;
            matches.Push(&X);
            matches.Push(&Y);
            matches.Push(&W);
            matches.Push(&Z);
            hints.Push(matches);
        }
    }
    // . W
    // Y v
    // X Z
    if (X.y_ > 1 && X.x_ < width_-1)
    {
        Match& Y = matches_(X.x_, X.y_-1);
        Match& v = matches_(X.x_+1, X.y_-1);
        Match& W = matches_(X.x_+1, X.y_-2);
        Match& Z = matches_(X.x_+1, X.y_);

        if (HaveSameMatchType(X, Y, Z, W) && IsSelectableObject(v) &&
            HasNoWalls(X, WO_WALLNORTH | WO_WALLRIGHT) && HasNoWalls(Y, WO_WALLSOUTH | WO_WALLRIGHT) &&
            HasNoWalls(Z, WO_WALLNORTH | WO_WALLLEFT) && HasNoWalls(W, WO_WALLSOUTH) &&
            HasNoWalls(v, WO_WALLNORTH | WO_WALLSOUTH | WO_WALLLEFT))
        {
            Vector<Match*> matches;
            matches.Push(&X);
            matches.Push(&Y);
            matches.Push(&W);
            matches.Push(&Z);
            hints.Push(matches);
        }
    }
    // W .
    // v Y
    // Z X
    if (X.y_ > 1 && X.x_ > 0)
    {
        Match& Y = matches_(X.x_, X.y_-1);
        Match& v = matches_(X.x_-1, X.y_-1);
        Match& W = matches_(X.x_-1, X.y_-2);
        Match& Z = matches_(X.x_-1, X.y_);

        if (HaveSameMatchType(X, Y, Z, W) && IsSelectableObject(v) &&
            HasNoWalls(X, WO_WALLNORTH | WO_WALLLEFT) && HasNoWalls(Y, WO_WALLSOUTH | WO_WALLLEFT) &&
            HasNoWalls(Z, WO_WALLNORTH | WO_WALLRIGHT) && HasNoWalls(W, WO_WALLSOUTH) &&
            HasNoWalls(v, WO_WALLNORTH | WO_WALLSOUTH | WO_WALLRIGHT))
        {
            Vector<Match*> matches;
            matches.Push(&X);
            matches.Push(&Y);
            matches.Push(&W);
            matches.Push(&Z);
            hints.Push(matches);
        }
    }
    // Z X
    // v Y
    // W .
    if (X.y_ < height_-2 && X.x_ > 0)
    {
        Match& Y = matches_(X.x_, X.y_+1);
        Match& v = matches_(X.x_-1, X.y_+1);
        Match& W = matches_(X.x_-1, X.y_+2);
        Match& Z = matches_(X.x_-1, X.y_);

        if (HaveSameMatchType(X, Y, Z, W) && IsSelectableObject(v) &&
            HasNoWalls(X, WO_WALLSOUTH | WO_WALLLEFT) && HasNoWalls(Y, WO_WALLNORTH | WO_WALLLEFT) &&
            HasNoWalls(Z, WO_WALLSOUTH | WO_WALLRIGHT) && HasNoWalls(W, WO_WALLNORTH) &&
            HasNoWalls(v, WO_WALLNORTH | WO_WALLSOUTH | WO_WALLRIGHT))
        {
            Vector<Match*> matches;
            matches.Push(&X);
            matches.Push(&Y);
            matches.Push(&W);
            matches.Push(&Z);
            hints.Push(matches);
        }
    }
    // X Z
    // Y v
    // . W
    if (X.y_ < height_-2 && X.x_ < width_-1)
    {
        Match& Y = matches_(X.x_, X.y_+1);
        Match& v = matches_(X.x_+1, X.y_+1);
        Match& W = matches_(X.x_+1, X.y_+2);
        Match& Z = matches_(X.x_+1, X.y_);

        if (HaveSameMatchType(X, Y, Z, W) && IsSelectableObject(v) &&
            HasNoWalls(X, WO_WALLSOUTH | WO_WALLRIGHT) && HasNoWalls(Y, WO_WALLNORTH | WO_WALLRIGHT) &&
            HasNoWalls(Z, WO_WALLSOUTH | WO_WALLLEFT) && HasNoWalls(W, WO_WALLNORTH) &&
            HasNoWalls(v, WO_WALLNORTH | WO_WALLSOUTH | WO_WALLLEFT))
        {
            Vector<Match*> matches;
            matches.Push(&X);
            matches.Push(&Y);
            matches.Push(&W);
            matches.Push(&Z);
            hints.Push(matches);
        }
    }
}


void MatchGrid::GetAllHorizontalMatches(const Match& entry, Vector<Match*>& matches, Vector<Match*>& brokenrocks, Vector<WallInfo>* hittedwalls)
{
//    URHO3D_LOGINFOF("MatchGrid() - GetAllHorizontalMatches : entry=%s !", entry.ToString().CString());

    if (hittedwalls)
    {
        Vector<WallInfo>& hittedw = *hittedwalls;

        unsigned char y = entry.y_;

        // check to left
        for (int x=entry.x_; x >= 0; x--)
        {
            Match& match = matches_(x, y);

            if (grid_(x, y).wallorientation_ & WO_WALLRIGHT)
            {
                hittedw.Push(WallInfo(x, y, WO_WALLRIGHT));
                break;
            }

            Match::AddDistinctEntry(&match, match.ctype_ == ROCKS ? brokenrocks : matches);

            if (grid_(x, y).wallorientation_ & WO_WALLLEFT)
            {
                hittedw.Push(WallInfo(x, y, WO_WALLLEFT));
                break;
            }
        }

        // check to right
        for (int x=entry.x_; x < width_; x++)
        {
            Match& match = matches_(x, y);

            if (grid_(x, y).wallorientation_ & WO_WALLLEFT)
            {
                hittedw.Push(WallInfo(x, y, WO_WALLLEFT));
                break;
            }

            Match::AddDistinctEntry(&match, match.ctype_ == ROCKS ? brokenrocks : matches);

            if (grid_(x, y).wallorientation_ & WO_WALLRIGHT)
            {
                hittedw.Push(WallInfo(x, y, WO_WALLRIGHT));
                break;
            }
        }
    }
    else
    {
        for (int x=0; x < width_; x++)
            matches.Push(&matches_(x, entry.y_));
    }
}

void MatchGrid::GetAllVerticalMatches(const Match& entry, Vector<Match*>& matches, Vector<Match*>& brokenrocks, Vector<WallInfo>* hittedwalls)
{
//    URHO3D_LOGINFOF("MatchGrid() - GetAllVerticalMatches : entry=%s !", entry.ToString().CString());

    if (hittedwalls)
    {
        Vector<WallInfo>& hittedw = *hittedwalls;

        unsigned char x = entry.x_;

        // check to top
        for (int y=entry.y_; y >= 0; y--)
        {
            Match& match = matches_(x, y);

            if (grid_(x, y).wallorientation_ & WO_WALLSOUTH)
            {
                hittedw.Push(WallInfo(x, y, WO_WALLSOUTH));
                break;
            }

            Match::AddDistinctEntry(&match, match.ctype_ == ROCKS ? brokenrocks : matches);

            if (grid_(x, y).wallorientation_ & WO_WALLNORTH)
            {
                hittedw.Push(WallInfo(x, y, WO_WALLNORTH));
                break;
            }
        }

        // check to bottom
        for (int y=entry.y_; y < height_; y++)
        {
            Match& match = matches_(x, y);

            if (grid_(x, y).wallorientation_ & WO_WALLNORTH)
            {
                hittedw.Push(WallInfo(x, y, WO_WALLNORTH));
                break;
            }

            Match::AddDistinctEntry(&match, match.ctype_ == ROCKS ? brokenrocks : matches);

            if (grid_(x, y).wallorientation_ & WO_WALLSOUTH)
            {
                hittedw.Push(WallInfo(x, y, WO_WALLSOUTH));
                break;
            }
        }
    }
    else
    {
        for (int y=0; y < height_; y++)
            matches.Push(&matches_(entry.x_, y));
    }
}

void MatchGrid::GetAllWallsAround(const Match& entry, int range, Vector<WallInfo>& hittedwalls)
{
	int xmin = 0;
	int xmax = width_ - 1;
	int ymin = 0;
	int ymax = height_ - 1;

	if (entry.x_ > range)
		xmin = entry.x_ - range;
	if (entry.y_ > range)
		ymin = entry.y_ - range;
    if (entry.x_ + range < width_-1)
        xmax = entry.x_ + range;
    if (entry.y_ + range < height_-1)
        ymax = entry.y_ + range;

	for (int y=ymin; y <= ymax; y++)
		for (int x = xmin; x <= xmax; x++)
		{
			if (grid_(x, y).wallorientation_ != 0)
				hittedwalls.Push(WallInfo(x, y, grid_(x, y).wallorientation_));
		}

	URHO3D_LOGINFOF("MatchGrid() - GetAllWallsAround : entry=%s hittedwalls=%u!", entry.ToString().CString(), hittedwalls.Size());
}
