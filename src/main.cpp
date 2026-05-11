#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);

#include <iostream>
#include <thread>
#include <chrono>
#include <vtkNew.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkNamedColors.h>
#include <vtkImageData.h>
#include <vtkUnsignedIntArray.h>
#include <vtkLookupTable.h>
#include <vtkDataSetMapper.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkInteractorStyleImage.h>
#include <vtkCallbackCommand.h>
#include <vtkDoubleArray.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkColorTransferFunction.h>
#include <vtkStreamTracer.h>
#include <vtkLineSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkProperty2D.h>
#include <vtkArrowSource.h>
#include <vtkCamera.h>

#include <callbackdata.h>
#include <vtkslider.h>
#include <lattice.h>


void KeyPressCallbackFunction(vtkObject* caller, long unsigned int eventId,
	void* clientData, void* callData) {

	vtkRenderWindowInteractor* interactor = static_cast<vtkRenderWindowInteractor*>(caller);
	CallbackData* data = static_cast<CallbackData*>(clientData);

	std::string key = interactor->GetKeySym();

	if (key == "v" || key == "V") {
		data->viewState = (data->viewState + 1) % 3;
		if (data->viewState == 0) {
			data->pressureMapper->SetScalarModeToUseCellFieldData();
			data->pressureMapper->SetLookupTable(data->flagLut);
			data->pressureMapper->SelectColorArray("Flags");
			data->pressureMapper->SetScalarRange(0, 3);
		}
		else if (data->viewState == 1) {
			data->pressureMapper->SetScalarModeToUseCellFieldData();
			data->pressureMapper->SetLookupTable(data->densityLut);
			data->pressureMapper->SelectColorArray("Density");
			data->pressureMapper->SetScalarRange(0.99, 1.01);
		}
		else if (data->viewState == 2) {
			data->pressureMapper->SetScalarModeToUsePointFieldData();
			data->pressureMapper->SetLookupTable(data->velocityLut);
			data->pressureMapper->SelectColorArray("VelocityMag");
			data->pressureMapper->SetScalarRange(0.0, 0.03);
		}
		interactor->GetRenderWindow()->Render();
	}

	if (key == "p" || key == "P") {
		*(data->isPaused) = !(*(data->isPaused));
		if (*(data->isPaused)) std::cout << "Simulation PAUSIERT" << std::endl;
		else std::cout << "Simulation fortgesetzt" << std::endl;
	}

	if (key == "s" || key == "S") {
		int visible = data->streamActor->GetVisibility();
		data->streamActor->SetVisibility(!visible);
		interactor->GetRenderWindow()->Render();
		std::cout << "Streamlines: " << (visible ? "OFF" : "ON") << std::endl;
	}

	if (key == "g" || key == "G") {
		data->geometryState = (data->geometryState + 1) % 2;

		UpdateGeometry(data);

		if (data->geometryState == 0) {
			std::cout << "Geometrie gewechselt: BALL" << std::endl;
			data->chordWidget->EnabledOff();
			data->angleWidget->EnabledOff();
		}
		else {
			std::cout << "Geometrie gewechselt: FLÜGEL" << std::endl;
			data->chordWidget->EnabledOn();
			data->angleWidget->EnabledOn();
		}
	}

	if (key == "m" || key == "M") {
		data->splitMode = !data->splitMode;
		if (data->splitMode) {
			data->renLeft->SetViewport(0.0, 0.0, 0.5, 1.0);
			data->renRight->SetViewport(0.5, 0.0, 1.0, 1.0);
			data->renRight->SetActiveCamera(data->renLeft->GetActiveCamera());

			data->pressureMapper->SelectColorArray("Density");
			data->pressureMapper->SetLookupTable(data->densityLut);
			data->velocityActor->SetVisibility(true);
		}
		else {
			data->renLeft->SetViewport(0.0, 0.0, 1.0, 1.0);
			data->renRight->SetViewport(0.0, 0.0, 0.0, 0.0);

			if (data->viewState == 2) data->pressureMapper->SelectColorArray("VelocityMag");
		}
		interactor->GetRenderWindow()->Render();
	}

	if (key == "a" || key == "A") {
		// Sichtbarkeit umkehren
		int isVisible = data->arrowActor->GetVisibility();
		data->arrowActor->SetVisibility(!isVisible);

		std::cout << "Kraft-Pfeil: " << (isVisible ? "AUS" : "AN") << std::endl;
		data->renderWindow->Render();
	}
}

