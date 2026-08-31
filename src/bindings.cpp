#include "OLED_SSD1306.hpp"
#include <pybind11/pybind11.h>

// NOLINTBEGIN
namespace py = pybind11;

// Use the macro to define the Python module and expose the class
PYBIND11_MODULE(display_controller, module)
{
    module.doc() = "Display controller";

    // Bind the Calculator class
    py::class_<SSD1306>(module, "SSD1306")
        .def(py::init()) // Binds the constructor
        .def("begin", &SSD1306::begin, "Begin the display controller")
        .def("set_top_text", &SSD1306::setTopText, "Set the display top text")
        .def("set_battery_charge", &SSD1306::setBatteryCharge, "Set the battery charge level (0-100)")
        .def("set_network_off", &SSD1306::setNetworkSymbolOff, "Enable the network off flag")
        .def("set_main_text", &SSD1306::setMainText, "Set the display main text")
        .def("set_battery_charging", &SSD1306::setBatteryCharging, "Set the battery charging flag")
        .def("update", &SSD1306::OLEDupdate, "Update the display");
}
// NOLINTEND
