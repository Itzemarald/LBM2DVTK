#include <lattice.h>
#include <numbers>

Lattice2D::Lattice2D(usize nx, usize ny) : m_Nx(nx + 2), m_Ny(ny + 2), m_Bx(0.), m_By(0.), m_Diameter(0.), m_densities(m_Nx * m_Ny * VALS_PER_CELL, 0.), m_densities_next(m_Nx* m_Ny* VALS_PER_CELL, 0.), m_flags(m_Nx * m_Ny, 0) {}

void Lattice2D::setup_ball(double bx, double by, double diameter) {
	double r = diameter / 2.0;
	for (usize i = 0; i < m_Ny; i++) {
		for (usize j = 0; j < m_Nx; j++) {
			double dx = (double)j - bx;
			double dy = (double)i - by;
			if (dx * dx + dy * dy < r * r) {
				get_flag(j, i) = 4;
				for (usize d = 0; d < VALS_PER_CELL; ++d) {
					Direction dir = static_cast<Direction>(d);
					get_density_dir(j, i, dir) = get_lattice_weight(dir);
				}
			}
		}
	}

	setup_boundary();
}

void Lattice2D::setup_airfoil(double bx, double by, double chord, double angle_deg) {
	double t = 0.12;
	double rad = angle_deg * 3.14159265 / 180.0;
	double cosA = cos(rad);
	double sinA = sin(rad);

	for (usize i = 0; i < m_Ny; i++) {
		for (usize j = 0; j < m_Nx; j++) {
			double dx = (double)j - bx;
			double dy = (double)i - by;
			double x_rot = dx * cosA + dy * sinA;
			double y_rot = -dx * sinA + dy * cosA;

			if (x_rot >= 0 && x_rot <= chord) {
				double x_norm = x_rot / chord;
				double half_thickness = 5.0 * t * chord * (
					0.2969 * sqrt(x_norm) - 0.1260 * x_norm -
					0.3516 * pow(x_norm, 2) + 0.2843 * pow(x_norm, 3) -
					0.1015 * pow(x_norm, 4));
				if (abs(y_rot) <= half_thickness) {
					get_flag(j, i) = 4;
					for (usize d = 0; d < VALS_PER_CELL; ++d) {
						Direction dir = static_cast<Direction>(d);
						get_density_dir(j, i, dir) = get_lattice_weight(dir);
					}
				}
			}
		}
	}

	setup_boundary();
}
 
void Lattice2D::setup_boundary() {
	for (usize i = 0; i < m_Ny; i++) {
		get_flag(0, i) = 2;				// Velocity boundary cells inflow from left
		get_flag(m_Nx - 1, i) = 3;			// Density boundary cells outflow from right
	}

	for (usize i = 0; i < m_Nx; i++) {
		get_flag(i, 0) = 1;				// No slip wall top
		get_flag(i, m_Ny - 1) = 1;			// No slip wall bottom
	}
}

void Lattice2D::setup_wind_tunnel(double bx, double by, double diameter) {
	m_Bx = bx;
	m_By = by;
	m_Diameter = diameter;
	for (usize y = 0; y < m_Ny; ++y) {
		for (usize x = 0; x < m_Nx; ++x) {
			for (usize i = 0; i < VALS_PER_CELL; ++i) {
				Direction dir = static_cast<Direction>(i);
				get_density_dir(x, y, dir) = get_lattice_weight(dir);
				get_density_next_dir(x, y, dir) = get_lattice_weight(dir);
			}
		}
	}

	double chord = 160.0;
	double t = 0.12;
	double angle = -10.;
	double rad = angle * std::numbers::pi / 180.0;
	setup_airfoil(bx, by, chord, angle);
	setup_boundary();
}

double Lattice2D::get_density_cell(usize x, usize y) const {
	if (get_flag(x, y) != 0) {
		return 0.0;
	}
	double rho = 0.;
	for (int i = 0; i < VALS_PER_CELL; ++i) {
		rho += get_density_dir(x, y, static_cast<Direction>(i));
	}
	return rho;
}

vec2<double> Lattice2D::get_velocity_cell(usize x, usize y) const {
	vec2<double> u;
	for (int i = 0; i < VALS_PER_CELL; ++i) {
		u += get_unit_vector(static_cast<Direction>(i)) * get_density_dir(x, y, static_cast<Direction>(i));
	}
	return u;
}

std::tuple<double, vec2<double>> Lattice2D::get_cell_data(usize x, usize y) {
	double rho = 0.;
	vec2<double> j;
	for (int i = 0; i < VALS_PER_CELL; ++i) {
		Direction dir = static_cast<Direction>(i);
		double f = get_density_dir(x, y, dir);
		rho += f;
		j += get_unit_vector(dir) * f;
	}

	vec2<double> u = (rho > 1e-10) ? j * (1. / rho) : vec2<double>{ 0, 0 };
	return { rho, u };
}

