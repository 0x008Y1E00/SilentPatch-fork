#include "StdAfxSA.h"

#include <functional>
#include <algorithm>
#include <vector>
#include "VehicleSA.h"
#include "TimerSA.h"
#include "PedSA.h"
#include "DelimStringReader.h"

static constexpr float PHOENIX_FLUTTER_PERIOD	= 70.0f;
static constexpr float PHOENIX_FLUTTER_AMP		= 0.13f;
static constexpr float SWEEPER_BRUSH_SPEED      = 0.3f;

static constexpr float PI = 3.14159265358979323846f;

std::vector<int32_t>		vecRotorExceptions;

float CAutomobile::ms_engineCompSpeed;

static bool ShouldIgnoreRotor( int32_t id )
{
	return std::find( vecRotorExceptions.begin(), vecRotorExceptions.end(), id ) != vecRotorExceptions.end();
}

static void*	varVehicleRender = AddressByVersion<void*>(0x6D0E60, 0x6D1680, 0x70C0B0);
WRAPPER void CVehicle::Render() { VARJMP(varVehicleRender); }
static void*	varIsLawEnforcementVehicle = AddressByVersion<void*>(0x6D2370, 0x6D2BA0, 0x70D8C0);
WRAPPER bool CVehicle::IsLawEnforcementVehicle() { VARJMP(varIsLawEnforcementVehicle); }

auto			FindPlayerPed = AddressByVersion<CPed*(*)(int)>( 0x56E210, 0, 0 ); // TODO: DO

void (CVehicle::*CVehicle::orgVehiclePreRender)();
void (CAutomobile::*CAutomobile::orgAutomobilePreRender)();
void (CPlane::*CPlane::orgPlanePreRender)();
CVehicle* (CStoredCar::*CStoredCar::orgRestoreCar)();

static int32_t random(int32_t from, int32_t to)
{
	return from + ( Int32Rand() % (to-from) );
}

static RwObject* GetCurrentAtomicObject( RwFrame* frame )
{
	RwObject* obj = nullptr;
	RwFrameForAllObjects( frame, [&obj]( RwObject* object ) -> RwObject* {
		if ( RpAtomicGetFlags(object) & rpATOMICRENDER )
		{
			obj = object;
			return nullptr;
		}
		return object;
	} );
	return obj;
}

static RwFrame* GetFrameFromName( RwFrame* topFrame, const char* name )
{
	class GetFramePredicate
	{
	public:
		RwFrame* foundFrame = nullptr;

		GetFramePredicate( const char* name )
			: m_name( name )
		{
		}

		RwFrame* operator() ( RwFrame* frame )
		{
			if ( strcmp( m_name, GetFrameNodeName(frame) ) == 0 )
			{
				foundFrame = frame;
				return nullptr;
			}
			RwFrameForAllChildren( frame, *this );
			return foundFrame != nullptr ? nullptr : frame;
		}
	
	private:
		const char* const m_name;
	};

	GetFramePredicate p( name );
	RwFrameForAllChildren( topFrame, p );
	return p.foundFrame;
}

void ReadRotorFixExceptions(const wchar_t* pPath)
{
	constexpr size_t SCRATCH_PAD_SIZE = 32767;
	WideDelimStringReader reader( SCRATCH_PAD_SIZE );

	GetPrivateProfileSectionW( L"RotorFixExceptions", reader.GetBuffer(), reader.GetSize(), pPath );
	while ( const wchar_t* str = reader.GetString() )
	{
		int32_t toList = wcstol( str, nullptr, 0 );
		if ( toList > 0 )
			vecRotorExceptions.push_back( toList );
	}
}

void CVehicle::SetComponentAtomicAlpha(RpAtomic* pAtomic, int nAlpha)
{
	RpGeometry*	pGeometry = RpAtomicGetGeometry(pAtomic);
	pGeometry->flags |= rpGEOMETRYMODULATEMATERIALCOLOR;

	RpGeometryForAllMaterials( pGeometry, [nAlpha] (RpMaterial* material) {
		material->color.alpha = RwUInt8(nAlpha);
		return material;
	} );
}

bool CVehicle::CustomCarPlate_TextureCreate(CVehicleModelInfo* pModelInfo)
{
	char		PlateText[CVehicleModelInfo::PLATE_TEXT_LEN+1];
	const char*	pOverrideText = pModelInfo->GetCustomCarPlateText();

	if ( pOverrideText )
		strncpy_s(PlateText, pOverrideText, CVehicleModelInfo::PLATE_TEXT_LEN);
	else
		CCustomCarPlateMgr::GeneratePlateText(PlateText, CVehicleModelInfo::PLATE_TEXT_LEN);

	PlateText[CVehicleModelInfo::PLATE_TEXT_LEN] = '\0';
	PlateTexture = CCustomCarPlateMgr::CreatePlateTexture(PlateText, pModelInfo->m_nPlateType);
	if ( pModelInfo->m_nPlateType != -1 )
		PlateDesign = pModelInfo->m_nPlateType;
	else if ( IsLawEnforcementVehicle() )
		PlateDesign = CCustomCarPlateMgr::GetMapRegionPlateDesign();
	else
 		PlateDesign = random(0, 20) == 0 ? int8_t(random(0, 3)) : CCustomCarPlateMgr::GetMapRegionPlateDesign();

	assert(PlateDesign >= 0 && PlateDesign < 3);

	pModelInfo->m_plateText[0] = '\0';
	pModelInfo->m_nPlateType = -1;

	return true;
}

