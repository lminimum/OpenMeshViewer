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
	if (renderMode == Wireframe)
	{
		// 线段索引
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
				indices << faceIndices[0] << faceIndices[1];
				indices << faceIndices[1] << faceIndices[2];
				indices << faceIndices[2] << faceIndices[0];
			}
		}
	}
	else
	{
		// 面片索引
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

	if (!meshLoaded || indexCount == 0) {
		return;
	}

	glEnable(GL_DEPTH_TEST);

	QOpenGLShaderProgram* activeProgram = (renderMode == Solid) ? solidProgram : wireframeProgram;
	if (!activeProgram) {
		return;
	}

	bool bindOk = activeProgram->bind();
	vao.bind();

	// 设置模型矩阵
	modelMatrix.setToIdentity();
	modelMatrix.translate(translation);

	// 视图矩阵：计算相机位置
	float radius = zoom;
	float pitch = qBound(-89.0f, rotationX, 89.0f);
	float yaw = rotationY;
	float camX = boundingBoxCenter.x() + radius * cos(qDegreesToRadians(pitch)) * sin(qDegreesToRadians(yaw));
	float camY = boundingBoxCenter.y() + radius * sin(qDegreesToRadians(pitch));
	float camZ = boundingBoxCenter.z() + radius * cos(qDegreesToRadians(pitch)) * cos(qDegreesToRadians(yaw));
	QVector3D cameraPos(camX, camY, camZ);
	viewMatrix.setToIdentity();
	viewMatrix.lookAt(cameraPos, boundingBoxCenter, QVector3D(0, 1, 0));

	// 设置 uniform
	activeProgram->setUniformValue("model", modelMatrix);
	activeProgram->setUniformValue("view", viewMatrix);
	activeProgram->setUniformValue("projection", projectionMatrix);
	QVector3D color(foregroundColor.redF(), foregroundColor.greenF(), foregroundColor.blueF());
	activeProgram->setUniformValue("uForegroundColor", color);


	if (renderMode == Solid)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
	}
	else if (renderMode == Wireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_LINES, indexCount, GL_UNSIGNED_INT, 0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	//暂未实现
	else if (renderMode == HiddenLine)
	{

		// Pass 1: 画填充面（不写颜色，只写深度）——建立深度缓冲
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_POLYGON_OFFSET_FILL); // 推远一点避免冲突
		glPolygonOffset(1.0f, 1.0f);       // 推远的参数可调试

		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // 不写颜色
		glDepthMask(GL_TRUE);                                // 写深度
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);

		// Pass 2: 画线框（不写深度，只写颜色）——前景覆盖
		glDisable(GL_POLYGON_OFFSET_FILL);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);     // 写颜色
		glDepthMask(GL_FALSE);                               // 不写深度
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glLineWidth(1.0f);
		glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);

		// 状态恢复
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
	}

	vao.release();
	activeProgram->release();
}

void MeshViewerWidget::resizeGL(int width, int height)
{
	float nearPlane = boundingBoxDiameter * 0.01f;
	float farPlane = boundingBoxDiameter * 10000.0f;

	nearPlane = std::max(0.01f, nearPlane);
	farPlane = std::max(nearPlane + 1.0f, farPlane);

	projectionMatrix.setToIdentity();
	projectionMatrix.perspective(45.0f, width / float(height), nearPlane, farPlane);
}

void MeshViewerWidget::toggleRenderMode(RenderMode mode)
{
	if (renderMode != mode)
	{
		renderMode = mode;
		makeCurrent();
		updateMeshBuffers();
		doneCurrent();
		update();
	}
}

void MeshViewerWidget::mousePressEvent(QMouseEvent* event)
{
	lastMousePosition = event->pos();
}

