#pragma once
#include <vtkSliderWidget.h>
#include <vtkSliderRepresentation2D.h>
#include <vtkCommand.h>
#include <callbackdata.h>
#include <defines.h>
#include <lattice.h>

class vtkSliderCallbackSteps : public vtkCommand {
public:
    static vtkSliderCallbackSteps* New() { return new vtkSliderCallbackSteps; }
    usize* vtk_steps;

    virtual void Execute(vtkObject* caller, unsigned long, void*) override {
        vtkSliderWidget* sliderWidget = reinterpret_cast<vtkSliderWidget*>(caller);
        double value = static_cast<vtkSliderRepresentation*>(sliderWidget->GetRepresentation())->GetValue();

        *(this->vtk_steps) = static_cast<usize>(value);
    }
};

class vtkSliderCallbackSpeed : public vtkCommand {
public:
    static vtkSliderCallbackSpeed* New() { return new vtkSliderCallbackSpeed; }
    vec2<double>* u_in = nullptr;

    virtual void Execute(vtkObject* caller, unsigned long, void*) override {
        vtkSliderWidget* sliderWidget = reinterpret_cast<vtkSliderWidget*>(caller);
        auto rep = reinterpret_cast<vtkSliderRepresentation*>(sliderWidget->GetRepresentation());

        if (u_in) {
            double newValue = rep->GetValue();
            u_in->x = newValue;
        }
    }
};

class vtkSliderCallbackRe : public vtkCommand {
public:
    static vtkSliderCallbackRe* New() { return new vtkSliderCallbackRe; }

    double* Re = nullptr;
    double* tau = nullptr;
    vec2<double>* u_in = nullptr;
    double Ny = 0.0;

    virtual void Execute(vtkObject* caller, unsigned long, void*) override {
        vtkSliderWidget* sliderWidget = reinterpret_cast<vtkSliderWidget*>(caller);
        auto rep = reinterpret_cast<vtkSliderRepresentation*>(sliderWidget->GetRepresentation());

        if (Re && tau && u_in) {
            *Re = rep->GetValue();
            double nu = (u_in->x * Ny) / (*Re);
            *tau = 3.0 * nu + 0.5;

            if (*tau < 0.501) *tau = 0.501;
        }
    }
};

class vtkSliderCallbackChord : public vtkCommand {
public:
    static vtkSliderCallbackChord* New() { return new vtkSliderCallbackChord; }
    CallbackData* data;
    virtual void Execute(vtkObject* caller, unsigned long, void*) override {
        auto slider = reinterpret_cast<vtkSliderWidget*>(caller);
        double val = static_cast<vtkSliderRepresentation*>(slider->GetRepresentation())->GetValue();
        data->chord = val;
        UpdateGeometry(data);
    }
};

class vtkSliderCallbackAngle : public vtkCommand {
public:
    static vtkSliderCallbackAngle* New() { return new vtkSliderCallbackAngle; }
    CallbackData* data;
    virtual void Execute(vtkObject* caller, unsigned long, void*) override {
        auto slider = reinterpret_cast<vtkSliderWidget*>(caller);
        double val = static_cast<vtkSliderRepresentation*>(slider->GetRepresentation())->GetValue();
        data->angle = val;
        UpdateGeometry(data);
    }
};

class MousePaintStyle : public vtkInteractorStyleImage {
public:
    static MousePaintStyle* New();
    CallbackData* data;
    bool isPainting = false;
    bool isErasing = false;
    int brushSize = 1;

    virtual void OnChar() override {
        char key = this->Interactor->GetKeyCode();
        if (key == '+') {
            brushSize++;
            std::cout << "Pinselgroesse: " << brushSize << std::endl;
        }
        else if (key == '-' && brushSize > 1) {
            brushSize--;
            std::cout << "Pinselgroesse: " << brushSize << std::endl;
        }
        vtkInteractorStyleImage::OnChar();
    }

    virtual void OnLeftButtonDown() override { this->isPainting = true; Paint(4); }
    virtual void OnLeftButtonUp() override { this->isPainting = false; }
    virtual void OnRightButtonDown() override { this->isErasing = true; Paint(0); }
    virtual void OnRightButtonUp() override { this->isErasing = false; }

    virtual void OnMouseMove() override {
        if (this->isPainting) Paint(4);
        else if (this->isErasing) Paint(0);
        vtkInteractorStyleImage::OnMouseMove();
    }

    void Paint(int flagValue) {
        int* pos = this->Interactor->GetEventPosition();
        vtkRenderer* ren = this->Interactor->FindPokedRenderer(pos[0], pos[1]);
        if (!ren) return;

        ren->SetDisplayPoint(pos[0], pos[1], 0);
        ren->DisplayToWorld();
        double* worldPos = ren->GetWorldPoint();

        double x = worldPos[0];
        double y = worldPos[1];
        if (worldPos[3] != 0.0) {
            x /= worldPos[3];
            y /= worldPos[3];
        }

        int centerX = static_cast<int>(std::round(x));
        int centerY = static_cast<int>(std::round(y));

        int offset = brushSize / 2;

        for (int dy = -offset; dy <= offset; ++dy) {
            for (int dx = -offset; dx <= offset; ++dx) {
                int lx = std::clamp(centerX + dx, 1, data->nx_sim - 2);
                int ly = std::clamp(centerY + dy, 1, data->ny_sim - 2);

                if (lx >= 1 && lx < data->nx_sim - 1 && ly >= 1 && ly < data->ny_sim - 1) {
                    if (dx * dx + dy * dy <= offset * offset) {
                        data->lattice->get_flag(lx, ly) = flagValue;
                        int idx = ly * data->nx_sim + lx;
                        data->flagArray->SetValue(idx, flagValue);

                        for (int d = 0; d < 9; ++d) {
                            Lattice2D::Direction dir = static_cast<Lattice2D::Direction>(d);
                            double w = data->lattice->get_lattice_weight(dir);
                            data->lattice->get_density_dir(lx, ly, dir) = w;
                            data->lattice->get_density_next_dir(lx, ly, dir) = w;
                        }
                    }
                }
            }
        }

        data->flagArray->Modified();
        this->Interactor->GetRenderWindow()->Render();
    }
};
vtkStandardNewMacro(MousePaintStyle);