/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageEuclideanDistance.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageEuclideanDistance.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

vtkCxxRevisionMacro(vtkImageEuclideanDistance, "1.18");
vtkStandardNewMacro(vtkImageEuclideanDistance);

//----------------------------------------------------------------------------
// This defines the default values for the EDT parameters 
vtkImageEuclideanDistance::vtkImageEuclideanDistance()
{
  this->MaximumDistance = VTK_INT_MAX;
  this->Initialize = 1;
  this->ConsiderAnisotropy = 1;
  this->Algorithm = VTK_EDT_SAITO;
}

//----------------------------------------------------------------------------
// This extent of the components changes to real and imaginary values.
void vtkImageEuclideanDistance::IterativeRequestInformation(
  vtkInformation* vtkNotUsed(input), vtkInformation* output)
{
  output->Set(vtkDataObject::SCALAR_NUMBER_OF_COMPONENTS(),1);
  output->Set(vtkDataObject::SCALAR_TYPE(),VTK_DOUBLE);
}

//----------------------------------------------------------------------------
// This method tells the superclass that the whole input array is needed
// to compute any output region.
void vtkImageEuclideanDistance::IterativeRequestUpdateExtent(
  vtkInformation* input, vtkInformation* vtkNotUsed(output) )
{
  int *wExt = input->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  input->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),wExt,6);
}

//----------------------------------------------------------------------------
// This templated execute method handles any type input, but the output
// is always doubles.
template <class TT>
void vtkImageEuclideanDistanceCopyData(vtkImageEuclideanDistance *self,
                                       vtkImageData *inData, TT *inPtr,
                                       vtkImageData *outData, int outExt[6], 
                                       double *outPtr )
{
  int inInc0, inInc1, inInc2;
  TT *inPtr0, *inPtr1, *inPtr2;

  int outMin0, outMax0, outMin1, outMax1, outMin2, outMax2;
  int outInc0, outInc1, outInc2;
  double *outPtr0, *outPtr1, *outPtr2;
  
  int idx0, idx1, idx2;
  
  // Reorder axes
  self->PermuteExtent(outExt, outMin0,outMax0,outMin1,outMax1,outMin2,outMax2);
  self->PermuteIncrements(inData->GetIncrements(), inInc0, inInc1, inInc2);
  self->PermuteIncrements(outData->GetIncrements(), outInc0, outInc1, outInc2);
  
  inPtr2 = inPtr;
  outPtr2 = outPtr;
  for (idx2 = outMin2; idx2 <= outMax2; ++idx2)
    {
    inPtr1 = inPtr2;
    outPtr1 = outPtr2;
    for (idx1 = outMin1; idx1 <= outMax1; ++idx1)
      {
      inPtr0 = inPtr1;
      outPtr0 = outPtr1;

      for (idx0 = outMin0; idx0 <= outMax0; ++idx0)
        {
        *outPtr0 = *inPtr0 ;
        inPtr0 += inInc0;
        outPtr0 += outInc0;
        }
      inPtr1 += inInc1;
      outPtr1 += outInc1;
      }
    inPtr2 += inInc2;
    outPtr2 += outInc2;
    }
}

//----------------------------------------------------------------------------
// This templated execute method handles any type input, but the output
// is always doubles.
template <class T>
void vtkImageEuclideanDistanceInitialize(vtkImageEuclideanDistance *self,
                                         vtkImageData *inData, T *inPtr,
                                         vtkImageData *outData, 
                                         int outExt[6], double *outPtr )
{
  int inInc0, inInc1, inInc2;
  T *inPtr0, *inPtr1, *inPtr2;

  int outMin0, outMax0, outMin1, outMax1, outMin2, outMax2;
  int outInc0, outInc1, outInc2;
  double *outPtr0, *outPtr1, *outPtr2;
  
  int idx0, idx1, idx2;
  double maxDist;
  
  // Reorder axes
  self->PermuteExtent(outExt, outMin0,outMax0,outMin1,outMax1,outMin2,outMax2);
  self->PermuteIncrements(inData->GetIncrements(), inInc0, inInc1, inInc2);
  self->PermuteIncrements(outData->GetIncrements(), outInc0, outInc1, outInc2);
  
  if ( self->GetInitialize() == 1 ) 
    // Initialization required. Input image is only used as binary mask, 
    // so all non-zero values are set to maxDist
    //
    {      
    maxDist = self->GetMaximumDistance();

    inPtr2 = inPtr;
    outPtr2 = outPtr;
    for (idx2 = outMin2; idx2 <= outMax2; ++idx2)
      {
      inPtr1 = inPtr2;
      outPtr1 = outPtr2;
      for (idx1 = outMin1; idx1 <= outMax1; ++idx1)
        {
        inPtr0 = inPtr1;
        outPtr0 = outPtr1;
              
        for (idx0 = outMin0; idx0 <= outMax0; ++idx0)
          {
          if( *inPtr0 == 0 ) {*outPtr0 = 0;}
          else {*outPtr0 = maxDist;}
                  
          inPtr0 += inInc0;
          outPtr0 += outInc0;
          }
              
        inPtr1 += inInc1;
        outPtr1 += outInc1;
        }
      inPtr2 += inInc2;
      outPtr2 += outInc2;
      }
    }
  else   
    // No initialization required. We just copy inData to outData.
    {
    vtkImageEuclideanDistanceCopyData( self, 
                                       inData, (T *)(inPtr), 
                                       outData, outExt, (double *)(outPtr) );
    }
}

