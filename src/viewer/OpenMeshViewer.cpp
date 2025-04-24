// OpenMeshViewer.cpp : Defines the entry point for the application.

#include "OpenMeshViewer.h"
#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QMatrix4x4>
#include <QFile>
#include <QTextStream>
#include <cmath>

// MeshViewerWidget implementation
MeshViewerWidget::MeshViewerWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      meshLoaded(false),
      program(nullptr),
      vertexBuffer(QOpenGLBuffer::VertexBuffer),
      indexBuffer(QOpenGLBuffer::IndexBuffer),
      rotationX(0.0f),
      rotationY(0.0f),
      zoom(5.0f),
      indexCount(0)
{
    setFocusPolicy(Qt::StrongFocus);
}

MeshViewerWidget::~MeshViewerWidget()
{
    makeCurrent();

    if (program)
    {
        delete program;
        program = nullptr;
    }
    vao.destroy();
    vertexBuffer.destroy();
    indexBuffer.destroy();

    doneCurrent();
}

bool MeshViewerWidget::loadMesh(const QString &filename)
{
    if (!OpenMesh::IO::read_mesh(mesh, filename.toStdString()))
    {
        return false;
    }

    // Update vertex normals
    mesh.request_face_normals();
    mesh.request_vertex_normals();
    mesh.update_normals();

    meshLoaded = true;

    // If context is already available, update mesh buffers
    if (context())
    {
        makeCurrent();
        updateMeshBuffers();
        doneCurrent();
    }

    resetView();
    update();

    return true;
}

void MeshViewerWidget::resetView()
{
    rotationX = 0.0f;
    rotationY = 0.0f;
    zoom = 5.0f;

    modelMatrix.setToIdentity();
    viewMatrix.setToIdentity();
    viewMatrix.translate(0.0f, 0.0f, -zoom);

    update();
}

void MeshViewerWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // Create shader program
    program = new QOpenGLShaderProgram();
    program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/Shaders/basic.vert.glsl");
    program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/Shaders/basic.frag.glsl");
    program->link();

    // Create VAO and buffers
    vao.create();
    vao.bind();

    vertexBuffer.create();
    indexBuffer.create();

    vao.release();

    // If a mesh is already loaded, update buffers
    if (meshLoaded)
    {
        updateMeshBuffers();
    }
}

void MeshViewerWidget::updateMeshBuffers()
{
    if (!meshLoaded)
        return;

    vao.bind();

    // Prepare vertex data (position and normal)
    QVector<GLfloat> vertices;
    for (Mesh::VertexIter v_it = mesh.vertices_begin(); v_it != mesh.vertices_end(); ++v_it)
    {
        Mesh::Point p = mesh.point(*v_it);
        Mesh::Normal n = mesh.normal(*v_it);

        // Position
        vertices << p[0] << p[1] << p[2];
        // Normal
        vertices << n[0] << n[1] << n[2];
    }

    // Prepare index data for triangular faces
    QVector<GLuint> indices;
    for (Mesh::FaceIter f_it = mesh.faces_begin(); f_it != mesh.faces_end(); ++f_it)
    {
        for (Mesh::FaceVertexIter fv_it = mesh.fv_iter(*f_it); fv_it.is_valid(); ++fv_it)
        {
            indices << fv_it->idx();
        }
    }

    // Store number of indices for later use
    indexCount = indices.size();

    // Upload vertex data
    vertexBuffer.bind();
    vertexBuffer.allocate(vertices.constData(), vertices.size() * sizeof(GLfloat));

    // Set vertex attribute pointers
    program->bind();

    // Position attribute
    int posAttr = program->attributeLocation("position");
    program->enableAttributeArray(posAttr);
    program->setAttributeBuffer(posAttr, GL_FLOAT, 0, 3, 6 * sizeof(GLfloat));

    // Normal attribute
    int normalAttr = program->attributeLocation("normal");
    program->enableAttributeArray(normalAttr);
    program->setAttributeBuffer(normalAttr, GL_FLOAT, 3 * sizeof(GLfloat), 3, 6 * sizeof(GLfloat));

    // Upload index data
    indexBuffer.bind();
    indexBuffer.allocate(indices.constData(), indices.size() * sizeof(GLuint));

    program->release();
    vao.release();
}

void MeshViewerWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!meshLoaded || !program)
        return;

    program->bind();
    vao.bind();

    // Update model matrix with current rotation
    modelMatrix.setToIdentity();
    modelMatrix.rotate(rotationX, 1.0f, 0.0f, 0.0f);
    modelMatrix.rotate(rotationY, 0.0f, 1.0f, 0.0f);

    // Update view matrix with current zoom
    viewMatrix.setToIdentity();
    viewMatrix.translate(0.0f, 0.0f, -zoom);

    // Set uniforms
    program->setUniformValue("model", modelMatrix);
    program->setUniformValue("view", viewMatrix);
    program->setUniformValue("projection", projectionMatrix);

    // Draw mesh
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);

    vao.release();
    program->release();
}

void MeshViewerWidget::resizeGL(int width, int height)
{
    // Update projection matrix
    projectionMatrix.setToIdentity();
    projectionMatrix.perspective(45.0f, width / float(height), 0.1f, 100.0f);
}

void MeshViewerWidget::mousePressEvent(QMouseEvent *event)
{
    lastMousePosition = event->pos();
}

void MeshViewerWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        QPoint delta = event->pos() - lastMousePosition;
        rotationY += 0.5f * delta.x();
        rotationX += 0.5f * delta.y();

        update();
    }

    lastMousePosition = event->pos();
}

void MeshViewerWidget::wheelEvent(QWheelEvent *event)
{
    zoom -= event->angleDelta().y() / 120.0f;
    zoom = qMax(1.0f, qMin(zoom, 15.0f));

    viewMatrix.setToIdentity();
    viewMatrix.translate(0.0f, 0.0f, -zoom);

    update();
}

// MainWindow implementation
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Create central widget with mesh viewer
    QWidget *centralWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    meshViewer = new MeshViewerWidget();
    layout->addWidget(meshViewer);

    setCentralWidget(centralWidget);

    // Create menus and actions
    createActions();
    createMenus();

    // Set window properties
    setWindowTitle("OpenMesh Viewer");
    resize(800, 600);

    // Set status bar message
    statusBar()->showMessage("Ready");
}

void MainWindow::createActions()
{
    QMenu *fileMenu = menuBar()->addMenu("&File");

    QAction *openAction = new QAction("&Open", this);
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    fileMenu->addAction(openAction);

    fileMenu->addSeparator();

    QAction *exitAction = new QAction("E&xit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(exitAction);

    QMenu *helpMenu = menuBar()->addMenu("&Help");

    QAction *aboutAction = new QAction("&About", this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::about);
    helpMenu->addAction(aboutAction);
}

void MainWindow::createMenus()
{
    // Menus are already created in createActions()
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

void MainWindow::about()
{
    QMessageBox::about(this, "About OpenMesh Viewer",
                       "OpenMesh Viewer\n\n"
                       "A simple 3D mesh viewer using Qt and OpenMesh\n"
                       "© 2025");
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
