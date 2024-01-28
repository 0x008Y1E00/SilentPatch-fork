#include "StdAfxSA.h"
#include "ModelInfoSA.h"

#include <iterator>

static void* BaseModelInfoShutdown = AddressByVersion<void*>(0x4C4D50, 0x4C4DD0, 0x4CF590);
WRAPPER void CBaseModelInfo::Shutdown() { VARJMP(BaseModelInfoShutdown); }

static void* varSetVehicleColour = AddressByVersion<void*>( 0x4C84B0, 0x4C86B0, 0x4D2DB0 );
WRAPPER void CVehicleModelInfo::SetVehicleColour( int32_t color1, int32_t color2, int32_t color3, int32_t color4 ) { VARJMP(varSetVehicleColour); }

RwTexture* (*CCustomCarPlateMgr::CreatePlateTexture)(const char* pText, signed char nDesign) = AddressByVersion<RwTexture*(*)(const char*,signed char)>(0x6FDEA0, 0x6FE6D0, 0x736AC0);
signed char (*CCustomCarPlateMgr::GetMapRegionPlateDesign)() = AddressByVersion<signed char(*)()>(0x6FD7A0, 0x6FDFD0, 0x7363E0);
void (*CCustomCarPlateMgr::SetupMaterialPlatebackTexture)(RpMaterial* pMaterial, signed char nDesign) = AddressByVersion<void(*)(RpMaterial*,signed char)>(0x6FDE50, 0x6FE680, 0x736A80);

bool (*CCustomCarPlateMgr::GeneratePlateText)(char* pBuf, int nLen); // Read from InjectDelayedPatches

CBaseModelInfo** const			ms_modelInfoPtrs = *AddressByVersion<CBaseModelInfo***>(0x509CB1, 0x4C0C96, 0x403DB7);

int8_t* CVehicleModelInfo::ms_compsUsed = *AddressByVersion<int8_t**>( 0x4C973B + 2, Memory::PatternAndOffset("8B CE A2 ? ? ? ? E8", 2 + 1) );
int8_t* CVehicleModelInfo::ms_compsToUse = *AddressByVersion<int8_t**>( 0x4C8057 + 2, Memory::PatternAndOffset("0F BE C0 C6 05 ? ? ? ? FE 5E", 3 + 2) );


static RwTexture** const		ms_aDirtTextures = *AddressByVersion<RwTexture***>( 0x5D5DCC + 3, 0, 0x5F259C + 3 );
void RemapDirt( CVehicleModelInfo* modelInfo, uint32_t dirtID )
{
	RpMaterial** materials = modelInfo->m_numDirtMaterials > CVehicleModelInfo::IN_PLACE_BUFFER_DIRT_SIZE ? modelInfo->m_dirtMaterials : modelInfo->m_staticDirtMaterials;

	for ( size_t i = 0; i < modelInfo->m_numDirtMaterials; i++ )
	{
		RpMaterialSetTexture( materials[i], ms_aDirtTextures[dirtID] );
	}
}

uint32_t CVehicleModelInfo::GetNumRemaps() const
{
	uint32_t count = 0;
	while ( m_awRemapTxds[count].Get() != -1 && count < _countof(m_awRemapTxds) )
	{
		count++;
	}
	return count;
}

void CVehicleModelInfo::Shutdown()
{
	CBaseModelInfo::Shutdown();

	delete[] m_dirtMaterials;
	m_dirtMaterials = nullptr;
}

void CVehicleModelInfo::FindEditableMaterialList()
{
	std::vector<RpMaterial*> editableMaterials;

	auto GetEditableMaterialListCB = [&]( RpAtomic* atomic ) -> RpAtomic* {
		RpGeometryForAllMaterials( RpAtomicGetGeometry(atomic), [&]( RpMaterial* material ) -> RpMaterial* {
			if ( RwTexture* texture = RpMaterialGetTexture(material) )
			{
				if ( const char* texName = RwTextureGetName(texture) )
				{
					if ( strcmp(texName, "vehiclegrunge256") == 0 )
					{
						editableMaterials.push_back( material );
					}
				}
			}
			return material;
		} );
		return atomic;
	};

	RpClumpForAllAtomics(reinterpret_cast<RpClump*>(pRwObject), GetEditableMaterialListCB);

	for ( uint32_t i = 0; i < m_pVehicleStruct->m_nNumExtras; i++ )
		GetEditableMaterialListCB(m_pVehicleStruct->m_apExtras[i]);

	m_numDirtMaterials = editableMaterials.size();
	if ( m_numDirtMaterials > IN_PLACE_BUFFER_DIRT_SIZE )
	{
		m_dirtMaterials = new RpMaterial* [m_numDirtMaterials];
		std::copy( editableMaterials.begin(), editableMaterials.end(), stdext::make_checked_array_iterator(m_dirtMaterials, m_numDirtMaterials) );
	}
	else
	{
		m_dirtMaterials = nullptr;

		// Use existing space instead of allocating new space
		std::copy( editableMaterials.begin(), editableMaterials.end(), m_staticDirtMaterials );
	}

	m_nPrimaryColor = -1;
	m_nSecondaryColor = -1;
	m_nTertiaryColor = -1;
	m_nQuaternaryColor = -1;
}

void CVehicleModelInfo::SetCarCustomPlate()
{
	m_plateText[0] = '\0';
	m_nPlateType = -1;
}

void CVehicleModelInfo::ResetCompsForNoExtras()
{
	ms_compsUsed[0] = ms_compsUsed[1] = -1;
	ms_compsToUse[0] = ms_compsToUse[1] = -2;
}

void CCustomCarPlateMgr::SetupClumpAfterVehicleUpgrade(RpClump* pClump, void* /*unused*/, signed char nDesign)
{
	UNREFERENCED_PARAMETER(nDesign);
}