//----------------------------------------------------------------------------
// Execute Saito's algorithm.
//
// T. Saito and J.I. Toriwaki. New algorithms for Euclidean distance 
// transformations of an n-dimensional digitised picture with applications.
// Pattern Recognition, 27(11). pp. 1551--1565, 1994. 
// 
// Notations stay as close as possible to those used in the paper.
//
void vtkImageEuclideanDistanceExecuteSaito(vtkImageEuclideanDistance *self,
                                           vtkImageData *outData, 
                                           int outExt[6], double *outPtr )
{
  
  int outMin0, outMax0, outMin1, outMax1, outMin2, outMax2;
  int outInc0, outInc1, outInc2;
  double *outPtr0, *outPtr1, *outPtr2;
  int idx0, idx1, idx2, inSize0;
  double maxDist;
  double *sq;
  double *buff,buffer;
  int df,a,b,n;
  double m;
  double spacing;
  
  // Reorder axes (The outs here are just placeholdes
  self->PermuteExtent(outExt, outMin0,outMax0,outMin1,outMax1,outMin2,outMax2);
  self->PermuteIncrements(outData->GetIncrements(), outInc0, outInc1, outInc2);
  
  inSize0 = outMax0 - outMin0 + 1;  
  maxDist = self->GetMaximumDistance();

  buff= (double *)calloc(outMax0+1,sizeof(double));
    
  // precompute sq[]. Anisotropy is handled here by using Spacing information
  sq = (double *)calloc(inSize0*2+2,sizeof(double));
  for(df=2*inSize0+1;df>inSize0;df--)
    {
    sq[df]=maxDist;
    }
  
  if ( self->GetConsiderAnisotropy() )
    {
    spacing = outData->GetSpacing()[ self->GetIteration() ];
    }
  else
    {
    spacing = 1;
    }
  
  spacing*=spacing;
  
  for(df=inSize0;df>=0;df--) 
    {
    sq[df]=df*df*spacing;
    }
  
  if ( self->GetIteration() == 0 ) 
    {
    outPtr2 = outPtr;
    for (idx2 = outMin2; idx2 <= outMax2; ++idx2)
      {
      outPtr1 = outPtr2;
      for (idx1 = outMin1; idx1 <= outMax1; ++idx1)
        {
        outPtr0 = outPtr1;
        df= inSize0 ;
        for (idx0 = outMin0; idx0 <= outMax0; ++idx0)
          {
          if(*outPtr0 != 0)
            {
            df++ ;
            if(sq[df] < *outPtr0) {*outPtr0 = sq[df];}
            }
          else 
            {
            df=0;
            }
          outPtr0 += outInc0;
          }
              
        outPtr0 -= outInc0;
        df= inSize0 ;
        for (idx0 = outMax0; idx0 >= outMin0; --idx0)
          {
          if(*outPtr0 != 0)
            {
            df++ ;
            if(sq[df] < *outPtr0) {*outPtr0 = sq[df];}
            }
          else 
            {
            df=0;
            }
          outPtr0 -= outInc0;
          }
              
        outPtr1 += outInc1;
        }
      outPtr2 += outInc2;
      }      
    }
  else // next iterations are all identical. 
    {     
    outPtr2 = outPtr;
    for (idx2 = outMin2; idx2 <= outMax2; ++idx2)
      {
      outPtr1 = outPtr2;
      for (idx1 = outMin1; idx1 <= outMax1; ++idx1)
        {
        outPtr0 = outPtr1;
              
        // Buffer current values 
        for (idx0 = outMin0; idx0 <= outMax0; ++idx0)
          {
          buff[idx0]= *outPtr0;
          outPtr0 += outInc0;
          }
              
        // forward scan 
        a=0; buffer=buff[ outMin0 ];
        outPtr0 = outPtr1;
        outPtr0 += outInc0;
              
        for (idx0 = outMin0+1; idx0 <= outMax0; ++idx0)
          {
          if(a>0) {a--;}
          if(buff[idx0]>buffer+sq[1]) 
            {
            b=(int)(floor)((((buff[idx0]-buffer)/spacing)-1)/2); 
            if((idx0+b)>outMax0) {b=(outMax0)-idx0;}
                      
            for(n=a;n<=b;n++) 
              {
              m=buffer+sq[n+1];
              if(buff[idx0+n]<=m) {n=b;}
              else if(m<*(outPtr0+n*outInc0)) {*(outPtr0+n*outInc0)=m;}
              }
            a=b; 
            }
          else
            {
            a=0;
            }
                  
          buffer=buff[idx0];
          outPtr0 += outInc0;
          }
              
        outPtr0 -= 2*outInc0;
        a=0;
        buffer=buff[outMax0];
    
        for(idx0=outMax0-1;idx0>=outMin0; --idx0) 
          {
          if(a>0) {a--;}
          if(buff[idx0]>buffer+sq[1]) 
            {
            b=(int)(floor)((((buff[idx0]-buffer)/spacing)-1)/2); 
            if((idx0-b)<outMin0) {b=idx0-outMin0;}
                    
            for(n=a;n<=b;n++) 
              {
              m=buffer+sq[n+1];
              if(buff[idx0-n]<=m) {n=b;}
              else if(m<*(outPtr0-n*outInc0)) {*(outPtr0-n*outInc0)=m;}
              }
            a=b;  
            }
          else
            {
            a=0;
            }
          
          buffer=buff[idx0];
          outPtr0 -= outInc0;
          }
        outPtr1 += outInc1;
        }
      outPtr2 += outInc2;            
      } 
    }
  
  free(buff);
  free(sq);
}


