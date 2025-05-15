#include "MainWindow.h"
#include "MeshViewerWidget.h"

#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QMenuBar>
#include <QStandardPaths>
#include <QFile>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    QWidget* centralWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);

    meshViewer = new MeshViewerWidget();
    layout->addWidget(meshViewer);

    setCentralWidget(centralWidget);

    createActions();
    createMenus();

    setWindowTitle("OpenMesh Viewer");
    resize(800, 600);

    statusBar()->showMessage("Ready");
}

void MainWindow::createActions()
{
    QMenu* fileMenu = menuBar()->addMenu("&File");

    QAction* openAction = new QAction("&Open", this);
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    fileMenu->addAction(openAction);

    fileMenu->addSeparator();

    QAction* exitAction = new QAction("E&xit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(exitAction);

    QMenu* toggleMenu = menuBar()->addMenu("&Toggle");
    QAction* toggleRenderModeAction = new QAction("Toggle Render Mode", this);
    connect(toggleRenderModeAction, &QAction::triggered, this, &MainWindow::toggleRenderMode);
    toggleMenu->addAction(toggleRenderModeAction);
}

void MainWindow::createMenus()
{
    // 菜单已经创建在createActions里
}

void MainWindow::openFile()
{
    QString filename = QFileDialog::getOpenFileName(this,
        "Open Mesh File", "", "Mesh Files (*.obj *.off *.stl *.ply);;All Files (*)");

    if (filename.isEmpty())
        return;

    statusBar()->showMessage("Loading mesh...");

    if (meshViewer->loadMesh(filename))
    {
        statusBar()->showMessage("Mesh loaded successfully", 3000);
    }
    else
    {
        QMessageBox::critical(this, "Error", "Failed to load mesh file");
        statusBar()->showMessage("Failed to load mesh", 3000);
    }
}

void MainWindow::loadDefaultModel()
{
    QFile resourceFile(":/models/Models/Dino.ply");
    if (resourceFile.open(QIODevice::ReadOnly))
    {
        QByteArray data = resourceFile.readAll();

        QString tempFilePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/Dino_temp.ply";
        QFile tempFile(tempFilePath);
        if (tempFile.open(QIODevice::WriteOnly))
        {
            tempFile.write(data);
            tempFile.close();
            meshViewer->loadMesh(tempFilePath);
            tempFile.remove();
        }
        else
        {
            QMessageBox::critical(this, "Error", "Failed to create temporary file");
        }
    }
}

void MainWindow::toggleRenderMode()
{
    if (meshViewer)
    {
        meshViewer->toggleRenderMode();
    }

    statusBar()->showMessage("Render mode toggled", 2000);
}
