#include <iostream>
#include <chrono>
#include <thread>
#include <random>
#include <array>
#include <fstream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

// Scale factor for the pixels
#define SCALE_FACTOR 8

class Chip_8 {
private:
	sf::RenderWindow &chip_8_window;
	bool focused{ true };

	std::uint16_t opcode{};
	std::array<std::uint8_t, 4096> memory{};
	std::array<std::uint8_t, 16> V{};
	std::uint16_t I{};
	std::uint16_t pc{};
	std::array<std::uint16_t, 16> stack{};
	std::uint8_t sp{};
	std::array<std::uint8_t, 16> key{};

	std::array<std::uint8_t, 64 * 32> gfx{};
	std::uint8_t delay_timer{};
	std::uint8_t sound_timer{};

	// random number generator
	std::mt19937 mersenne{ static_cast<std::mt19937::result_type>(std::time(nullptr)) };
	std::uniform_int_distribution<> distribution{ 0x0, 0xFF };

	std::array<std::uint8_t, 80> fontset{ {
		0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80, // F
	}};

	sf::SoundBuffer sound_buffer{};
	sf::Sound sound{};

	// Opcode functions
	void clear_screen();
	void call(std::uint16_t address);
	void add_vx_to_vy(std::uint8_t x, std::uint8_t y);
	void _return();
	void jump(std::uint16_t address);
	void skip_if_vx_eq_n(std::uint8_t x, std::uint8_t n);
    	void skip_if_vx_ne_n(std::uint8_t x, std::uint8_t n);
	void skip_if_vx_eq_vy(std::uint8_t x, std::uint8_t y);
	void set_vx_to_n(std::uint8_t x, std::uint8_t n);
	void add_n_to_vx(std::uint8_t x, std::uint8_t n);
	void set_vx_to_vy(std::uint8_t x, std::uint8_t y);
	void set_vx_to_vx_or_vy(std::uint8_t x, std::uint8_t y);
	void set_vx_to_vx_and_vy(std::uint8_t x, std::uint8_t y);
	void set_vx_to_vx_xor_vy(std::uint8_t x, std::uint8_t y);
	void sub_vy_from_vx(std::uint8_t x, std::uint8_t y);
	void least_vx_to_vf_right_shift_vx(std::uint8_t x);
	void sub_vx_from_vy(std::uint8_t x, std::uint8_t y);
	void most_vx_to_vf_left_shift_vx(std::uint8_t x);
	void skip_if_vx_ne_vy(std::uint8_t x, std::uint8_t y);
	void jump_to_address_plus_v0(std::uint16_t address);
	void set_vx_to_rand_and_n(std::uint8_t x, std::uint8_t n);
	void draw(std::uint8_t x, std::uint8_t y, std::uint8_t n);
	void skip_if_vx_pressed(std::uint8_t x);
    	void set_vx_to_delay(std::uint8_t x);
	void skip_if_vx_not_pressed(std::uint8_t x);
	void wait_vx_key_press(std::uint8_t x);
	void set_i_to_bcd_of_vx(std::uint8_t x);
	void set_delay_to_vx(std::uint8_t x);
	void set_sound_timer_to_vx(std::uint8_t x);
	void store_v0_to_vx_in_i(std::uint8_t x);
	void read_i_into_v0_to_vx(std::uint8_t x);
	void add_i_and_vx(std::uint8_t x);
	void set_i_to_sprite_vx(uint8_t x);
	void set_i_to_n(uint16_t n);

public:
	void load_rom(std::ifstream &inf);
	void emulate_cycle();
	void init_display();
	void focus();
	void unfocus();
	void set_keys();

	Chip_8(sf::RenderWindow &window);
};

void Chip_8::clear_screen()
{
	for (std::size_t i = 0; i < gfx.max_size(); ++i)
		gfx[i] = 0;
	chip_8_window.clear();
	chip_8_window.display();
}

void Chip_8::call(std::uint16_t address)
{
	try {
		stack[sp] = pc + 2;
	} catch (const std::out_of_range) {
		std::cerr << "*** Stack Overflow ***" << std::endl;
		std::abort();
	}
	++sp;
	pc = address;
}

