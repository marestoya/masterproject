
#include <QApplication>
#include "ui/MainWindow.h"
#include "ParametrizationBatchTest.hpp"
#include <unity_connection.hpp>
#include <unity_simulation.hpp>
#include <unity_joint_config.hpp>
#include <franka_proxy_share/franka_proxy_util.hpp>
#include <Eigen/Geometry>
#include <array>
#include <cmath>




int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}
  