void UpdateGeometry(CallbackData* data) {
	data->lattice->clear_geometry();

	if (data->geometryState == 0) {
		data->lattice->setup_ball(data->bx, data->by, data->diameter);
	}
	else {
		data->lattice->setup_airfoil(data->bx, data->by, data->chord, data->angle);
	}

	for (int i = 0; i < data->nx_sim * data->ny_sim; i++) {
		data->flagArray->SetValue(i, static_cast<int>(data->lattice->get_flag(i)));
	}
	data->flagArray->Modified();
	data->renderWindow->Render();
}

int main(int argc, char* argv[]) {
	vtkNew<vtkNamedColors> colors;
	vtkNew<vtkRenderer> renderer;
	renderer->SetBackground(colors->GetColor3d("MidnightBlue").GetData());

	vtkNew<vtkTextActor> txtActor;
	txtActor->SetInput("Iteration: 0");
	txtActor->GetTextProperty()->SetFontSize(18);
	txtActor->GetTextProperty()->SetColor(1.0, 1.0, 1.0);
	txtActor->GetTextProperty()->BoldOn();
	txtActor->SetPosition(10, 10); 
	renderer->AddViewProp(txtActor);

	vtkNew<vtkArrowSource> arrowSource;
	vtkNew<vtkPolyDataMapper> arrowMapper;
	arrowMapper->SetInputConnection(arrowSource->GetOutputPort());
	vtkNew<vtkActor> arrowActor;
	arrowActor->SetMapper(arrowMapper);
	arrowActor->GetProperty()->SetColor(1.0, 1.0, 0.0);
	renderer->AddActor(arrowActor);

	vtkNew<vtkRenderWindow> render_window;
	render_window->AddRenderer(renderer);
	render_window->SetWindowName("WindTunnel 2D");
	render_window->SetSize(800, 600);

	vtkNew<vtkRenderWindowInteractor> interactor;
	interactor->SetRenderWindow(render_window);

	Lattice2D lattice = Lattice2D::Lattice2D(400, 250);
	lattice.setup_wind_tunnel(50., 250. / 2., 30.);
	double u_in_mag = 0.02;
	double Re = 20.0;
	double Ny = static_cast<double>(lattice.get_Ny() - 2);

	double nu = (u_in_mag * Ny) / Re;
	double tau = 3. * nu + 0.5;
	vec2<double> u_in{ u_in_mag, 0. };

	const int nx_sim = static_cast<int>(lattice.get_Nx());
	const int ny_sim = static_cast<int>(lattice.get_Ny());

	//lattice.print();

	vtkNew<vtkImageData> image_data;
	image_data->SetDimensions(nx_sim + 1, ny_sim + 1, 1);
	image_data->SetSpacing(1., 1., 1.);
	image_data->SetOrigin(0., 0., 0.);

	vtkNew<vtkUnsignedIntArray> flag_array;
	flag_array->SetName("Flags");
	flag_array->SetNumberOfComponents(1);
	flag_array->SetNumberOfTuples(nx_sim * ny_sim);

	for (usize y = 0; y < ny_sim; y++) {
		for (usize x = 0; x < nx_sim; x++) {
			usize idx = y * nx_sim + x;
			flag_array->SetValue(idx, static_cast<int>(lattice.get_flag(x, y)));
		}
	}

	image_data->GetCellData()->SetScalars(flag_array);

	vtkNew<vtkDoubleArray> density_array;
	density_array->SetName("Density");
	density_array->SetNumberOfComponents(1);
	density_array->SetNumberOfTuples(nx_sim * ny_sim);
	image_data->GetCellData()->AddArray(density_array);

	for (usize y = 0; y < ny_sim; ++y) {
		for (usize x = 0; x < nx_sim; ++x) {
			auto [rho, u] = lattice.get_cell_data(x, y);
			density_array->SetValue(y * nx_sim + x, rho);
		}
	}

	vtkNew<vtkDoubleArray> velocity_mag_array;
	velocity_mag_array->SetName("VelocityMag");
	velocity_mag_array->SetNumberOfComponents(1);
	velocity_mag_array->SetNumberOfTuples((nx_sim + 1) * (ny_sim + 1));
	image_data->GetPointData()->AddArray(velocity_mag_array);

	vtkNew<vtkDoubleArray> velocity_vector_array;
	velocity_vector_array->SetName("VelocityVectors");
	velocity_vector_array->SetNumberOfComponents(3);
	velocity_vector_array->SetNumberOfTuples((nx_sim + 1) * (ny_sim + 1));
	image_data->GetPointData()->AddArray(velocity_vector_array);
	image_data->GetPointData()->SetActiveVectors("VelocityVectors");

	vtkNew<vtkColorTransferFunction> colorFn;
	colorFn->AddRGBPoint(0.0, 0.23, 0.29, 0.75);
	colorFn->AddRGBPoint(0.015, 0.86, 0.86, 0.86);
	colorFn->AddRGBPoint(0.03, 0.70, 0.01, 0.15);

	vtkNew<vtkLookupTable> velocityLut;
	velocityLut->SetNumberOfTableValues(256);
	velocityLut->Build();
	for (int i = 0; i < 256; ++i) {
		double rgb[3];
		colorFn->GetColor(static_cast<double>(i) / 255.0 * 0.03, rgb);
		velocityLut->SetTableValue(i, rgb[0], rgb[1], rgb[2], 1.0);
	}

	vtkNew<vtkLookupTable> lut;
	lut->SetNumberOfTableValues(4);
	lut->Build();
	lut->SetTableValue(0, 0.1, 0.2, 0.4, 1.0);
	lut->SetTableValue(1, 0.8, 0.8, 0.8, 1.0);
	lut->SetTableValue(2, 0.2, 0.8, 0.2, 1.0);
	lut->SetTableValue(3, 0.8, 0.2, 0.2, 1.0);
	lut->SetRange(0, 3);

	vtkNew<vtkLookupTable> densityLut;
	densityLut->SetNumberOfTableValues(256);
	densityLut->SetHueRange(0.666, 0.0);
	densityLut->Build();

	vtkNew<vtkLineSource> seeds;
	seeds->SetPoint1(5.0, 5.0, 0.0);
	seeds->SetPoint2(5.0, ny_sim - 5.0, 0.0);
	seeds->SetResolution(30);

	vtkNew<vtkStreamTracer> tracer;
	tracer->SetInputData(image_data);
	tracer->SetSourceConnection(seeds->GetOutputPort());
	tracer->SetMaximumPropagation(nx_sim * 1.5);
	tracer->SetIntegrationDirectionToBoth();

	vtkNew<vtkPolyDataMapper> streamMapper;
	streamMapper->SetInputConnection(tracer->GetOutputPort());
	streamMapper->SetScalarRange(0.0, 0.05);

	vtkNew<vtkActor> streamActor;
	streamActor->SetMapper(streamMapper);
	streamActor->SetPosition(0, 0, 0.1);
	streamActor->GetProperty()->SetLineWidth(2.0);
	renderer->AddActor(streamActor);

	vtkNew<vtkDataSetMapper> pressureMapper;
	pressureMapper->SetInputData(image_data);
	pressureMapper->SetScalarModeToUseCellFieldData();
	pressureMapper->SelectColorArray("Density");
	pressureMapper->SetLookupTable(densityLut);
	pressureMapper->SetScalarRange(0.99, 1.01);

	vtkNew<vtkActor> pressureActor;
	pressureActor->SetMapper(pressureMapper);

	vtkNew<vtkDataSetMapper> velocityMapper;
	velocityMapper->SetInputData(image_data);
	velocityMapper->SetScalarModeToUsePointFieldData();
	velocityMapper->SelectColorArray("VelocityMag");
	velocityMapper->SetLookupTable(velocityLut);
	velocityMapper->SetScalarRange(0.0, 0.03);

	vtkNew<vtkActor> velocityActor;
	velocityActor->SetMapper(velocityMapper);

	vtkNew<vtkRenderer> renLeft;
	vtkNew<vtkRenderer> renRight;

	vtkNew<vtkRenderer> renOverlay;
	renOverlay->SetViewport(0, 0, 1, 1);
	renOverlay->SetLayer(1);
	renOverlay->InteractiveOff();

	renLeft->SetLayer(0);
	renRight->SetLayer(0);

	render_window->SetNumberOfLayers(2);
	render_window->AddRenderer(renOverlay);

	renOverlay->AddViewProp(txtActor);

	render_window->AddRenderer(renLeft);
	render_window->AddRenderer(renRight);

	renLeft->GetActiveCamera()->ParallelProjectionOn();
	renRight->GetActiveCamera()->ParallelProjectionOn();
	renLeft->ResetCamera(0, nx_sim, 0, ny_sim, 0, 0);
	renRight->ResetCamera(0, nx_sim, 0, ny_sim, 0, 0);

	renLeft->SetViewport(0.0, 0.0, 1.0, 1.0);
	renRight->SetViewport(0.0, 0.0, 0.0, 0.0);

	renLeft->AddActor(pressureActor);
	renRight->AddActor(velocityActor);

	renLeft->SetBackground(colors->GetColor3d("MidnightBlue").GetData());
	renRight->SetBackground(colors->GetColor3d("MidnightBlue").GetData());

	renLeft->AddActor(pressureActor);
	renLeft->AddActor(streamActor);
	renLeft->AddViewProp(txtActor);
	renLeft->AddActor(arrowActor);

	renRight->AddActor(velocityActor);

	render_window->Render();
	vtkNew<vtkInteractorStyleImage> style;
	interactor->SetInteractorStyle(style);
	


	usize timesteps = 500000;
	usize vtk_steps = 1;

	vtkNew<vtkSliderRepresentation2D> sliderRep;
	sliderRep->SetMinimumValue(1.0); 
	sliderRep->SetMaximumValue(1000.0);
	sliderRep->SetValue(static_cast<double>(vtk_steps));
	sliderRep->SetTitleText("Visualisierungs-Intervall (Steps)");


	sliderRep->GetPoint1Coordinate()->SetCoordinateSystemToNormalizedDisplay();
	sliderRep->GetPoint1Coordinate()->SetValue(0.7, 0.08);
	sliderRep->GetPoint2Coordinate()->SetCoordinateSystemToNormalizedDisplay();
	sliderRep->GetPoint2Coordinate()->SetValue(0.95, 0.08);

	sliderRep->GetTitleProperty()->SetColor(1, 1, 1);
	sliderRep->GetLabelProperty()->SetColor(1, 1, 1);
	sliderRep->GetSliderProperty()->SetColor(0.2, 0.8, 0.2);

	vtkNew<vtkSliderWidget> sliderWidget;
	sliderWidget->SetInteractor(interactor);
	sliderWidget->SetRepresentation(sliderRep);
	sliderWidget->SetDefaultRenderer(renOverlay);
	sliderWidget->SetAnimationModeToAnimate();

	vtkNew<vtkSliderCallbackSteps> sliderCb;
	sliderCb->vtk_steps = &vtk_steps;
	sliderWidget->AddObserver(vtkCommand::InteractionEvent, sliderCb);

	sliderWidget->EnabledOn();

	vtkNew<vtkSliderRepresentation2D> speedRep;
	speedRep->SetMinimumValue(0.001);
	speedRep->SetMaximumValue(0.1);
	speedRep->SetValue(u_in.x);
	speedRep->SetTitleText("Inlet Velocity (u_in)");

	speedRep->GetPoint1Coordinate()->SetCoordinateSystemToNormalizedDisplay();
	speedRep->GetPoint1Coordinate()->SetValue(0.05, 0.18);
	speedRep->GetPoint2Coordinate()->SetCoordinateSystemToNormalizedDisplay();
	speedRep->GetPoint2Coordinate()->SetValue(0.25, 0.18);

	speedRep->GetSliderProperty()->SetColor(0.8, 0.2, 0.2); 
	speedRep->GetTitleProperty()->SetColor(1, 1, 1);

	vtkNew<vtkSliderWidget> speedWidget;
	speedWidget->SetInteractor(interactor);
	speedWidget->SetRepresentation(speedRep);
	speedWidget->SetDefaultRenderer(renOverlay);

	vtkNew<vtkSliderCallbackSpeed> speedCb;
	speedCb->u_in = &u_in;
	speedWidget->AddObserver(vtkCommand::InteractionEvent, speedCb);

	speedWidget->EnabledOn();

	vtkNew<vtkSliderRepresentation2D> reRep;
	reRep->SetMinimumValue(10.0);
	reRep->SetMaximumValue(2000.0);
	reRep->SetValue(Re);
	reRep->SetTitleText("Reynolds Number (Re)");

	reRep->GetPoint1Coordinate()->SetCoordinateSystemToNormalizedDisplay();
	reRep->GetPoint1Coordinate()->SetValue(0.05, 0.92);
	reRep->GetPoint2Coordinate()->SetCoordinateSystemToNormalizedDisplay();
	reRep->GetPoint2Coordinate()->SetValue(0.25, 0.92);

	reRep->GetSliderProperty()->SetColor(0.2, 0.2, 0.8);

	vtkNew<vtkSliderWidget> reWidget;
	reWidget->SetInteractor(interactor);
	reWidget->SetRepresentation(reRep);
	reWidget->SetDefaultRenderer(renOverlay);

	vtkNew<vtkSliderCallbackRe> reCb;
	reCb->Re = &Re;
	reCb->tau = &tau;
	reCb->u_in = &u_in;
	reCb->Ny = Ny;
	reWidget->AddObserver(vtkCommand::InteractionEvent, reCb);

	reWidget->EnabledOn();

	CallbackData cbData;
	cbData.pressureMapper = pressureMapper;
	cbData.velocityMapper = velocityMapper;
	cbData.flagLut = lut;
	cbData.densityLut = densityLut;
	cbData.velocityLut = velocityLut;
	cbData.streamActor = streamActor;
	cbData.lattice = &lattice;
	cbData.flagArray = flag_array;
	cbData.renderWindow = render_window;
	cbData.nx_sim = nx_sim;
	cbData.ny_sim = ny_sim;
	cbData.bx = lattice.get_Bx();
	cbData.by = lattice.get_By();
	cbData.diameter = 30.0;
	cbData.renLeft = renLeft;
	cbData.renRight = renRight;
	cbData.pressureActor = pressureActor;
	cbData.velocityActor = velocityActor;
	cbData.arrowActor = arrowActor;

	vtkNew<vtkCallbackCommand> keypressCallback;
	keypressCallback->SetCallback(KeyPressCallbackFunction);
	keypressCallback->SetClientData(&cbData);

	interactor->AddObserver(vtkCommand::KeyPressEvent, keypressCallback);

	vtkNew<vtkSliderRepresentation2D> chordRep;
	chordRep->SetMinimumValue(50.0);
	chordRep->SetMaximumValue(300.0);
	chordRep->SetValue(cbData.chord);
	chordRep->SetTitleText("Wing Size (Chord)");
	chordRep->GetPoint1Coordinate()->SetCoordinateSystemToNormalizedDisplay();
	chordRep->GetPoint2Coordinate()->SetCoordinateSystemToNormalizedDisplay();
	chordRep->GetPoint1Coordinate()->SetValue(0.7, 0.92);
	chordRep->GetPoint2Coordinate()->SetValue(0.95, 0.92);

	vtkNew<vtkSliderWidget> chordWidget;
	chordWidget->SetInteractor(interactor);
	chordWidget->SetRepresentation(chordRep);
	chordWidget->SetDefaultRenderer(renOverlay);

	vtkNew<vtkSliderCallbackChord> chordCb;
	chordCb->data = &cbData;
	chordWidget->AddObserver(vtkCommand::InteractionEvent, chordCb);
	chordWidget->EnabledOn();

	vtkNew<vtkSliderRepresentation2D> angleRep;
	angleRep->SetMinimumValue(-45.0);
	angleRep->SetMaximumValue(45.0);
	angleRep->SetValue(cbData.angle);
	angleRep->SetTitleText("Angle of Attack");
	angleRep->GetPoint1Coordinate()->SetCoordinateSystemToNormalizedDisplay();
	angleRep->GetPoint2Coordinate()->SetCoordinateSystemToNormalizedDisplay();
	angleRep->GetPoint1Coordinate()->SetValue(0.7, 0.72);
	angleRep->GetPoint2Coordinate()->SetValue(0.95, 0.72);

	vtkNew<vtkSliderWidget> angleWidget;
	angleWidget->SetInteractor(interactor);
	angleWidget->SetRepresentation(angleRep);
	angleWidget->SetDefaultRenderer(renOverlay);
	vtkNew<vtkSliderCallbackAngle> angleCb;
	angleCb->data = &cbData;
	angleWidget->AddObserver(vtkCommand::InteractionEvent, angleCb);
	angleWidget->EnabledOn();

	cbData.geometryState = 1;
	cbData.chordWidget = chordWidget;
	cbData.angleWidget = angleWidget;
	bool paused = true;
	cbData.isPaused = &paused;

	vtkNew<MousePaintStyle> paintStyle;
	paintStyle->data = &cbData;
	interactor->SetInteractorStyle(paintStyle);

	double oldRhoMin = 0.999;
	double oldRhoMax = 1.001;
	const double alpha = 0.1;

	for (usize t = 0; t < timesteps;) {
		interactor->ProcessEvents();
		
		if (interactor->GetDone() || !render_window) {
			break;
		}

		if (!paused) {
			lattice.collide_step(tau);
			lattice.handle_boundaries(u_in, 1.0);
			lattice.stream_step();

			if (t % vtk_steps == 0 && t > 1) {
				double rho_min = 1e9;
				double rho_max = -1e9;

				const int nx = nx_sim;
				const int ny = ny_sim;

				vec2<double> force = lattice.calculate_forces();

				double rho_ref = 1.0;
				double u2 = u_in.x * u_in.x;
				double L = (cbData.geometryState == 0) ? cbData.diameter : cbData.chord;

				double Cd = (2.0 * force.x) / (rho_ref * u2 * L);
				double Cl = (2.0 * force.y) / (rho_ref * u2 * L);

				#pragma omp parallel for collapse(2)
				for (int y = 0; y <= ny_sim; ++y) {
					for (int x = 0; x <= nx_sim; ++x) {
						usize vtk_pt_idx = (usize)y * (nx + 1) + (usize)x;


						int safe_x = std::min(x, nx - 1);
						int safe_y = std::min(y, ny - 1);

						auto [rho, u] = lattice.get_cell_data(safe_x, safe_y);
						
						if (lattice.get_flag(safe_x, safe_y) != 0) {
							u.x = 0.0;
							u.y = 0.0;
						}
						
						double v_vec[3] = { u.x, u.y, 0.0 };
						velocity_vector_array->SetTuple(vtk_pt_idx, v_vec);
						velocity_mag_array->SetValue(vtk_pt_idx, std::sqrt(u.x * u.x + u.y * u.y));

						if (x < nx && y < ny) {
							usize vtk_cell_idx = (usize)y * nx + (usize)x;
							density_array->SetValue(vtk_cell_idx, rho);

							rho_min = std::min(rho_min, rho);
							rho_max = std::max(rho_max, rho);
						}
					}
				}

				double delta = std::max(
					std::abs(rho_max - 1.0),
					std::abs(rho_min - 1.0)
				);

				double targetMin = 1.0 - delta;
				double targetMax = 1.0 + delta;

				oldRhoMin = (1.0 - alpha) * oldRhoMin + alpha * targetMin;
				oldRhoMax = (1.0 - alpha) * oldRhoMax + alpha * targetMax;

				if (cbData.viewState == 1 || cbData.splitMode) {
					pressureMapper->SetScalarRange(0.999, 1.001);
				}

				arrowActor->SetPosition(cbData.bx + (cbData.geometryState == 1 ? cbData.chord / 2 : 0), cbData.by, 0.5);

				double scaleFactor = 500.0;
				double angle_rad = atan2(force.y, force.x);
				double angle_deg = angle_rad * 180.0 / 3.14159265;
				arrowActor->SetOrientation(0, 0, angle_deg);
				
				double force_mag = sqrt(force.x * force.x + force.y * force.y);
				double s = force_mag * scaleFactor;
				arrowActor->SetScale(s, s * 0.5, 1.0);

				std::string msg = "Iteration: " + std::to_string(t) +
					" | Cd: " + std::to_string(Cd).substr(0, 5) +
					" | Cl: " + std::to_string(Cl).substr(0, 5) +
					" | rho range:" + std::to_string(oldRhoMin) + " - " + std::to_string(oldRhoMax);

				txtActor->SetInput(msg.c_str());

				if (!render_window || render_window->GetNeverRendered()) break;

				velocity_mag_array->Modified();
				density_array->Modified();
				velocity_vector_array->Modified();
				image_data->GetPointData()->Modified();
				render_window->Render();
			}
			t++;

		} else {
			render_window->Render();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	interactor->Start();

	return EXIT_SUCCESS;
}