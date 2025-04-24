// OpenMeshViewer.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>
#include <string>

// Qt includes
#include <QMainWindow>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QMouseEvent>
#include <QFileDialog>
#include <QMessageBox>

// OpenMesh includes
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>

// Define the mesh type
typedef OpenMesh::TriMesh_ArrayKernelT<> Mesh;

// MeshViewerWidget class - OpenGL widget for rendering the mesh
class MeshViewerWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    MeshViewerWidget(QWidget* parent = nullptr);
    ~MeshViewerWidget();

    bool loadMesh(const QString& filename);
    void resetView();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void updateMeshBuffers();
    
    Mesh mesh;
    bool meshLoaded;
    
    QOpenGLShaderProgram* program;
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vertexBuffer;
    QOpenGLBuffer indexBuffer;
    int indexCount;  // Store the number of indices for drawing
    
    QMatrix4x4 modelMatrix;
    QMatrix4x4 viewMatrix;
    QMatrix4x4 projectionMatrix;
    
    QPoint lastMousePosition;
    float rotationX, rotationY;
    float zoom;
};

// MainWindow class - the application's main window
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);

private slots:
    void openFile();
    void about();

private:
    MeshViewerWidget* meshViewer;
    void createActions();
    void createMenus();
};
