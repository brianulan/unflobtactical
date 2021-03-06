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

#ifndef UFOATTACK_BATTLE_SCENE_INCLUDED
#define UFOATTACK_BATTLE_SCENE_INCLUDED

#include "scene.h"
#include "unit.h"
#include "../engine/ufoutil.h"
#include "../engine/map.h"
#include "gamelimits.h"
#include "targets.h"
#include "../grinliz/glbitarray.h"
#include "../grinliz/glvector.h"

#include "../engine/uirendering.h"
#include "../gamui/gamui.h"
#include "tacticalendscene.h"
#include "battlevisibility.h"
#include "consolewidget.h"
#include "firewidget.h"

class Model;
class UIButtonBox;
class UIButtonGroup;
class UIBar;
class UIImage;
class Engine;
class Texture;
class AI;
struct MapDamageDesc;

// Needs to be a POD because it gets 'union'ed in a bunch of events.
// size is important for the same reason.
struct MotionPath
{
	int						pathLen;
	U8						pathData[MAX_TU*2];

	grinliz::Vector2<S16>	GetPathAt( int i ) 
	{
		GLRELASSERT( i < pathLen );
		grinliz::Vector2<S16> v = { pathData[i*2+0], pathData[i*2+1] };
		return v;
	}
	void Init( const MP_VECTOR< grinliz::Vector2<S16> >& pathCache );
	void CalcDelta( int i0, int i1, grinliz::Vector2I* vec, float* rot );
	void Travel( float* travelDistance, int* pathPos, float* fraction );
	void GetPos( int step, float fraction, float* x, float* z, float* rot );
private:
	float DeltaToRotation( int dx, int dy );
};

/*
	Naming:			

	Length is always consistent for easy parsing:
	FARM_16_TILE_00.xml				Map
	FARM_16_TILE_00_TEX.png			Ground texture			(16: 128x128 pixels, 32: 256x256 pixels, 64: 512x512 pixels)
	FARM_16_TILE_00_DAY.png			Daytime light map		(16x16, 32x32, 64x64)
	FARM_16_TILE_00_NGT.png			Night light map

	Set:
		FARM

	Size:
		16, 32, 64

	Type:
		TILE		basic map tileset
		UFOx		UFO (small, on the ground)
		LAND		lander

	Variation:
		00-99
*/
class BattleScene : public Scene, public IPathBlocker
{
public:
	BattleScene( Game* );
	virtual ~BattleScene();

	virtual void Activate();
	virtual void DeActivate();

	virtual void Resize();
	virtual void Tap(	int action, 
						const grinliz::Vector2F& screen,
						const grinliz::Ray& world );

	virtual void Zoom( int style, float normal );
	virtual void Rotate( float degreesFromStart );
	virtual void CancelInput() {}
	virtual void SceneResult( int sceneID, int result );

	virtual void JoyButton( int id, bool down );
	virtual void JoyDPad( int id, int dir );
	virtual void JoyStick( int id, const grinliz::Vector2F& value );

	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D );
	virtual void DoTick( U32 currentTime, U32 deltaTime );
	virtual void Draw3D();
	virtual void DrawHUD();
	virtual void HandleHotKeyMask( int mask );

	virtual SavePathType CanSave()										{ return SAVEPATH_TACTICAL; }
	virtual void Save( tinyxml2::XMLPrinter* );
	virtual void Load( const tinyxml2::XMLElement* doc );

	// debugging / MapMaker
	void MouseMove( int x, int y );
	void RotateSelection( int delta );
	void DeleteAtSelection();
	void DeltaCurrentMapItem( int d );
	void SetLightMap( float r, float g, float b );

	virtual void MakePathBlockCurrent( Map* map, const void* user );

	const Model* GetModel( const Unit* unit );
	const Model* GetWeaponModel( const Unit* unit );
	const Unit* GetUnit( const Model* m, bool isWeaponModel ) {
		return UnitFromModel( m, isWeaponModel );
	}
	int GetUnitID( const Unit* u ) const { return u-units; }