void Chip_8::add_vx_to_vy(std::uint8_t x, std::uint8_t y)
{
	if (V[x] > (0xFF - V[y]))
		V[0xF] = 1; // carry
	else
		V[0xF] = 0;
	V[x] += V[y];
}

void Chip_8::_return()
{
	pc = stack[--sp];
}

void Chip_8::jump(std::uint16_t address)
{
	pc = address;
}

void Chip_8::skip_if_vx_eq_n(std::uint8_t x, std::uint8_t n)
{
	if (V[x] == n)
		pc += 2;
}

void Chip_8::skip_if_vx_ne_n(std::uint8_t x, std::uint8_t n)
{
	if (V[x] != n)
		pc += 2;
}

void Chip_8::skip_if_vx_eq_vy(std::uint8_t x, std::uint8_t y)
{
	if (V[x] == V[y])
		pc += 2;
}

void Chip_8::set_vx_to_n(std::uint8_t x, std::uint8_t n)
{
	V[x] = n;
}

void Chip_8::add_n_to_vx(std::uint8_t x, std::uint8_t n)
{
	V[x] += n;
}

void Chip_8::set_vx_to_vy(std::uint8_t x, std::uint8_t y)
{
	V[x] = V[y];
}

void Chip_8::set_vx_to_vx_or_vy(std::uint8_t x, std::uint8_t y)
{
	V[x] |= V[y];
}

void Chip_8::set_vx_to_vx_and_vy(std::uint8_t x, std::uint8_t y)
{
	V[x] &= V[y];
}

void Chip_8::set_vx_to_vx_xor_vy(std::uint8_t x, std::uint8_t y)
{
	V[x] ^= V[y];
}

void Chip_8::sub_vy_from_vx(std::uint8_t x, std::uint8_t y)
{
	if (V[x] > V[y])
		V[0xF] = 1;
	else
		V[0xF] = 0;
	V[x] -= V[y];
}

void Chip_8::sub_vx_from_vy(std::uint8_t x, std::uint8_t y)
{
	if (V[y] > V[x])
		V[0xF] = 1;
	else
		V[0xF] = 0;

	V[x] = V[y] - V[x];
}

void Chip_8::least_vx_to_vf_right_shift_vx(std::uint8_t x)
{
	V[0xF] = V[x] & 0x1;
	V[x] >>= 1;
}

void Chip_8::most_vx_to_vf_left_shift_vx(std::uint8_t x)
{
	V[0xF] = !!(V[x] & 0x80);
	V[x] <<= 1;
}

void Chip_8::skip_if_vx_ne_vy(std::uint8_t x, std::uint8_t y)
{
	if (V[x] != V[y])
		pc += 2;
}

void Chip_8::set_i_to_n(std::uint16_t n)
{
	I = n;
}

void Chip_8::jump_to_address_plus_v0(std::uint16_t address)
{
	pc = address + V[0];
}

void Chip_8::set_vx_to_rand_and_n(std::uint8_t x, std::uint8_t n)
{
	V[x] = distribution(mersenne) & n;
}

void Chip_8::draw(std::uint8_t x, std::uint8_t y, std::uint8_t n)
{
	std::uint8_t pixel{ 0 };
	V[0xF] = 0;

	for (std::size_t row{ 0 }; row < n; ++row) {
		std::uint8_t sprite_row = memory[I + row];
		for (std::size_t i{ 0 }; i < 8; ++i) {
			std::uint8_t sprite_pixel = (1 << (7 - i)) & sprite_row;
			std::uint8_t gfx_pixel = gfx[(V[x] + i) % 64 + ((V[y] + row) % 32) * 64];
			if (sprite_pixel && gfx_pixel)
				V[0xF] = 1;
			gfx[(V[x] + i) % 64 + ((V[y] + row) % 32) * 64] = sprite_pixel ^ gfx_pixel;
		}
	}

	chip_8_window.clear();

	for (std::size_t x{ 0 }, y{ 0 }, i{ 0 }; i < gfx.max_size(); ++i, ++x) {
		if (x != 0 && x % 64 == 0) {
			x = 0;
			++y;
		}
		if (gfx[i]) {
			sf::RectangleShape sprite(sf::Vector2f(1 * SCALE_FACTOR, 1 * SCALE_FACTOR));
			sprite.setPosition(x * SCALE_FACTOR, y * SCALE_FACTOR);
			chip_8_window.draw(sprite);
		}
	}
	chip_8_window.display();
}