void CVehicle::CustomCarPlate_BeforeRenderingStart(CVehicleModelInfo* pModelInfo)
{
	for ( size_t i = 0; i < pModelInfo->m_apPlateMaterials->m_numPlates; i++ )
	{
		RpMaterialSetTexture(pModelInfo->m_apPlateMaterials->m_plates[i], PlateTexture);
	}

	for ( size_t i = 0; i < pModelInfo->m_apPlateMaterials->m_numPlatebacks; i++ )
	{
		CCustomCarPlateMgr::SetupMaterialPlatebackTexture(pModelInfo->m_apPlateMaterials->m_platebacks[i], PlateDesign);
	}
}

void CVehicle::SetComponentRotation( RwFrame* component, eRotAxis axis, float angle, bool absolute )
{
	if ( component == nullptr ) return;

	CMatrix matrix( RwFrameGetMatrix(component) );
	if ( absolute )
	{
		if ( axis == ROT_AXIS_X ) matrix.SetRotateXOnly(angle);
		else if ( axis == ROT_AXIS_Y ) matrix.SetRotateYOnly(angle);
		else if ( axis == ROT_AXIS_Z ) matrix.SetRotateZOnly(angle);
	}
	else
	{
		const CVector pos = matrix.GetPos();
		matrix.SetTranslateOnly(0.0f, 0.0f, 0.0f);

		if ( axis == ROT_AXIS_X ) matrix.RotateX(angle);
		else if ( axis == ROT_AXIS_Y ) matrix.RotateY(angle);
		else if ( axis == ROT_AXIS_Z ) matrix.RotateZ(angle);

		matrix.GetPos() += pos;
	}
	matrix.UpdateRW();
}

void CHeli::Render()
{
	double		dRotorsSpeed, dMovingRotorSpeed;
	bool		bDisplayRotors = !ShouldIgnoreRotor( m_nModelIndex.Get() );
	bool		bHasMovingRotor = m_pCarNode[13] != nullptr && bDisplayRotors;
	bool		bHasMovingRotor2 = m_pCarNode[15] != nullptr && bDisplayRotors;

	m_nTimeTillWeNeedThisCar = CTimer::m_snTimeInMilliseconds + 3000;

	if ( m_fRotorSpeed > 0.0 )
		dRotorsSpeed = std::min(1.7 * (1.0/0.22) * m_fRotorSpeed, 1.5);
	else
		dRotorsSpeed = 0.0;

	dMovingRotorSpeed = dRotorsSpeed - 0.4;
	if ( dMovingRotorSpeed < 0.0 )
		dMovingRotorSpeed = 0.0;

	int			nStaticRotorAlpha = static_cast<int>(std::min((1.5-dRotorsSpeed) * 255.0, 255.0));
	int			nMovingRotorAlpha = static_cast<int>(std::min(dMovingRotorSpeed * 175.0, 175.0));

	if ( m_pCarNode[12] != nullptr )
	{
		RpAtomic*	pOutAtomic = (RpAtomic*)GetCurrentAtomicObject( m_pCarNode[12] );
		if ( pOutAtomic != nullptr )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingRotor ? nStaticRotorAlpha : 255);
	}

	if ( m_pCarNode[14] != nullptr )
	{
		RpAtomic*	pOutAtomic = (RpAtomic*)GetCurrentAtomicObject( m_pCarNode[14] );
		if ( pOutAtomic != nullptr )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingRotor2 ? nStaticRotorAlpha : 255);
	}

	if ( m_pCarNode[13] != nullptr )
	{
		RpAtomic*	pOutAtomic = (RpAtomic*)GetCurrentAtomicObject( m_pCarNode[13] );
		if ( pOutAtomic != nullptr )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingRotor ? nMovingRotorAlpha : 0);
	}

	if ( m_pCarNode[15] != nullptr )
	{
		RpAtomic*	pOutAtomic = (RpAtomic*)GetCurrentAtomicObject( m_pCarNode[15] );
		if ( pOutAtomic != nullptr )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingRotor2 ? nMovingRotorAlpha : 0);
	}

	CEntity::Render();
}