//----------------------------------------------------------------------------
// Execute Saito's algorithm, modified for Cache Efficiency
//
void vtkImageEuclideanDistanceExecuteSaitoCached(
  vtkImageEuclideanDistance *self,
  vtkImageData *outData, int outExt[6], double *outPtr )
{
  
  int outMin0, outMax0, outMin1, outMax1, outMin2, outMax2;
  int outInc0, outInc1, outInc2;
  double *outPtr0, *outPtr1, *outPtr2;
  //
  int idx0, idx1, idx2, inSize0;
  
  double maxDist;
  
  double *sq;
  double *buff,*temp,buffer;
  int df,a,b,n;
  double m;
  
  double spacing;
  
  // Reorder axes (The outs here are just placeholdes
  self->PermuteExtent(outExt, outMin0,outMax0,outMin1,outMax1,outMin2,outMax2);
  self->PermuteIncrements(outData->GetIncrements(), outInc0, outInc1, outInc2);
  
  inSize0 = outMax0 - outMin0 + 1;  
  maxDist = self->GetMaximumDistance();

  buff= (double *)calloc(outMax0+1,sizeof(double));
  temp= (double *)calloc(outMax0+1,sizeof(double));
    
  // precompute sq[]. Anisotropy is handled here by using Spacing information
  sq = (double *)calloc(inSize0*2+2,sizeof(double));
  for(df=2*inSize0+1;df>inSize0;df--) 
    {
    sq[df]=maxDist;
    }
  if ( self->GetConsiderAnisotropy() )
    {
    spacing = outData->GetSpacing()[ self->GetIteration() ];
    }
  else
    {
    spacing = 1;
    }
  spacing*=spacing;
  
  for(df=inSize0;df>=0;df--) 
    {
    sq[df]=df*df*spacing;
    }
  
  if ( self->GetIteration() == 0 ) 
    {
    outPtr2 = outPtr;
    for (idx2 = outMin2; idx2 <= outMax2; ++idx2)
      {
      outPtr1 = outPtr2;
      for (idx1 = outMin1; idx1 <= outMax1; ++idx1)
        {
        outPtr0 = outPtr1;
        df= inSize0 ;
        for (idx0 = outMin0; idx0 <= outMax0; ++idx0)
          {
          if(*outPtr0 != 0)
            {
            df++ ;
            if(sq[df] < *outPtr0) {*outPtr0 = sq[df];}
            }
          else 
            {
            df=0;
            }
                  
          outPtr0 += outInc0;
          }
              
        outPtr0 -= outInc0;
        df= inSize0 ;
        for (idx0 = outMax0; idx0 >= outMin0; --idx0)
          {
          if(*outPtr0 != 0)
            {
            df++ ;
            if(sq[df] < *outPtr0) {*outPtr0 = sq[df];}
            }
          else 
            {
            df=0;
            }
          
          outPtr0 -= outInc0;
          }
              
        outPtr1 += outInc1;
        }
      outPtr2 += outInc2;
      }      
    }
  else // next iterations are all identical. 
    {     
    outPtr2 = outPtr;
    for (idx2 = outMin2; idx2 <= outMax2; ++idx2)
      {
      outPtr1 = outPtr2;
      for (idx1 = outMin1; idx1 <= outMax1; ++idx1)
        {
        // Buffer current values 
        outPtr0 = outPtr1;
        for (idx0 = outMin0; idx0 <= outMax0; ++idx0)
          {
          temp[idx0] = buff[idx0]= *outPtr0;
          outPtr0 += outInc0;
          }
              
        // forward scan 
        a=0; buffer=buff[ outMin0 ];
        outPtr0 = temp ;
        outPtr0 ++;
              
        for (idx0 = outMin0+1; idx0 <= outMax0; ++idx0)
          {
          if(a>0) {a--;}
          
          if(buff[idx0]>buffer+sq[1]) 
            {
            b=(int)(floor)((((buff[idx0]-buffer)/spacing)-1)/2); 
            if((idx0+b)>outMax0) {b=(outMax0)-idx0;}
                      
            for(n=a;n<=b;n++) 
              {
              m=buffer+sq[n+1];
              if(buff[idx0+n]<=m) {n=b;}  
              else if(m<*(outPtr0+n)) {*(outPtr0+n)=m;}
              }
            a=b; 
            }
          else
            {
            a=0;
            }
                  
          buffer=buff[idx0];
          outPtr0 ++;
          }
              
        // backward scan
        outPtr0 -= 2;
        a=0;
        buffer=buff[outMax0];
    
        for(idx0=outMax0-1;idx0>=outMin0; --idx0) 
          {
          if(a>0) {a--;}
          if(buff[idx0]>buffer+sq[1]) 
            {
            b=(int)(floor)((((buff[idx0]-buffer)/spacing)-1)/2); 
            if((idx0-b)<outMin0) b=idx0-outMin0;
                    
            for(n=a;n<=b;n++) 
              {
              m=buffer+sq[n+1];
              if(buff[idx0-n]<=m) {n=b;}
              else if(m<*(outPtr0-n)) {*(outPtr0-n)=m;}
              }
            a=b;  
            }
          else
            {
            a=0;
            }
          buffer=buff[idx0];
          outPtr0 --;
          }

        // Unbuffer current values 
        outPtr0 = outPtr1;
        for (idx0 = outMin0; idx0 <= outMax0; ++idx0)
          {
          *outPtr0 = temp[idx0];
          outPtr0 += outInc0;
          }

        outPtr1 += outInc1;
        }
      outPtr2 += outInc2;            
      } 
    }
  
  free(buff);
  free(temp);
  free(sq);
}
//----------------------------------------------------------------------------
void vtkImageEuclideanDistance::AllocateOutputScalars(vtkImageData *outData)
{
  outData->SetExtent(outData->GetWholeExtent());
  outData->AllocateScalars();
}

