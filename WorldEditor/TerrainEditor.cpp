#include "TerrainEditor.h"
#include "RenderSystem.h"
#include "Scene.h"

CTerrainEditor::CTerrainEditor() : CTerrain(),
m_bShowLayer0(true),
m_bShowLayer1(true),
m_bShowAttribute(false),
m_bShowGrid(false),
m_bShowBrushDecal(false)
{
}

CTerrainEditor::~CTerrainEditor()
{
}

bool CTerrainEditor::create()
{
	CTerrain::create();
	createBrush();
	return true;
}

#define  BRUSH_SIZE 8
void CTerrainEditor::createBrush()
{
	m_BrushDecal.create(GetVertexXCount(),BRUSH_SIZE);
}

void CTerrainEditor::drawAttribute()
{
	CRenderSystem& R = GetRenderSystem();
	{
		R.SetAlphaTestFunc(false);
		R.SetBlendFunc(true);
		R.SetTextureColorOP(0,TBOP_SOURCE1,TBS_TFACTOR);
		R.SetTextureAlphaOP(0,TBOP_SOURCE1,TBS_TFACTOR);
	}
	for (std::map<unsigned char,TerrainSub>::iterator it = m_mapRenderAttributeSubs.begin(); it!=m_mapRenderAttributeSubs.end(); it++)
	{
		Color32 c=0x80000000;
		if (it->first&ATTRIBUTE_SAFE) // safe
		{
			c.g=255;
		}
		if (it->first&ATTRIBUTE_BREAK) // break
		{
			c.r=255;
		}
		if (it->first&ATTRIBUTE_UNVISIBLE) // unvisible
		{
			c.b=255;
		}
		if (it->first&(1<<1))
		{
			c=0x80778899;
		}
		if (it->first&(1<<4))
		{
			c=0x80778899;
		}
		R.SetTextureFactor(c);
		R.drawIndexedSubset(it->second);
	}
}

void CTerrainEditor::Render()
{
	//CTerrain::Render();

	if (m_bShowBox)
	{
		DrawCubeBoxes();
	}
	// 地形 VB设置一次
	if (Prepare())
	{
		// 地块
		if (m_bShowLayer0)
		{
			drawLayer0();
		}
		if (m_bShowLayer1)
		{
			drawLayer1();
		}
		if (m_bShowAttribute)
		{
			drawAttribute();
		}
		if (m_bShowGrid)
		{
			drawGrid();
		}

		// 阴影
		//if (LightMapPrepare())
		//{
		//	
		//}
		//LightMapFinish();
		if (m_bShowBrushDecal)
		{
			m_BrushDecal.Render();
		}
	}

	renderGrass();
}

void CTerrainEditor::drawGrid()
{
	CRenderSystem& R = GetRenderSystem();
	R.setTextureMatrix(0,TTF_DISABLE);
	if (R.prepareMaterial("grid"))
	{
		draw();
		R.finishMaterial();
	}
}

void CTerrainEditor::markEdit()
{
	if(m_setReback.size()>0)
	{
		if(m_setReback.back().empty())
		{
			return;
		}
	}
	m_setReback.resize(m_setReback.size()+1);
}