void CPlane::Render()
{
	double		dRotorsSpeed, dMovingRotorSpeed;
	bool		bDisplayRotors = !ShouldIgnoreRotor( m_nModelIndex.Get() );
	bool		bHasMovingProp = m_pCarNode[13] != nullptr && bDisplayRotors;
	bool		bHasMovingProp2 = m_pCarNode[15] != nullptr && bDisplayRotors;

	m_nTimeTillWeNeedThisCar = CTimer::m_snTimeInMilliseconds + 3000;

	if ( m_fPropellerSpeed > 0.0 )
		dRotorsSpeed = std::min(1.7 * (1.0/0.31) * m_fPropellerSpeed, 1.5);
	else
		dRotorsSpeed = 0.0;

	dMovingRotorSpeed = dRotorsSpeed - 0.4;
	if ( dMovingRotorSpeed < 0.0 )
		dMovingRotorSpeed = 0.0;

	int			nStaticRotorAlpha = static_cast<int>(std::min((1.5-dRotorsSpeed) * 255.0, 255.0));
	int			nMovingRotorAlpha = static_cast<int>(std::min(dMovingRotorSpeed * 175.0, 175.0));

	if ( m_pCarNode[12] != nullptr )
	{
		RpAtomic*	pOutAtomic = (RpAtomic*)GetCurrentAtomicObject( m_pCarNode[12] );
		if ( pOutAtomic != nullptr )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingProp ? nStaticRotorAlpha : 255);
	}

	if ( m_pCarNode[14] != nullptr )
	{
		RpAtomic*	pOutAtomic = (RpAtomic*)GetCurrentAtomicObject( m_pCarNode[14] );
		if ( pOutAtomic != nullptr )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingProp2 ? nStaticRotorAlpha : 255);
	}

	if ( m_pCarNode[13] != nullptr )
	{
		RpAtomic*	pOutAtomic = (RpAtomic*)GetCurrentAtomicObject( m_pCarNode[13] );
		if ( pOutAtomic != nullptr )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingProp ? nMovingRotorAlpha : 0);
	}

	if ( m_pCarNode[15] != nullptr )
	{
		RpAtomic*	pOutAtomic = (RpAtomic*)GetCurrentAtomicObject( m_pCarNode[15] );
		if ( pOutAtomic != nullptr )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingProp2 ? nMovingRotorAlpha : 0);
	}

	CVehicle::Render();
}

void CPlane::Fix_SilentPatch()
{
	// Reset bouncing panels
	// No reset on Vortex
	for ( ptrdiff_t i = m_nModelIndex.Get() == 539 ? 1 : 0; i < 3; i++ )
	{
		m_aBouncingPanel[i].m_nNodeIndex = -1;
	}
}

void CPlane::PreRender()
{
	(this->*(orgPlanePreRender))();

	const int32_t extID = m_nModelIndex.Get();

	auto copyRotation = [&]( size_t src, size_t dest ) {
		if ( m_pCarNode[src] != nullptr && m_pCarNode[dest] != nullptr )
		{
			RwMatrix* lhs = RwFrameGetMatrix( m_pCarNode[dest] );
			const RwMatrix* rhs = RwFrameGetMatrix( m_pCarNode[src] );

			lhs->at = rhs->at;
			lhs->up = rhs->up;
			lhs->right = rhs->right;
			RwMatrixUpdate( lhs );
		}
	};

	if ( extID == 511 )
	{
		copyRotation( 18, 21 );
	}

	if ( extID == 513 )
	{
		copyRotation( 19, 23 );
		copyRotation( 20, 24 );
	}
}

void CBoat::PreRender_SilentPatch()
{
	(this->*(orgVehiclePreRender))();

	// Fixed moving prop for Predator/Tropic/Reefer
	const int32_t extID = m_nModelIndex.Get();
	if ( (extID == 430 || extID == 453 || extID == 454) && m_pBoatNode[1] == nullptr )
	{
		m_pBoatNode[1] = GetFrameFromName( RpClumpGetFrame(m_pRwObject), "boat_moving" );
	}
}

void CAutomobile::PreRender()
{
	// For rotating engine components
	ms_engineCompSpeed = m_nVehicleFlags.bEngineOn ? CTimer::m_fTimeStep : 0.0f;

	(this->*(orgAutomobilePreRender))();

	const int32_t extID = m_nModelIndex.Get();
	if ( extID == 603 )
	{
		ProcessPhoenixBlower( extID );
	}

	if ( extID == 574 )
	{
		ProcessSweeper();
	}

	if ( extID == 582 )
	{
		ProcessNewsvan();
	}
}

void CAutomobile::Fix_SilentPatch()
{
	ResetFrames();

	// Reset bouncing panels
	const int32_t extID = m_nModelIndex.Get();
	for ( ptrdiff_t i = (extID == 525 && m_pCarNode[21]) || (extID == 531 && m_pCarNode[17]) ? 1 : 0; i < 3; i++ )
	{
		// Towtruck/Tractor fix
		m_aBouncingPanel[i].m_nNodeIndex = -1;
	}
}