void MeshViewerWidget::mouseMoveEvent(QMouseEvent* event)
{
	QPoint delta = event->pos() - lastMousePosition;
	lastMousePosition = event->pos();

	if (event->buttons() & Qt::RightButton)
	{
		rotationY += 0.5f * delta.x();
		rotationX += 0.5f * delta.y();

		// 限制rotationX，避免翻转过头
		rotationX = qBound(-90.0f, rotationX, 90.0f);
	}
	else if (event->buttons() & Qt::LeftButton)
	{
		// 根据当前旋转角度计算相机的方向向量
		QMatrix4x4 rotationMatrix;
		rotationMatrix.setToIdentity();
		rotationMatrix.rotate(rotationX, 1.0f, 0.0f, 0.0f);
		rotationMatrix.rotate(rotationY, 0.0f, 1.0f, 0.0f);

		// 右方向（相机的局部X轴）
		QVector3D right = rotationMatrix * QVector3D(1, 0, 0);
		// 上方向（相机的局部Y轴）
		QVector3D up = rotationMatrix * QVector3D(0, 1, 0);

		float panSpeed = 0.001f * zoom;

		// 鼠标右键拖拽，方向与鼠标移动一致，所以负号表示“拖拽模型跟随鼠标移动”
		translation += right * delta.x() * panSpeed;
		translation += -up * delta.y() * panSpeed;
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
	vertexBuffer.allocate(nullptr, 0);
	indexBuffer.bind();
	indexBuffer.allocate(nullptr, 0);
	vao.release();
	doneCurrent();

	update();
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

	translation = QVector3D(0, 0, 0);

	zoom = boundingBoxDiameter * 1.2f;

	update();

}

void MeshViewerWidget::buildExampleMesh(int type)
{
	mesh.clear();

	if (type == 0) {
		mesh.clear();

		std::vector<Mesh::VertexHandle> vhandles;

		std::vector<float> row_y = { 2.0f, 1.0f, 0.0f };
		std::vector<int> row_counts = { 3, 4, 3 };  

		for (int row = 0; row < 3; ++row) {
			float y = row_y[row];
			int count = row_counts[row];
			for (int i = 0; i < count; ++i) {
				float x = i * 1.0f;
				if (row == 1) x -= 0.5f; 
				vhandles.push_back(mesh.add_vertex(Mesh::Point(x, y, 0.0f)));
			}
		}

		// 顶点编号参考（共 10 个）：
		//   0   1   2       → 上排 3 个
		// 3   4   5   6    → 中排 4 个
		//   7   8   9       → 下排 3 个

		// 定义 10 个面（每次加入两个三角形）
		std::vector<std::vector<int>> faces = {
			{0, 3, 4},
			{1, 0, 4},
			{1, 4, 5},
			{1, 5, 2},
			{2, 5, 6},
			{3, 7, 4},
			{4, 7, 8},
			{4, 8, 5},
			{5, 8, 9},
			{5, 9, 6}
		};

		for (const auto& f : faces) {
			std::vector<Mesh::VertexHandle> face = {
				vhandles[f[0]],
				vhandles[f[1]],
				vhandles[f[2]]
			};
			auto fh = mesh.add_face(face);
			if (!fh.is_valid()) {
				std::cerr << "Face failed to add: " << f[0] << ", " << f[1] << ", " << f[2] << std::endl;
			}
		}
	}
	else if (type == 1) {
		Mesh::VertexHandle v0 = mesh.add_vertex(Mesh::Point(0.0f, 0.0f, 0.0f));
		Mesh::VertexHandle v1 = mesh.add_vertex(Mesh::Point(1.0f, 1.0f, 0.0f));
		Mesh::VertexHandle v2 = mesh.add_vertex(Mesh::Point(2.0f, 0.0f, 0.0f));
		Mesh::VertexHandle v3 = mesh.add_vertex(Mesh::Point(1.0f, -1.0f, 0.0f));

		mesh.add_face({ v0, v1, v2 });
		mesh.add_face({ v0, v2, v3 });
	}
	else if (type == 2) {
		Mesh::VertexHandle v0 = mesh.add_vertex(Mesh::Point(0.0f, 0.0f, 0.0f));
		Mesh::VertexHandle v1 = mesh.add_vertex(Mesh::Point(1.0f, 1.0f, 0.0f));
		Mesh::VertexHandle v2 = mesh.add_vertex(Mesh::Point(2.0f, 0.0f, 0.0f));
		Mesh::VertexHandle v3 = mesh.add_vertex(Mesh::Point(1.0f, -1.0f, 0.0f));

		mesh.add_face({ v0, v1, v2 });
		mesh.add_face({ v0, v2, v3 });
	}

	mesh.request_face_normals();
	mesh.request_vertex_normals();
	mesh.update_normals();

	meshLoaded = true;

	centerAndScaleMesh();

	makeCurrent();
	updateMeshBuffers();
	doneCurrent();

	resetView();

	update();
}
