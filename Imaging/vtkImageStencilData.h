/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStencilData.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageStencilData - efficient description of an image stencil
// .SECTION Description
// vtkImageStencilData describes an image stencil in a manner which is
// efficient both in terms of speed and storage space.  The stencil extents
// are stored for each x-row across the image (multiple extents per row if
// necessary) and can be retrieved via the GetNextExtent() method.
// .SECTION see also
// vtkImageStencilSource vtkImageStencil

#ifndef __vtkImageStencilData_h
#define __vtkImageStencilData_h


#include "vtkDataObject.h"

class VTK_IMAGING_EXPORT vtkImageStencilData : public vtkDataObject
{
public:
  static vtkImageStencilData *New();
  vtkTypeRevisionMacro(vtkImageStencilData, vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent);


#ifndef VTK_REMOVE_LEGACY_CODE
  // Description:
  // For legacy compatibility.  Do not use.
  virtual vtkDataObject* MakeObject();
#endif
  
  void Initialize();
  void DeepCopy(vtkDataObject *o);
  void ShallowCopy(vtkDataObject *f);
  void InternalImageStencilDataCopy(vtkImageStencilData *s);

  // Description:
  // Get the data type as an integer (this will return VTK_DATA_OBJECT
  // for now, maybe a proper type constant will be reserved later).
  int GetDataObjectType() { return VTK_DATA_OBJECT; }

  // Description:
  // The extent type is 3D, just like vtkImageData.
  int GetExtentType() { return VTK_3D_EXTENT; };

  // Description:
  // Given the total output x extent [xMin,xMax] and the current y, z indices,
  // return each sub-extent [r1,r2] that lies within within the unclipped
  // region in sequence.  A value of '0' is returned if no more sub-extents
  // are available.  The variable 'iter' must be initialized to zero before
  // the first call, unless you want the complementary sub-extents in which
  // case you must initialize 'iter' to -1.  The variable 'iter' is used
  // internally to keep track of which sub-extent should be returned next.
  int GetNextExtent(int &r1, int &r2, int xMin, int xMax,
                    int yIdx, int zIdx, int &iter);

  // Description:
  // This method is used by vtkImageStencilDataSource to add an x 
  // sub extent [r1,r2] for the x row (yIdx,zIdx).  The specified sub
  // extent must not intersect any other sub extents along the same x row.
  // As well, r1 and r2 must both be within the total x extent
  // [Extent[0],Extent[1]].
  void InsertNextExtent(int r1, int r2, int yIdx, int zIdx);

  // Description:
  // Set the desired spacing for the stencil.
  // This must be called before the stencil is Updated, ideally 
  // in the ExecuteInformation method of the imaging filter that
  // is using the stencil.
  vtkSetVector3Macro(Spacing, float);
  vtkGetVector3Macro(Spacing, float);

  // Description:
  // Set the desired origin for the stencil.
  // This must be called before the stencil is Updated, ideally 
  // in the ExecuteInformation method of the imaging filter that
  // is using the stencil.
  vtkSetVector3Macro(Origin, float);
  vtkGetVector3Macro(Origin, float);

  // Description:
  // Set the extent of the data.  This is should be called only 
  // by vtkImageStencilSource, as it is part of the basic pipeline
  // functionality.
  vtkSetVector6Macro(Extent, int);
  vtkGetVector6Macro(Extent, int);

  // Description:
  // Save the previous data spacing.  This should be called only by
  // vtkImageStencilSource.
  vtkSetVector3Macro(OldSpacing, float);
  vtkGetVector3Macro(OldSpacing, float);

  // Description: 
  // Save the previous data origin.  This should be called only by
  // vtkImageStencilSource.
  vtkSetVector3Macro(OldOrigin, float);
  vtkGetVector3Macro(OldOrigin, float);

  // Description:
  // Allocate space for the sub-extents.  This is called by
  // vtkImageStencilSource.
  void AllocateExtents();

  // Description:
  // WARNING: INTERNAL METHOD - NOT FOR GENERAL USE. 
  // THIS METHOD IS PART OF THE PIPELINE UPDATE FUNCTIONALITY.
  // Propagate the update back up the pipeline, and perform the actual 
  // work of updating on the way down. When the propagate arrives at a
  // port, block and wait for the asynchronous update to finish on the
  // other side.
  // This propagation may early terminate based on the PipelineMTime.
  void UpdateData();

  // Description:
  // WARNING: INTERNAL METHOD - NOT FOR GENERAL USE. 
  // THIS METHOD IS PART OF THE PIPELINE UPDATE FUNCTIONALITY.
  // The update extent for this object is propagated up the pipeline.
  // This propagation may early terminate based on the PipelineMTime.
  void PropagateUpdateExtent();

  // Description:
  // WARNING: INTERNAL METHOD - NOT FOR GENERAL USE. 
  // THIS METHOD IS PART OF THE PIPELINE UPDATE FUNCTIONALITY.
  // Propagate back up the pipeline for ports and trigger the update on the
  // other side of the port to allow for asynchronous parallel processing in
  // the pipeline.
  // This propagation may early terminate based on the PipelineMTime.
  void TriggerAsynchronousUpdate();

protected:
  vtkImageStencilData();
  ~vtkImageStencilData();

  // Description:
  // Check to see of the Spacing and Origin are different
  // from the OldSpacing and OldOrigin.
  int SpacingOrOriginHasChanged();

  // Description:
  // Previous Spacing and Origin values, so that it is possible to
  // know when an update is required because they have changed.
  float OldSpacing[3];
  float OldOrigin[3];

  // Description:
  // The Spacing and Origin of the data.
  float Spacing[3];
  float Origin[3];
  
  // Description:
  // The actual 'data' is stored here.
  int NumberOfExtentEntries;
  int *ExtentListLengths;
  int **ExtentLists;
private:
  vtkImageStencilData(const vtkImageStencilData&);  // Not implemented.
  void operator=(const vtkImageStencilData&);  // Not implemented.
};

#endif



