// Standard Window Animation DLL for NPS WPS Enhancer
// Copyright (C) 1995, N.P.S.
// 100 or more columns are necessary to read this file properly

#define INCL_PM
#include <os2.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include "npswpswa.h"

// to export

extern "C" BOOL EXPENTRY SpinFrame(AnimationData *padat);
extern "C" BOOL EXPENTRY VortexFrames(AnimationData *padat);
extern "C" BOOL EXPENTRY ScatterGatherFrames(AnimationData *padat);
extern "C" BOOL EXPENTRY SpikeFrames(AnimationData *padat);
extern "C" BOOL EXPENTRY FireworksFrames(AnimationData *padat);

const FIXED Fixed1 = 1 << 16;
static POINTL PointZero = { 0, 0 };
static MATRIXLF Matrix1 = { Fixed1, 0, 0, 0, Fixed1, 0, 0, 0, 1 }; // unit matrix

struct DisjointLine
{  POINTL ptStart, ptEnd;
};

LONG fixedMul(LONG lMultiplicand, FIXED fMultiplier);
FIXED fixedDiv(LONG lNumerator, LONG lDenominator);
POINTL operator*(const POINTL &point, const MATRIXLF &matrix);
MATRIXLF operator*(const MATRIXLF &matrix1, const MATRIXLF &matrix2);
MATRIXLF mix(const MATRIXLF &matrix1, const MATRIXLF &matrix2, FIXED fMix);
POINTL mix(const POINTL &point1, const POINTL &point2, FIXED fMix);

void drawDisjoint(HPS hps, DisjointLine *paDisjointLine, LONG cLines);
LONG setDisjointLine(DisjointLine *pDisjointLine, const POINTL& ptStart, const POINTL &ptEnd);
LONG setDisjointBox(DisjointLine *pDisjointLine,
		    const POINTL& ptCenter, const POINTL &ptRelRightTop);

inline POINTL operator+(POINTL &point1, POINTL &point2)
{  POINTL pointResult;
   pointResult.x = point1.x + point2.x;
   pointResult.y = point1.y + point2.y;
   return pointResult;
}

inline POINTL operator-(POINTL &point1, POINTL &point2)
{  POINTL pointResult;
   pointResult.x = point1.x - point2.x;
   pointResult.y = point1.y - point2.y;
   return pointResult;
}

inline MATRIXLF& operator*=(MATRIXLF &matrix1, const MATRIXLF &matrix2)
{  return matrix1 = matrix1 * matrix2;
}

BOOL EXPENTRY SpinFrame(AnimationData *padat)
{  if (padat->animCallType == AnimationDraw
       || padat->animCallType == AnimationErase)
   {  POINTL aPtRectangle[4];
      aPtRectangle[1].x = aPtRectangle[0].x = padat->ptRelRightTop.x;
      aPtRectangle[1].y = -(aPtRectangle[0].y = padat->ptRelRightTop.y);

      MATRIXLF matrix;
      FIXED afxScale[2];
      afxScale[0] = afxScale[1] = fixedDiv(padat->lStep, padat->cTotalSteps);
      GpiScale(padat->hps, &matrix, TRANSFORM_REPLACE, afxScale, &PointZero);
      GpiRotate(padat->hps, &matrix, TRANSFORM_ADD,
		fixedDiv(padat->lParameter * (padat->lStep - padat->cTotalSteps),
			 padat->cTotalSteps),
		&PointZero);
      aPtRectangle[0] = aPtRectangle[0] * matrix;
      aPtRectangle[1] = aPtRectangle[1] * matrix;

      if ((aPtRectangle[0].x != 0 || aPtRectangle[0].y != 0)
	  && (aPtRectangle[1].x != 0 || aPtRectangle[1].y != 0))
      {  aPtRectangle[2] = padat->ptCenter + aPtRectangle[0];
	 aPtRectangle[3] = padat->ptCenter + aPtRectangle[1];
	 aPtRectangle[0] = padat->ptCenter - aPtRectangle[0];
	 aPtRectangle[1] = padat->ptCenter - aPtRectangle[1];
	 GpiMove(padat->hps, &aPtRectangle[3]);
	 GpiPolyLine(padat->hps, 4, aPtRectangle);
      }
   }

   return TRUE;
}

