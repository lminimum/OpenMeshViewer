#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>

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
    void chooseBackgroundColor();
    void chooseLightColor();
    void chooseForegroundColor();

private:
    void createActions();
    void createMenus();

    MeshViewerWidget* meshViewer;

    QPushButton* bgColorButton;
    QPushButton* lightColorButton;
    QPushButton* fgColorButton;
};

#endif // MAINWINDOW_H
