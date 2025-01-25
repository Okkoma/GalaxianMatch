#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/StringUtils.h>

#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>

#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/SpriterInstance2D.h>
#include <Urho3D/Urho2D/CollisionShape2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/PhysicsEvents2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>

#include "GameAttributes.h"
#include "GameStatics.h"
#include "GameEvents.h"
#include "GameHelpers.h"
#include "GameRand.h"
#include "TimerRemover.h"

#include "MAN_Matches.h"

#include "TicTacToeLogic.h"

const char JoueurX = 'x', JoueurO = 'o', CelluleVide = ' ', Draw = 'd';
const float TicTacToeScaleRatio = 1.65f;
const float TicTacToeAnimationTime = 1.f;
const float TicTacToePlayDelay = 30.f;

char board_[3][3];

// Fonction pour afficher le plateau de jeu
void afficherPlateau(char plateau[3][3])
{
    String logstr = "board\n";
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            logstr += String(plateau[i][j]);
            if (j < 2)
                logstr += " | ";
        }
        logstr += "\n";
        if (i < 2)
        {
            logstr += "---------\n";
        }
    }
    URHO3D_LOGINFOF("%s", logstr.CString());
}

// Fonction pour vider le plateau de jeu
void viderPlateau(char *ptr)
{
    for (int i = 0; i < 9; i++)
        *ptr++ = CelluleVide;
}

// Fonctions pour vérifier s'il y a un gagnant
char verifierGagnant(char plateau[3][3])
{
    // verification des lignes
    for (int i = 0; i < 3; i++)
        if (plateau[i][0] != CelluleVide)
            if (plateau[i][0] == plateau[i][1] && plateau[i][1] == plateau[i][2])
                return plateau[i][0];

    // verification des colonnes
    for (int i = 0; i < 3; i++)
        if (plateau[0][i] != CelluleVide)
            if (plateau[0][i] == plateau[1][i] && plateau[1][i] == plateau[2][i])
                return plateau[0][i];

    // Vérification des diagonales
    if (plateau[0][0] == plateau[1][1] && plateau[1][1] == plateau[2][2] &&
            plateau[0][0] != CelluleVide)
        return plateau[0][0];
    if (plateau[0][2] == plateau[1][1] && plateau[1][1] == plateau[2][0] &&
            plateau[0][2] != CelluleVide)
        return plateau[0][2];

    return CelluleVide;
}

char verifierGagnant(char *ptr)
{
    // Cast du pointeur vers char en un pointeur pointant vers un tableau de 3 chars
    return verifierGagnant((char(*)[3])ptr);
}

// Ceci est la fonction minimax. Elle examine toutes les
// façons possibles dont le jeu peut se dérouler et renvoie
// la valeur du plateau
int minimax(char *plateau, char joueur, int meilleurScore, int maxprofondeur)
{
    char gagnant = verifierGagnant(plateau);
    // Si le Maximiseur a gagné le jeu, retourne son score évalué
    if (gagnant == JoueurX)
        return 10;
    // Si le Minimiseur a gagné le jeu, retourne son score évalué
    if (gagnant == JoueurO)
        return -10;

    // S'il n'y a plus de coups possibles et aucun gagnant, c'est une égalité
    bool coupPossible = false;
    for (int i = 0; i < 9; i++)
    {
        if (plateau[i] == CelluleVide)
        {
            coupPossible = true;
            break;
        }
    }
    if (!coupPossible)
        return 0;

    maxprofondeur--;
    if (maxprofondeur <= 0)
        return meilleurScore;

    // Si c'est le tour du Maximiseur
    if (joueur == JoueurX)
    {
        int meilleur = -1000;

        // Parcourir toutes les cases
        for (int i = 0; i < 9; i++)
        {
            // Vérifier si la case est vide
            char &cellule = plateau[i];
            if (cellule == CelluleVide)
            {
                // Jouer le coup
                cellule = JoueurX;

                // Appeler minimax de manière récursive et choisir
                // la valeur maximale
                meilleurScore = minimax(plateau, JoueurO, meilleur, maxprofondeur);
                meilleur = Max(meilleur, meilleurScore);

                // Annuler le coup
                cellule = CelluleVide;
            }
        }
        return meilleur;
    }

    // Si c'est le tour du Minimiseur
    else
    {
        int meilleur = 1000;

        // Parcourir toutes les cases
        for (int i = 0; i < 9; i++)
        {
            // Vérifier si la case est vide
            char &cellule = plateau[i];
            if (cellule == CelluleVide)
            {
                // Jouer le coup
                cellule = JoueurO;

                // Appeler minimax de manière récursive et choisir
                // la valeur minimale
                meilleurScore = minimax(plateau, JoueurX, meilleur, maxprofondeur);
                meilleur = Min(meilleur, meilleurScore);

                // Annuler le coup
                cellule = CelluleVide;
            }
        }
        return meilleur;
    }
}

