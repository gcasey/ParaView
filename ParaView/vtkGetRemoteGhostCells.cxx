/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGetRemoteGhostCells.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkGetRemoteGhostCells.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include "vtkPolyDataWriter.h"

vtkGetRemoteGhostCells* vtkGetRemoteGhostCells::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkGetRemoteGhostCells");
  if(ret)
    {
    return (vtkGetRemoteGhostCells*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkGetRemoteGhostCells;
}

vtkGetRemoteGhostCells::vtkGetRemoteGhostCells()
{
  this->Controller = NULL;
  this->Locator = vtkPointLocator::New();
}

vtkGetRemoteGhostCells::~vtkGetRemoteGhostCells()
{
  this->SetController(NULL);
  this->Locator->Delete();
  this->Locator = NULL;
}

void vtkGetRemoteGhostCells::Execute()
{
  int validFlag, myId, numProcs, numPoints, pointId, *pointIds, newPointId;
  float point[3];
  int numCells, numCellPoints;
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  int i = 0, j, k, l, id, gl;
  vtkPoints *points = vtkPoints::New();
  vtkPoints *cellPoints;
  vtkCellArray *polys;
  vtkIdList *cellIds = vtkIdList::New();
  vtkGenericCell *cell = vtkGenericCell::New();
  vtkGhostLevels *ghostLevels = vtkGhostLevels::New();
  float *myPoints, *remotePoints;
  int numRemotePoints;
  vtkIdList *insertedCells = vtkIdList::New();
  vtkIdList *newCells = vtkIdList::New();
  vtkIdList *currentPoints = vtkIdList::New();
  vtkIdList **remoteCells;
  int cellId, remoteCellId, newCellId;
  vtkPolyData *polyData = vtkPolyData::New();
  vtkPolyData *remotePolyData = vtkPolyData::New();
  vtkPointLocator *outputLocator = vtkPointLocator::New();
  vtkPoints *polyDataPoints = vtkPoints::New();
  float *bounds = input->GetBounds(), *remoteBounds = bounds;
  int ghostLevel = this->GetGhostLevel();
  int pointIncr, cellIdCount;
  int *cellIdMap;
  
  if (!this->Controller)
    {
    vtkErrorMacro("need controller to get remote ghost cells");
    return;
    }
  
  this->Locator->InitPointInsertion(vtkPoints::New(), input->GetBounds());
  
  myId = this->Controller->GetLocalProcessId();
  numProcs = this->Controller->GetNumberOfProcesses();
  numPoints = input->GetNumberOfPoints();
  polys = input->GetPolys();
  remoteCells = (vtkIdList**)malloc(numProcs * sizeof(vtkIdList));
  
  for (id = 0; id < numProcs; id++)
    {
    if (id != myId)
      {
      this->Controller->Send(input->GetBounds(), 6, id, VTK_BOUNDS_TAG);
      }
    remoteCells[id] = vtkIdList::New();
    }
  
  for (id = 0; id < numProcs; id++)
    {
    if (id != myId)
      {
      this->Controller->Receive(remoteBounds, 6, id, VTK_BOUNDS_TAG);
      if (remoteBounds[0] < bounds[0])
	{
	bounds[0] = remoteBounds[0];
	}
      if (remoteBounds[1] > bounds[1])
	{
	bounds[1] = remoteBounds[1];
	}
      if (remoteBounds[2] < bounds[2])
	{
	bounds[2] = remoteBounds[2];
	}
      if (remoteBounds[3] > bounds[3])
	{
	bounds[3] = remoteBounds[3];
	}
      if (remoteBounds[4] < bounds[4])
	{
	bounds[4] = remoteBounds[4];
	}
      if (remoteBounds[5] > bounds[5])
	{
	bounds[5] = remoteBounds[5];
	}
      }
    }
  
  outputLocator->InitPointInsertion(points, bounds);
  
  polyData->GetPointData()->CopyAllocate(input->GetPointData());
  polyData->GetPointData()->CopyGhostLevelsOff();
  polyData->GetCellData()->CopyAllocate(input->GetCellData());
  polyData->GetCellData()->CopyGhostLevelsOff();
  output->GetPointData()->CopyAllocate(input->GetPointData());
  output->GetPointData()->CopyGhostLevelsOff();
  output->GetCellData()->CopyAllocate(input->GetCellData());
  output->GetCellData()->CopyGhostLevelsOff();
  
  for (j = 0; j < numPoints; j++)
    {
    input->GetPoint(j, point);
    this->Locator->InsertNextPoint(point);
    outputLocator->InsertNextPoint(point);
    output->GetPointData()->CopyData(input->GetPointData(), j, j);
    }
  
  for (j = 0; j < polys->GetNumberOfCells(); j++)
    {
    ghostLevels->InsertNextGhostLevel(0);
    output->GetCellData()->CopyData(input->GetCellData(), j, j);
    }
  
  output->SetPoints(points);
  output->SetPolys(polys);
  output->GetCellData()->SetGhostLevels(ghostLevels);
  
  for (gl = 0; gl < ghostLevel; gl++)
    {
    output->DeleteCells();
    output->BuildLinks();
    pointIncr = 0;
    myPoints = new float[output->GetNumberOfPoints()*3];
    for (i = 0; i < output->GetNumberOfCells(); i++)
      {
      if (output->GetCellData()->GetGhostLevels()->GetGhostLevel(i) == gl)
	{
	output->GetCell(i, cell);
	for (j = 0; j < cell->GetNumberOfPoints(); j++)
	  {
	  cell->GetPoints()->GetPoint(j, point);
	  if (currentPoints->IsId(cell->GetPointId(j)) == -1)
	    {
	    currentPoints->InsertNextId(cell->GetPointId(j));
	    for (k = 0; k < 3; k++)
	      {
	      myPoints[pointIncr*3+k] = point[k];
	      }
	    pointIncr++;
	    }
	  }
	}
      }
    currentPoints->Reset();
    numPoints = pointIncr;
    for (id = 0; id < numProcs; id++)
      {
      if (id != myId)
	{
	this->Controller->Send((int*)(&numPoints), 1, id, VTK_NUM_POINTS_TAG);
	this->Controller->Send(myPoints, numPoints*3, id,
			       VTK_POINT_COORDS_TAG);
	}
      }
    
    for (id = 0; id < numProcs; id++)
      {
      if (id != myId)
	{
	cellIdMap = new int[input->GetNumberOfCells()];
	cellIdCount = 0;
	polyData->Allocate(input->GetNumberOfCells());
	this->Controller->Receive((int*)(&numRemotePoints), 1, id,
				  VTK_NUM_POINTS_TAG);
	if (myId == 1)
	  {
	  cerr << "received " << numRemotePoints << " points" << endl;
	  }
	remotePoints = new float[numRemotePoints*3];
	this->Controller->Receive(remotePoints, numRemotePoints*3, id,
				  VTK_POINT_COORDS_TAG);
	for (i = 0; i < numRemotePoints; i++)
	  {
	  point[0] = remotePoints[i*3];
	  point[1] = remotePoints[i*3+1];
	  point[2] = remotePoints[i*3+2];
	  if ((pointId = this->Locator->IsInsertedPoint(point)) >= 0)
	    {
	    output->GetPointCells(pointId, cellIds);
	    numCells = cellIds->GetNumberOfIds();
	    for (j = 0; j < numCells; j++)
	      {
	      cellId = cellIds->GetId(j);
	      output->GetCell(cellId, cell);
	      if ((insertedCells->IsId(cellId) == -1) &&
		  output->GetCellData()->GetGhostLevels()->
		  GetGhostLevel(cellId) == 0)
		{
		insertedCells->InsertNextId(cellId);
		cellIdMap[cellIdCount] = cellId;
		cellIdCount++;
		}
	      } // for all point cells
	    } // if point in my data
	  } // for all points received from this process
	polyData->CopyCells(input, insertedCells);
      
	this->Controller->Send(polyData, id, VTK_POLY_DATA_TAG);
	this->Controller->Send(cellIdMap, cellIdCount, id, VTK_CELL_ID_TAG);
	if (myId == 1)
	  {
	  cerr << "sent " << polyData->GetNumberOfCells() << " cells" << endl;
	  }
	insertedCells->Reset();
	polyData->Reset();
	polyDataPoints->Reset();
	delete [] remotePoints;
	delete [] cellIdMap;
	} // if not my process
      } // for all processes (find point cells)
  
    for (id = 0; id < numProcs; id++)
      {
      if (id != myId)
	{
	this->Controller->Receive(remotePolyData, id, VTK_POLY_DATA_TAG);
	numCells = remotePolyData->GetNumberOfCells();
	cellIdMap = new int[numCells];
	this->Controller->Receive(cellIdMap, numCells, id, VTK_CELL_ID_TAG);
	for (j = 0; j < numCells; j++)
	  {
	  if (remoteCells[id]->IsId(cellIdMap[j]) == -1)
	    {
	    remoteCells[id]->InsertNextId(cellIdMap[j]);
	    newCells->InsertNextId(j);
	    ghostLevels->InsertNextGhostLevel(gl+1);
	    output->DeleteCells();
	    output->BuildLinks();
	    }
	  } // for all cells sent by this process
	output->CopyCells(remotePolyData, newCells, outputLocator);
	if (myId == 0)
	  {
	  cerr << "output has " << output->GetNumberOfCells() << " cells"
	       << endl;
	  }
	delete [] cellIdMap;
	} // if not my process
      } // for all processes
  
    output->GetCellData()->SetGhostLevels(ghostLevels);
    delete [] myPoints;
    newCells->Reset();
    }
  
  points->Delete();
  points = NULL;
  outputLocator->Delete();
  outputLocator = NULL;
  cellIds->Delete();
  cellIds = NULL;
  cell->Delete();
  cell = NULL;
  ghostLevels->Delete();
  ghostLevels = NULL;
  insertedCells->Delete();
  polyData->Delete();
  remotePolyData->Delete();
  currentPoints->Delete();
  for (i = 0; i < numProcs; i++)
    {
    remoteCells[i]->Delete();
    }
  free(remoteCells);
}

void vtkGetRemoteGhostCells::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Controller (" << this->Controller << ")\n";
  os << indent << "Locator (" << this->Locator << ")/n";
}
