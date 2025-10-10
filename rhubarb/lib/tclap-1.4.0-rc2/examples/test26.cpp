#include "tclap/CmdLine.h"
#include <iterator>

using namespace TCLAP;

// Define a simple 3D vector type
struct Vect3D {
    double v[3];

    std::ostream &print(std::ostream &os) const {
        std::copy(v, v + 3, std::ostream_iterator<double>(os, " "));
        return os;
    }
};

// operator>> will be used to assign to the vector since the default
// is that all types are ValueLike.
std::istream &operator>>(std::istream &is, Vect3D &v) {
    if (!(is >> v.v[0] >> v.v[1] >> v.v[2]))
        throw TCLAP::ArgParseException(" Argument is not a 3D vector");

    return is;
}

int main(int argc, char *argv[]) {
    CmdLine cmd("Command description message", ' ', "0.9");
    ValueArg<Vect3D> vec("v", "vect", "vector", true, Vect3D(), "3D vector",
                         cmd);

    try {
        cmd.parse(argc, argv);
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    vec.getValue().print(std::cout);
    std::cout << std::endl;
}