// Fonction pour que le bot choisisse le meilleur coup
int meilleurCoupBot(char plateau[3][3], int difficulte)
{
    int meilleurScore = 1000;
    int meilleurCoup = -1;

    // pour les difficultes 2-3 faire certains coups au hasard.
    if (difficulte > 1 && difficulte < 4)
    {
        if (GameRand::GetRand(OBJRAND, 100) < 100/difficulte)
        {
            // obtenir les cellules vides
            int icellulesVides[9];
            int numCellulesVides = 0;
            for (int i = 0; i < 9; i++)
            {
                if (plateau[i / 3][i % 3] == CelluleVide)
                {
                    icellulesVides[numCellulesVides] = i;
                    numCellulesVides++;
                }
            }
            // retourner une cellule vide au hasard.

            return numCellulesVides ? icellulesVides[GameRand::GetRand(OBJRAND, numCellulesVides)] : -1;
        }
        difficulte = 4;
    }

    int profondeur = difficulte;

    // Parcourir toutes les cases, évaluer la fonction minimax pour
    // toutes les cases vides. Et retourner la case avec la valeur optimale.
    for (int i = 0; i < 9; i++)
    {
        char &cellule = plateau[i / 3][i % 3];

        // Vérifier si la case est vide
        if (cellule == CelluleVide)
        {
            if (meilleurCoup == -1)
                meilleurCoup = i;

            // Calculer la fonction d'évaluation
            cellule = JoueurO; // Jouer le coup
            int score = minimax(plateau[0], JoueurX, meilleurScore, profondeur);
            cellule = CelluleVide; // Annuler le coup

            // Si la valeur du coup actuel est
            // supérieure à la meilleure valeur, alors mettre à jour
            // la meilleure valeur
            if (score < meilleurScore)
            {
                meilleurScore = score;
                meilleurCoup = i;
            }
        }
    }

    return meilleurCoup;
}


Vector3 tictactoeInitialScale_;

TicTacToeLogic::TicTacToeLogic(Context* context) :
    BossLogic(context),
    ticTacToeMode_(false),
    currentplayer_(JoueurO),
    winner_(CelluleVide),
    turn_(0),
    case_(-1),
    difficulty_(0)
{ }

TicTacToeLogic::~TicTacToeLogic()
{
//    URHO3D_LOGINFOF("~TicTacToeLogic()");

}

void TicTacToeLogic::RegisterObject(Context* context)
{
    context->RegisterFactory<TicTacToeLogic>();
    URHO3D_COPY_BASE_ATTRIBUTES(BossLogic);
}

void TicTacToeLogic::SubscribeToEvents()
{
    BossLogic::SubscribeToEvents();
    SubscribeToEvent(node_, GAME_BOSSSTATECHANGE, URHO3D_HANDLER(TicTacToeLogic, OnBossStateChange));
}

void TicTacToeLogic::UnsubscribeFromEvents()
{
    BossLogic::UnsubscribeFromEvents();
    UnsubscribeFromEvent(node_, GAME_BOSSSTATECHANGE);
}

void TicTacToeLogic::OnSetEnabled()
{
    URHO3D_LOGINFOF("TicTacToeLogic() - OnSetEnabled : Node=%s(%u) enabled=%s ...", node_->GetName().CString(), node_->GetID(), IsEnabledEffective() ? "true" : "false");

    BossLogic::OnSetEnabled();
    tictactoeInitialScale_ = node_->GetScale();

    URHO3D_LOGINFOF("TicTacToeLogic() - OnSetEnabled : Node=%s(%u) enabled=%s ... OK !", node_->GetName().CString(), node_->GetID(), IsEnabledEffective() ? "true" : "false");
}

