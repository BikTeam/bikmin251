#include "Game/Entities/ItemPikihead.h"
#include "Game/PikiMgr.h"
#include "JSystem/J3D/J3DModelLoader.h"
#include "JSystem/J3D/J3DTransform.h"
#include "SysShape/ModelMgr.h"

namespace Game {
namespace ItemPikihead {

// allocModels__Q34Game12ItemPikihead3MgrFv
void Mgr::allocModels()
{
	m_monoObjectMgr.alloc(101);
	onAlloc();
}

void Mgr::onLoadResources()
{
    m_modelData[0] = pikiMgr->m_pikiModels[7];

	loadArchive("arc.szs");
	JKRArchive* arc = openTextArc("texts.szs");
	loadAnimMgr(arc, "pikiheadAnimMgr.txt");
	closeTextArc(arc);
}

int getEmptyIndex(MonoObjectMgr<Item>* mgr)
{
    if (!pikiMgr) return mgr->getEmptyIndex();

	for (int i = 0; i < mgr->m_max; i++) {
		if (mgr->m_openIds[i] && pikiMgr->m_openIds[i]) {
			return i;
		}
	}
	return -1;
}

// birthPikiHead__Q24Game12ItemPikiheadFP41MonoObjectMgr<Q34Game12ItemPikihead4Item>
Item* birthPikiHead(MonoObjectMgr<Item>* mgr)
{
    int index = getEmptyIndex(mgr);
	if (index != -1) {
		Item* result = &mgr->m_array[index];
		mgr->m_openIds[index] = false;
		mgr->m_activeCount++;
		return result;
	}
	return nullptr;
}

// createPikiheadModel__Q34Game12ItemPikihead3MgrFPQ34Game12ItemPikihead4Item
SysShape::Model* Mgr::createPikiheadModel(Item* item)
{
	OSReport("heap index: %i\n", item->_184);
    SysShape::Model* model = pikiMgr->createModel(7, item->_184);

    JKRSolidHeap* solidHeap = pikiMgr->m_modelMgr->m_heaps[item->_184];
    sys->startChangeCurrentHeap(solidHeap);
    
	onCreateModel(model);

    sys->endChangeCurrentHeap();

	return model;
}

} // namespace ItemPikiHead

int PikiMgr::getSharedEmptyIndex()
{
    if (!ItemPikihead::mgr) return getEmptyIndex();

    for (int i = 0; i < m_max; i++) {
		if (m_openIds[i] && ItemPikihead::mgr->m_monoObjectMgr.m_openIds[i]) {
			return i;
		}
	}
	return -1;
}

// birthPikiModel__Q24Game7PikiMgrFv
Piki* PikiMgr::birthPikiModel() 
{
	int index = getSharedEmptyIndex();
	if (index != -1) {
		Piki* result = &m_array[index];
		m_openIds[index] = false;
		m_activeCount++;
		return result;
	}
	return nullptr;
}

void PikiMgr::loadBmd(int id, char* name)
{
	char pathbuf[256];
	sprintf(pathbuf, "piki_model/%s.bmd", name);
	J3DModelData* data = J3DModelLoaderDataBase::load(m_modelArchive->getResource(pathbuf), 0x60010);
	{
		const u32 lightObjNum = 0;
		const u32 texGenNum   = 0;
		const u32 texCoordNum = 4;
		const u32 tevStageNum = 0;
		u32 dlFlags           = CREATE_DIFF_FLAG(lightObjNum, texGenNum, texCoordNum, tevStageNum);
		data->newSharedDisplayList(dlFlags);
		data->makeSharedDL();
	}

	m_pikiModels[id] = data;
}

// loadPikiHead__Q24Game7PikiMgrFv
void PikiMgr::loadPikiHead()
{
    JKRArchive* arc = JKRArchive::mount("user/Kando/objects/pikihead/arc.szs", JKRArchive::EMM_Mem, sys->mSysHeap, JKRArchive::EMD_Head);

    J3DModelData* data = J3DModelLoaderDataBase::load(arc->getResource("pikihead.bmd"), 0x20000);
	{
		const u32 lightObjNum = 0;
		const u32 texGenNum   = 0;
		const u32 texCoordNum = 4;
		const u32 tevStageNum = 0;
		u32 dlFlags           = CREATE_DIFF_FLAG(lightObjNum, texGenNum, texCoordNum, tevStageNum);
		data->newSharedDisplayList(dlFlags);
	}

    m_pikiModels[7] = data;
}

void PikiMgr::createModelCallback(SysShape::Model*)
{
    // do nothing
}

SysShape::Model* PikiMgr::createModel(int id, int num)
{
    SysShape::Model* model = m_modelMgr->createModel(id, num);
	if (id < 7) {
		model->m_j3dModel->calcMaterial();
		model->m_j3dModel->makeDL();
		model->m_j3dModel->lock();
	}
	return model;
}

// im lazy
namespace
{
	enum J3DModelLoaderFlagTypes {
		J3DMLF_None = 0x00000000,