void Lattice2D::collide_step(double tau) {
	const double inv_tau = 1. / tau;
	const int ny = static_cast<int>(get_Ny());
	const int nx = static_cast<int>(get_Nx());
	#pragma omp parallel for collapse(2)
	for (int y = 1; y < ny - 1; y++) {
		for (int x = 1; x < nx - 1; x++) {
			if (get_flag(x, y) != 0) continue;

			auto [rho, u] = get_cell_data(x, y);
			double u_square = u.dot(u);
			
			for (usize i = 0; i < VALS_PER_CELL; ++i) {
				Direction dir = static_cast<Direction>(i);
				vec2<double> dir_vec = get_unit_vector(dir);

				double cu = dir_vec.dot(u);
				double cu_square = cu * cu;

				//double feq = get_lattice_weight(dir) * (rho + 3. * cu + 4.5 * cu_square - 1.5 * u_square); // optimize for small speed
				double feq = get_lattice_weight(dir) * rho * (1.0 + 3. * cu + 4.5 * cu_square - 1.5 * u_square);
				
				double f_old = get_density_dir(x, y, dir);
				get_density_dir(x, y, dir) = f_old - inv_tau * (f_old - feq);
			}
		}
	}
}

void Lattice2D::handle_boundaries(vec2<double> u_in, double rho_out) {
	const int ny = static_cast<int>(get_Ny());
	const int nx = static_cast<int>(get_Nx());
	#pragma omp parallel for collapse(2)
	for (int y = 0; y < ny; ++y) {
		for (int x = 0; x < nx; ++x) {
			usize flag = get_flag(x, y);
			if (flag == 0) continue;

			for (usize i = 0; i < VALS_PER_CELL; ++i) {
				Direction dir = static_cast<Direction>(i);
				vec2<double> c_q = get_unit_vector(dir);

				int x_fluid = static_cast<int>(x) - static_cast<int>(c_q.x);
				int y_fluid = static_cast<int>(y) - static_cast<int>(c_q.y);

				if (x_fluid < 0 || x_fluid >= static_cast<int>(m_Nx) ||
					y_fluid < 0 || y_fluid >= static_cast<int>(m_Ny)) {
					continue;
				}

				if (get_flag(x_fluid, y_fluid) != 0) {
					continue;
				}

				Direction dir_opp = get_opposite_unit_direction(dir);

				double f_q = get_density_dir(x_fluid, y_fluid, dir);
				double w_q = get_lattice_weight(dir);

				switch (flag) {
				case 1:
				case 4:
					get_density_dir(x, y, dir_opp) = f_q;
					break;

				case 2:
					get_density_dir(x, y, dir_opp) = f_q - 6.0 * w_q * c_q.dot(u_in);
					break;

				case 3:
					auto [rho, u] = get_cell_data(x_fluid, y_fluid);

					double c_u = c_q.dot(u);
					double u_square = u.dot(u);

					get_density_dir(x, y, dir_opp) = -f_q + 2. * w_q * (rho_out + 4.5 * c_u * c_u - 1.5 * u_square);
					break;
				}
			}
		}
	}
}

void Lattice2D::stream_step() {
	const int ny = static_cast<int>(get_Ny());
	const int nx = static_cast<int>(get_Nx());
	#pragma omp parallel for collapse(2)
	for (int y = 1; y < ny - 1; ++y) {
		for (int x = 1; x < nx - 1; ++x) {
			if (get_flag(x, y) != 0)
				continue;
			for (usize i = 0; i < VALS_PER_CELL; ++i) {
				Direction dir = static_cast<Direction>(i);
				
				vec2<double> c_q = get_unit_vector(dir);
				int cx = static_cast<int>(c_q.x);
				int cy = static_cast<int>(c_q.y);

				usize source_x = static_cast<usize>(static_cast<int>(x) - cx);
				usize source_y = static_cast<usize>(static_cast<int>(y) - cy);

				get_density_next_dir(x, y, dir) = get_density_dir(source_x, source_y, dir);
			}
		}
	}
	m_densities.swap(m_densities_next);
}

vec2<double> Lattice2D::calculate_forces() {
	vec2<double> force{ 0.0,0.0 };

	for (int y = 1; y < (int)m_Ny - 1; ++y) {
		for (int x = 1; x < (int)m_Nx - 1; ++x) {

			if (get_flag(x, y) != 4)
				continue;

			for (int i = 1; i < 9; ++i) {
				Direction q = static_cast<Direction>(i);
				vec2<double> c = get_unit_vector(q);

				int fx = x - (int)c.x;
				int fy = y - (int)c.y;

				if (get_flag(fx, fy) == 0) {
					double fin =
						get_density_dir(fx, fy, q);

					double fout =
						get_density_dir(
							x, y,
							get_opposite_unit_direction(q)
						);

					force += (fin + fout) * c;
				}
			}
		}
	}
	return force;
}