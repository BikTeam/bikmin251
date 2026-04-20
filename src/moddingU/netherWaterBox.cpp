#include "Game/seaMgr.h"
#include "Game/GameSystem.h"
#include "Game/gameStages.h"
#include "SysShape/Model.h"

namespace Game {

// createDiffedWaterboxModel__4GameFPQ28SysShape5Model
// in gameSeaMgr.s
SysShape::Model* createDiffedWaterboxModel(SysShape::Model* model)
{
	model->m_j3dModel->newDifferedDisplayList(J3DMDF_DiffColorReg);
	return model;
}

// waterBoxChangeMaterial__4GameFPQ24Game12AABBWaterBox
// in gameSeaMgr.s
void waterBoxChangeMaterial(AABBWaterBox* waterBox)
{
	J3DModelData* modelData = waterBox->m_model->m_j3dModel->m_modelData;

	J3DGXColorS10 sColor(255, 255, 255, 255);
	if (gameSystem->m_section->getCurrentCourseInfo()->m_courseIndex == 2) {
		// Load different water color in Course index 2 (Yakushima)
		sColor = J3DGXColorS10(221, 152, 33, 127);
	}

	for (int i = 0; i < modelData->getMaterialCount1(); i++) {
		J3DTevBlock* block = modelData->m_materialTable.m_materials1[i]->m_tevBlock;
		block->setTevColor(0, &sColor);
	}

	waterBox->m_model->m_j3dModel->diff();
}

} // namespace Game
