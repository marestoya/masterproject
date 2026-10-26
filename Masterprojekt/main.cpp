
#include <QApplication>
#include "ui/MainWindow.h"
#include "CostModelBatchTest.hpp"

int main(int argc, char* argv[])
{
    /*QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();*/

    try
    {
        CostModelBatchTest::run(
            R"(C:\Users\marsp\source\repos\Masterprojekt\Masterprojekt)"
        );
    }
    catch (const std::exception& e)
    {
        std::cerr << "Cost model test failed: "
            << e.what() << '\n';
        return 1;
    }

    return 0;
}