BOOL EXPENTRY VortexFrames(AnimationData *padat)
{  switch (padat->animCallType)
   {  case AnimationInitialize:
	 GpiRotate(padat->hps, (MATRIXLF *)padat->achBuffer, TRANSFORM_REPLACE,
		   72 * Fixed1, &padat->ptCenter);
	 break;
      case AnimationDraw:
      case AnimationErase:
      {  POINTL ptBoxRel;
	 ptBoxRel.x = padat->ptRelRightTop.x * padat->lStep / padat->cTotalSteps;
	 ptBoxRel.y = padat->ptRelRightTop.y * padat->lStep / padat->cTotalSteps;

	 MATRIXLF matrix;
	 FIXED afxScale[2];
	 afxScale[0] = afxScale[1] = fixedDiv((padat->cTotalSteps - padat->lStep) * 5,
					      padat->cTotalSteps * 3);
	 GpiTranslate(padat->hps, &matrix, TRANSFORM_REPLACE, &padat->ptRelRightTop);
	 GpiScale(padat->hps, &matrix, TRANSFORM_ADD, afxScale, &padat->ptCenter);
	 GpiRotate(padat->hps, &matrix, TRANSFORM_ADD,
		   fixedDiv(padat->lParameter * padat->lStep, padat->cTotalSteps),
		   &padat->ptCenter);

	 DisjointLine adjline[5 * 4];
	 LONG cLines = 0;
	 POINTL ptBoxCenter = padat->ptCenter * matrix;

	 for (int iLoop = 0; iLoop < 5; iLoop++)
	 {  cLines += setDisjointBox(&adjline[cLines], ptBoxCenter, ptBoxRel);
	    ptBoxCenter = ptBoxCenter * *(MATRIXLF *)padat->achBuffer;
	 }

	 drawDisjoint(padat->hps, adjline, cLines);
	 break;
      }
   }

   return TRUE;
}

BOOL EXPENTRY ScatterGatherFrames(AnimationData *padat)
{  if (padat->animCallType == AnimationDraw
       || padat->animCallType == AnimationErase)
   {  LONG lDivisor = padat->lParameter;
      POINTL ptBoxRel;
      
      ptBoxRel.x = padat->ptRelRightTop.x * padat->lStep / (padat->cTotalSteps * lDivisor);
      ptBoxRel.y = padat->ptRelRightTop.y * padat->lStep / (padat->cTotalSteps * lDivisor);

      MATRIXLF matrix;
      FIXED afxScale[2];
      afxScale[0] = afxScale[1] = fixedDiv(padat->cTotalSteps * 3 - padat->lStep * 2,
					   padat->cTotalSteps);
      GpiScale(padat->hps, &matrix, TRANSFORM_REPLACE, afxScale, &PointZero);
      GpiTranslate(padat->hps, &matrix, TRANSFORM_ADD, &padat->ptCenter);

      DisjointLine adjline[10 * 10 * 4];  // The maximum number of rects is 10 * 10
      LONG cLines = 0;

      for (int iRow = 0; iRow < lDivisor; iRow++)
      {  for (int iColumn = 0; iColumn < padat->lParameter; iColumn++)
	 {  POINTL ptTileCenter;
	    ptTileCenter.x = (iRow * 2 - lDivisor + 1) * padat->ptRelRightTop.x / lDivisor;
	    ptTileCenter.y = (iColumn * 2 - lDivisor + 1) * padat->ptRelRightTop.y / lDivisor;
	    ptTileCenter = ptTileCenter * matrix;
	    cLines += setDisjointBox(&adjline[cLines], ptTileCenter, ptBoxRel);
	 }
      }

      drawDisjoint(padat->hps, adjline, cLines);
   }

   return TRUE;
}