void TicTacToeLogic::OnBossStateChange(StringHash eventType, VariantMap& eventData)
{
    if (!ticTacToeMode_)
    {
        if (GetState() >= HURT && GetState() <= BREAK5)
        {
            // Enter in tictactoe mode
            ticTacToeMode_ = true;

            difficulty_++;
            if (difficulty_ > 4)
                difficulty_ = 3;

            URHO3D_LOGINFOF("TicTacToeLogic() - OnBossStateChange : enter in tictacoe mode difficulty=%d!", difficulty_);

            BossLogic::UnsubscribeFromEvents();

            // Desactive MatchesManager
            MatchesManager::UnsubscribeFromEvents();
            MatchesManager::SetPhysicsEnable(false);

            // Boss Move/Zoom, Remove Crosses And Circle
            node_->RemoveObjectAnimation();
            SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(GetContext()));
            SharedPtr<ValueAnimation> scaleAnimation(new ValueAnimation(GetContext()));
            scaleAnimation->SetKeyFrame(0.f, tictactoeInitialScale_);
            scaleAnimation->SetKeyFrame(TicTacToeAnimationTime, tictactoeInitialScale_ * TicTacToeScaleRatio);
            objectAnimation->AddAttributeAnimation("Scale", scaleAnimation, WM_ONCE);
            SharedPtr<ValueAnimation> moveAnimation(new ValueAnimation(GetContext()));
            moveAnimation->SetKeyFrame(0.f, node_->GetPosition());
            moveAnimation->SetKeyFrame(TicTacToeAnimationTime, GameStatics::cameraNode_->GetPosition());
            objectAnimation->AddAttributeAnimation("Position", moveAnimation, WM_ONCE);
            node_->SetObjectAnimation(objectAnimation);

            animators_.Front()->ResetCharacterMapping();
            animators_.Front()->ApplyCharacterMap(String("static"));

            node_->GetDerivedComponent<CollisionShape2D>()->SetEnabled(false);
            SetState(STATIC);

            // Start TicTacToe Game
            StartGame(JoueurX);
        }
    }
}


void TicTacToeLogic::StartGame(char firstplayer)
{
    viderPlateau(board_[0]);

    turn_ = 0;
    case_ = -1;
    currentplayer_ = firstplayer_ = firstplayer;
    elapedTime_ = turnTime_ = 0.f;
    winner_ = CelluleVide;

    SubscribeToEvent(GetScene(), E_SCENEPOSTUPDATE, URHO3D_HANDLER(TicTacToeLogic, HandleGame));
    SubscribeToEvent(E_TOUCHBEGIN, URHO3D_HANDLER(TicTacToeLogic, HandleGame));
    SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(TicTacToeLogic, HandleGame));
}