void CTerrainEditor::doEdit(MAP_EDIT_RECORD& mapEditRecordIn,MAP_EDIT_RECORD& mapEditRecordOut)
{
	if(mapEditRecordIn.empty())
	{
		return;
	}

	bool bUpdateCube = false;
	bool bUpdateIB = false;

	int nBeginX = mapEditRecordIn.begin()->first.x;
	int nBeginY = mapEditRecordIn.begin()->first.y;

	for(std::map<EditTarget,EditValue>::iterator it=mapEditRecordIn.begin();it!=mapEditRecordIn.end();it++)
	{
		if(mapEditRecordOut.find(it->first)==mapEditRecordOut.end())
		{
			EditValue& editValue = mapEditRecordOut[it->first];
			switch(it->first.type)
			{
			case CTerrainBrush::BRUSH_TYPE_TERRAIN_HEIGHT:
				editValue.fHeight = getVertexHeight(it->first.x,it->first.y);
				break;
			case CTerrainBrush::BRUSH_TYPE_TERRAIN_ATT:
				editValue.uAtt = getCellAttribute(it->first.x,it->first.y);
				break;
			case CTerrainBrush::BRUSH_TYPE_TERRAIN_COLOR:
				editValue.color = getVertexColor(it->first.x,it->first.y).c;
				break;
			case CTerrainBrush::BRUSH_TYPE_TERRAIN_TILE_LAYER1:
				editValue.nTileID = GetCellTileID(it->first.x,it->first.y,0);
				break;
			case CTerrainBrush::BRUSH_TYPE_TERRAIN_TILE_LAYER2:
				editValue.nTileID = GetCellTileID(it->first.x,it->first.y,1);
				break;
			case CTerrainBrush::BRUSH_TYPE_MAX:
				break;
			default:
				break;
			}
		}

		switch(it->first.type)
		{
		case CTerrainBrush::BRUSH_TYPE_TERRAIN_HEIGHT:
			setVertexHeight(it->first.x,it->first.y,it->second.fHeight);
			break;
		case CTerrainBrush::BRUSH_TYPE_TERRAIN_ATT:
			setCellAttribute(it->first.x,it->first.y,it->second.uAtt);
			break;
		case CTerrainBrush::BRUSH_TYPE_TERRAIN_COLOR:
			setVertexColor(it->first.x,it->first.y,it->second.color);
			break;
		case CTerrainBrush::BRUSH_TYPE_TERRAIN_TILE_LAYER1:
			SetCellTileID(it->first.x,it->first.y,it->second.nTileID, 0);
			break;
		case CTerrainBrush::BRUSH_TYPE_TERRAIN_TILE_LAYER2:
			SetCellTileID(it->first.x,it->first.y,it->second.nTileID, 1);
			break;
		case CTerrainBrush::BRUSH_TYPE_MAX:
			break;
		default:
			break;
		}

		std::map<EditTarget,EditValue>::iterator itNext = it;
		itNext++;

		if(mapEditRecordIn.end()==itNext||it->first.type!=itNext->first.type||it->first.y!=itNext->first.y)
		{
			switch(it->first.type)
			{
			case CTerrainBrush::BRUSH_TYPE_TERRAIN_HEIGHT:
			case CTerrainBrush::BRUSH_TYPE_TERRAIN_COLOR:
				updateVB(nBeginX,nBeginY,it->first.x,it->first.y);
				break;
			default:
				break;
			}

			switch(it->first.type)
			{
			case CTerrainBrush::BRUSH_TYPE_TERRAIN_HEIGHT:
				bUpdateCube = true;
				bUpdateIB = true;
				break;
			case CTerrainBrush::BRUSH_TYPE_TERRAIN_ATT:
			case CTerrainBrush::BRUSH_TYPE_TERRAIN_TILE_LAYER1:
			case CTerrainBrush::BRUSH_TYPE_TERRAIN_TILE_LAYER2:
				bUpdateIB = true;
				break;
			default:
				break;
			}

			if(mapEditRecordIn.end()!=itNext)
			{
				nBeginX = itNext->first.x;
				nBeginY = itNext->first.y;
			}
		}
	}
	if(bUpdateCube)
	{
		UpdateCubeBBox(m_RootCube);
	}
	if(bUpdateIB)
	{
		updateIB();
	}
}

void CTerrainEditor::doEdit(MAP_EDIT_RECORD& mapEditRecordIn)
{
	if(mapEditRecordIn.empty())
	{
		return;
	}
	if(m_setReback.empty())
	{
		m_setReback.resize(m_setReback.size()+1);
	}
	doEdit(mapEditRecordIn,*m_setReback.rbegin());
	m_setRedo.clear();
}

void CTerrainEditor::doEdit(std::vector<MAP_EDIT_RECORD>& mapEditRecordIn,std::vector<MAP_EDIT_RECORD>& mapEditRecordOut)
{
	if(mapEditRecordIn.empty())
	{
		return;
	}
	mapEditRecordOut.resize(mapEditRecordOut.size()+1);
	doEdit(mapEditRecordIn.back(),mapEditRecordOut.back());
	mapEditRecordIn.pop_back();
}

void CTerrainEditor::rebackEdit()
{
	doEdit(m_setReback,m_setRedo);
}

void CTerrainEditor::redoEdit()
{
	doEdit(m_setRedo,m_setReback);
}