BOOL EXPENTRY SpikeFrames(AnimationData *padat)
{  struct SpikeData
   {  POINTL aPtTriangleEnd[16][3], aPtEndCenter[16], aPtTriangleCenter[16];
      MATRIXLF aMatrixCircle[16];
   };

   switch (padat->animCallType)
   {  case AnimationInitialize:
      {  LONG xLeft = padat->rectWindow.xLeft,
	      xRight = padat->rectWindow.xRight,
	      yBottom = padat->rectWindow.yBottom,
	      yTop = padat->rectWindow.yTop;
	    
	 POINTL *paPtTriangle = ((SpikeData *)padat->achBuffer)->aPtTriangleEnd[0],
	        *paPtCenter = ((SpikeData *)padat->achBuffer)->aPtEndCenter;
	 for (int iIndex = 0; iIndex < 16; iIndex++, paPtTriangle += 3, paPtCenter++)
	 {  paPtTriangle[0] = padat->ptCenter;

	    if (iIndex < 4)
	    {  paPtTriangle[1].y = paPtTriangle[2].y = yTop;
	       paPtTriangle[1].x = (xLeft * (4 - iIndex) + xRight * iIndex) / 4;
	       paPtTriangle[2].x = (xLeft * (3 - iIndex) + xRight * (iIndex + 1)) / 4;
	    }
	    else if (iIndex < 8)
	    {  paPtTriangle[1].x = paPtTriangle[2].x = xRight;
	       paPtTriangle[1].y = (yTop * (8 - iIndex) + yBottom * (iIndex - 4)) / 4;
	       paPtTriangle[2].y = (yTop * (7 - iIndex) + yBottom * (iIndex - 3)) / 4;
	    }
	    else if (iIndex < 12)
	    {  paPtTriangle[1].y = paPtTriangle[2].y = yBottom;
	       paPtTriangle[1].x = (xRight * (12 - iIndex) + xLeft * (iIndex - 8)) / 4;
	       paPtTriangle[2].x = (xRight * (11 - iIndex) + xLeft * (iIndex - 7)) / 4;
	    }
	    else
	    {  paPtTriangle[1].x = paPtTriangle[2].x = xLeft;
	       paPtTriangle[1].y = (yBottom * (16 - iIndex) + yTop * (iIndex - 12)) / 4;
	       paPtTriangle[2].y = (yBottom * (15 - iIndex) + yTop * (iIndex - 11)) / 4;
	    }

	    paPtCenter->x = (paPtTriangle[0].x + paPtTriangle[1].x + paPtTriangle[2].x) / 3;
	    paPtCenter->y = (paPtTriangle[0].y + paPtTriangle[1].y + paPtTriangle[2].y) / 3;
	 }

	 paPtCenter = ((SpikeData *)padat->achBuffer)->aPtTriangleCenter;
	 POINTL ptTriangleCenter;
	 ptTriangleCenter.x = padat->ptCenter.x;
	 ptTriangleCenter.y =
	    padat->ptCenter.y + (padat->ptRelRightTop.x + padat->ptRelRightTop.y) * 4 / 5;
	 
	 for (iIndex = 0; iIndex < 16; iIndex++)
	 {  MATRIXLF matrix;
	    GpiRotate(padat->hps, &matrix, TRANSFORM_REPLACE,
		      (33 * Fixed1) + (-22 * Fixed1) * iIndex, &padat->ptCenter);
	    paPtCenter[iIndex] = ptTriangleCenter * matrix;

	    POINTL ptTemp =
	       paPtCenter[iIndex] - ((SpikeData *)padat->achBuffer)->aPtEndCenter[iIndex];
	    GpiTranslate(padat->hps, &((SpikeData *)padat->achBuffer)->aMatrixCircle[iIndex],
			 TRANSFORM_REPLACE, &ptTemp);
	 }

	 break;
      }
      case AnimationDraw:
      case AnimationErase:
      {  POINTL aPtTriangle[3];
	 DisjointLine adjline[16 * 3];
	 int cLines = 0;
	 FIXED fixedFactor;

	 MATRIXLF matrix;
	 FIXED afxScale[2];
	 afxScale[0] = afxScale[1] = fixedDiv(padat->lStep, padat->cTotalSteps);

	 if (padat->lStep < padat->cTotalSteps / 2)
	 {  fixedFactor = (Fixed1 - fixedDiv(padat->lStep * 2, padat->cTotalSteps))
	       * padat->lParameter;

	    for (int iIndex = 0; iIndex < 16; iIndex++)
	    {  GpiScale(padat->hps, &matrix, TRANSFORM_REPLACE, afxScale,
			&((SpikeData *)padat->achBuffer)->aPtEndCenter[iIndex]);
	       GpiRotate(padat->hps, &matrix, TRANSFORM_ADD, fixedFactor,
			 &((SpikeData *)padat->achBuffer)->aPtEndCenter[iIndex]);
	       matrix *= ((SpikeData *)padat->achBuffer)->aMatrixCircle[iIndex];

	       for (int iAngle = 0; iAngle < 3; iAngle++)
	       {  aPtTriangle[iAngle] = 
		     ((SpikeData *)padat->achBuffer)->aPtTriangleEnd[iIndex][iAngle] * matrix;
	       }

	       for (iAngle = 0; iAngle < 3; iAngle++)
	       {  cLines += setDisjointLine(&adjline[cLines], aPtTriangle[iAngle],
					    aPtTriangle[(iAngle + 1) % 3]);
	       }
	    }
	 }
	 else
	 {  fixedFactor = fixedDiv(padat->lStep * 2 - padat->cTotalSteps, padat->cTotalSteps);
	    
	    for (int iIndex = 0; iIndex < 16; iIndex++)
	    {  GpiScale(padat->hps, &matrix, TRANSFORM_REPLACE, afxScale,
			&((SpikeData *)padat->achBuffer)->aPtEndCenter[iIndex]);
	       matrix *= mix(Matrix1, ((SpikeData *)padat->achBuffer)->aMatrixCircle[iIndex],
			     fixedFactor);

	       for (int iAngle = 0; iAngle < 3; iAngle++)
	       {  aPtTriangle[iAngle] = 
		     ((SpikeData *)padat->achBuffer)->aPtTriangleEnd[iIndex][iAngle] * matrix;
	       }

	       for (iAngle = 0; iAngle < 3; iAngle++)
	       {  cLines += setDisjointLine(&adjline[cLines], aPtTriangle[iAngle],
					    aPtTriangle[(iAngle + 1) % 3]);
	       }
	    }
	 }

	 drawDisjoint(padat->hps, adjline, cLines);
	 break;
      }
   }

   return TRUE;
}

