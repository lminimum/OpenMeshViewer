#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class MeshViewerWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    void loadDefaultModel();

private slots:
    void openFile();
    void toggleRenderMode();

private:
    void createActions();
    void createMenus();

    MeshViewerWidget* meshViewer;
};

#endif // MAINWINDOW_H
