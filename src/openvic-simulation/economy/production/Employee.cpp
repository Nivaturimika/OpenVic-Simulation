#include "Employee.hpp"

using namespace OpenVic;

Employee::Employee(Pop& new_pop, const pop_size_t new_size)
	: pop { new_pop },
	size { new_size }
	{}