BOOL EXPENTRY FireworksFrames(AnimationData *padat)
{  const int NUMBER_OF_RECTS = 10;

   struct FireworksData
   {  POINTL aPtCenter[NUMBER_OF_RECTS];
   };

   switch (padat->animCallType)
   {  case AnimationInitialize:
      {  POINTL *paPtCenter = ((FireworksData *)padat->achBuffer)->aPtCenter;
	 POINTL ptRectCenter;
	 ptRectCenter.x = padat->ptCenter.x;
	 ptRectCenter.y =
	    padat->ptCenter.y + (padat->ptRelRightTop.x + padat->ptRelRightTop.y) * 5 / 3;
	 
	 for (int iIndex = 0; iIndex < NUMBER_OF_RECTS; iIndex++)
	 {  MATRIXLF matrix;
	    GpiRotate(padat->hps, &matrix, TRANSFORM_REPLACE,
		      iIndex * (360 * Fixed1 / NUMBER_OF_RECTS), &padat->ptCenter);
	    paPtCenter[iIndex] = ptRectCenter * matrix;
	 }

	 break;
      }
      case AnimationDraw:
      case AnimationErase:
      {  POINTL ptTemp;
	 DisjointLine adjline[NUMBER_OF_RECTS * 4];
	 int cLines = 0;
	 FIXED fixedFactor = fixedDiv(padat->lStep, padat->cTotalSteps);

	 MATRIXLF matrix;
	 FIXED afxScale[2];
	 afxScale[0] = afxScale[1] = fixedFactor;

	 POINTL aPtRect[4], aPtTemp[4];
	 aPtRect[0].x = aPtRect[3].x = -padat->ptRelRightTop.x;
	 aPtRect[1].x = aPtRect[2].x = padat->ptRelRightTop.x;
	 aPtRect[0].y = aPtRect[1].y = padat->ptRelRightTop.y;
	 aPtRect[2].y = aPtRect[3].y = -padat->ptRelRightTop.y;

	 for (int iIndex = 0; iIndex < NUMBER_OF_RECTS; iIndex++)
	 {  GpiScale(padat->hps, &matrix, TRANSFORM_REPLACE, afxScale, &PointZero);
	    GpiRotate(padat->hps, &matrix, TRANSFORM_ADD,
		      (Fixed1 - fixedFactor) * padat->lParameter, &PointZero);
	    POINTL ptTemp;
	    ptTemp = mix(padat->ptCenter, ((FireworksData *)padat->achBuffer)->aPtCenter[iIndex],
			 fixedFactor);
	    GpiTranslate(padat->hps, &matrix, TRANSFORM_ADD, &ptTemp);
      
	    for (int iAngle = 0; iAngle < 4; iAngle++)
	    {  aPtTemp[iAngle] = aPtRect[iAngle] * matrix;
	    }
      
	    for (iAngle = 0; iAngle < 4; iAngle++)
	    {  cLines += setDisjointLine(&adjline[cLines], aPtTemp[iAngle],
					 aPtTemp[(iAngle + 1) % 4]);
	    }
	 }

	 drawDisjoint(padat->hps, adjline, cLines);
	 break;
      }
   }

   return TRUE;
}