void CTerrainEditor::brushATT(float fPosX, float fPosY, byte uAtt, float fRadius)
{
	MAP_EDIT_RECORD mapEditRecord;
	if (isCellIn(fPosX, fPosY))
	{
		fPosX -=0.5f;
		fPosY -=0.5f;
		EditTarget editTarget;
		EditValue editValue;
		editTarget.type = CTerrainBrush::BRUSH_TYPE_TERRAIN_ATT;
		for (int y=fPosY-fRadius; y<fPosY+fRadius; y++)
		{
			for (int x=fPosX-fRadius; x<fPosX+fRadius; x++)
			{
				editTarget.x = x;
				editTarget.y = y;
				if (isCellIn(x,y))
				{
					Vec2D vLength(fPosX-x, fPosY-y);
					float fOffset = 1.0f-vLength.length()/fRadius;
					if (fOffset>0)
					{
						editValue.uAtt = uAtt;
						mapEditRecord[editTarget]=editValue;
					}
				}
			}
		}
	}
	doEdit(mapEditRecord);
}

void CTerrainEditor::brushTileLayer1(float fPosX, float fPosY, int nTileID, float fRadius)
{
	MAP_EDIT_RECORD mapEditRecord;
	if (isCellIn(fPosX, fPosY))
	{
		fPosX -=0.5f;
		fPosY -=0.5f;
		EditTarget editTarget;
		EditValue editValue;

		editTarget.type = CTerrainBrush::BRUSH_TYPE_TERRAIN_TILE_LAYER1;

		for (int y=fPosY-fRadius; y<fPosY+fRadius; y++)
		{
			for (int x=fPosX-fRadius; x<fPosX+fRadius; x++)
			{
				editTarget.x = x;
				editTarget.y = y;
				if (isCellIn(x,y))
				{
					Vec2D vLength(fPosX-x, fPosY-y);
					float fOffset = 1.0f-vLength.length()/fRadius;
					if (fOffset>0)
					{
						editValue.nTileID=nTileID;
						mapEditRecord[editTarget]=editValue;
					}
				}
			}
		}
	}
	doEdit(mapEditRecord);
}

void CTerrainEditor::brushColor(float fPosX, float fPosY, Color32 colorPaint, float fRadius, float fHardness, float fStrength)
{
	MAP_EDIT_RECORD mapEditRecord;
	if (isPointIn(fPosX, fPosY))
	{
		EditTarget editTarget;
		EditValue editValue;
		editTarget.type = CTerrainBrush::BRUSH_TYPE_TERRAIN_COLOR;
		for (int y=fPosY-fRadius; y<fPosY+fRadius; y++)
		{
			for (int x=fPosX-fRadius; x<fPosX+fRadius; x++)
			{
				editTarget.x = x;
				editTarget.y = y;
				if (isPointIn(x,y))
				{
					Vec2D vLength(fPosX-x, fPosY-y);
					float fOffset = 1.0f-vLength.length()/fRadius;
					fOffset = min(fOffset/(1.0f-fHardness),1.0f);
					if (fOffset>0)
					{
						float fRate=fOffset*fStrength;
						Color32 color = getVertexColor(x,y);
						colorPaint.a = color.a;
						editValue.color = Color32::slerp(fRate, color, colorPaint).c;
						mapEditRecord[editTarget]=editValue;
					}
				}
			}
		}
	}
	doEdit(mapEditRecord);
}