		J3DMLF_MtxSoftImageCalc = 0x00000001,
		J3DMLF_MtxMayaCalc      = 0x00000002,
		J3DMLF_MtxBasicCalc     = 0x00000004,
		J3DMLF_MtxTypeMask
		= J3DMLF_MtxSoftImageCalc | J3DMLF_MtxMayaCalc | J3DMLF_MtxBasicCalc | J3DMLF_04, // 0 - 2 (0 = Basic, 1 = SoftImage, 2 = Maya)

		J3DMLF_UseImmediateMtx          = 0x00000010,
		J3DMLF_UsePostTexMtx            = 0x00000020,
		J3DMLF_NoMatrixTransform        = 0x00000100,
		J3DMLF_DoBdlMaterialCalc        = 0x00002000,
		J3DMLF_TevNumShift              = 0x00010000,
		J3DMLF_UseSingleSharedDL        = 0x00040000,
		J3DMLF_UseUniqueMaterials       = 0x00200000,
		J3DMLF_Material_UseIndirect     = 0x01000000,
		J3DMLF_Material_TexGen_Block4   = 0x08000000,
		J3DMLF_Material_PE_Full         = 0x10000000,
		J3DMLF_Material_PE_FogOff       = 0x20000000,
		J3DMLF_Material_Color_LightOn   = 0x40000000,
		J3DMLF_Material_Color_AmbientOn = 0x80000000
	};
} // namespace


/**
 * @note Address: 0x8015EBD8
 * @note Size: 0x298
 */
void PikiMgr::load(int viewNum)
{
	JKRHeap* heap = JKRHeap::sCurrentHeap;
	heap->getFreeSize();
	JKRArchive* arc;
	
	int* isClean =  (int*)0x80000f98;
	if (*isClean)
		arc = JKRArchive::mount("/user/Kando/piki/pclns.szs", JKRArchive::EMM_Mem, sys->mSysHeap, JKRArchive::EMD_Head);
	else 
		arc = JKRArchive::mount("/user/Kando/piki/pikis.szs", JKRArchive::EMM_Mem, sys->mSysHeap, JKRArchive::EMD_Head);

	m_modelArchive   = arc;
	heap->getFreeSize();

	JUT_ASSERTLINE(450, arc, "pikis.szs not found !\n");

	loadBmd(Blue, "piki_p2_blue");
	loadBmd(Red, "piki_p2_red");
	loadBmd(Yellow, "piki_p2_yellow");
	loadBmd(White, "piki_p2_white");
	loadBmd(Purple, "piki_p2_black");
	loadBmd(Bulbmin, "piki_kochappy");
	loadBmd(Carrot, "piki_ninjin");
	loadPikiHead();

	m_happaModel[Leaf] = J3DModelLoaderDataBase::load(arc->getResource("happa_model/leaf.bmd"), J3DMLF_Material_PE_FogOff);
	m_happaModel[Bud]
	    = J3DModelLoaderDataBase::load(arc->getResource("happa_model/bud.bmd"), J3DMLF_UseUniqueMaterials | J3DMLF_UseSingleSharedDL);
	m_happaModel[Flower]
	    = J3DModelLoaderDataBase::load(arc->getResource("happa_model/flower.bmd"), J3DMLF_UseUniqueMaterials | J3DMLF_UseSingleSharedDL);
	m_happaModel[Bud_Red]
	    = J3DModelLoaderDataBase::load(arc->getResource("happa_model/bud_red.bmd"), J3DMLF_UseUniqueMaterials | J3DMLF_UseSingleSharedDL);
	m_happaModel[Flower_Red] = J3DModelLoaderDataBase::load(arc->getResource("happa_model/flower_red.bmd"),
	                                                       J3DMLF_UseUniqueMaterials | J3DMLF_UseSingleSharedDL);

	sys->heapStatusStart("pikmin-ModelMgr", nullptr);
	m_modelMgr = new SysShape::ModelMgr(PikiColorCount + 1, m_pikiModels, 101, 0x20000, viewNum, nullptr);
	sys->heapStatusEnd("pikmin-ModelMgr");

	for (int i = 0; i < ARRAY_SIZE(m_happaModel); i++) {
		J3DModelData* model = m_happaModel[i];

		const u32 lightObjNum = 0;
		const u32 texGenNum   = 0;
		const u32 texCoordNum = 4;
		const u32 tevStageNum = 0;
		u32 dlFlags           = CREATE_DIFF_FLAG(lightObjNum, texGenNum, texCoordNum, tevStageNum);
		model->newSharedDisplayList(dlFlags);
		model->simpleCalcMaterial(0, *(Mtx*)j3dDefaultMtx);
		model->makeSharedDL();
	}
}


} // namespace Game
