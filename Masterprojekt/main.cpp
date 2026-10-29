
#include <QApplication>
#include "ui/MainWindow.h"
#include "ParametrizationBatchTest.hpp"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();

   /* try
    {
        GlobalImplicitAssignmentTest::run(
            R"(C:\Users\marsp\source\repos\Masterprojekt\Masterprojekt)"
        );
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }*/

    return 0;
}
