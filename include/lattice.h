#pragma once
#include <iostream>
#include <defines.h>
#include <vector>
#include <stdexcept>
#include <tuple>

class Lattice2D {
public:
	enum Direction {
		C = 0,
		E,
		W,
		N,
		S,
		NE,
		SW,
		NW,
		SE,
		VALS_PER_CELL
	};

	constexpr static vec2<double> get_unit_vector(Direction dir) {
		switch (dir) {
		case C:
			return { 0., 0. };
		case E:
			return { 1., 0. };
		case W:
			return { -1., 0. };
		case N:
			return { 0., 1. };
		case S:
			return { 0., -1. };
		case NE:
			return { 1., 1. };
		case SW:
			return { -1., -1. };
		case NW:
			return { -1., 1. };
		case SE:
			return { 1., -1. };
			
		default:
			throw std::runtime_error("Invalid direction.");
		}
	}

	constexpr static Direction get_opposite_unit_direction(Direction dir) {
		switch (dir) {
		case C:
			return C;
		case N:
			return S;
		case E:
			return W;
		case W:
			return E;
		case S:
			return N;
		case NE:
			return SW;
		case SW:
			return NE;
		case NW:
			return SE;
		case SE:
			return NW;			
		default:
			throw std::runtime_error("Invalid directrion.");
		}
	}

	constexpr static double get_lattice_weight(Direction dir) {
		switch (dir) {
		case C:
			return 4./9.;
		case N:
		case E:
		case W:
		case S:
			return 1./9.;
		case NE:
		case SW:
		case NW:
		case SE:
			return 1./36.;
		default:
			throw std::runtime_error("Invalid directrion.");
		}
	}

	inline usize& get_flag(usize x, usize y) { 
		return m_flags[y * m_Nx + x]; }
	inline const usize& get_flag(usize x, usize y) const { return m_flags[y * m_Nx + x]; }

	inline usize& get_flag(usize i) {
		return m_flags[i];
	}
	inline const usize& get_flag(usize i) const { return m_flags[i]; }

	inline const usize& get_Nx() const { return m_Nx; }
	inline const usize& get_Ny() const { return m_Ny; }
	inline const usize& get_Bx() const { return m_Bx; }
	inline const usize& get_By() const { return m_By; }
	inline const usize& get_Diameter() const { return m_Diameter; }
	
	inline const double& get_density_dir(usize x, usize y, Direction dir) const { return m_densities[(y * m_Nx + x) * Direction::VALS_PER_CELL + static_cast<usize>(dir)]; }
	inline double& get_density_dir(usize x, usize y, Direction dir) { return m_densities[(y * m_Nx + x) * Direction::VALS_PER_CELL + static_cast<usize>(dir)]; }
	inline const double& get_density_next_dir(usize x, usize y, Direction dir) const { return m_densities_next[(y * m_Nx + x) * Direction::VALS_PER_CELL + static_cast<usize>(dir)]; }
	inline double& get_density_next_dir(usize x, usize y, Direction dir) { return m_densities_next[(y * m_Nx + x) * Direction::VALS_PER_CELL + static_cast<usize>(dir)]; }

	double get_density_cell(usize x, usize y) const;
	vec2<double> get_velocity_cell(usize x, usize y) const;

	std::tuple<double, vec2<double>> get_cell_data(usize x, usize y);

	void setup_wind_tunnel(double bx, double by, double diameter);

	void collide_step(double tau);
	void handle_boundaries(vec2<double> u_in, double rho_out = 1.);
	void stream_step();

	vec2<double> calculate_forces();

	void setup_boundary();
	void setup_ball(double bx, double by, double diameter);
	void setup_airfoil(double bx, double by, double chord, double angle_deg);
	void clear_geometry() {
		std::fill(m_flags.begin(), m_flags.end(), 0);
	}

	void print() {
		for (int i = 0; i < m_Ny; i++) {
			for (int j = 0; j < m_Nx; j++) {
				std::cout << get_flag(j, i) << " ";
			}
			std::cout << std::endl;
		}
	}
	Lattice2D(usize nx, usize ny);
	~Lattice2D() {};
private:
	usize m_Nx, m_Ny;
	double m_Bx, m_By, m_Diameter;
	std::vector<double> m_densities, m_densities_next;
	std::vector<usize> m_flags;
};