//----------------------------------------------------------------------------
// This method is passed input and output Datas, and executes the
// EuclideanDistance algorithm to fill the output from the input.
void vtkImageEuclideanDistance::IterativeRequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkImageData *inData = vtkImageData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *outData = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  this->AllocateOutputScalars(outData);
  
  void *inPtr;
  void *outPtr;

  vtkDebugMacro(<<"Executing image euclidean distance");
  
  int outExt[6];
  outData->GetWholeExtent( outExt );
  
  inPtr = inData->GetScalarPointerForExtent(inData->GetUpdateExtent());
  outPtr = outData->GetScalarPointer();
   
  if (!inPtr)
    {
    vtkErrorMacro(<< "Execute: No scalars for update extent.")
    return;
    }
   
  
  // this filter expects that the output be doubles.
  if (outData->GetScalarType() != VTK_DOUBLE)
    {
    vtkErrorMacro(<< "Execute: Output must be be type double.");
    return;
    }
  
  // this filter expects input to have 1 components
  if (outData->GetNumberOfScalarComponents() != 1 )
    {
    vtkErrorMacro(<< "Execute: Cannot handle more than 1 components");
    return;
    }
  
  if ( this->GetIteration() == 0 )
    {
    switch (inData->GetScalarType())
      {
      vtkTemplateMacro6(vtkImageEuclideanDistanceInitialize,
                        this, 
                        inData, (VTK_TT *)(inPtr), 
                        outData, outExt, (double *)(outPtr) );
      default:
        vtkErrorMacro(<< "Execute: Unknown ScalarType");
        return;
      } 
    } 
  else 
    { 
    if( inData != outData )
      switch (inData->GetScalarType())
        {    
        vtkTemplateMacro6(vtkImageEuclideanDistanceCopyData,
                          this, 
                          inData, (VTK_TT *)(inPtr), 
                          outData, outExt, (double *)(outPtr) );
        }
    }
  
  // Call the specific algorithms. 
  switch( this->GetAlgorithm() ) 
    {
    case VTK_EDT_SAITO:
      vtkImageEuclideanDistanceExecuteSaito( this, outData, outExt, 
                                             (double *)(outPtr) );
      break;
    case VTK_EDT_SAITO_CACHED:
      vtkImageEuclideanDistanceExecuteSaitoCached( this, outData, outExt, 
                                                   (double *)(outPtr) );
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown Algorithm");
    }
  
  this->UpdateProgress((this->GetIteration()+1.0)/3.0);
}


