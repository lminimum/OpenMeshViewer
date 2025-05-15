#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include "MeshViewerWidget.h"

class MeshViewerWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    void loadDefaultModel();

private slots:
    void toggleRenderMode(RenderMode mode);
    void openFile();
	void deleteMesh();
	void saveFile();
    void chooseBackgroundColor();
    void chooseLightColor();
    void chooseForegroundColor();
    void remeshMesh();

private:
    void createActions();
    void createMenus();

    MeshViewerWidget* meshViewer;

    QPushButton* bgColorButton;
    QPushButton* lightColorButton;
    QPushButton* fgColorButton;
};

#endif
