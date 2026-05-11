#pragma once
#include <vtkDataSetMapper.h>
#include <vtkLookupTable.h>
#include <vtkUnsignedIntArray.h>
#include <vtkActor.h>
#include <vtkRenderWindow.h>

class vtkSliderWidget;

struct CallbackData {
    vtkDataSetMapper* pressureMapper;
    vtkDataSetMapper* velocityMapper;
    vtkLookupTable* flagLut;
    vtkLookupTable* densityLut;
    vtkLookupTable* velocityLut;
    vtkSliderWidget* chordWidget;
    vtkSliderWidget* angleWidget;
    vtkActor* streamActor;
    class Lattice2D* lattice;
    vtkUnsignedIntArray* flagArray;
    vtkRenderWindow* renderWindow;
    int nx_sim, ny_sim;
    int viewState = 0;
    bool* isPaused;
    double bx, by, diameter;
    double chord = 160.0;
    double angle = -10.0;
    int geometryState = 1;
    vtkRenderer* renLeft;
    vtkRenderer* renRight;
    vtkActor* pressureActor;
    vtkActor* velocityActor;
    bool splitMode = false;
    vtkActor* arrowActor;
};

void UpdateGeometry(CallbackData* data);