void CTerrainEditor::brushTileLayer2(float fPosX, float fPosY, int nTileID, float fRadius, float fHardness, float fStrength)
{
	MAP_EDIT_RECORD mapEditRecord;
	if (isPointIn(fPosX, fPosY))
	{
		EditTarget editTarget;
		EditValue editValue;
		for (int y=fPosY-fRadius; y<fPosY+fRadius; y++)
		{
			for (int x=fPosX-fRadius; x<fPosX+fRadius; x++)
			{
				editTarget.x = x;
				editTarget.y = y;
				if (isPointIn(x,y))
				{
					Vec2D vLength(fPosX-x, fPosY-y);
					float fOffset = 1.0f-vLength.length()/fRadius;
					fOffset = min(fOffset/(1.0f-fHardness),1.0f);
					if (fOffset>0)
					{
						editTarget.set(CTerrainBrush::BRUSH_TYPE_TERRAIN_TILE_LAYER2,x,y);
						editValue.nTileID=nTileID;
						mapEditRecord[editTarget]=editValue;

						editTarget.set(CTerrainBrush::BRUSH_TYPE_TERRAIN_TILE_LAYER2,x,y-1);
						editValue.nTileID=nTileID;
						mapEditRecord[editTarget]=editValue;

						editTarget.set(CTerrainBrush::BRUSH_TYPE_TERRAIN_TILE_LAYER2,x-1,y-1);
						editValue.nTileID=nTileID;
						mapEditRecord[editTarget]=editValue;

						editTarget.set(CTerrainBrush::BRUSH_TYPE_TERRAIN_TILE_LAYER2,x-1,y);
						editValue.nTileID=nTileID;
						mapEditRecord[editTarget]=editValue;

						editTarget.set(CTerrainBrush::BRUSH_TYPE_TERRAIN_COLOR,x,y);
						float fRate=fOffset*fStrength;
						Color32 color = getVertexColor(x,y);
						color.a = min(max(color.a+fRate*255,0),255);
						editValue.color = color.c;
						mapEditRecord[editTarget]=editValue;
					}
				}
			}
		}
	}
	doEdit(mapEditRecord);
	mapEditRecord.clear();
	if (isPointIn(fPosX, fPosY))
	{
		EditTarget editTarget;
		EditValue editValue;
		for (int y=fPosY-fRadius; y<fPosY+fRadius; y++)
		{
			for (int x=fPosX-fRadius; x<fPosX+fRadius; x++)
			{
				editTarget.x = x;
				editTarget.y = y;
				if (isPointIn(x,y))
				{
					Vec2D vLength(fPosX-x, fPosY-y);
					float fOffset = 1.0f-vLength.length()/fRadius;
					fOffset = min(fOffset/(1.0f-fHardness),1.0f);
					if (fOffset>0)
					{
						//  _______________________
						// |-2,+1|-1,+1| 0,+1|+1,+1|
						// |_____|_____|_____|_____|
						// |-2, 0|-1, 0| 0, 0|+1, 0|
						// |_____|_____|_____|_____|
						// |-2,-1|-1,-1| 0,-1|+1,-1|
						// |_____|_____|_____|_____|
						// |-2,-2|-1,-2| 0,-2|+1,-2|
						// |_____|_____|_____|_____|

						if (GetCellTileID(x-2,y,1)!=nTileID||
							GetCellTileID(x-2,y+1,1)!=nTileID||
							GetCellTileID(x-1,y+1,1)!=nTileID)
						{
							editTarget.set(CTerrainBrush::BRUSH_TYPE_TERRAIN_COLOR,x-1,y+1);
							Color32 color = getVertexColor(editTarget.x,editTarget.y);
							color.a = 0;
							editValue.color = color.c;
							mapEditRecord[editTarget]=editValue;
						}
						if (GetCellTileID(x-1,y+1,1)!=nTileID||
							GetCellTileID(x,y+1,1)!=nTileID)
						{
							editTarget.set(CTerrainBrush::BRUSH_TYPE_TERRAIN_COLOR,x,y+1);
							Color32 color = getVertexColor(editTarget.x,editTarget.y);
							color.a = 0;
							editValue.color = color.c;
							mapEditRecord[editTarget]=editValue;
						}
						if (GetCellTileID(x,y+1,1)!=nTileID||
							GetCellTileID(x+1,y+1,1)!=nTileID||
							GetCellTileID(x+1,y,1)!=nTileID)
						{
							editTarget.set(CTerrainBrush::BRUSH_TYPE_TERRAIN_COLOR,x+1,y+1);
							Color32 color = getVertexColor(editTarget.x,editTarget.y);
							color.a = 0;
							editValue.color = color.c;
							mapEditRecord[editTarget]=editValue;
						}
						if (GetCellTileID(x+1,y,1)!=nTileID||
							GetCellTileID(x+1,y-1,1)!=nTileID)
						{
							editTarget.set(CTerrainBrush::BRUSH_TYPE_TERRAIN_COLOR,x+1,y);
							Color32 color = getVertexColor(editTarget.x,editTarget.y);
							color.a = 0;
							editValue.color = color.c;
							mapEditRecord[editTarget]=editValue;
						}
						if (GetCellTileID(x+1,y-1,1)!=nTileID||
							GetCellTileID(x+1,y-2,1)!=nTileID||
							GetCellTileID(x,y-2,1)!=nTileID)
						{
							editTarget.set(CTerrainBrush::BRUSH_TYPE_TERRAIN_COLOR,x+1,y-1);
							Color32 color = getVertexColor(editTarget.x,editTarget.y);
							color.a = 0;
							editValue.color = color.c;
							mapEditRecord[editTarget]=editValue;
						}
						if (GetCellTileID(x,y-2,1)!=nTileID||
							GetCellTileID(x-1,y-2,1)!=nTileID)
						{
							editTarget.set(CTerrainBrush::BRUSH_TYPE_TERRAIN_COLOR,x,y-1);
							Color32 color = getVertexColor(editTarget.x,editTarget.y);
							color.a = 0;
							editValue.color = color.c;
							mapEditRecord[editTarget]=editValue;
						}
						if (GetCellTileID(x-1,y-2,1)!=nTileID||
							GetCellTileID(x-2,y-2,1)!=nTileID||
							GetCellTileID(x-2,y-1,1)!=nTileID)
						{
							editTarget.set(CTerrainBrush::BRUSH_TYPE_TERRAIN_COLOR,x-1,y-1);
							Color32 color = getVertexColor(editTarget.x,editTarget.y);
							color.a = 0;
							editValue.color = color.c;
							mapEditRecord[editTarget]=editValue;
						}
						if (GetCellTileID(x-2,y-1,1)!=nTileID||
							GetCellTileID(x-2,y,1)!=nTileID)
						{
							editTarget.set(CTerrainBrush::BRUSH_TYPE_TERRAIN_COLOR,x-1,y);
							Color32 color = getVertexColor(editTarget.x,editTarget.y);
							color.a = 0;
							editValue.color = color.c;
							mapEditRecord[editTarget]=editValue;
						}
					}
				}
			}
		}
	}
	doEdit(mapEditRecord);
	mapEditRecord.clear();
	if (isPointIn(fPosX, fPosY))
	{
		EditTarget editTarget;
		EditValue editValue;
		for (int y=fPosY-fRadius; y<fPosY+fRadius; y++)
		{
			for (int x=fPosX-fRadius; x<fPosX+fRadius; x++)
			{
				editTarget.x = x;
				editTarget.y = y;
				if (isPointIn(x,y))
				{
					Vec2D vLength(fPosX-x, fPosY-y);
					float fOffset = 1.0f-vLength.length()/fRadius;
					fOffset = min(fOffset/(1.0f-fHardness),1.0f);
					if (fOffset>0)
					{
						if(getVertexColor(x,y).a==0xFF&&
							getVertexColor(x,y+1).a==0xFF&&
							getVertexColor(x+1,y+1).a==0xFF&&
							getVertexColor(x+1,y).a==0xFF)
						{
							editTarget.set(CTerrainBrush::BRUSH_TYPE_TERRAIN_TILE_LAYER1,x,y);
							editValue.nTileID=nTileID;
							mapEditRecord[editTarget]=editValue;
						}

						if(getVertexColor(x,y).a==0xFF&&
							getVertexColor(x+1,y).a==0xFF&&
							getVertexColor(x+1,y-1).a==0xFF&&
							getVertexColor(x,y-1).a==0xFF)
						{
							editTarget.set(CTerrainBrush::BRUSH_TYPE_TERRAIN_TILE_LAYER1,x,y-1);
							editValue.nTileID=nTileID;
							mapEditRecord[editTarget]=editValue;
						}

						if(getVertexColor(x,y).a==0xFF&&
							getVertexColor(x-1,y).a==0xFF&&
							getVertexColor(x-1,y-1).a==0xFF&&
							getVertexColor(x,y-1).a==0xFF)
						{
							editTarget.set(CTerrainBrush::BRUSH_TYPE_TERRAIN_TILE_LAYER1,x-1,y-1);
							editValue.nTileID=nTileID;
							mapEditRecord[editTarget]=editValue;
						}

						if(getVertexColor(x,y).a==0xFF&&
							getVertexColor(x-1,y).a==0xFF&&
							getVertexColor(x-1,y+1).a==0xFF&&
							getVertexColor(x,y+1).a==0xFF)
						{
							editTarget.set(CTerrainBrush::BRUSH_TYPE_TERRAIN_TILE_LAYER1,x-1,y);
							editValue.nTileID=nTileID;
							mapEditRecord[editTarget]=editValue;
						}
					}
				}
			}
		}
	}
	doEdit(mapEditRecord);
	mapEditRecord.clear();
	if (isPointIn(fPosX, fPosY))
	{
		EditTarget editTarget;
		EditValue editValue;
		for (int y=fPosY-fRadius; y<fPosY+fRadius; y++)
		{
			for (int x=fPosX-fRadius; x<fPosX+fRadius; x++)
			{
				editTarget.x = x;
				editTarget.y = y;
				if (isPointIn(x,y))
				{
					Vec2D vLength(fPosX-x, fPosY-y);
					float fOffset = 1.0f-vLength.length()/fRadius;
					fOffset = min(fOffset/(1.0f-fHardness),1.0f);
					if (fOffset>0)
					{
						editTarget.set(CTerrainBrush::BRUSH_TYPE_TERRAIN_COLOR,x,y);
						float fRate=fOffset*fStrength;
						Color32 color = getVertexColor(x,y);

						if(GetCellTileID(x,y,0)==nTileID&&
							GetCellTileID(x-1,y,0)==nTileID&&
							GetCellTileID(x,y-1,0)==nTileID&&
							GetCellTileID(x-1,y-1,0)==nTileID)
						{
							color.a = 0;
							editValue.color = color.c;
							mapEditRecord[editTarget]=editValue;
						}
					}
				}
			}
		}
	}
	doEdit(mapEditRecord);
}