void Chip_8::set_vx_to_delay(std::uint8_t x)
{
	V[x] = delay_timer;
}

void Chip_8::set_sound_timer_to_vx(std::uint8_t x)
{
	sound_timer = V[x];
}

void Chip_8::set_i_to_sprite_vx(std::uint8_t x)
{
	I = V[x] * 0x5;
}

void Chip_8::store_v0_to_vx_in_i(std::uint8_t x)
{
	for (std::uint8_t i{ 0 }; i <= x; ++i)
		memory[I + i] = V[i];
}

void Chip_8::read_i_into_v0_to_vx(std::uint8_t x)
{
	for (std::uint8_t i{ 0 }; i <= x; ++i)
		V[i] = memory[I + i];
}

void Chip_8::add_i_and_vx(std::uint8_t x)
{
	I = I + V[x];
}

void Chip_8::skip_if_vx_pressed(std::uint8_t x)
{
	if (key[V[x]])
		pc += 2;
}

void Chip_8::skip_if_vx_not_pressed(std::uint8_t x)
{
	if (!key[V[x]]) {
		pc += 2;
	}
}

void Chip_8::wait_vx_key_press(std::uint8_t x)
{
	while (chip_8_window.isOpen()) {
		sf::Event event;
		while (chip_8_window.pollEvent(event)) {
			if (event.type == sf::Event::Closed)
				chip_8_window.close();
			if (event.type == sf::Event::GainedFocus)
				focus();
			if (event.type == sf::Event::LostFocus)
				unfocus();
			if (!focused)
				continue;
			if (event.type == sf::Event::KeyPressed) {
				set_keys();
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::X))
					V[x] = 0x0;
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q))
					V[x] = 0x1;
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2))
					V[x] = 0x2;
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3))
					V[x] = 0x3;
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1))
					V[x] = 0x4;
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
					V[x] = 0x5;
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::E))
					V[x] = 0x6;
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
					V[x] = 0x7;
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
					V[x] = 0x8;
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
					V[x] = 0x9;
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z))
					V[x] = 0xA;
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::C))
					V[x] = 0xB;
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num4))
					V[x] = 0xC;
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::R))
					V[x] = 0xD;
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::F))
					V[x] = 0xE;
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::V))
					V[x] = 0xF;
			}
		}
	}
}

void Chip_8::set_i_to_bcd_of_vx(std::uint8_t x)
{
	memory[I] = (V[x] / 100) % 10;
	memory[I + 1] = (V[x] / 10) % 10;
	memory[I + 2] = (V[x] / 1) % 10;
}

void Chip_8::set_delay_to_vx(std::uint8_t x)
{
	delay_timer = V[x];
}

void Chip_8::focus()
{
	focused = true;
}

void Chip_8::unfocus()
{
	focused = false;
}