void CAutomobile::ResetFrames()
{
	RpClump*	pOrigClump = reinterpret_cast<RpClump*>(ms_modelInfoPtrs[ m_nModelIndex.Get() ]->pRwObject);
	if ( pOrigClump != nullptr )
	{
		// Instead of setting frame rotation to (0,0,0) like R* did, obtain the original frame matrix from CBaseNodelInfo clump
		for ( ptrdiff_t i = 8; i < 25; i++ )
		{
			if ( m_pCarNode[i] != nullptr )
			{
				// Find a frame in CBaseModelInfo object
				RwFrame* origFrame = GetFrameFromName( RpClumpGetFrame(pOrigClump), GetFrameNodeName(m_pCarNode[i]) );
				if ( origFrame != nullptr )
				{
					// Found a frame, reset it
					*RwFrameGetMatrix(m_pCarNode[i]) = *RwFrameGetMatrix(origFrame);
					RwMatrixUpdate(RwFrameGetMatrix(m_pCarNode[i]));
				}
			}
		}
	}
}

void CAutomobile::ProcessPhoenixBlower( int32_t modelID )
{
	if ( m_pCarNode[20] == nullptr ) return;

	RpClump*	pOrigClump = reinterpret_cast<RpClump*>(ms_modelInfoPtrs[ modelID ]->pRwObject);
	if ( pOrigClump != nullptr )
	{
		RwFrame* origFrame = GetFrameFromName( RpClumpGetFrame(pOrigClump), GetFrameNodeName(m_pCarNode[20]) );
		if ( origFrame != nullptr )
		{
			*RwFrameGetMatrix(m_pCarNode[20]) = *RwFrameGetMatrix(origFrame);
		}
	}

	float finalAngle = 0.0f;
	if ( m_fGasPedal > 0.0f )
	{
		if ( m_fSpecialComponentAngle < 1.3f )
		{
			finalAngle = m_fSpecialComponentAngle = std::min( m_fSpecialComponentAngle + 0.1f * CTimer::m_fTimeStep, 1.3f );
		}
		else
		{
			finalAngle = m_fSpecialComponentAngle + (std::sin( (CTimer::m_snTimeInMilliseconds % 10000) / PHOENIX_FLUTTER_PERIOD ) * PHOENIX_FLUTTER_AMP);
		}
	}
	else
	{
		if ( m_fSpecialComponentAngle > 0.0f )
		{
			finalAngle = m_fSpecialComponentAngle = std::max( m_fSpecialComponentAngle - 0.05f * CTimer::m_fTimeStep, 0.0f );
		}
	}

	SetComponentRotation( m_pCarNode[20], ROT_AXIS_X, finalAngle, false );
}

void CAutomobile::ProcessSweeper()
{
	if ( !m_nVehicleFlags.bEngineOn ) return;

	if ( GetStatus() == STATUS_PLAYER || GetStatus() == STATUS_PHYSICS || GetStatus() == STATUS_SIMPLE )
	{
		if ( m_pCarNode[20] == nullptr )
		{
			m_pCarNode[20] = GetFrameFromName( RpClumpGetFrame(m_pRwObject), "misca" );
		}
		if ( m_pCarNode[21] == nullptr )
		{
			m_pCarNode[21] = GetFrameFromName( RpClumpGetFrame(m_pRwObject), "miscb" );
		}

		const float angle = CTimer::m_fTimeStep * SWEEPER_BRUSH_SPEED;

		SetComponentRotation( m_pCarNode[20], ROT_AXIS_Z, angle, false );
		SetComponentRotation( m_pCarNode[21], ROT_AXIS_Z, -angle, false );
	}
}

void CAutomobile::ProcessNewsvan()
{
	if ( GetStatus() == STATUS_PLAYER )
	{
		// TODO: Point at something? Like nearest collectable or safehouse
		m_fGunOrientation += CTimer::m_fTimeStep * 0.05f;
		if ( m_fGunOrientation > 2.0f * PI ) m_fGunOrientation -= 2.0f * PI;
		SetComponentRotation( m_pCarNode[20], ROT_AXIS_Z, m_fGunOrientation );
	}
}

CVehicle* CStoredCar::RestoreCar_SilentPatch()
{
	CVehicle* vehicle = (this->*(orgRestoreCar))();

	// Fixup bomb stuff
	if ( vehicle->GetClass() == VEHICLE_AUTOMOBILE || vehicle->GetClass() == VEHICLE_BIKE )
	{
		vehicle->SetBombOnBoard( m_bombType );
		vehicle->SetBombOwner( FindPlayerPed(-1) );
	}

	return vehicle;
}