void CTerrainEditor::brushHeight(float fPosX, float fPosY, float fRadius, float fHardness, float fStrength, float fMin, float fMax, bool bSmooth)
{
	MAP_EDIT_RECORD mapEditRecord;
	if (isPointIn(fPosX, fPosY))
	{
		EditTarget editTarget;
		EditValue editValue;
		editTarget.type = CTerrainBrush::BRUSH_TYPE_TERRAIN_HEIGHT;
		for (int y=fPosY-fRadius; y<fPosY+fRadius; y++)
		{
			for (int x=fPosX-fRadius; x<fPosX+fRadius; x++)
			{
				editTarget.x = x;
				editTarget.y = y;
				if (isPointIn(x,y))
				{
					Vec2D vLength(fPosX-x, fPosY-y);
					float fOffset = 1.0f-vLength.length()/fRadius;
					fOffset = min(fOffset/(1.0f-fHardness),1.0f);
					if (fOffset>0)
					{
						float fHeight = getVertexHeight(x,y);
						if (bSmooth)
						{
							float fRate = fOffset/fRadius;
							fHeight = fHeight*fRate+
								(getVertexHeight(x+1,y)+
								getVertexHeight(x-1,y)+
								getVertexHeight(x,y+1)+
								getVertexHeight(x,y-1))
								*0.25f*(1.0f-fRate);
						}
						else
						{
							fHeight +=fOffset*m_BrushDecal.GetStrength();
						}
						editValue.fHeight = min(max(fHeight,fMin),fMax);
						mapEditRecord[editTarget]=editValue;
					}
				}
			}
		}
	}
	doEdit(mapEditRecord);
}

