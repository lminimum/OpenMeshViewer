#pragma once

#include <QMainWindow>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QColor>
#include <QMouseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QMatrix4x4>

#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>

enum RenderMode {
    //Points,
    Solid,     
    Wireframe,
    HiddenLine
};

using Mesh = OpenMesh::TriMesh_ArrayKernelT<>;

class MeshViewerWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    Mesh& getMesh() {
        return mesh;
    }

    explicit MeshViewerWidget(QWidget* parent = nullptr);
    ~MeshViewerWidget() override;

    bool isMeshLoaded() const;
    bool loadMesh(const QString& filename);
    void resetView();
	void clearMesh();
    bool saveMesh(const QString& filename);
    void toggleRenderMode(RenderMode mode);
    void setBackgroundColor(const QColor& color);
    void setLightColor(const QColor& color);
	void setForegroundColor(const QColor& color);

    QVector3D translation;    
    RenderMode renderMode;

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void updateMeshBuffers(); 
    void centerAndScaleMesh();

    QOpenGLShaderProgram* solidProgram = nullptr;   
    QOpenGLShaderProgram* wireframeProgram = nullptr;
    QOpenGLVertexArrayObject vao;                  
    QOpenGLBuffer vertexBuffer{ QOpenGLBuffer::VertexBuffer }; 
    QOpenGLBuffer indexBuffer{ QOpenGLBuffer::IndexBuffer };   

    Mesh mesh;                  
    bool meshLoaded = false;   
    QColor backgroundColor = Qt::black;
    QColor lightColor = Qt::red;
    QColor foregroundColor = Qt::white;
    int indexCount = 0;

    QMatrix4x4 modelMatrix;     
    QMatrix4x4 viewMatrix;      
    QMatrix4x4 projectionMatrix;

    QPoint lastMousePosition;
    float boundingBoxDiameter = 1.0f;
    QVector3D boundingBoxCenter;
    float rotationX = 0.0f;  
    float rotationY = 0.0f;     
    float zoom = 1.0f;
};