// drawing functions

void drawDisjoint(HPS hps, DisjointLine *paDisjointLine, LONG cLines)
{  // draw disjoint lines
   // GpiPolyLineDisjoint is much faster than a few GpiLine's

   if (cLines > 0) GpiPolyLineDisjoint(hps, cLines * 2, (PPOINTL)paDisjointLine);
}

LONG setDisjointLine(DisjointLine *pDisjointLine, const POINTL& ptStart, const POINTL &ptEnd)
{  // set the line data between ptStart and ptEnd
   // return 1 if the line is valid, 0 if not

   if (ptStart.x == ptEnd.x && ptStart.y == ptEnd.y) return 0;
   else
   {  pDisjointLine->ptStart = ptStart;
      pDisjointLine->ptEnd = ptEnd;
      return 1;  // 1 struct was set
   }
}

LONG setDisjointBox(DisjointLine *pDisjointLine,
		    const POINTL& ptCenter, const POINTL &ptRelRightTop)
{  // set the line data of the rectangle
   // return the number of valid lines, 0 means there's no valid line

   POINTL aPtRectangle[4];

   for (int iIndex = 0; iIndex < 4; iIndex++)
   {  aPtRectangle[iIndex].x =
	 ptCenter.x + (iIndex == 0 || iIndex == 3 ? -ptRelRightTop.x : ptRelRightTop.x);
      aPtRectangle[iIndex].y = ptCenter.y + (iIndex <= 1 ? -ptRelRightTop.y : ptRelRightTop.y);
   }

   LONG cLines = 0;
   for (iIndex = 0; iIndex < 4; iIndex++)
   {  if (setDisjointLine(pDisjointLine, aPtRectangle[iIndex], aPtRectangle[(iIndex + 1) % 4]))
      {  pDisjointLine++;
	 cLines++;
      }
   }

   return cLines;
}

// helper functions for FIXED etc.

LONG fixedMul(LONG lMultiplicand, FIXED fxMultiplier)
{  // returns lMultiplicand * fxMultiplier as a LONG
   // also returns a FIXED if lMultiplicand is a FIXED

   return lMultiplicand * (fxMultiplier >> 16) 
      + (lMultiplicand >> 16) * (fxMultiplier & 0xffff)
	 + ((ULONG)((lMultiplicand & 0xffff) * (fxMultiplier & 0xffff)) >> 16);
}