private:
	enum {
		BTN_TAKE_OFF,
		BTN_HELP,
		BTN_END_TURN,
		BTN_TARGET,
		BTN_CHAR_SCREEN,

		BTN_ROTATE_CCW,
		BTN_ROTATE_CW,

		BTN_PREV,
		BTN_NEXT,

		BTN_COUNT
	};

	enum {
		ACTION_NONE,
		ACTION_MOVE,
		ACTION_ROTATE,
		ACTION_SHOOT,
		ACTION_DELAY,
		ACTION_HIT,
		ACTION_CAMERA_BOUNDS,
		ACTION_PSI_ATTACK
	};

	struct MoveAction	{
		int			pathStep;
		float		pathFraction;
		MotionPath	path;
	};

	struct RotateAction {
		float rotation;
	};

	struct ShootAction {
		int					mode;
		float				chanceToHit;
		float				range;
		grinliz::Vector3F	target;
	};

	struct PsiAction {
		int targetID;
	};

	struct DelayAction {
		U32 delay;
	};

	struct HitAction {
		DamageDesc						damageDesc;		// damage done.
		const WeaponItemDef::Weapon*	weapon;			// by what
		
		grinliz::Vector3F	p;				// point of impact
		grinliz::Vector3F	n;				// normal from shooter to target
		Model*				m;				// model impacted - may be 0
	};

	struct CameraAction {
		grinliz::Vector3F	start;
		grinliz::Vector3F	end;
		int					time;
		int					timeLeft;
	};

	struct CameraBoundsAction {
		grinliz::Vector3F	target;
		float				speed;
		bool				center;
	};

	struct Action
	{
		int actionID;
		Unit* unit;			// unit performing the action (sometimes null)

		union {
			MoveAction			move;
			RotateAction		rotate;
			ShootAction			shoot;
			PsiAction			psi;
			DelayAction			delay;
			HitAction			hit;
			CameraAction		camera;
			CameraBoundsAction	cameraBounds;
		} type;

		void Clear()							{ actionID = ACTION_NONE; memset( &type, 0, sizeof( type ) ); }
		void Init( int id, Unit* unit )			{ Clear(); actionID = id; this->unit = unit; }
		bool NoAction()							{ return actionID == ACTION_NONE; }
	};
	CStack< Action > actionStack;

	void PushEndScene();
	void PushRotateAction( Unit* src, const grinliz::Vector3F& dst, bool quantize );
	
	// Try to shoot. Return true if success.
	bool PushShootAction(	Unit* src, 
							const grinliz::Vector3F& target, 
							float targetWidth, 
							float targetHeight,
							int mode,
							float useError,				// if 0, perfect shot. <1 improve, 1 normal error, >1 more error
							bool clearMoveIfShoot );	// clears move commands if needed

	void PushScrollOnScreen( const grinliz::Vector3F& v, bool center=false );

	// Process the current action. Returns flags that describe what happened.
	enum { 
		STEP_COMPLETE			= 0x01,		// the step of a unit on a path completed. The unit is centered on the map grid
		UNIT_ACTION_COMPLETE	= 0x02,		// shooting, rotating, some other unit action finished (resulted in Pop() )
		OTHER_ACTION_COMPLETE	= 0x04,		// a non unit action (camera motion) finised (resulted in a Pop() )
	};		
	int ProcessAction( U32 deltaTime );
	int ProcessActionShoot( Action* action, Unit* unit );
	int ProcessActionHit( Action* action );	
	bool ProcessActionCameraBounds( U32 deltaTime, Action* action );
	void ProcessPsiAttack( Action* action );
	
	grinliz::Rectangle2F CalcInsetUIBounds();

	void OrderNextPrev();

	void StopForNewTeamTarget();
	void DoReactionFire();

	int CenterRectIntersection( const grinliz::Vector2F& p,
								const grinliz::Rectangle2F& rect,
								grinliz::Vector2F* out );

	MP_VECTOR< grinliz::Vector2<S16> >	pathCache;

	// Show the UI zones arount the selected unit
	struct NearPathState {
		const Unit*			unit;
		grinliz::Vector2I	pos;	// pos and tu are probably redundant, but let's be careful.
		float				tu;
		grinliz::Vector2I	dest;	// if a particular destination, this should be set.

		void Clear() { unit = 0; pos.Set( -1, -1 ); tu = -1.0f; dest.Set( -2, -2 ); }
	};
	NearPathState			nearPathState;
	grinliz::Vector2I		confirmDest;
	void ShowNearPath( const Unit* unit );		// call freely; does nothing if the current path is valid.

	// set the fire widget to the primary and secondary weapon
	float Travel( U32 timeMSec, float speed ) { return speed * (float)timeMSec / 1000.0f; }

	void InitUnits();
	void TestHitTesting();
	void TestCoordinates();

	Unit* UnitFromModel( const Model* m, bool useWeaponModel=false );
	Unit* GetUnitFromTile( int x, int z );

	bool HandleIconTap( const gamui::UIItem* item );
	void HandleNextUnit( int bias );
	void HandleRotation( float bias );
	void SetFogOfWar();
	void SetUnitOverlays();

	struct Selection
	{
		Selection()			{ Clear(); }
		void Clear()		{ soldierUnit = 0; targetUnit = 0; targetPos.Set( -1, -1 ); }
		void ClearTarget()	{ targetUnit = 0; targetPos.Set( -1, -1 ); }
		bool FireMode()		{ return targetUnit || (targetPos.x >= 0 && targetPos.y >= 0 ); }

		Unit*	soldierUnit;
		
		Unit*				targetUnit;
		grinliz::Vector2I	targetPos;
	};
	Selection selection;

	bool	SelectedSoldier()		{ return selection.soldierUnit != 0; }
	Unit*	SelectedSoldierUnit()	{ return selection.soldierUnit; }

	bool	HasTarget()				{ return selection.targetUnit || selection.targetPos.x >= 0; }
	bool	AlienTargeted()			{ return selection.targetUnit != 0; }
	Unit*	AlienUnit()				{ return selection.targetUnit; }

	void	SetSelection( Unit* unit );

	void NextTurn( bool saveOnTerranTurn );
	void GenerateCrawler( const Unit* unitKilled, const Unit* shooter );
	void UpgradeCrawlerToSpitter( Unit* unit );

	void Drag( int action, bool uiActivated, const grinliz::Vector2F& view );
	void DragUnitStart( const grinliz::Vector2I& map );
	void DragUnitMove( const grinliz::Vector2I& map );
	void DragUnitEnd( const grinliz::Vector2I& map );

	bool GamuiHasCapture()	{ return gamui2D.TapCaptured() || gamui3D.TapCaptured(); }

	bool isDragging;

	grinliz::Vector3F	dragStart3D;
	grinliz::Vector3F	dragEnd3D;
	grinliz::Vector2F	dragStartUI;
	grinliz::Vector2F	dragEndUI;
	grinliz::Vector2F	joyDrag;

	float				dragLength;
	grinliz::Vector3F	dragStartCameraWC;
	grinliz::Matrix4	dragMVPI;

	Unit*				dragUnit;
	grinliz::Vector2I	dragUnitDest;

	UIRenderer			uiRenderer;

	gamui::Image		turnImage;
	gamui::PushButton	alienTarget;

	gamui::Image		menuImage;
	gamui::PushButton	exitButton;
	gamui::PushButton	helpButton;
	gamui::PushButton	nextTurnButton;
	gamui::ToggleButton	targetButton;
	gamui::ToggleButton orbitButton;
	gamui::PushButton	invButton;
	enum { ROTATE_CCW_BUTTON, ROTATE_CW_BUTTON, PREV_BUTTON, NEXT_BUTTON, CONTROL_BUTTON_COUNT };
	gamui::PushButton	controlButton[4];
	FireWidget			fireWidget;
	NameRankUI			nameRankUI;
	OKCancelUI			moveOkayCancelUI;
	DecoEffect			decoEffect;
	gamui::Image		dpad;
	gamui::Image		imageL1, imageR1, imageL2, imageR2;

	gamui::Image		unitImage0[MAX_UNITS];			// map space
	gamui::Image		unitImage1[MAX_UNITS];			// map space
	gamui::Image		selectionImage;
	gamui::Image		targetArrow[MAX_ALIENS];		// screen space

	gamui::Image		dragBar[2];

	int	subTurnOrder[MAX_TERRANS];
	int	subTurnCount;
	int turnCount;
	
	Engine*			engine;
	TacMap*			tacMap;
	Storage*		lockedStorage;	// locked for use by the character scene
	grinliz::Random random;			// "the" random number generator for the battle
	int				currentTeamTurn;
	AI*				aiArr[3];
	int				currentUnitAI;
	bool			battleEnding;		// not saved - used to prevent event loops
	bool			cameraSet;
	float			orbit;

	struct TargetEvent
	{
		U8 team;		// 1: team, 0: unit
		U8 viewerID;	// unit id of viewer, or teamID if team event
		U8 targetID;	// unit id of target

		void Dump() { GLOUTPUT(( "TargetEvent team=%d viewerID=%d targetID=%d\n", team, viewerID, targetID )); }
	};

	grinliz::BitArray<MAX_UNITS, MAX_UNITS, 1>	unitVis;	// map of "previous" unit vis. Difference between this and current creates targetEvents.
	CDynArray< TargetEvent >					targetEvents;

	CDynArray< grinliz::Vector2I > doors;
	void ProcessDoors();
	bool ProcessAI();			// return true if turn over.
	void ProcessInventoryAI( Unit* unit );			// return true if turn over.

	// Updates what units can and can not see. Sets the 'Targets' structure above,
	// and generates targetEvents.
	void CalcTeamTargets();
	void DumpTargetEvents();

	Visibility visibility;

	Unit*				units;
	UnitRenderer		unitRenderers[MAX_UNITS];
	gamui::DigitalBar	hpBars[MAX_UNITS];

	// MapMaker
	void UpdatePreview();
	const char* SelectionDesc();
	Model* mapmaker_mapSelection;
	Model* mapmaker_preview;
	int    mapmaker_currentMapItem;
};



#endif // UFOATTACK_BATTLE_SCENE_INCLUDED