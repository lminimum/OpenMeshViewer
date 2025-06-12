# ✨ 计算机图形学御用代码 ✨

> Ciallo～(∠◠ڼ◠)⌒☆

本项目是计算机图形学学习与实验的御用代码库，集成了 [OpenMesh](https://www.graphics.rwth-aachen.de/software/openmesh/) 与 [Qt6](https://www.qt.io/)，适用于网格处理、建模算法和图形界面展示。

---

## 📦 使用说明

1. **准备依赖**  
   请确保你已正确安装以下依赖项：

   - ✅ [OpenMesh](https://www.graphics.rwth-aachen.de/software/openmesh/)
   - ✅ [Qt6](https://www.qt.io/)（建议安装 Qt6.5 及以上）

2. **配置项目**

   修改根目录下的 `CMakePresets.json` 文件，根据你的本地环境配置 `OpenMesh` 和 `Qt6` 的路径，例如：

   ```json
   "cacheVariables": {
     "OpenMesh_DIR": "D:/Libraries/OpenMesh",
     "Qt6_DIR": "C:/Qt/6.5.0/msvc2019_64/lib/cmake/Qt6"
   }