FIXED fixedDiv(LONG lNumerator, LONG lDenominator)
{  // returns lNumerator / lDenominator as a FIXED
   // also works well for 2 FIXED's

   if (lNumerator == 0 || lDenominator == 0) return 0;

   LONG lSignBit = (lNumerator ^ lDenominator) >> (sizeof(LONG) * 8 - 1);
   lNumerator = abs(lNumerator);
   lDenominator = abs(lDenominator);

   int cShiftRemain = sizeof(SHORT) * 8;
   while ((lDenominator & 1) == 0 && cShiftRemain > 0)
   {  lDenominator >>= 1;
      cShiftRemain--;
   }
   
   ULONG ulQuotient = 0, ulTemp;

   do
   {  while ((lNumerator & 0x80000000) == 0 && cShiftRemain > 0)
      {  lNumerator <<= 1;
	 cShiftRemain--;
      }

      ulTemp = (ULONG)lNumerator / (ULONG)lDenominator;
      lNumerator -= ulTemp * lDenominator;
      ulQuotient += ulTemp << cShiftRemain;

   } while (lNumerator != 0 && cShiftRemain > 0);

   return (ulQuotient ^ lSignBit) - lSignBit;  /* reverse sign if lSignBit = -1 */
}

POINTL operator*(const POINTL &point, const MATRIXLF &matrix)
{  // returns POINTL transformed by MATRIXLF

   POINTL pointResult;
   pointResult.x = fixedMul(point.x, matrix.fxM11) + fixedMul(point.y, matrix.fxM21) + matrix.lM31;
   pointResult.y = fixedMul(point.x, matrix.fxM12) + fixedMul(point.y, matrix.fxM22) + matrix.lM32;
   return pointResult;
}   

POINTL mix(const POINTL &pointSource1, const POINTL &pointSource2, FIXED fMix)
{  // mix 2 POINTL's according to FIXED

   POINTL pointResult;
   pointResult.x = fixedMul(pointSource1.x, fMix) + fixedMul(pointSource2.x, Fixed1 - fMix);
   pointResult.y = fixedMul(pointSource1.y, fMix) + fixedMul(pointSource2.y, Fixed1 - fMix);
   return pointResult;
}

MATRIXLF operator*(const MATRIXLF &mtx1, const MATRIXLF &mtx2)
{  // returns product of 2 MATRIXLF's

   MATRIXLF mtxProduct;
   mtxProduct.fxM11 = fixedMul(mtx1.fxM11, mtx2.fxM11) + fixedMul(mtx1.fxM12, mtx2.fxM21);
   mtxProduct.fxM12 = fixedMul(mtx1.fxM11, mtx2.fxM12) + fixedMul(mtx1.fxM12, mtx2.fxM22);
   mtxProduct.lM13 = 0;
   mtxProduct.fxM21 = fixedMul(mtx1.fxM21, mtx2.fxM11) + fixedMul(mtx1.fxM22, mtx2.fxM21);
   mtxProduct.fxM22 = fixedMul(mtx1.fxM21, mtx2.fxM12) + fixedMul(mtx1.fxM22, mtx2.fxM22);
   mtxProduct.lM23 = 0;
   mtxProduct.lM31 = fixedMul(mtx1.lM31, mtx2.fxM11) + fixedMul(mtx1.lM32, mtx2.fxM21) + mtx2.lM31;
   mtxProduct.lM32 = fixedMul(mtx1.lM31, mtx2.fxM12) + fixedMul(mtx1.lM32, mtx2.fxM22) + mtx2.lM32;
   mtxProduct.lM33 = 1;
   return mtxProduct;
}

MATRIXLF mix(const MATRIXLF &matrix1, const MATRIXLF &matrix2, FIXED fMix)
{  // mix 2 MATRIXLF's according to FIXED

   MATRIXLF matrixResult;
   for (int iIndex = 0; iIndex < 9; iIndex++)
   {  ((PLONG)&matrixResult)[iIndex] =
	 fixedMul(((PLONG)&matrix1)[iIndex], fMix)
	    + fixedMul(((PLONG)&matrix2)[iIndex], Fixed1 - fMix);
   }

   return matrixResult;
}
