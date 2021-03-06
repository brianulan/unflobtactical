/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "tacticalendscene.h"
#include "../engine/uirendering.h"
#include "../engine/engine.h"
#include "../engine/text.h"
#include "game.h"
#include "cgame.h"
#include "../grinliz/glstringutil.h"
#include "tacticalintroscene.h"
#include "research.h"

using namespace grinliz;
using namespace gamui;

TacticalEndScene::TacticalEndScene( Game* _game ) : Scene( _game )
{
	//Engine* engine = GetEngine();

	gamui::RenderAtom nullAtom;
	backgroundUI.Init( game, &gamui2D, false );

	for( int i=0; i<TEXT_ROW*TEXT_COL; ++i ) {
		textTable[i].Init( &gamui2D );
	}

	victory.Init( &gamui2D );
	const Unit* soldiers = game->battleData.Units( TERRAN_UNITS_START );
	const Unit* aliens =   game->battleData.Units( ALIEN_UNITS_START );
	const Unit* civs =     game->battleData.Units( CIV_UNITS_START );

	int nSoldiers = Unit::Count( soldiers, MAX_TERRANS, -1 );
	int nSoldiersStanding = Unit::Count( soldiers, MAX_TERRANS, Unit::STATUS_ALIVE );
	int nSoldiersDown      =   Unit::Count( soldiers, MAX_TERRANS, Unit::STATUS_KIA )
						     + Unit::Count( soldiers, MAX_TERRANS, Unit::STATUS_UNCONSCIOUS )
							 + Unit::Count( soldiers, MAX_TERRANS, Unit::STATUS_MIA );

	int nAliens = Unit::Count( aliens, MAX_ALIENS, -1 );
	int nAliensAlive = Unit::Count( aliens, MAX_ALIENS, Unit::STATUS_ALIVE );
	int nAliensKIA = Unit::Count( aliens, MAX_ALIENS, Unit::STATUS_KIA );

	int nCivsAlive = Unit::Count( civs, MAX_CIVS, Unit::STATUS_ALIVE );
	int nCivsKIA = Unit::Count( civs, MAX_CIVS, Unit::STATUS_KIA );

	if ( nSoldiersStanding>0 && nAliensAlive==0 ) {
		victory.SetText( "Victory!" );
	}
	else if ( nSoldiersStanding==0 ) {
		if ( game->battleData.GetScenario() == TERRAN_BASE ) {
			victory.SetText( "Base Destroyed." );
		}
		else {
			victory.SetText( "Defeat." );
		}
	}
	else {
		victory.SetText( "Mission Summary:" );
	}

	const char* text[TEXT_ROW] = { "Soldiers standing",  "Soldiers down",
		                           "Aliens survived", "Aliens killed", 
								   "Civs Saved", "Civs Killed" };

	int value[TEXT_ROW]		   = {	nSoldiersStanding, nSoldiersDown,
									nAliensAlive, nAliensKIA,
									nCivsAlive, nCivsKIA
								 };

	// Lose points for soldiers killed,
	// Gain points for aliens killed,
	// Gain + Lose points for civs

	int score[3] = { 0 };

	for( int i=0; i<MAX_TERRANS; ++i ) {
		if ( soldiers[i].InUse() && !soldiers[i].IsAlive() ) {
			score[0] -= soldiers[i].GetStats().ScoreLevel();
		}
	}
	for( int i=0; i<MAX_ALIENS; ++i ) {
		if ( aliens[i].InUse() && !aliens[i].IsAlive() ) {
			score[1] += aliens[i].GetStats().ScoreLevel();
		}
	}
	if ( !game->battleData.GetDayTime() ) {
		// 50% bonus for night.
		score[1] = score[1]*3/2;
	}
	// Adjust for odds, based on the starting out numbers.
	if ( nSoldiers ) {
		score[1] = score[1] * nAliens / nSoldiers;
	}

	for( int i=0; i<MAX_CIVS; ++i ) {
		if ( civs[i].InUse() ) {
			if ( civs[i].IsAlive() )
				score[2] += civs[i].GetStats().ScoreLevel();
			else
				score[2] -= civs[i].GetStats().ScoreLevel();
		}
	}

	for( int i=0; i<TEXT_ROW; ++i ) {
		textTable[i*TEXT_COL].SetText( text[i] );

		CStr<16> sBuf = value[i];
		textTable[i*TEXT_COL+1].SetText( sBuf.c_str() );

		if ( i&1 ) {
			sBuf = score[i/2];
			textTable[i*TEXT_COL+2].SetText( sBuf.c_str() );
		}
	}

	if ( nSoldiersStanding>0 && nAliensAlive==0 ) {
		int row=0;
		for( int i=0; i<EL_MAX_ITEM_DEFS; ++i ) {
			if ( row == ITEM_NUM )
				break;

			const ItemDef* itemDef = game->GetItemDefArr().GetIndex(i);
			if ( !itemDef )
				continue;

			if ( game->battleData.GetStorage().GetCount(i) ) {
				char buf[30];
				const char* display = itemDef->displayName.c_str();

				int count =  game->battleData.GetStorage().GetCount(i);
				SNPrintf( buf, 30, "%s +%d", display, count );

				items[row].Init( &gamui2D );
				items[row].SetText( buf );
				++row;
			}
		}
	}
	CStr<16> totalBuf = (score[0]+score[1]+score[2]);
	totalScoreValue.Init( &gamui2D );
	totalScoreValue.SetText( totalBuf.c_str() );

	totalScoreLabel.Init( &gamui2D );
	totalScoreLabel.SetText( "Total Score" );

	const gamui::ButtonLook& look = game->GetButtonLook( Game::GREEN_BUTTON );
	okayButton.Init( &gamui2D, look );
	//okayButton.SetText( "OK" );
	okayButton.SetDeco(  Game::CalcDecoAtom( DECO_OKAY_CHECK, true ), 
		                 Game::CalcDecoAtom( DECO_OKAY_CHECK, false ) );	
	okayButton.SetSize( GAME_BUTTON_SIZE_F(), GAME_BUTTON_SIZE_F() );
}