void CTerrainEditor::Brush(float fPosX, float fPosY)
{
	float fRadius = m_BrushDecal.GetRadius()*m_BrushDecal.GetSize();
	switch(m_BrushDecal.GetBrushType())
	{
	case CTerrainBrush::BRUSH_TYPE_TERRAIN_HEIGHT:
		brushHeight(fPosX, fPosY, fRadius, m_BrushDecal.GetHardness(), m_BrushDecal.GetStrength(),
			m_BrushDecal.getHeightMin(), m_BrushDecal.getHeightMax(), m_BrushDecal.getBrushHeightType()==CTerrainBrush::BHT_SMOOTH);
		break;
	case CTerrainBrush::BRUSH_TYPE_TERRAIN_ATT:
		brushATT(fPosX, fPosY, m_BrushDecal.GetAtt(), fRadius);
		break;
	case CTerrainBrush::BRUSH_TYPE_TERRAIN_COLOR:
		brushColor(fPosX, fPosY, m_BrushDecal.getColor(), fRadius, m_BrushDecal.GetHardness(), m_BrushDecal.GetStrength());
		break;
	case CTerrainBrush::BRUSH_TYPE_TERRAIN_TILE_LAYER1:
		if(m_BrushDecal.GetStrength()>0)
		{
			brushTileLayer1(fPosX, fPosY, m_BrushDecal.GetTileID(0,0), fRadius);
		}
		else
		{
			brushTileLayer1(fPosX, fPosY, 0xFF, fRadius);
		}
		break;
	case CTerrainBrush::BRUSH_TYPE_TERRAIN_TILE_LAYER2:
		
		if(m_BrushDecal.GetStrength()>0)
		{
			brushTileLayer2(fPosX, fPosY, m_BrushDecal.GetTileID(0,0), fRadius, m_BrushDecal.GetHardness(), m_BrushDecal.GetStrength());
		}
		else
		{
			brushTileLayer2(fPosX, fPosY, 0xFF, fRadius, m_BrushDecal.GetHardness(), m_BrushDecal.GetStrength());
		}
		break;
	default:
		break;
	}
}


