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
      indexCount(0),
	  renderMode(Wireframe)
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

    centerAndScaleMesh(); 
    resetView();
    update();

    return true;
}

void MeshViewerWidget::resetView()
{
    rotationX = 0.0f;
    rotationY = 0.0f;

    // 缩放距离根据模型大小设置
    zoom = boundingBoxDiameter * 1.2f;

    // 平移重置
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
    glClearColor(backgroundColor.redF(), backgroundColor.greenF(), backgroundColor.blueF(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!meshLoaded || indexCount == 0)
        return;

    QOpenGLShaderProgram* activeProgram = (renderMode == Solid) ? solidProgram : wireframeProgram;
    if (!activeProgram) return;

    activeProgram->bind();
    vao.bind();

    // 模型矩阵：先平移模型中心到原点，平移由translation控制（一般为0）
    modelMatrix.setToIdentity();
    modelMatrix.translate(translation);

    // 视图矩阵：计算相机位置，绕boundingBoxCenter旋转
    float radius = zoom;
    // 限制俯仰角避免上下翻转
    float pitch = qBound(-89.0f, rotationX, 89.0f);
    float yaw = rotationY;

    // 计算相机在球面上的坐标
    float camX = boundingBoxCenter.x() + radius * cos(qDegreesToRadians(pitch)) * sin(qDegreesToRadians(yaw));
    float camY = boundingBoxCenter.y() + radius * sin(qDegreesToRadians(pitch));
    float camZ = boundingBoxCenter.z() + radius * cos(qDegreesToRadians(pitch)) * cos(qDegreesToRadians(yaw));
    QVector3D cameraPos(camX, camY, camZ);

    viewMatrix.setToIdentity();
    viewMatrix.lookAt(cameraPos, boundingBoxCenter, QVector3D(0, 1, 0));

    activeProgram->setUniformValue("model", modelMatrix);
    activeProgram->setUniformValue("view", viewMatrix);
    activeProgram->setUniformValue("projection", projectionMatrix);
    QVector3D color(foregroundColor.redF(), foregroundColor.greenF(), foregroundColor.blueF());
    activeProgram->setUniformValue("uForegroundColor", color);

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
    float nearPlane = boundingBoxDiameter * 0.01f;
    float farPlane = boundingBoxDiameter * 50.0f;

    nearPlane = std::max(0.01f, nearPlane);
    farPlane = std::max(nearPlane + 1.0f, farPlane);

    projectionMatrix.setToIdentity();
    projectionMatrix.perspective(45.0f, width / float(height), nearPlane, farPlane);
}

void MeshViewerWidget::toggleRenderMode(RenderMode mode)
{
    if (renderMode)
    {
        renderMode = mode;
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
        rotationX = qBound(-89.0f, rotationX, 89.0f);
    }
    else if (event->buttons() & Qt::RightButton)
    {
        // 计算相机的方向向量
        float pitch = qDegreesToRadians(rotationX);
        float yaw = qDegreesToRadians(rotationY);

        // 计算相机的前方向
        QVector3D forward(
            cos(pitch) * sin(yaw),
            sin(pitch),
            cos(pitch) * cos(yaw)
        );
        forward.normalize();

        // 计算相机右向量（世界上向量0,1,0叉乘前向量）
        QVector3D worldUp(0, 1, 0);
        QVector3D right = QVector3D::crossProduct(forward, worldUp).normalized();

        // 计算相机上向量（右向量叉乘前向量）
        QVector3D up = QVector3D::crossProduct(right, forward).normalized();

        float panSpeed = 0.001f * zoom;

        // 用鼠标移动量乘以右向量和上向量，叠加到 translation 上
        translation -= right * (float)delta.x() * panSpeed;
        translation -= up * (float)delta.y() * panSpeed;
    }

    update();
}

void MeshViewerWidget::wheelEvent(QWheelEvent* event)
{
    float baseStep = boundingBoxDiameter * 0.02f;
    zoom -= baseStep * (event->angleDelta().y() / 120.0f);
    zoom = qBound(boundingBoxDiameter * 0.1f, zoom, boundingBoxDiameter * 50.0f);
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

bool MeshViewerWidget::saveMesh(const QString& filename)  
{  
   // 使用 OpenMesh 保存  
   try {  
       return OpenMesh::IO::write_mesh(mesh, filename.toStdString());  
   }  
   catch (...) {  
       return false;  
   }  
}   

void MeshViewerWidget::clearMesh()
{
    mesh.clear();
    meshLoaded = false;
    indexCount = 0;

    makeCurrent();
    vao.bind();
    vertexBuffer.bind();
    vertexBuffer.allocate(nullptr, 0); // 清空 GPU 缓冲区
    indexBuffer.bind();
    indexBuffer.allocate(nullptr, 0);
    vao.release();
    doneCurrent();

    update(); // 触发重绘
}

bool MeshViewerWidget::isMeshLoaded() const
{
    return meshLoaded;
}

void MeshViewerWidget::centerAndScaleMesh()
{
    if (!meshLoaded)
        return;

    OpenMesh::Vec3f minPt(std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max());
    OpenMesh::Vec3f maxPt(std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest());

    for (auto v_it = mesh.vertices_begin(); v_it != mesh.vertices_end(); ++v_it)
    {
        auto p = mesh.point(*v_it);
        minPt.minimize(p);
        maxPt.maximize(p);
    }

    OpenMesh::Vec3f center = (minPt + maxPt) * 0.5f;
    boundingBoxCenter = QVector3D(center[0], center[1], center[2]);
    boundingBoxDiameter = (maxPt - minPt).length();
    if (boundingBoxDiameter < 1e-6f) {
        boundingBoxDiameter = 1.0f;
    }

    // translation = -boundingBoxCenter; // 不要平移模型，模型保持原点
    translation = QVector3D(0, 0, 0);

    zoom = boundingBoxDiameter * 1.2f;

    update();
}