//----------------------------------------------------------------------------
// For streaming and threads.  Splits output update extent into num pieces.
// This method needs to be called num times.  Results must not overlap for
// consistent starting extent.  Subclass can override this method.
// This method returns the number of peices resulting from a successful split.
// This can be from 1 to "total".  
// If 1 is returned, the extent cannot be split.
int vtkImageEuclideanDistance::SplitExtent(int splitExt[6], int startExt[6], 
                             int num, int total)
{
  int splitAxis;
  int min, max;

  vtkDebugMacro("SplitExtent: ( " << startExt[0] << ", " << startExt[1] << ", "
                << startExt[2] << ", " << startExt[3] << ", "
                << startExt[4] << ", " << startExt[5] << "), " 
                << num << " of " << total);

  // start with same extent
  memcpy(splitExt, startExt, 6 * sizeof(int));

  splitAxis = 2;
  min = startExt[4];
  max = startExt[5];
  while ((splitAxis == this->Iteration) || (min == max))
    {
    splitAxis--;
    if (splitAxis < 0)
      { // cannot split
      vtkDebugMacro("  Cannot Split");
      return 1;
      }
    min = startExt[splitAxis*2];
    max = startExt[splitAxis*2+1];
    }

  // determine the actual number of pieces that will be generated
  if ((max - min + 1) < total)
    {
    total = max - min + 1;
    }
  
  if (num >= total)
    {
    vtkDebugMacro("  SplitRequest (" << num 
                  << ") larger than total: " << total);
    return total;
    }
  
  // determine the extent of the piece
  splitExt[splitAxis*2] = min + (max - min + 1)*num/total;
  if (num == total - 1)
    {
    splitExt[splitAxis*2+1] = max;
    }
  else
    {
    splitExt[splitAxis*2+1] = (min-1) + (max - min + 1)*(num+1)/total;
    }
  
  vtkDebugMacro("  Split Piece: ( " <<splitExt[0]<< ", " <<splitExt[1]<< ", "
                << splitExt[2] << ", " << splitExt[3] << ", "
                << splitExt[4] << ", " << splitExt[5] << ")");
  fflush(stderr);

  return total;
}

void vtkImageEuclideanDistance::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Initialize: " 
     << (this->Initialize ? "On\n" : "Off\n");
  
  os << indent << "Consider Anisotropy: " 
     << (this->ConsiderAnisotropy ? "On\n" : "Off\n");
  
  os << indent << "Initialize: " << this->Initialize << "\n";
  os << indent << "Maximum Distance: " << this->MaximumDistance << "\n";

  os << indent << "Algorithm: ";
  if ( this->Algorithm == VTK_EDT_SAITO )
    {
    os << "Saito\n";
    }
  else 
    {
    os << "Saito Cached\n";
    }
}
  
