void CTerrainEditor::CreateIB()
{
	S_DEL(m_pIB);
	m_pIB = GetRenderSystem().GetHardwareBufferMgr().CreateIndexBuffer(GetCellCount()*3*6*sizeof(unsigned short),CHardwareIndexBuffer::IT_16BIT,CHardwareBuffer::HBU_WRITE_ONLY);
}

void CTerrainEditor::updateIB()
{
	CTerrain::updateIB();
	m_mapRenderAttributeSubs.clear();
	if (m_pIB)
	{
		for (LIST_CUBES::iterator it=m_RenderCubesList.begin(); it!=m_RenderCubesList.end(); it++)
		{
			const BBox& bbox = (*it)->bbox;
			for (size_t y = bbox.vMin.z; y<bbox.vMax.z; ++y)
			{
				for (size_t x = bbox.vMin.x; x<bbox.vMax.x; ++x)
				{
					unsigned long uIndex = getVertexIndex(x,y);

					const unsigned char uAttribute = getCellAttribute(x,y);
					if (uAttribute!=0x0)
					{
						m_mapRenderAttributeSubs[uAttribute].myVertexIndex(uIndex);
					}
				}
			}
		}
		unsigned long m_uShowAttributeIBCount = 0;

		for (std::map<unsigned char,TerrainSub>::iterator it = m_mapRenderAttributeSubs.begin(); it!=m_mapRenderAttributeSubs.end(); it++)
		{
			it->second.istart = m_uShowTileIBCount+m_uShowAttributeIBCount;
			m_uShowAttributeIBCount += it->second.icount;
			it->second.icount = 0;
			it->second.vcount=it->second.vcount-it->second.istart+1+GetVertexXCount()+1;
		}

		unsigned long uTempVertexXCount = GetVertexXCount();
		unsigned short* index = (unsigned short*)m_pIB->lock(m_uShowTileIBCount*sizeof(unsigned short), m_uShowAttributeIBCount*sizeof(unsigned short),CHardwareBuffer::HBL_NO_OVERWRITE);
		for (LIST_CUBES::iterator it=m_RenderCubesList.begin(); it!=m_RenderCubesList.end(); it++)
		{
			const BBox& bbox = (*it)->bbox;
			for (size_t y = bbox.vMin.z; y<bbox.vMax.z; ++y)
			{
				for (size_t x = bbox.vMin.x; x<bbox.vMax.x; ++x)
				{
					unsigned long uIndex = getVertexIndex(x,y);

					const unsigned char uAttribute = getCellAttribute(x,y);
					if (uAttribute!=0x0)
					{
						TerrainSub& sub = m_mapRenderAttributeSubs[uAttribute];
						{
							// 2	 3
							//	*---*
							//	| / |
							//	*---*
							// 0	 1
							unsigned short* p = index+sub.istart+sub.icount-m_uShowTileIBCount;
							sub.icount+=6;
							*p = uIndex;
							p++;
							*p = uIndex+uTempVertexXCount;
							p++;
							*p = uIndex+uTempVertexXCount+1;
							p++;

							*p = uIndex;
							p++;
							*p = uIndex+uTempVertexXCount+1;
							p++;
							*p = uIndex+1;
							p++;
						}
					}
				}
			}
		}
		m_pIB->unlock();
	}
}