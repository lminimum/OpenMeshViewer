#include "MainWindow.h"
#include "MeshViewerWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QToolBar>
#include <QGroupBox>
#include <QPushButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QColorDialog>
#include <QLabel>
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
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);

    // ➤ 左侧控件面板
    QWidget* controlPanel = new QWidget;
    QVBoxLayout* controlLayout = new QVBoxLayout(controlPanel);

    // ─ 工具栏（用按钮模拟）
    QToolBar* toolbar = new QToolBar;
    toolbar->addAction(QIcon(":/icons/open.png"), "Open");
    toolbar->addAction(QIcon(":/icons/save.png"), "Save");
    toolbar->addAction(QIcon(":/icons/delete.png"), "Delete");
    controlLayout->addWidget(toolbar);

    // ─ 颜色设置
    QGroupBox* colorGroup = new QGroupBox("颜色");
    QFormLayout* colorForm = new QFormLayout;
    QPushButton* bgBtn = new QPushButton(" ");
    QPushButton* lightBtn = new QPushButton(" ");
    QPushButton* fgBtn = new QPushButton(" ");

    bgBtn->setStyleSheet("background-color: white");
    lightBtn->setStyleSheet("background-color: darkred");
    fgBtn->setStyleSheet("background-color: black");

    colorForm->addRow("背景", bgBtn);
    colorForm->addRow("光源", lightBtn);
    colorForm->addRow("前景", fgBtn);
    colorGroup->setLayout(colorForm);
    controlLayout->addWidget(colorGroup);

    // ─ OpenGL 设置
    QGroupBox* oglGroup = new QGroupBox("OpenGL");
    QVBoxLayout* oglLayout = new QVBoxLayout;
    oglLayout->addWidget(new QCheckBox("反锯齿"));
    oglLayout->addWidget(new QCheckBox("Gouraud 着色"));
    oglLayout->addWidget(new QCheckBox("光照"));
    QCheckBox* showBackface = new QCheckBox("显示背面");
    showBackface->setChecked(true);
    oglLayout->addWidget(showBackface);
    oglLayout->addWidget(new QRadioButton("点"));
    QRadioButton* lineRadio = new QRadioButton("线");
    lineRadio->setChecked(true);
    oglLayout->addWidget(lineRadio);
    oglLayout->addWidget(new QRadioButton("面"));
    oglLayout->addWidget(new QRadioButton("面线混合"));
    oglGroup->setLayout(oglLayout);
    controlLayout->addWidget(oglGroup);

    // ─ 动画
    QGroupBox* animGroup = new QGroupBox("动画");
    QVBoxLayout* animLayout = new QVBoxLayout;
    animLayout->addWidget(new QCheckBox("旋转"));
    animGroup->setLayout(animLayout);
    controlLayout->addWidget(animGroup);

    controlLayout->addStretch();  // 填充底部空白

    // ➤ 中央区域为 MeshViewer
    meshViewer = new MeshViewerWidget();
    mainLayout->addWidget(controlPanel);
    mainLayout->addWidget(meshViewer, 1);  // 拉伸占满

    setCentralWidget(centralWidget);
    createActions();
    createMenus();
    setWindowTitle("OpenMesh Viewer");
    resize(1000, 600);
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