TacticalEndScene::~TacticalEndScene()
{
}


void TacticalEndScene::Resize() 
{
	enum {
		DESC,
		COUNT,
		SCORE,
		ITEM
	};

	const Screenport& port = GetEngine()->GetScreenport();
	backgroundUI.background.SetSize( port.UIWidth(), port.UIHeight() );

	//									description  count				 score					items
	const float X_ORIGIN[4]  = { GAME_GUTTER_X(), GAME_GUTTER_X()+150.0f, GAME_GUTTER_X()+200.0f,	GAME_GUTTER_X()+270.0f };
	const float Y_ORIGIN		= GAME_GUTTER_Y();
	const float SPACING		= GAME_GUTTER_X();
	float yPos = GAME_GUTTER_Y();

	victory.SetPos( X_ORIGIN[DESC], yPos );
	yPos += SPACING;

	for( int i=0; i<TEXT_ROW; ++i ) {
		textTable[i*TEXT_COL].SetPos( X_ORIGIN[DESC], yPos );
		textTable[i*TEXT_COL+1].SetPos( X_ORIGIN[COUNT], yPos );
		textTable[i*TEXT_COL+2].SetPos( X_ORIGIN[SCORE], yPos );
		yPos += SPACING;
	}
	for( int i=0; i<ITEM_NUM; ++i ) {
		items[i].SetPos( X_ORIGIN[ITEM], Y_ORIGIN + (float)i*SPACING );
	}
	totalScoreValue.SetPos( X_ORIGIN[SCORE], yPos );
	totalScoreLabel.SetPos( X_ORIGIN[DESC], yPos );
	okayButton.SetPos( 0, (float)(port.UIHeight() - (GAME_BUTTON_SIZE() + 5)) );
}


void TacticalEndScene::Activate()
{
	GetEngine()->SetMap( 0 );
}



void TacticalEndScene::DrawHUD()
{
}


void TacticalEndScene::Tap(	int action, 
							const grinliz::Vector2F& screen,
							const grinliz::Ray& world )
{
	Vector2F ui;
	GetEngine()->GetScreenport().ViewToUI( screen, &ui );

	const gamui::UIItem* item = 0;
	if ( action == GAME_TAP_DOWN ) {
		gamui2D.TapDown( ui.x, ui.y );
		return;
	}
	else if ( action == GAME_TAP_CANCEL ) {
		gamui2D.TapCancel();
		return;
	}
	else if ( action == GAME_TAP_UP ) {
		item = gamui2D.TapUp( ui.x, ui.y );
	}

	if ( item == &okayButton ) {
		game->PopScene();
		game->PushScene( Game::UNIT_SCORE_SCENE, 0 );
	}
}

