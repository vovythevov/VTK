/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPoints.h
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
// .NAME vtkStructuredPoints - A subclass of ImageData.
// .SECTION Description
// StructuredPoints is a subclass of ImageData that requires the data extent
// to exactly match the update extent. Normall image data allows that the
// data extent may be larger than the update extent.
// StructuredPoints also defines the origin differently that vtkImageData.
// For structured points the origin is the location of first point. 
// Whereas images define the origin as the location of point 0, 0, 0.
// Image Origin is stored in ivar, and structured points
// have special methods for setting/getting the origin/extents.


#ifndef __vtkStructuredPoints_h
#define __vtkStructuredPoints_h

#include "vtkImageData.h"

  
class VTK_COMMON_EXPORT vtkStructuredPoints : public vtkImageData
{
public:
  static vtkStructuredPoints *New();
  vtkTypeRevisionMacro(vtkStructuredPoints,vtkImageData);

  // Description:
  // To simplify filter superclasses,
  int GetDataObjectType() {return VTK_STRUCTURED_POINTS;}

protected:
  vtkStructuredPoints();
  ~vtkStructuredPoints() {};
private:
  vtkStructuredPoints(const vtkStructuredPoints&);  // Not implemented.
  void operator=(const vtkStructuredPoints&);  // Not implemented.
};

#endif



