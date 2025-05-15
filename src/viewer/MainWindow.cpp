#include "MainWindow.h"
#include "algorithm/IsotropicRemesher.h"
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

    QWidget* controlPanel = new QWidget;
    QVBoxLayout* controlLayout = new QVBoxLayout(controlPanel);

    QToolBar* toolbar = new QToolBar;

    QAction* openAction = toolbar->addAction("Open");
    QAction* saveAction = toolbar->addAction("Save");
    QAction* deleteAction = toolbar->addAction("Delete");

    controlLayout->addWidget(toolbar);

    // 连接动作到槽函数
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    connect(deleteAction, &QAction::triggered, this, &MainWindow::deleteMesh);

    // ─ 颜色设置
    QGroupBox* colorGroup = new QGroupBox("颜色");
    QFormLayout* colorForm = new QFormLayout;
    bgColorButton = new QPushButton(" ");
    lightColorButton = new QPushButton(" ");
    fgColorButton = new QPushButton(" ");

    bgColorButton->setStyleSheet("background-color: black");
    lightColorButton->setStyleSheet("background-color: darkred");
    fgColorButton->setStyleSheet("background-color: white");

    colorForm->addRow("背景", bgColorButton);
    colorForm->addRow("光源", lightColorButton);
    colorForm->addRow("前景", fgColorButton);
    colorGroup->setLayout(colorForm);
    controlLayout->addWidget(colorGroup);

    // 连接槽函数
    connect(bgColorButton, &QPushButton::clicked, this, &MainWindow::chooseBackgroundColor);
    connect(lightColorButton, &QPushButton::clicked, this, &MainWindow::chooseLightColor);
    connect(fgColorButton, &QPushButton::clicked, this, &MainWindow::chooseForegroundColor);

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
	QRadioButton* faceRadio = new QRadioButton("面");
	QRadioButton* hiddenLineRadio = new QRadioButton("线面结合");
    lineRadio->setChecked(true);
    oglLayout->addWidget(lineRadio);
    oglLayout->addWidget(faceRadio);
    oglLayout->addWidget(hiddenLineRadio);
    oglGroup->setLayout(oglLayout);
    controlLayout->addWidget(oglGroup);

    connect(lineRadio, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked)
            toggleRenderMode(Wireframe);
        });
    connect(faceRadio, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked)
            toggleRenderMode(Solid);
        });
    connect(hiddenLineRadio, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked)
            toggleRenderMode(HiddenLine);
        });

    // ─ 动画
    QGroupBox* animGroup = new QGroupBox("动画");
    QVBoxLayout* animLayout = new QVBoxLayout;
    animLayout->addWidget(new QCheckBox("旋转"));
    animGroup->setLayout(animLayout);
    controlLayout->addWidget(animGroup);

    QPushButton* remeshButton = new QPushButton("Remesh");
    controlLayout->addWidget(remeshButton);

    connect(remeshButton, &QPushButton::clicked, this, &MainWindow::remeshMesh);
    controlLayout->addStretch();  // 填充底部空白

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

void MainWindow::saveFile()
{
    if (!meshViewer || !meshViewer->isMeshLoaded()) {
        QMessageBox::warning(this, "Warning", "No mesh to save.");
        return;
    }

    QString filename = QFileDialog::getSaveFileName(
        this, "Save Mesh", "", "OBJ Files (*.obj);;OFF Files (*.off);;PLY Files (*.ply);;STL Files (*.stl)");

    if (filename.isEmpty())
        return;

    if (meshViewer->saveMesh(filename)) {
        statusBar()->showMessage("Mesh saved successfully", 3000);
    }
    else {
        QMessageBox::critical(this, "Error", "Failed to save the mesh.");
    }
}

void MainWindow::deleteMesh()
{
    if (!meshViewer || !meshViewer->isMeshLoaded()) {
        QMessageBox::warning(this, "Warning", "No mesh loaded.");
        return;
    }

    meshViewer->clearMesh();
    statusBar()->showMessage("Mesh deleted", 3000);
}

void MainWindow::loadDefaultModel()
{
    QFile resourceFile(":/models/Models/King.ply");
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

void MainWindow::toggleRenderMode(RenderMode mode)
{
    if (meshViewer)
    {
        meshViewer->toggleRenderMode( mode);
    }

    statusBar()->showMessage("Render mode toggled", 2000);
}

void MainWindow::chooseBackgroundColor()
{
    QColor color = QColorDialog::getColor(Qt::black, this, "选择背景颜色");
    if (color.isValid()) {
        bgColorButton->setStyleSheet(QString("background-color: %1").arg(color.name()));
        meshViewer->setBackgroundColor(color);  
    }
}

void MainWindow::chooseLightColor()
{
    QColor color = QColorDialog::getColor(Qt::red, this, "选择光源颜色");
    if (color.isValid()) {
        lightColorButton->setStyleSheet(QString("background-color: %1").arg(color.name()));
        meshViewer->setLightColor(color);  
    }
}

void MainWindow::chooseForegroundColor()
{
    QColor color = QColorDialog::getColor(Qt::white, this, "选择前景颜色");
    if (color.isValid()) {
        fgColorButton->setStyleSheet(QString("background-color: %1").arg(color.name()));
        meshViewer->setForegroundColor(color); 
    }
}

void MainWindow::remeshMesh()
{
    if (!meshViewer->isMeshLoaded()) {
        QMessageBox::warning(this, "提示", "请先加载一个网格");
        return;
    }

    Mesh& mesh = meshViewer->getMesh();
    IsotropicRemesher remesher(mesh);
    remesher.remesh();  

    meshViewer->update(); 
    QMessageBox::information(this, "完成", "Remesh 操作已成功完成！");
}