void Chip_8::emulate_cycle()
{
	if (pc >= memory.size())
		return;
	opcode = (memory[pc] << 8) | memory[pc + 1];

	switch (opcode & 0xF000) {
        case 0x0000:
		switch (opcode & 0x00FF) {
		case 0x00E0:
			clear_screen();
			pc += 2;
			break;
		case 0x00EE:
			_return();
			break;
		default:
			std::cerr << "Unkown opcode " << std::hex << opcode << std::endl;
			std::abort();
		}
		break;
	case 0x1000:
		jump(opcode & 0x0FFF);
		break;
	case 0x2000:
		call(opcode & 0x0FFF);
		break;
	case 0x3000:
		skip_if_vx_eq_n((opcode & 0x0F00) >> 8, opcode & 0x00FF);
		pc += 2;
		break;
	case 0x4000:
		skip_if_vx_ne_n((opcode & 0x0F00) >> 8, opcode & 0x00FF);
		pc += 2;
		break;
	case 0x5000:
		skip_if_vx_eq_vy((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
		pc += 2;
		break;
	case 0x6000:
		set_vx_to_n((opcode & 0x0F00) >> 8, opcode & 0x00FF);
		pc += 2;
		break;
	case 0x7000:
		add_n_to_vx((opcode & 0x0F00) >> 8, opcode & 0x00FF);
		pc += 2;
		break;
	case 0x8000:
		switch (opcode & 0x000F) {
		case 0x0000:
			set_vx_to_vy((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
			pc += 2;
			break;
		case 0x0001:
			set_vx_to_vx_or_vy((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
			pc += 2;
			break;
		case 0x0002:
			set_vx_to_vx_and_vy((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
			pc += 2;
			break;
		case 0x0003:
			set_vx_to_vx_xor_vy((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
			pc += 2;
			break;
		case 0x0004:
			add_vx_to_vy((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
			pc += 2;
			break;
		case 0x0005:
			sub_vy_from_vx((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
			pc += 2;
			break;
		case 0x0006:
			least_vx_to_vf_right_shift_vx((opcode & 0x0F00) >> 8);
			pc += 2;
			break;
		case 0x0007:
			sub_vx_from_vy((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
			pc += 2;
			break;
		case 0x000E:
			most_vx_to_vf_left_shift_vx((opcode & 0x0F00) >> 8);
			pc += 2;
			break;
		default:
			std::cerr << "Unkown opcode " << std::hex << opcode << std::endl;
			std::abort();
		}
		break;
	case 0x9000:
		skip_if_vx_ne_vy((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
		pc += 2;
		break;
	case 0xA000:
		set_i_to_n(opcode & 0x0FFF);
		pc += 2;
		break;
	case 0xB000:
		jump_to_address_plus_v0(opcode & 0x0FFF);
		break;
	case 0xC000:
		set_vx_to_rand_and_n((opcode & 0x0F00) >> 8, opcode & 0x00FF);
		pc += 2;
		break;
	case 0xD000:
		draw((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4, opcode & 0x000F);
		pc += 2;
		break;
	case 0xE000:
		switch (opcode & 0x00FF) {
		case 0x009E:
			skip_if_vx_pressed((opcode & 0x0F00) >> 8);
			pc += 2;
			break;
		case 0x00A1:
			skip_if_vx_not_pressed((opcode & 0x0F00) >> 8);
			pc += 2;
			break;
		default:
			std::cerr << "Unkown opcode " << std::hex << opcode << std::endl;
			std::abort();
		}
		break;
	case 0xF000:
		switch (opcode & 0x00FF) {
		case 0x0033:
			set_i_to_bcd_of_vx((opcode & 0x0F00) >> 8);
			pc += 2;
			break;
		case 0x0015:
			set_delay_to_vx((opcode & 0x0F00) >> 8);
			pc += 2;
			break;
		case 0x0055:
			store_v0_to_vx_in_i((opcode & 0x0F00) >> 8);
			pc += 2;
			break;
		case 0x0065:
			read_i_into_v0_to_vx((opcode & 0x0F00) >> 8);
			pc += 2;
			break;
		case 0x0007:
			set_vx_to_delay((opcode & 0x0F00) >> 8);
			pc += 2;
			break;
		case 0x0018:
			set_sound_timer_to_vx((opcode & 0x0F00) >> 8);
			pc += 2;
			break;
		case 0x0029:
			set_i_to_sprite_vx((opcode & 0x0F00) >> 8);
			pc += 2;
			break;
		case 0x000A:
			wait_vx_key_press((opcode & 0x0F00) >> 8);
			pc += 2;
			break;
		case 0x001E:
			add_i_and_vx((opcode & 0x0F00) >> 8);
			pc += 2;
			break;
		default:
			std::cerr << "Unkown opcode " << std::hex << opcode << std::endl;
			std::abort();
		}
		break;
	default:
		std::cerr << "Unkown opcode " << std::hex << opcode << std::endl;
		std::abort();
	}

	if (delay_timer > 0)
		--delay_timer;
	if (sound_timer > 0) {
		if (sound_timer == 1)
			sound.play();
		--sound_timer;
	}
}

/* 
 * CHIP-8		QWERTY
 * 1 2 3 C	->	1 2 3 4
 * 4 5 6 D	->	Q W E R
 * 7 8 9 E	->	A S D F
 * A 0 B F	->	Z X C V
 */
void Chip_8::set_keys()
{
	if (!focused)
		return;

	key[0x0] = sf::Keyboard::isKeyPressed(sf::Keyboard::X);
	key[0x1] = sf::Keyboard::isKeyPressed(sf::Keyboard::Q);
	key[0x2] = sf::Keyboard::isKeyPressed(sf::Keyboard::Num2);
	key[0x3] = sf::Keyboard::isKeyPressed(sf::Keyboard::Num3);
	key[0x4] = sf::Keyboard::isKeyPressed(sf::Keyboard::Num1);
	key[0x5] = sf::Keyboard::isKeyPressed(sf::Keyboard::W);
	key[0x6] = sf::Keyboard::isKeyPressed(sf::Keyboard::E);
	key[0x7] = sf::Keyboard::isKeyPressed(sf::Keyboard::A);
	key[0x8] = sf::Keyboard::isKeyPressed(sf::Keyboard::S);
	key[0x9] = sf::Keyboard::isKeyPressed(sf::Keyboard::D);
	key[0xA] = sf::Keyboard::isKeyPressed(sf::Keyboard::Z);
	key[0xB] = sf::Keyboard::isKeyPressed(sf::Keyboard::C);
	key[0xC] = sf::Keyboard::isKeyPressed(sf::Keyboard::Num4);
	key[0xD] = sf::Keyboard::isKeyPressed(sf::Keyboard::R);
	key[0xE] = sf::Keyboard::isKeyPressed(sf::Keyboard::F);
	key[0xF] = sf::Keyboard::isKeyPressed(sf::Keyboard::V);
}

void Chip_8::load_rom(std::ifstream &inf)
{
	const uint16_t rom_mem_pos{ 0x200 };
	std::vector<char> buffer;

	inf.seekg(0, inf.end);
	std::size_t len = inf.tellg();
	inf.seekg(0, inf.beg);

	if (len > 0) {
		buffer.resize(len);
		inf.read(&buffer[0], len);
	}

	try {
		unsigned i{ rom_mem_pos };
		for (char b: buffer) {
			memory[i] = b;
			++i;
		}
	} catch (const std::out_of_range) {
        	std::cerr << "*** ROM is too large ***" << std::endl;
		std::abort();
	}
}

Chip_8::Chip_8(sf::RenderWindow &window)
    : chip_8_window{ window }
{
	pc = 0x200;
	focused = true;

	// Load fontset
	std::uint8_t i{ 0 };
	for (auto font: fontset) {
		memory[i] = font;
		++i;
	}

	std::fill(key.begin(), key.end(), 0);

	// Initialize display
	chip_8_window.clear();
	chip_8_window.display();

	// Initialize sound
	sound_buffer.loadFromFile("beep.wav");
	sound.setBuffer(sound_buffer);
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		std::cout << "No ROM provided" << std::endl;
		return EXIT_SUCCESS;
	}

	std::ifstream inf{ argv[1], std::ios::binary };

	if (inf.fail()) {
		std::cerr << "Could not open ROM " << argv[1] << std::endl;
		return EXIT_FAILURE;
	}

	sf::RenderWindow window(sf::VideoMode(64 * SCALE_FACTOR, 32 * SCALE_FACTOR), "Chip 8", sf::Style::Titlebar | sf::Style::Close);
	Chip_8 chip_8{ window };

	chip_8.load_rom(inf);

	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) {
				window.close();
			}
			if (event.type == sf::Event::GainedFocus) {
				chip_8.focus();
			}
			if (event.type == sf::Event::LostFocus) {
				chip_8.unfocus();
			}
			if (event.type == sf::Event::KeyPressed || event.type == event.KeyReleased) {
				chip_8.set_keys();
			}
		}

		chip_8.emulate_cycle();
		std::this_thread::sleep_for(std::chrono::microseconds{ 1500 });
	}

	return EXIT_SUCCESS;
}