void TicTacToeLogic::HandleGame(StringHash eventType, VariantMap& eventData)
{
    if (eventType == E_MOUSEBUTTONDOWN || eventType == E_TOUCHBEGIN)
    {
        IntVector2 position;
        if (eventType == E_TOUCHBEGIN)
        {
            position.x_ = eventData[TouchBegin::P_X].GetInt();
            position.y_ = eventData[TouchBegin::P_Y].GetInt();
        }
        else
        {
            position = GameStatics::input_->GetMousePosition();
        }

        Vector2 wposition = GameHelpers::ScreenToWorld2D(position);
        RigidBody2D* body;
        CollisionShape2D* shape;
        GameStatics::rootScene_->GetComponent<PhysicsWorld2D>()->GetPhysicElements(wposition, body, shape, M_MAX_UNSIGNED);
        if (body == body_)
        {
            if (shape && shape->IsTrigger())
            {
                Node* timelineNode = shape->GetNode();
                if (timelineNode->GetName().StartsWith("Trig_Case"))
                {
                    int c = ToInt(String(timelineNode->GetName()[9])) -1;
                    if (board_[c/3][c%3] == CelluleVide)
                    {
                        case_ = c;
                        URHO3D_LOGINFOF("TicTacToeLogic() - OnBeginContact : case=%d", case_);
                    }
                }
            }
        }
    }
    else if (eventType == E_SCENEPOSTUPDATE)
    {
        float timestep = eventData[SceneUpdate::P_TIMESTEP].GetFloat();
        elapedTime_ += timestep;
//        URHO3D_LOGINFOF("TicTacToeLogic() - HandleGame : elapsedTime=%F ...", elapedTime_);
        turnTime_ += timestep;

        if (turn_ < 8)
        {
            if (winner_ == CelluleVide)
            {
                bool played = false;

                if (currentplayer_ == JoueurX)
                {
                    // the player selected a case (setted by OnBeginContact)
                    if (case_ != -1)
                    {
                        URHO3D_LOGINFOF("TicTacToeLogic() - HandleGame : JoueurX case=%d", case_);
                        board_[case_/3][case_%3] = JoueurX;

                        played = true;
                    }
                }
                else if (turnTime_ > 1.f)
                {
                    case_ = meilleurCoupBot(board_, difficulty_);

                    URHO3D_LOGINFOF("TicTacToeLogic() - HandleGame : JoueurO case=%d ...", case_);
                    board_[case_/3][case_%3] = JoueurO;

                    played = true;
                }

                if (played)
                {
                    afficherPlateau(board_);

                    // add the animation for the played case.
                    String animation = String("case") + String(case_+1);
                    String cmap = animation + String("_") + String(currentplayer_ == JoueurO ? "circle":"cross");
                    if (!animators_.Front()->IsCharacterMapApplied(cmap))
                    {
                        animators_.Front()->ApplyCharacterMap(cmap);
                        animators_.Front()->SetAnimation(animation);
                    }

                    // check for a winner
                    winner_ = verifierGagnant(board_);

                    // next turn
                    turnTime_ = 0.f;
                    turn_++;
                    case_ = -1;
                    currentplayer_ = (currentplayer_ == JoueurX) ? JoueurO : JoueurX;
                }
            }
        }
        else
        {
            // last turn : auto mode
            if (winner_ == CelluleVide && turnTime_ > 1.f)
            {
                case_ = -1;

                for (int i = 0; i < 3; i++)
                    for (int j = 0; j < 3; j++)
                        if (board_[i][j] == CelluleVide)
                        {
                            case_ = i * 3 + j;
                            board_[i][j] = firstplayer_;
                            break;
                        }

                if (case_ != -1)
                {
                    afficherPlateau(board_);

                    // add the animation for the played case.
                    String animation = String("case") + String(case_+1);
                    String cmap = animation + String("_") + String(firstplayer_ == JoueurO ? "circle":"cross");
                    if (!animators_.Front()->IsCharacterMapApplied(cmap))
                    {
                        animators_.Front()->ApplyCharacterMap(cmap);
                        animators_.Front()->SetAnimation(animation);
                    }

                    turnTime_ = 0.f;
                }

                winner_ = verifierGagnant(board_);
                if (winner_ == CelluleVide)
                    winner_ = Draw;
            }
        }

        if (winner_ == CelluleVide && elapedTime_ > TicTacToePlayDelay)
            winner_ = JoueurO;

        if (winner_ != CelluleVide && turnTime_ > 1.f)
        {
            // Stop TicTacToe Game
            ticTacToeMode_ = false;

            URHO3D_LOGINFOF("TicTacToeLogic() - HandleGame : exit tictactoe mode : Winner=%c !", winner_);

            // Restart normal zoom
            node_->RemoveObjectAnimation();
            SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(GetContext()));
            SharedPtr<ValueAnimation> scaleAnimation(new ValueAnimation(GetContext()));
            scaleAnimation->SetKeyFrame(0.f, node_->GetScale());
            scaleAnimation->SetKeyFrame(TicTacToeAnimationTime, tictactoeInitialScale_);
            objectAnimation->AddAttributeAnimation("Scale", scaleAnimation, WM_ONCE);
            node_->SetObjectAnimation(objectAnimation);

            SetState(IDLE);
            if (winner_ == JoueurX)
                Hit(10, true, true);

            node_->GetDerivedComponent<CollisionShape2D>()->SetEnabled(true);

            SubscribeToEvents();

            // Reactive MatchesManager
            MatchesManager::SubscribeToEvents();
            MatchesManager::SetPhysicsEnable(true);
        }
    }
}

