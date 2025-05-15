#include "MeshViewerWidget.h"
#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QMatrix4x4>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <cmath>

MeshViewerWidget::MeshViewerWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      meshLoaded(false),
      //program(nullptr),
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
    translation = QVector3D(0.0f, 0.0f, 0.0f);

    if (solidProgram)
    {
        delete solidProgram;
        solidProgram = nullptr;
    }
    if (wireframeProgram)
    {
        delete wireframeProgram;
        wireframeProgram = nullptr;
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
    translation = QVector3D(0.0f, 0.0f, 0.0f);

    update();
}

void MeshViewerWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    // 初始背景色
    glClearColor(backgroundColor.redF(), backgroundColor.greenF(), backgroundColor.blueF(), 1.0f);

    GLfloat lightPos[] = { 0.0f, 0.0f, 1.0f, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    solidProgram = new QOpenGLShaderProgram();
    solidProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/Shaders/solid.vert.glsl");
    solidProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/Shaders/solid.frag.glsl");
    solidProgram->link();

    wireframeProgram = new QOpenGLShaderProgram();
    wireframeProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/Shaders/basic.vert.glsl");
    wireframeProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/Shaders/basic.frag.glsl");
    wireframeProgram->link();

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

    // 准备顶点数据（位置和法线）
    QVector<GLfloat> vertices;
    for (Mesh::VertexIter v_it = mesh.vertices_begin(); v_it != mesh.vertices_end(); ++v_it)
    {
        Mesh::Point p = mesh.point(*v_it);
        Mesh::Normal n = mesh.normal(*v_it);

        // 顶点位置
        vertices << p[0] << p[1] << p[2];
        // 顶点法线（如果你有需要可以使用）
        vertices << n[0] << n[1] << n[2];
    }

    // 准备索引数据，假设每个面是三角形
    QVector<GLuint> indices;
    indices.clear();
    for (Mesh::FaceIter f_it = mesh.faces_begin(); f_it != mesh.faces_end(); ++f_it)
    {
        QList<GLuint> faceIndices;
        for (Mesh::FaceVertexIter fv_it = mesh.fv_iter(*f_it); fv_it.is_valid(); ++fv_it)
        {
            faceIndices.append(fv_it->idx());
        }

        if (faceIndices.size() == 3)
        {
            if (renderMode == Wireframe)
            {
                // 三条边作为线段
                indices << faceIndices[0] << faceIndices[1];
                indices << faceIndices[1] << faceIndices[2];
                indices << faceIndices[2] << faceIndices[0];
            }
            else
            {
                // 三角面片
                indices << faceIndices[0] << faceIndices[1] << faceIndices[2];
            }
        }
    }

    // 存储索引数量
    indexCount = indices.size();

    // 上传顶点数据
    vertexBuffer.bind();
    vertexBuffer.allocate(vertices.constData(), vertices.size() * sizeof(GLfloat));

    // 设置顶点属性指针
    /*program->bind();*/
    QOpenGLShaderProgram* activeProgram = (renderMode == Solid) ? solidProgram : wireframeProgram;
    activeProgram->bind();

    // 位置属性
    int posAttr = activeProgram->attributeLocation("position");
    activeProgram->enableAttributeArray(posAttr);
    activeProgram->setAttributeBuffer(posAttr, GL_FLOAT, 0, 3, 6 * sizeof(GLfloat));

    // 法线属性
    int normalAttr = activeProgram->attributeLocation("normal");
    activeProgram->enableAttributeArray(normalAttr);
    activeProgram->setAttributeBuffer(normalAttr, GL_FLOAT, 3 * sizeof(GLfloat), 3, 6 * sizeof(GLfloat));

    // 上传索引数据
    indexBuffer.bind();
    indexBuffer.allocate(indices.constData(), indices.size() * sizeof(GLuint));

    activeProgram->release();
    vao.release();
}

void MeshViewerWidget::paintGL()
{
    // 应用背景色
    glClearColor(backgroundColor.redF(), backgroundColor.greenF(), backgroundColor.blueF(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 设置光源颜色
    GLfloat diffuse[] = {
        lightColor.redF(),
        lightColor.greenF(),
        lightColor.blueF(),
        1.0f
    };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

    // 设置前景颜色（绘制网格时使用）
    glColor3f(foregroundColor.redF(), foregroundColor.greenF(), foregroundColor.blueF());

    if (!meshLoaded)
        return;

    QOpenGLShaderProgram* activeProgram = (renderMode == Solid) ? solidProgram : wireframeProgram;
    if (!activeProgram) return;

    activeProgram->bind();
    vao.bind();

    modelMatrix.setToIdentity();
    modelMatrix.translate(translation);
    modelMatrix.rotate(rotationX, 1.0f, 0.0f, 0.0f);
    modelMatrix.rotate(rotationY, 0.0f, 1.0f, 0.0f);

    viewMatrix.setToIdentity();
    viewMatrix.lookAt(
        QVector3D(0, 0, zoom),  
        QVector3D(0, 0, 0),     
        QVector3D(0, 1, 0));   

    activeProgram->setUniformValue("model", modelMatrix);
    activeProgram->setUniformValue("view", viewMatrix);
    activeProgram->setUniformValue("projection", projectionMatrix);

    if (renderMode == Solid)
    {
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    }
    else if (renderMode == Wireframe)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_LINES, indexCount, GL_UNSIGNED_INT, 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    vao.release();
    activeProgram->release();
}

void MeshViewerWidget::resizeGL(int width, int height)
{
    // Update projection matrix
    projectionMatrix.setToIdentity();
    projectionMatrix.perspective(45.0f, width / float(height), 0.1f, 100.0f);
}

void MeshViewerWidget::toggleRenderMode()
{
    if (renderMode == Solid)
    {
        renderMode = Wireframe;
    }
    else
    {
        renderMode = Solid;
    }

    makeCurrent();       
    updateMeshBuffers();  
    doneCurrent();

    update(); 
}

void MeshViewerWidget::mousePressEvent(QMouseEvent* event)
{
    lastMousePosition = event->pos();
}

void MeshViewerWidget::mouseMoveEvent(QMouseEvent* event)
{
    QPoint delta = event->pos() - lastMousePosition;
    lastMousePosition = event->pos();

    if (event->buttons() & Qt::LeftButton)
    {
        rotationY += 0.5f * delta.x();
        rotationX += 0.5f * delta.y();
    }
    else if (event->buttons() & Qt::RightButton)
    {
        float panSpeed = 0.001f * zoom; 

        translation -= QVector3D(-delta.x() * panSpeed, delta.y() * panSpeed, 0.0f);
    }

    update();
}

void MeshViewerWidget::wheelEvent(QWheelEvent* event)
{
    float zoomStep = 0.2f; 
    zoom -= zoomStep * (event->angleDelta().y() / 120.0f);
    zoom = qBound(0.1f, zoom, 50.0f);  

    update();
}

void MeshViewerWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_R)
    {
        resetView();
    }
}

void MeshViewerWidget::setBackgroundColor(const QColor& color)
{
    backgroundColor = color;
    update();  // 请求重绘
}

void MeshViewerWidget::setLightColor(const QColor& color)
{
    lightColor = color;
    update();
}

void MeshViewerWidget::setForegroundColor(const QColor& color)
{
    foregroundColor = color;
    update();
}

